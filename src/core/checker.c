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

#include "core/checker.h"
#include "core/file_collector.h"
#include "core/linker.h"
#include "core/manifest.h"
#include "core/pkg_manager.h"
#include "core/registry.h"

#include "utils/defs.h"
#include "utils/env.h"
#include "utils/fs.h"
#include "utils/logger.h"
#include "utils/path.h"
#include "utils/timer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

static void flush_stdin(void)
{
    int c;
    while ((c = getchar()) != '\n' && c != EOF) {
        ;
    }
}

static void build_install_command(const char *source_dir,
                                  const StringArray *pkgs,
                                  char *cmd,
                                  size_t cmd_size,
                                  char *out_mgr_name,
                                  size_t out_mgr_name_size,
                                  bool auto_install,
                                  bool dry_run)
{
    PkgManagerEntry mgr;
    bool resolved = pkg_manager_resolve(source_dir, NULL, &mgr, auto_install, dry_run);

    const char *tag = resolved ? mgr.name : "unknown";
    if (out_mgr_name && out_mgr_name_size > 0) {
        snprintf(out_mgr_name, out_mgr_name_size, "%s", tag);
    }

    char pkg_list[2048] = {0};
    size_t offset = 0;
    for (size_t i = 0; i < pkgs->count; i++) {
        char distro_pkg[256];
        registry_get_distro_pkg(source_dir, pkgs->items[i], tag, distro_pkg, sizeof(distro_pkg));
        char escaped_pkg[512];
        escape_shell_arg(distro_pkg, escaped_pkg, sizeof(escaped_pkg));
        int written = snprintf(pkg_list + offset,
                               sizeof(pkg_list) - offset,
                               "%s%s",
                               escaped_pkg,
                               (i + 1 < pkgs->count) ? " " : "");
        if (written > 0 && (size_t)written < sizeof(pkg_list) - offset) {
            offset += (size_t)written;
        } else {
            break;
        }
    }

    if (resolved && mgr.install_cmd[0] != '\0') {
        pkg_manager_build_command(&mgr, source_dir, pkg_list, cmd, cmd_size, auto_install, dry_run);
    } else {
        snprintf(cmd, cmd_size, "Manual compilation/installation required for: %s", pkg_list);
    }
}

static void parse_item_exclusions(const char *input, const StringArray *items, bool *selected)
{
    if (!selected || !items || items->count == 0) {
        return;
    }
    size_t count = items->count;

    char buf[256];
    snprintf(buf, sizeof(buf), "%s", input ? input : "");
    char *trimmed = trim_whitespace(buf);

    // 1. Default: Install ALL
    if (trimmed[0] == '\0' || strcmp(trimmed, "all") == 0 || strcmp(trimmed, "a") == 0 ||
        strcmp(trimmed, "y") == 0 || strcmp(trimmed, "Y") == 0 || strcmp(trimmed, "yes") == 0) {
        for (size_t i = 0; i < count; i++) {
            selected[i] = true;
        }
        return;
    }

    // 2. Exclude all / Cancel installation
    if (strcmp(trimmed, "none") == 0 || strcmp(trimmed, "n") == 0 || strcmp(trimmed, "N") == 0 ||
        strcmp(trimmed, "no") == 0 || strcmp(trimmed, "cancel") == 0 || strcmp(trimmed, "0") == 0) {
        for (size_t i = 0; i < count; i++) {
            selected[i] = false;
        }
        return;
    }

    // Check if user explicitly wrote "only ..." or "include ..." (inclusion mode override)
    bool is_only_mode = false;
    if (strncmp(trimmed, "only ", 5) == 0) {
        is_only_mode = true;
        trimmed = trim_whitespace(trimmed + 5);
    } else if (strncmp(trimmed, "include ", 8) == 0) {
        is_only_mode = true;
        trimmed = trim_whitespace(trimmed + 8);
    } else if (strncmp(trimmed, "except ", 7) == 0) {
        trimmed = trim_whitespace(trimmed + 7);
    } else if (strncmp(trimmed, "exclude ", 8) == 0) {
        trimmed = trim_whitespace(trimmed + 8);
    } else if (strncmp(trimmed, "skip ", 5) == 0) {
        trimmed = trim_whitespace(trimmed + 5);
    }

    if (is_only_mode) {
        for (size_t i = 0; i < count; i++) {
            selected[i] = false;
        }
    } else {
        // Default assumption: ALL packages are selected for installation
        for (size_t i = 0; i < count; i++) {
            selected[i] = true;
        }
    }

    char *saveptr = NULL;
    char *token = strtok_r(trimmed, " ,;\t", &saveptr);
    while (token != NULL) {
        char *tok = trim_whitespace(token);
        while (*tok == '!' || *tok == '^' || *tok == '-') {
            tok++;
        }
        if (*tok != '\0') {
            char *endptr = NULL;
            long idx = strtol(tok, &endptr, 10);
            if (endptr && *endptr == '\0' && idx > 0 && (size_t)idx <= count) {
                selected[(size_t)idx - 1] = is_only_mode ? true : false;
            } else {
                for (size_t i = 0; i < count; i++) {
                    if (strcasecmp(items->items[i], tok) == 0) {
                        selected[i] = is_only_mode ? true : false;
                        break;
                    }
                }
            }
        }
        token = strtok_r(NULL, " ,;\t", &saveptr);
    }
}

