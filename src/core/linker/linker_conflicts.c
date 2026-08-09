/*
 * Symlink & Dependency Manager (symdep)
 * Linker Conflict Detection & Resolution Submodule
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
    const char *source_dir;
    const char *pkg_name;
    const char *pkg_dir;
    const char *real_pkg_dir;
    const StringArray *raw_ignores;
    bool dry_run;
    size_t new_links;
    size_t replaced_links;
    size_t backups;
    size_t unchanged;
} ConflictContext;

static void prepare_conflict_cb(const char *file_path, const char *rel_path, void *user_data)
{
    (void)file_path;
    ConflictContext *ctx = (ConflictContext *)user_data;

    if (is_path_ignored(rel_path, ctx->raw_ignores)) {
        return;
    }

    char target_path[STOW_PATH_LARGE];
    join_path(target_path, sizeof(target_path), ctx->target_dir, rel_path);

    char pkg_file_path[STOW_PATH_LARGE];
    join_path(pkg_file_path, sizeof(pkg_file_path), ctx->pkg_dir, rel_path);

    char real_pkg_file_path[STOW_PATH_LARGE];
    join_path(real_pkg_file_path, sizeof(real_pkg_file_path), ctx->real_pkg_dir, rel_path);

    if (is_symlink(target_path)) {
        if (is_symlink_pointing_to(target_path, pkg_file_path, real_pkg_file_path)) {
            ctx->unchanged++;
        } else {
            char owner_pkg[256];
            if (get_symlink_owner_package(
                    target_path, ctx->source_dir, owner_pkg, sizeof(owner_pkg))) {
                if (ctx->dry_run) {
                    log_warn("[DRY-RUN] Conflict! Target '%s' is linked by package '%s'. Would replace with '%s'.",
                             rel_path,
                             owner_pkg,
                             ctx->pkg_name);
                } else {
                    log_warn("Conflict! Target '%s' is linked by package '%s'. Replacing with '%s'...",
                             rel_path,
                             owner_pkg,
                             ctx->pkg_name);
                }
            } else {
                if (ctx->dry_run) {
                    log_info("[DRY-RUN] Would replace symlink: %s -> %s", target_path, pkg_file_path);
                }
            }
            ctx->replaced_links++;
        }
    } else if (file_exists(target_path)) {
        char backup_path[STOW_PATH_HUGE];
        build_unique_backup_path(target_path, backup_path, sizeof(backup_path));

        if (ctx->dry_run) {
            log_warn("[DRY-RUN] Conflict! Would backup file: %s -> %s", target_path, backup_path);
        }
        ctx->backups++;
    } else {
        if (ctx->dry_run) {
            log_info("[DRY-RUN] Would create symlink: %s -> %s", target_path, pkg_file_path);
        }
        ctx->new_links++;
    }
}

void prepare_target_conflicts(const char *target_dir,
                              const char *source_dir,
                              const char *pkg_name,
                              const PkgFileList *pkg_files_param,
                              bool dry_run)
{
    char pkg_dir[STOW_PATH_LARGE];
    join_path(pkg_dir, sizeof(pkg_dir), source_dir, pkg_name);

    char real_pkg_dir[STOW_PATH_LARGE];
    if (is_symlink(pkg_dir)) {
        if (realpath(pkg_dir, real_pkg_dir) == NULL) {
            snprintf(real_pkg_dir, sizeof(real_pkg_dir), "%s", pkg_dir);
        }
    } else {
        snprintf(real_pkg_dir, sizeof(real_pkg_dir), "%s", pkg_dir);
    }

    if (dry_run) {
        log_info("[DRY-RUN] Previewing target paths & conflicts for package '%s'...", pkg_name);
    }

    StringArray raw_ignores;
    init_package_ignores(&raw_ignores, source_dir, pkg_dir);

    PkgFileList local_files;
    const PkgFileList *pkg_files = pkg_files_param;
    if (!pkg_files) {
        collect_package_files(pkg_dir, &raw_ignores, &local_files);
        pkg_files = &local_files;
    }

    ConflictContext ctx = {target_dir,
                           source_dir,
                           pkg_name,
                           pkg_dir,
                           real_pkg_dir,
                           &raw_ignores,
                           dry_run,
                           0,
                           0,
                           0,
                           0};
    for (size_t i = 0; i < pkg_files->count; i++) {
        prepare_conflict_cb(pkg_files->entries[i].full_path, pkg_files->entries[i].rel_path, &ctx);
    }

    if (dry_run) {
        log_info("[DRY-RUN] Summary for '%s': %zu new symlink(s), %zu replaced, %zu backed up, %zu unchanged.",
                 pkg_name,
                 ctx.new_links,
                 ctx.replaced_links,
                 ctx.backups,
                 ctx.unchanged);
    }

    if (!pkg_files_param) {
        pkg_file_list_free(&local_files);
    }
    str_array_free(&raw_ignores);
}

void handle_mutual_exclusions(const char *target_dir,
                              const char *source_dir,
                              const char *pkg_name,
                              bool dry_run)
{
    PackageManifest manifest;
    manifest_init(&manifest, pkg_name);
    manifest_load(&manifest, source_dir);

    for (size_t i = 0; i < manifest.conflicts.count; i++) {
        const char *conflict_pkg = manifest.conflicts.items[i];
        if (is_package_linked(target_dir, source_dir, conflict_pkg)) {
            if (dry_run) {
                log_warn("[DRY-RUN] Would unlink manifest-conflicting package '%s' before linking '%s'.",
                         conflict_pkg,
                         pkg_name);
            } else {
                log_warn("Unlinking manifest-conflicting package '%s' before linking '%s'...",
                         conflict_pkg,
                         pkg_name);
                unlink_package(source_dir, target_dir, conflict_pkg, dry_run);
            }
        }
    }

    manifest_free(&manifest);
}

typedef struct {
    const char *target_dir;
    const char *source_dir;
    const char *current_pkg;
    const StringArray *raw_ignores;
    StringArray conflicting_pkgs;
} DetectConflictsContext;

static void detect_conflicts_cb(const char *file_path, const char *rel_path, void *user_data)
{
    (void)file_path;
    DetectConflictsContext *ctx = (DetectConflictsContext *)user_data;

    if (is_path_ignored(rel_path, ctx->raw_ignores)) {
        return;
    }

    char target_path[STOW_PATH_LARGE];
    join_path(target_path, sizeof(target_path), ctx->target_dir, rel_path);

    if (is_symlink(target_path)) {
        char owner_pkg[256];
        if (get_symlink_owner_package(target_path, ctx->source_dir, owner_pkg, sizeof(owner_pkg))) {
            if (strcmp(owner_pkg, ctx->current_pkg) != 0) {
                if (!str_array_contains(&ctx->conflicting_pkgs, owner_pkg)) {
                    str_array_append(&ctx->conflicting_pkgs, owner_pkg);
                }
            }
        }
    }
}

void handle_dynamic_package_conflicts(const char *target_dir,
                                       const char *source_dir,
                                       const char *pkg_name,
                                       const PkgFileList *pkg_files_param,
                                       bool dry_run)
{
    char pkg_dir[STOW_PATH_LARGE];
    join_path(pkg_dir, sizeof(pkg_dir), source_dir, pkg_name);

    if (!is_dir(pkg_dir)) {
        return;
    }

    StringArray raw_ignores;
    init_package_ignores(&raw_ignores, source_dir, pkg_dir);

    DetectConflictsContext ctx;
    ctx.target_dir = target_dir;
    ctx.source_dir = source_dir;
    ctx.current_pkg = pkg_name;
    ctx.raw_ignores = &raw_ignores;
    str_array_init(&ctx.conflicting_pkgs);

    PkgFileList local_files;
    const PkgFileList *pkg_files = pkg_files_param;
    if (!pkg_files) {
        collect_package_files(pkg_dir, &raw_ignores, &local_files);
        pkg_files = &local_files;
    }

    for (size_t i = 0; i < pkg_files->count; i++) {
        detect_conflicts_cb(pkg_files->entries[i].full_path, pkg_files->entries[i].rel_path, &ctx);
    }

    if (!pkg_files_param) {
        pkg_file_list_free(&local_files);
    }

    for (size_t i = 0; i < ctx.conflicting_pkgs.count; i++) {
        const char *conflict_pkg = ctx.conflicting_pkgs.items[i];
        if (dry_run) {
            log_warn("[DRY-RUN] Package conflict detected! Package '%s' collides with linked package '%s'. Would unlink '%s' before linking '%s'.",
                     pkg_name,
                     conflict_pkg,
                     conflict_pkg,
                     pkg_name);
        } else {
            log_warn("Package conflict detected! Package '%s' collides with linked package '%s'. Unlinking '%s' before linking '%s'...",
                     pkg_name,
                     conflict_pkg,
                     conflict_pkg,
                     pkg_name);
            unlink_package(source_dir, target_dir, conflict_pkg, false);
        }
    }

    str_array_free(&ctx.conflicting_pkgs);
    str_array_free(&raw_ignores);
}
