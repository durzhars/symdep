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
                                  const char *distro,
                                  const StringArray *pkgs,
                                  char *cmd,
                                  size_t cmd_size,
                                  bool auto_install,
                                  bool dry_run)
{
    PkgManagerEntry mgr;
    bool resolved = pkg_manager_resolve(source_dir, NULL, &mgr, auto_install, dry_run);

    char pkg_list[2048] = {0};
    size_t offset = 0;
    for (size_t i = 0; i < pkgs->count; i++) {
        char distro_pkg[256];
        registry_get_distro_pkg(source_dir, pkgs->items[i], distro, distro_pkg, sizeof(distro_pkg));
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

static void handle_missing_dependencies(const char *source_dir,
                                        const char *distro,
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

    char install_cmd[4096];
    build_install_command(
        source_dir, distro, missing_pkgs, install_cmd, sizeof(install_cmd), auto_install, dry_run);
    printf("%sInstallation Command (%s):%s %s%s%s\n\n",
           COLOR_BOLD,
           distro,
           COLOR_RESET,
           COLOR_CYAN,
           install_cmd,
           COLOR_RESET);

    if (dry_run) {
        log_info("[DRY-RUN] Would prompt/execute installation command: %s", install_cmd);
    } else if (auto_install) {
        run_system_cmd(install_cmd);
    } else if (isatty(STDIN_FILENO)) {
        if (is_required) {
            printf("Would you like to install missing REQUIRED dependencies now? [Y/n] ");
        } else {
            printf("Would you like to install missing OPTIONAL plugins & tools now? [y/N] ");
        }
        fflush(stdout);
        int c = getchar();
        if (c != '\n' && c != EOF) {
            flush_stdin();
        }
        if (c == 'y' || c == 'Y' || (is_required && c == '\n')) {
            run_system_cmd(install_cmd);
        }
    }
}

void check_package_dependencies(const char *source_dir,
                                const char *target_pkg,
                                bool auto_install,
                                bool dry_run)
{
    char distro[64];
    get_distro_id(distro, sizeof(distro));

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

    handle_missing_dependencies(source_dir, distro, &missing_req, true, auto_install, dry_run);
    handle_missing_dependencies(source_dir, distro, &missing_opt, false, auto_install, dry_run);

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