static int execute_and_recover_install(const char *source_dir,
                                       const StringArray *pkgs,
                                       const char *initial_cmd,
                                       const char *mgr_name,
                                       bool auto_install,
                                       bool dry_run)
{
    if (!pkgs || pkgs->count == 0 || !initial_cmd || *initial_cmd == '\0') {
        return 0;
    }

    int ret = run_system_cmd(initial_cmd);
    if (ret == 0) {
        return 0;
    }

    log_warn("Package installation command exited with code %d.", ret);

    // Only attempt interactive recovery if running interactively and not dry-run / auto-install
    if (!isatty(STDIN_FILENO) || auto_install || dry_run) {
        return ret;
    }

    char sys_distro[64] = {0};
    get_distro_id(sys_distro, sizeof(sys_distro));
    const char *distro_tag = (sys_distro[0] != '\0') ? sys_distro : (mgr_name ? mgr_name : "unix");

    printf("\n%s[Interactive Recovery]%s Package manager encountered an error.\n",
           COLOR_YELLOW,
           COLOR_RESET);
    printf("If any package name differs on %s, enter the corrected name below (or Enter to "
           "skip).\n",
           distro_tag);

    bool any_registered = false;
    for (size_t i = 0; i < pkgs->count; i++) {
        const char *tool = pkgs->items[i];
        printf("  Enter package name for %s'%s'%s on %s [%s]: ",
               COLOR_CYAN,
               tool,
               COLOR_RESET,
               distro_tag,
               tool);
        fflush(stdout);
        char input[256] = {0};
        if (fgets(input, sizeof(input), stdin)) {
            char *trimmed = trim_whitespace(input);
            if (trimmed[0] != '\0') {
                registry_add_distro_mapping(source_dir, tool, distro_tag, trimmed);
                if (mgr_name && mgr_name[0] != '\0' && strcmp(mgr_name, distro_tag) != 0) {
                    registry_add_distro_mapping(source_dir, tool, mgr_name, trimmed);
                }
                log_success("Saved '%s@%s = %s' to symdep.registry.", tool, distro_tag, trimmed);
                any_registered = true;
            }
        }
    }

    if (any_registered) {
        char retry_cmd[4096];
        build_install_command(
            source_dir, pkgs, retry_cmd, sizeof(retry_cmd), NULL, 0, auto_install, dry_run);
        printf("\n%sRetrying installation with updated package mapping:%s %s%s%s\n",
               COLOR_BOLD,
               COLOR_RESET,
               COLOR_CYAN,
               retry_cmd,
               COLOR_RESET);
        return run_system_cmd(retry_cmd);
    }

    return ret;
}

