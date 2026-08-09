/*
 * Symlink & Dependency Manager (symdep)
 * Linker Status Inspection Submodule
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

#include "core/linker/internal.h"

typedef struct {
    const char *target_dir;
    const char *pkg_dir;
    const char *real_pkg_dir;
    const StringArray *raw_ignores;
    size_t total_files;
    size_t stowed_files;
} CheckLinkedStatsContext;

static void check_linked_stats_cb(const char *file_path, const char *rel_path, void *user_data)
{
    (void)file_path;
    CheckLinkedStatsContext *ctx = (CheckLinkedStatsContext *)user_data;

    ctx->total_files++;

    char target_path[STOW_PATH_LARGE];
    join_path(target_path, sizeof(target_path), ctx->target_dir, rel_path);

    char pkg_file_path[STOW_PATH_LARGE];
    join_path(pkg_file_path, sizeof(pkg_file_path), ctx->pkg_dir, rel_path);

    char real_pkg_file_path[STOW_PATH_LARGE];
    join_path(real_pkg_file_path, sizeof(real_pkg_file_path), ctx->real_pkg_dir, rel_path);

    if (is_symlink(target_path) &&
        is_symlink_pointing_to(target_path, pkg_file_path, real_pkg_file_path)) {
        ctx->stowed_files++;
    }
}

LinkStatus
get_package_link_status(const char *target_dir, const char *source_dir, const char *pkg_name)
{
    char pkg_dir[STOW_PATH_LARGE];
    join_path(pkg_dir, sizeof(pkg_dir), source_dir, pkg_name);

    if (!is_dir(pkg_dir)) {
        return LINK_STATUS_UNLINKED;
    }

    char real_pkg_dir[STOW_PATH_LARGE];
    if (is_symlink(pkg_dir)) {
        if (realpath(pkg_dir, real_pkg_dir) == NULL) {
            snprintf(real_pkg_dir, sizeof(real_pkg_dir), "%s", pkg_dir);
        }
    } else {
        snprintf(real_pkg_dir, sizeof(real_pkg_dir), "%s", pkg_dir);
    }

    StringArray raw_ignores;
    init_package_ignores(&raw_ignores, source_dir, pkg_dir);

    PkgFileList pkg_files;
    collect_package_files(pkg_dir, &raw_ignores, &pkg_files);

    CheckLinkedStatsContext ctx = {target_dir, pkg_dir, real_pkg_dir, &raw_ignores, 0, 0};
    for (size_t i = 0; i < pkg_files.count; i++) {
        check_linked_stats_cb(pkg_files.entries[i].full_path, pkg_files.entries[i].rel_path, &ctx);
    }

    pkg_file_list_free(&pkg_files);
    str_array_free(&raw_ignores);

    if (ctx.total_files == 0) {
        return LINK_STATUS_UNLINKED;
    }
    if (ctx.stowed_files == ctx.total_files) {
        return LINK_STATUS_LINKED;
    }
    if (ctx.stowed_files > 0) {
        return LINK_STATUS_PARTIAL;
    }
    return LINK_STATUS_UNLINKED;
}

bool is_package_linked(const char *target_dir, const char *source_dir, const char *pkg_name)
{
    char pkg_dir[STOW_PATH_LARGE];
    join_path(pkg_dir, sizeof(pkg_dir), source_dir, pkg_name);

    if (!is_dir(pkg_dir)) {
        return false;
    }

    StringArray raw_ignores;
    init_package_ignores(&raw_ignores, source_dir, pkg_dir);

    PkgFileList pkg_files;
    collect_package_files(pkg_dir, &raw_ignores, &pkg_files);

    bool linked = false;
    for (size_t i = 0; i < pkg_files.count; i++) {
        char target_path[STOW_PATH_LARGE];
        join_path(target_path, sizeof(target_path), target_dir, pkg_files.entries[i].rel_path);

        if (is_symlink(target_path) &&
            is_symlink_pointing_to(target_path, pkg_files.entries[i].full_path, NULL)) {
            linked = true;
            break;
        }
    }

    pkg_file_list_free(&pkg_files);
    str_array_free(&raw_ignores);
    return linked;
}

void list_packages_status(const char *source_dir, const char *target_dir)
{
    StringArray packages;
    str_array_init(&packages);
    get_all_packages(source_dir, &packages);

    printf("\n%s%s=== Package Symlink Status ===%s\n\n", COLOR_CYAN, COLOR_BOLD, COLOR_RESET);

    for (size_t i = 0; i < packages.count; i++) {
        const char *pkg = packages.items[i];
        LinkStatus status = get_package_link_status(target_dir, source_dir, pkg);
        if (status == LINK_STATUS_LINKED) {
            printf("  %s[LINKED]%s   %s\n", COLOR_GREEN, COLOR_RESET, pkg);
        } else if (status == LINK_STATUS_PARTIAL) {
            printf("  %s[PARTIAL]%s  %s\n", COLOR_YELLOW, COLOR_RESET, pkg);
        } else {
            printf("  %s[UNLINKED]%s %s\n", COLOR_RED, COLOR_RESET, pkg);
        }
    }
    printf("\n");

    str_array_free(&packages);
}
