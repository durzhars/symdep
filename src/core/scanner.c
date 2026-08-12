/*
 * Symlink & Dependency Manager (symdep)
 * Copyright (C) 2026 durzhars
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#define _GNU_SOURCE
#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L
#include "core/scanner.h"
#include "core/file_collector.h"
#include "core/manifest.h"
#include "core/registry.h"
#include "core/scanner/scanner_parser.h"

#include "utils/defs.h"
#include "utils/fs.h"
#include "utils/logger.h"
#include "utils/mem.h"
#include "utils/path.h"
#include <ctype.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void parse_shebang_interpreter(const char *first_line, StringArray *shebangs)
{
    if (strncmp(first_line, "#!", 2) != 0) {
        return;
    }

    char *copy = safe_strdup(first_line + 2);
    if (!copy) {
        return;
    }

    char *saveptr = NULL;
    char *token = strtok_r(copy, " \t\r\n", &saveptr);

    while (token) {
        char *trimmed = trim_whitespace(token);
        if (trimmed[0] != '\0' && trimmed[0] != '-') {
            char *base = strrchr(trimmed, '/');
            if (base) {
                trimmed = base + 1;
            }

            if (strcmp(trimmed, "env") != 0 && strcmp(trimmed, "exec") != 0) {
                if (strlen(trimmed) > 0 && !str_array_contains(shebangs, trimmed)) {
                    str_array_append(shebangs, trimmed);
                }
                break;
            }
        }
        token = strtok_r(NULL, " \t\r\n", &saveptr);
    }
    free(copy);
}

static bool is_package_tool_executable(const char *source_dir, const char *pkg_name)
{
    char path[STOW_PATH_LARGE];
    join_path(path, sizeof(path), source_dir, pkg_name);
    join_path(path, sizeof(path), path, pkg_name);
    if (file_exists(path) && access(path, X_OK) == 0) {
        return true;
    }

    join_path(path, sizeof(path), source_dir, pkg_name);
    join_path(path, sizeof(path), path, "bin");
    join_path(path, sizeof(path), path, pkg_name);
    if (file_exists(path) && access(path, X_OK) == 0) {
        return true;
    }

    return false;
}

static bool is_valid_tool_candidate(const char *source_dir, const char *tool_name)
{
    if (!tool_name || *tool_name == '\0') {
        return false;
    }

    if (is_executable_in_path(tool_name)) {
        return true;
    }

    if (is_tool_installed_dynamic(source_dir, tool_name)) {
        return true;
    }

    if (is_package_tool_executable(source_dir, tool_name)) {
        return true;
    }

    return false;
}

static bool is_binary_file(const char *filepath)
{
    FILE *fp = fopen(filepath, "rb");
    if (!fp) {
        return false;
    }
    unsigned char buf[1024];
    size_t n = fread(buf, 1, sizeof(buf), fp);
    fclose(fp);
    for (size_t i = 0; i < n; i++) {
        if (buf[i] == 0) {
            return true;
        }
    }
    return false;
}

static void process_single_file(const char *filepath,
                                const StringArray *candidate_tools,
                                const char *pkg_name,
                                StringArray *shebangs,
                                StringArray *invocations)
{
    FILE *fp = fopen(filepath, "r");
    if (!fp) {
        return;
    }

    char *linebuf = NULL;
    size_t linecap = 0;
    ssize_t linelen;
    bool is_first_line = true;

    while ((linelen = getline(&linebuf, &linecap, fp)) != -1) {
        (void)linelen;
        if (is_first_line) {
            is_first_line = false;
            parse_shebang_interpreter(linebuf, shebangs);
        }

        if (is_scanner_comment_line(linebuf)) {
            continue;
        }

        StringArray line_tokens;
        str_array_init(&line_tokens);
        scanner_extract_line_tokens(linebuf, &line_tokens);

        for (size_t k = 0; k < line_tokens.count; k++) {
            const char *tok = line_tokens.items[k];
            if (strcmp(tok, pkg_name) == 0) {
                continue;
            }
            if (candidate_tools && str_array_contains(candidate_tools, tok)) {
                if (!str_array_contains(invocations, tok)) {
                    str_array_append(invocations, tok);
                }
            }
        }

        str_array_free(&line_tokens);
    }

    free(linebuf);
    fclose(fp);
}

typedef struct {
    const StringArray *candidate_tools;
    const char *pkg_name;
    StringArray *shebangs;
    StringArray *invocations;
    const StringArray *ignore_patterns;
} ScanFileState;

static void scan_file_cb(const char *file_path, const char *rel_path, void *user_data)
{
    ScanFileState *st = (ScanFileState *)user_data;
    if (rel_path && *rel_path != '\0') {
        if (is_path_ignored(rel_path, st->ignore_patterns)) {
            return;
        }
    }

    if (file_exists(file_path) && !is_symlink(file_path)) {
        if (!is_binary_file(file_path)) {
            process_single_file(
                file_path, st->candidate_tools, st->pkg_name, st->shebangs, st->invocations);
        }
    }
}

static void parse_tool_selections(const char *input, size_t count, bool *selected)
{
    char buf[256];
    snprintf(buf, sizeof(buf), "%s", input);
    char *trimmed = trim_whitespace(buf);

    if (trimmed[0] == '\0' || strcmp(trimmed, "all") == 0 || strcmp(trimmed, "a") == 0 ||
        strcmp(trimmed, "y") == 0 || strcmp(trimmed, "Y") == 0 || strcmp(trimmed, "yes") == 0) {
        for (size_t i = 0; i < count; i++) {
            selected[i] = true;
        }
        return;
    }

    if (strcmp(trimmed, "none") == 0 || strcmp(trimmed, "n") == 0 || strcmp(trimmed, "N") == 0 ||
        strcmp(trimmed, "no") == 0 || strcmp(trimmed, "0") == 0) {
        for (size_t i = 0; i < count; i++) {
            selected[i] = false;
        }
        return;
    }

    for (size_t i = 0; i < count; i++) {
        selected[i] = false;
    }

    char *saveptr = NULL;
    char *token = strtok_r(trimmed, " ,;\t", &saveptr);
    while (token != NULL) {
        long idx = strtol(token, NULL, 10);
        if (idx > 0 && (size_t)idx <= count) {
            selected[(size_t)idx - 1] = true;
        }
        token = strtok_r(NULL, " ,;\t", &saveptr);
    }
}

void scan_package_opts(const char *source_dir,
                       const char *pkg_name,
                       bool interactive,
                       bool write_manifest,
                       bool dry_run)
{
    char pkg_dir[STOW_PATH_LARGE];
    join_path(pkg_dir, sizeof(pkg_dir), source_dir, pkg_name);

    if (!file_exists(pkg_dir)) {
        log_error("Package directory '%s' does not exist!", pkg_name);
        return;
    }

    log_info("Recursively scanning package content in '%s' for dependencies...", pkg_name);

    StringArray candidate_tools;
    str_array_init(&candidate_tools);

    // Collect candidate tools from valid source packages and registry
    StringArray all_pkgs;
    str_array_init(&all_pkgs);
    get_all_packages(source_dir, &all_pkgs);

    for (size_t i = 0; i < all_pkgs.count; i++) {
        const char *pname = all_pkgs.items[i];
        if (is_valid_tool_candidate(source_dir, pname)) {
            if (!str_array_contains(&candidate_tools, pname)) {
                str_array_append(&candidate_tools, pname);
            }
        }
    }
    str_array_free(&all_pkgs);

    registry_get_all_tools(source_dir, &candidate_tools);

    StringArray ignore_patterns;
    str_array_init(&ignore_patterns);
    get_default_ignore_patterns(&ignore_patterns);
    parse_ignore_file_raw(source_dir, &ignore_patterns);

    StringArray shebangs;
    StringArray invocations;
    str_array_init(&shebangs);
    str_array_init(&invocations);

    ScanFileState state = {&candidate_tools, pkg_name, &shebangs, &invocations, &ignore_patterns};
    walk_dir_files(pkg_dir, "", scan_file_cb, &state);

    // If package name itself is a system tool (e.g. hyprland, zsh, tmux), ensure it is in required
    // shebangs
    if (is_executable_in_path(pkg_name) && !str_array_contains(&shebangs, pkg_name)) {
        str_array_append(&shebangs, pkg_name);
    }

    PackageManifest existing_manifest;
    manifest_init(&existing_manifest, pkg_name);
    bool manifest_exists = manifest_load(&existing_manifest, source_dir);

    StringArray new_shebangs;
    str_array_init(&new_shebangs);
    for (size_t i = 0; i < shebangs.count; i++) {
        const char *sh = shebangs.items[i];
        if (manifest_exists) {
            if (str_array_contains(&existing_manifest.required, sh) ||
                str_array_contains(&existing_manifest.optional, sh) ||
                str_array_contains(&existing_manifest.conflicts, sh)) {
                continue;
            }
        }
        str_array_append(&new_shebangs, sh);
    }

    StringArray new_invocations;
    str_array_init(&new_invocations);
    for (size_t i = 0; i < invocations.count; i++) {
        const char *inv = invocations.items[i];
        if (manifest_exists) {
            if (str_array_contains(&existing_manifest.required, inv) ||
                str_array_contains(&existing_manifest.optional, inv) ||
                str_array_contains(&existing_manifest.conflicts, inv)) {
                continue;
            }
        }
        str_array_append(&new_invocations, inv);
    }

    printf("  %sScan Results for package '%s':%s\n", COLOR_BOLD, pkg_name, COLOR_RESET);
    printf("    %sDetected Shebangs (Required):%s ", COLOR_BOLD, COLOR_RESET);
    if (new_shebangs.count > 0) {
        for (size_t i = 0; i < new_shebangs.count; i++) {
            printf("%s ", new_shebangs.items[i]);
        }
    } else {
        printf("none");
    }
    printf("\n");

    printf("    %sDetected Invocations (Optional):%s ", COLOR_BOLD, COLOR_RESET);
    if (new_invocations.count > 0) {
        for (size_t i = 0; i < new_invocations.count; i++) {
            printf("%s ", new_invocations.items[i]);
        }
    } else {
        printf("none");
    }
    printf("\n\n");

    if (dry_run) {
        log_info("[DRY-RUN] Preview scan complete for package '%s'. No changes were made to disk.",
                 pkg_name);
        str_array_free(&shebangs);
        str_array_free(&invocations);
        str_array_free(&new_shebangs);
        str_array_free(&new_invocations);
        str_array_free(&candidate_tools);
        str_array_free(&ignore_patterns);
        manifest_free(&existing_manifest);
        return;
    }

    if (!write_manifest && !interactive) {
        str_array_free(&shebangs);
        str_array_free(&invocations);
        str_array_free(&new_shebangs);
        str_array_free(&new_invocations);
        str_array_free(&candidate_tools);
        str_array_free(&ignore_patterns);
        manifest_free(&existing_manifest);
        return;
    }

    PackageManifest manifest;
    manifest_init(&manifest, pkg_name);
    if (manifest_exists) {
        manifest_free(&manifest);
        manifest = existing_manifest;
        // zero out existing_manifest so double free doesn't occur
        memset(&existing_manifest, 0, sizeof(PackageManifest));
    }
    bool modified = false;

    for (size_t i = 0; i < new_shebangs.count; i++) {
        const char *sh = new_shebangs.items[i];
        if (!str_array_contains(&manifest.required, sh)) {
            str_array_append(&manifest.required, sh);
            modified = true;
        }
    }

    if (interactive && new_invocations.count > 0) {
        printf("  %s[?] Discovered %zu new tool invocation(s) in '%s':%s\n",
               COLOR_BOLD,
               new_invocations.count,
               pkg_name,
               COLOR_RESET);

        for (size_t i = 0; i < new_invocations.count; i++) {
            printf("      %zu. [x] %s\n", i + 1, new_invocations.items[i]);
        }
        printf("  %sSelect tools to add (e.g. 'all', 'none', '1,2', or press Enter [all]): %s",
               COLOR_BOLD,
               COLOR_RESET);
        fflush(stdout);

        bool *selected = calloc(new_invocations.count, sizeof(bool));
        if (selected) {
            char response[256] = {0};
            if (fgets(response, sizeof(response), stdin)) {
                parse_tool_selections(response, new_invocations.count, selected);
            } else {
                for (size_t i = 0; i < new_invocations.count; i++) {
                    selected[i] = true;
                }
            }

            for (size_t i = 0; i < new_invocations.count; i++) {
                if (selected[i]) {
                    const char *inv = new_invocations.items[i];
                    str_array_append(&manifest.optional, inv);
                    registry_add_tool(source_dir, inv);
                    modified = true;
                }
            }
            free(selected);
        }
    } else if (write_manifest) {
        for (size_t i = 0; i < new_invocations.count; i++) {
            const char *inv = new_invocations.items[i];
            str_array_append(&manifest.optional, inv);
            modified = true;
        }
    }
    str_array_free(&new_invocations);

    if (!manifest_exists || modified) {
        manifest_save(&manifest, source_dir);
        if (!manifest_exists) {
            log_success("Auto-generated '.symdeps' manifest for '%s'", pkg_name);
        } else {
            log_info("Updated '.symdeps' manifest for '%s' with newly detected invocations",
                     pkg_name);
        }
    }
    manifest_free(&manifest);

    str_array_free(&shebangs);
    str_array_free(&invocations);
    str_array_free(&candidate_tools);
    str_array_free(&ignore_patterns);
}

void scan_package(const char *source_dir, const char *pkg_name)
{
    scan_package_opts(source_dir, pkg_name, false, true, false);
}