static void handle_missing_dependencies(const char *source_dir,
                                        const StringArray *missing_pkgs,
                                        bool is_required,
                                        bool auto_install,
                                        bool dry_run)
{
    if (!missing_pkgs || missing_pkgs->count == 0) {
        return;
    }

    if (is_required) {
        log_error("Missing REQUIRED dependencies!");
    } else {
        log_warn("Missing OPTIONAL plugins & tools!");
    }

    char mgr_name[64] = "unknown";
    char install_cmd[4096];
    build_install_command(source_dir,
                          missing_pkgs,
                          install_cmd,
                          sizeof(install_cmd),
                          mgr_name,
                          sizeof(mgr_name),
                          auto_install,
                          dry_run);

    printf("%sInstallation Command (%s):%s %s%s%s\n\n",
           COLOR_BOLD,
           mgr_name,
           COLOR_RESET,
           COLOR_CYAN,
           install_cmd,
           COLOR_RESET);

    if (dry_run) {
        log_info("[DRY-RUN] Preview mode: Dependency installation skipped.");
        return;
    }

    if (auto_install) {
        log_info("Auto-installing %s dependencies via %s...",
                 is_required ? "required" : "optional",
                 mgr_name);
        execute_and_recover_install(
            source_dir, missing_pkgs, install_cmd, mgr_name, auto_install, dry_run);
        return;
    }

    if (!isatty(STDIN_FILENO)) {
        log_warn("Non-interactive environment detected. Skipping active installation of missing "
                 "dependencies.");
        return;
    }

    printf("Execute dependency installation command now? [Y/n] ");
    fflush(stdout);
    int c = getchar();
    if (c != '\n' && c != EOF) {
        flush_stdin();
    }
    if (c == 'y' || c == 'Y' || c == '\n') {
        execute_and_recover_install(
            source_dir, missing_pkgs, install_cmd, mgr_name, auto_install, dry_run);
    }
}

static void collect_package_missing_deps(const char *source_dir,
                                         const char *target_pkg,
                                         StringArray *out_missing_req,
                                         StringArray *out_missing_opt)
{
    StringArray all_pkgs;
    str_array_init(&all_pkgs);

    if (!target_pkg || strcmp(target_pkg, "all") == 0) {
        get_all_packages(source_dir, &all_pkgs);
    } else {
        str_array_append(&all_pkgs, target_pkg);
    }

    for (size_t i = 0; i < all_pkgs.count; i++) {
        const char *pkg = all_pkgs.items[i];
        PackageManifest manifest;
        manifest_init(&manifest, pkg);

        if (!manifest_load(&manifest, source_dir)) {
            manifest_free(&manifest);
            continue;
        }

        for (size_t j = 0; j < manifest.required.count; j++) {
            const char *dep = manifest.required.items[j];
            if (!is_tool_installed_dynamic(source_dir, dep)) {
                if (!str_array_contains(out_missing_req, dep)) {
                    str_array_append(out_missing_req, dep);
                }
            }
        }

        for (size_t j = 0; j < manifest.optional.count; j++) {
            const char *dep = manifest.optional.items[j];
            if (!is_tool_installed_dynamic(source_dir, dep)) {
                if (!str_array_contains(out_missing_opt, dep)) {
                    str_array_append(out_missing_opt, dep);
                }
            }
        }

        manifest_free(&manifest);
    }
    str_array_free(&all_pkgs);
}

int install_package_dependencies(const char *source_dir,
                                 const char *target_pkg,
                                 bool auto_install,
                                 bool dry_run)
{
    StringArray missing_req;
    StringArray missing_opt;
    str_array_init(&missing_req);
    str_array_init(&missing_opt);

    collect_package_missing_deps(source_dir, target_pkg, &missing_req, &missing_opt);

    if (missing_req.count == 0 && missing_opt.count == 0) {
        log_success("All dependencies for %s are already satisfied!",
                    target_pkg ? target_pkg : "all packages");
        str_array_free(&missing_req);
        str_array_free(&missing_opt);
        return 0;
    }

    char mgr_name[64] = "unknown";

    if (missing_req.count > 0) {
        char install_cmd[4096];
        build_install_command(source_dir,
                              &missing_req,
                              install_cmd,
                              sizeof(install_cmd),
                              mgr_name,
                              sizeof(mgr_name),
                              auto_install,
                              dry_run);

        printf("\n%sRequired Dependencies (%s):%s\n", COLOR_BOLD, mgr_name, COLOR_RESET);
        for (size_t i = 0; i < missing_req.count; i++) {
            printf("  %s[%zu]%s %s✗%s %s\n",
                   COLOR_CYAN,
                   i + 1,
                   COLOR_RESET,
                   COLOR_RED,
                   COLOR_RESET,
                   missing_req.items[i]);
        }
        printf("Command: %s%s%s\n\n", COLOR_CYAN, install_cmd, COLOR_RESET);

        if (dry_run) {
            log_info("[DRY-RUN] Would install required tools: %s", install_cmd);
        } else if (auto_install) {
            log_info("Installing required dependencies via %s...", mgr_name);
            execute_and_recover_install(
                source_dir, &missing_req, install_cmd, mgr_name, auto_install, dry_run);
        } else if (isatty(STDIN_FILENO)) {
            printf("Install missing REQUIRED dependencies now? [Y/n] (or enter numbers/names to "
                   "EXCLUDE): ");
            fflush(stdout);
            char response[256] = {0};
            if (fgets(response, sizeof(response), stdin)) {
                bool *selected = (bool *)calloc(missing_req.count, sizeof(bool));
                if (selected) {
                    parse_item_exclusions(response, &missing_req, selected);
                    StringArray selected_pkgs;
                    str_array_init(&selected_pkgs);
                    for (size_t i = 0; i < missing_req.count; i++) {
                        if (selected[i]) {
                            str_array_append(&selected_pkgs, missing_req.items[i]);
                        }
                    }
                    if (selected_pkgs.count > 0) {
                        char sel_cmd[4096];
                        build_install_command(source_dir,
                                              &selected_pkgs,
                                              sel_cmd,
                                              sizeof(sel_cmd),
                                              mgr_name,
                                              sizeof(mgr_name),
                                              auto_install,
                                              dry_run);
                        execute_and_recover_install(
                            source_dir, &selected_pkgs, sel_cmd, mgr_name, auto_install, dry_run);
                    }
                    str_array_free(&selected_pkgs);
                    free(selected);
                }
            }
        } else {
            log_error("Non-interactive environment detected. Pass -y / --install to confirm.");
            str_array_free(&missing_req);
            str_array_free(&missing_opt);
            return 1;
        }
    }

    if (missing_opt.count > 0) {
        char install_cmd[4096];
        build_install_command(source_dir,
                              &missing_opt,
                              install_cmd,
                              sizeof(install_cmd),
                              mgr_name,
                              sizeof(mgr_name),
                              auto_install,
                              dry_run);

        printf("\n%sOptional Plugins & Tools (%s):%s\n", COLOR_BOLD, mgr_name, COLOR_RESET);
        for (size_t i = 0; i < missing_opt.count; i++) {
            printf("  %s[%zu]%s %s⚡%s %s\n",
                   COLOR_CYAN,
                   i + 1,
                   COLOR_RESET,
                   COLOR_YELLOW,
                   COLOR_RESET,
                   missing_opt.items[i]);
        }
        printf("Command: %s%s%s\n\n", COLOR_CYAN, install_cmd, COLOR_RESET);

        if (dry_run) {
            log_info("[DRY-RUN] Would install optional tools: %s", install_cmd);
        } else if (auto_install) {
            log_info("Installing optional dependencies via %s...", mgr_name);
            execute_and_recover_install(
                source_dir, &missing_opt, install_cmd, mgr_name, auto_install, dry_run);
        } else if (isatty(STDIN_FILENO)) {
            printf("Install optional dependencies? [Y/n] (or enter numbers/names to EXCLUDE, e.g. "
                   "'1,3') [all]: ");
            fflush(stdout);
            char response[256] = {0};
            if (fgets(response, sizeof(response), stdin)) {
                bool *selected = (bool *)calloc(missing_opt.count, sizeof(bool));
                if (selected) {
                    parse_item_exclusions(response, &missing_opt, selected);
                    StringArray selected_pkgs;
                    str_array_init(&selected_pkgs);
                    for (size_t i = 0; i < missing_opt.count; i++) {
                        if (selected[i]) {
                            str_array_append(&selected_pkgs, missing_opt.items[i]);
                        }
                    }
                    if (selected_pkgs.count > 0) {
                        char sel_cmd[4096];
                        build_install_command(source_dir,
                                              &selected_pkgs,
                                              sel_cmd,
                                              sizeof(sel_cmd),
                                              mgr_name,
                                              sizeof(mgr_name),
                                              auto_install,
                                              dry_run);
                        execute_and_recover_install(
                            source_dir, &selected_pkgs, sel_cmd, mgr_name, auto_install, dry_run);
                    }
                    str_array_free(&selected_pkgs);
                    free(selected);
                }
            }
        }
    }

    str_array_free(&missing_req);
    str_array_free(&missing_opt);
    return 0;
}

void audit_package_dependencies_brief(const char *source_dir, const char *pkg_name)
{
    if (!source_dir || !pkg_name) {
        return;
    }

    PackageManifest manifest;
    manifest_init(&manifest, pkg_name);
    bool has_manifest = manifest_load(&manifest, source_dir);

    size_t req_count = has_manifest ? manifest.required.count : 0;
    size_t opt_count = has_manifest ? manifest.optional.count : 0;
    size_t total_count = req_count + opt_count;

    if (total_count == 0) {
        log_info("Dependencies: 0 declared (0 missing).");
        manifest_free(&manifest);
        return;
    }

    size_t req_missing = 0;
    for (size_t r = 0; r < req_count; r++) {
        if (!is_tool_installed_dynamic(source_dir, manifest.required.items[r])) {
            req_missing++;
        }
    }

    size_t opt_missing = 0;
    for (size_t o = 0; o < opt_count; o++) {
        if (!is_tool_installed_dynamic(source_dir, manifest.optional.items[o])) {
            opt_missing++;
        }
    }

    size_t total_missing = req_missing + opt_missing;
    size_t total_satisfied = total_count - total_missing;

    if (total_missing == 0) {
        log_info("Dependencies: %zu/%zu satisfied (%zu required, %zu optional).",
                 total_satisfied,
                 total_count,
                 req_count,
                 opt_count);
    } else {
        log_warn("Dependencies: %zu/%zu satisfied (%zu required missing, %zu optional missing). "
                 "Run 'symdep deps install %s' or 'symdep link -y' to install.",
                 total_satisfied,
                 total_count,
                 req_missing,
                 opt_missing,
                 pkg_name);
    }

    manifest_free(&manifest);
}

void check_package_dependencies(const char *source_dir,
                                const char *target_pkg,
                                bool auto_install,
                                bool dry_run)
{
    StringArray all_pkgs;
    str_array_init(&all_pkgs);
    if (target_pkg && strcmp(target_pkg, "all") != 0) {
        str_array_append(&all_pkgs, target_pkg);
    } else {
        get_all_packages(source_dir, &all_pkgs);
    }

    StringArray missing_req;
    StringArray missing_opt;
    str_array_init(&missing_req);
    str_array_init(&missing_opt);

    printf("\n%s%s=== Checking Package Dependencies & Optional Plugins ===%s\n\n",
           COLOR_CYAN,
           COLOR_BOLD,
           COLOR_RESET);
    fflush(stdout);

    for (size_t i = 0; i < all_pkgs.count; i++) {
        const char *pkg_name = all_pkgs.items[i];
        if (target_pkg && strcmp(target_pkg, "all") != 0 && strcmp(target_pkg, pkg_name) != 0) {
            continue;
        }

        PackageManifest manifest;
        manifest_init(&manifest, pkg_name);
        manifest_load(&manifest, source_dir);

        printf("%sPackage [%s]:%s\n", COLOR_BOLD, pkg_name, COLOR_RESET);

        printf("  %sRequired Dependencies:%s\n", COLOR_BOLD, COLOR_RESET);
        if (manifest.required.count > 0) {
            for (size_t r = 0; r < manifest.required.count; r++) {
                const char *tool = manifest.required.items[r];
                if (is_tool_installed_dynamic(source_dir, tool)) {
                    printf("    %s✓%s %s\n", COLOR_GREEN, COLOR_RESET, tool);
                } else {
                    printf("    %s✗%s %s %s(REQUIRED MISSING)%s\n",
                           COLOR_RED,
                           COLOR_RESET,
                           tool,
                           COLOR_RED,
                           COLOR_RESET);
                    if (!str_array_contains(&missing_req, tool)) {
                        str_array_append(&missing_req, tool);
                    }
                }
            }
        } else {
            printf("    %s✓%s none\n", COLOR_GREEN, COLOR_RESET);
        }

        printf("  %sOptional Plugins & Tools:%s\n", COLOR_BOLD, COLOR_RESET);
        if (manifest.optional.count > 0) {
            for (size_t o = 0; o < manifest.optional.count; o++) {
                const char *tool = manifest.optional.items[o];
                if (is_tool_installed_dynamic(source_dir, tool)) {
                    printf("    %s✓%s %s\n", COLOR_GREEN, COLOR_RESET, tool);
                } else {
                    printf("    %s⚡%s %s %s(optional missing)%s\n",
                           COLOR_YELLOW,
                           COLOR_RESET,
                           tool,
                           COLOR_YELLOW,
                           COLOR_RESET);
                    if (!str_array_contains(&missing_opt, tool)) {
                        str_array_append(&missing_opt, tool);
                    }
                }
            }
        } else {
            printf("    %s✓%s none\n", COLOR_GREEN, COLOR_RESET);
        }
        printf("\n");

        manifest_free(&manifest);
    }

    handle_missing_dependencies(source_dir, &missing_req, true, auto_install, dry_run);
    handle_missing_dependencies(source_dir, &missing_opt, false, auto_install, dry_run);

    if (missing_req.count == 0 && missing_opt.count == 0) {
        log_success("All required dependencies and optional plugins are installed!");
    }

    str_array_free(&missing_req);
    str_array_free(&missing_opt);
    str_array_free(&all_pkgs);
}

typedef struct {
    const char *source_dir;
    size_t broken_count;
} ScanBrokenContext;

static void scan_broken_cb(const char *symlink_path, void *user_data)
{
    ScanBrokenContext *ctx = (ScanBrokenContext *)user_data;
    char *target = read_symlink_target(symlink_path);
    if (!target || !file_exists(target)) {
        log_error("Broken symlink inside repo: %s -> %s (target missing)",
                  symlink_path,
                  target ? target : "NULL");
        ctx->broken_count++;
    }
    if (target) {
        free(target);
    }
}

typedef struct {
    const char *source_dir;
    size_t orphan_count;
} ScanOrphanContext;

static void scan_orphan_cb(const char *symlink_path, void *user_data)
{
    ScanOrphanContext *ctx = (ScanOrphanContext *)user_data;
    char *target = read_symlink_target(symlink_path);
    if (target && is_path_prefix(target, ctx->source_dir)) {
        const char *rel = target + strlen(ctx->source_dir);
        if (*rel == '/') {
            rel++;
        }
        const char *slash = strchr(rel, '/');
        if (slash) {
            size_t pkg_len = (size_t)(slash - rel);
            char pkg_name[256];
            if (pkg_len < sizeof(pkg_name)) {
                strncpy(pkg_name, rel, pkg_len);
                pkg_name[pkg_len] = '\0';
                char pkg_dir[STOW_PATH_LARGE];
                join_path(pkg_dir, sizeof(pkg_dir), ctx->source_dir, pkg_name);
                if (!is_dir(pkg_dir) || !file_exists(target)) {
                    log_warn("Unmanaged / Orphan symlink: %s -> %s (target file does not exist)",
                             symlink_path,
                             target);
                    ctx->orphan_count++;
                }
            }
        }
    }
    if (target) {
        free(target);
    }
}

void check_symlink_health(const char *source_dir, const char *target_dir)
{
    printf("\n%s%s=== Scanning Symlink Health & Integrity ===%s\n\n",
           COLOR_CYAN,
           COLOR_BOLD,
           COLOR_RESET);

    log_info("1. Scanning repo directory '%s' for broken symlinks...", source_dir);
    ScanBrokenContext broken_ctx = {source_dir, 0};
    walk_dir_symlinks(source_dir, 1, 6, scan_broken_cb, &broken_ctx);

    if (broken_ctx.broken_count == 0) {
        log_success("No broken symlinks found inside source repo!");
    } else {
        log_error("Found %zu broken symlink(s) inside source repo!", broken_ctx.broken_count);
    }

    printf("\n");
    log_info("2. Scanning target directory '%s' for unmanaged/orphan symlinks...", target_dir);
    ScanOrphanContext orphan_ctx = {source_dir, 0};
    walk_target_dir_symlinks_targeted(target_dir, source_dir, NULL, scan_orphan_cb, &orphan_ctx);

    if (orphan_ctx.orphan_count == 0) {
        log_success("No unmanaged / orphan symlinks pointing to source repo!");
    } else {
        log_warn("Found %zu unmanaged / orphan symlink(s) in target directory!",
                 orphan_ctx.orphan_count);
    }
    printf("\n");
}
