/*
 * Symlink & Dependency Manager (symdep)
 * Linker High-Level Operations Submodule (link, unlink, relink, link_all)
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

static void native_link_cb(const char *file_path, const char *rel_path, void *user_data)
{
    (void)file_path;
    PackageContext *ctx = (PackageContext *)user_data;

    if (is_path_ignored(rel_path, &ctx->raw_ignores)) {
        return;
    }

    char target_path[STOW_PATH_LARGE];
    join_path(target_path, sizeof(target_path), ctx->target_dir, rel_path);

    char pkg_file_path[STOW_PATH_LARGE];
    join_path(pkg_file_path, sizeof(pkg_file_path), ctx->pkg_dir, rel_path);

    char real_pkg_file_path[STOW_PATH_LARGE];
    if (is_symlink(pkg_file_path)) {
        if (realpath(pkg_file_path, real_pkg_file_path) == NULL) {
            snprintf(real_pkg_file_path, sizeof(real_pkg_file_path), "%s", pkg_file_path);
        }
    } else {
        snprintf(real_pkg_file_path, sizeof(real_pkg_file_path), "%s", pkg_file_path);
    }

    if (is_symlink(target_path)) {
        if (is_symlink_pointing_to(target_path, pkg_file_path, real_pkg_file_path)) {
            return;
        }
        unlink(target_path);
    } else if (file_exists(target_path)) {
        char backup_path[STOW_PATH_HUGE];
        build_unique_backup_path(target_path, backup_path, sizeof(backup_path));

        log_warn("Conflict! Backing up file: %s -> %s", target_path, backup_path);
        if (rename(target_path, backup_path) != 0) {
            log_error("Failed to backup conflicting file: %s: %s", target_path, strerror(errno));
            ctx->errors++;
            return;
        }
    }

    char parent_dir[STOW_PATH_LARGE];
    snprintf(parent_dir, sizeof(parent_dir), "%s", target_path);
    char *last_slash = strrchr(parent_dir, '/');
    if (last_slash) {
        *last_slash = '\0';
        mkdir_p(parent_dir, 0755);
    }

    PerfTimer op_timer = perf_timer_start("symlink");
    int sym_res = symlink(pkg_file_path, target_path);
    double op_us = perf_timer_elapsed_us(&op_timer);

    if (sym_res == 0) {
        if (perf_profiler_is_enabled()) {
            if (op_us >= 1000.0) {
                log_info("[PERF] LINK: %s => %s (completed in %.2f ms)",
                         rel_path,
                         pkg_file_path,
                         op_us / 1000.0);
            } else {
                log_info(
                    "[PERF] LINK: %s => %s (completed in %.0f us)", rel_path, pkg_file_path, op_us);
            }
        } else {
            log_info("LINK: %s => %s", rel_path, pkg_file_path);
        }
        ctx->created_count++;
    } else {
        log_error(
            "Failed to create symlink: %s -> %s: %s", target_path, pkg_file_path, strerror(errno));
        ctx->errors++;
    }
}

static void native_unlink_cb(const char *file_path, const char *rel_path, void *user_data)
{
    (void)file_path;
    PackageContext *ctx = (PackageContext *)user_data;

    if (is_path_ignored(rel_path, &ctx->raw_ignores)) {
        return;
    }

    char target_path[STOW_PATH_LARGE];
    join_path(target_path, sizeof(target_path), ctx->target_dir, rel_path);

    if (!is_symlink(target_path)) {
        return;
    }

    char pkg_file_path[STOW_PATH_LARGE];
    join_path(pkg_file_path, sizeof(pkg_file_path), ctx->pkg_dir, rel_path);

    if (is_symlink_pointing_to(target_path, pkg_file_path, ctx->real_pkg_dir)) {
        if (ctx->dry_run) {
            log_info("[DRY-RUN] Would unlink symlink: %s", target_path);
            ctx->unlinked_count++;
        } else {
            PerfTimer op_timer = perf_timer_start("unlink");
            int unl_res = unlink(target_path);
            double op_us = perf_timer_elapsed_us(&op_timer);

            if (unl_res == 0) {
                if (perf_profiler_is_enabled()) {
                    if (op_us >= 1000.0) {
                        log_info(
                            "[PERF] UNLINK: %s (completed in %.2f ms)", rel_path, op_us / 1000.0);
                    } else {
                        log_info("[PERF] UNLINK: %s (completed in %.0f us)", rel_path, op_us);
                    }
                } else {
                    log_info("UNLINK: %s", rel_path);
                }
                ctx->unlinked_count++;

                char parent[STOW_PATH_LARGE];
                size_t target_len = strlen(ctx->target_dir);
                size_t path_len = strlen(target_path);
                if (path_len < sizeof(parent)) {
                    memcpy(parent, target_path, path_len + 1);
                } else {
                    snprintf(parent, sizeof(parent), "%s", target_path);
                }
                char *last_slash = strrchr(parent, '/');
                if (last_slash) {
                    *last_slash = '\0';
                }
                while (strlen(parent) > target_len && is_path_prefix(parent, ctx->target_dir)) {
                    if (rmdir(parent) != 0) {
                        break;
                    }
                    last_slash = strrchr(parent, '/');
                    if (last_slash) {
                        *last_slash = '\0';
                    } else {
                        break;
                    }
                }
            } else {
                log_error("Failed to unlink symlink: %s: %s", target_path, strerror(errno));
                ctx->errors++;
            }
        }
    }
}

int link_package(const char *source_dir,
                 const char *target_dir,
                 const char *pkg_name,
                 bool auto_install,
                 bool dry_run)
{
    PerfTimer t_stow = perf_timer_start("link_package");

    if (dry_run) {
        log_info("[DRY-RUN] Previewing link operation for package '%s'...", pkg_name);
    } else {
        log_info("Linking package '%s'...", pkg_name);
    }

    PackageContext pctx;
    if (!package_context_init(&pctx, source_dir, target_dir, pkg_name, auto_install, dry_run)) {
        log_error("Package directory does not exist: %s/%s", source_dir, pkg_name);
        perf_timer_log(&t_stow);
        return -1;
    }

    PerfTimer t_chk = perf_timer_start("check_package_dependencies");
    check_package_dependencies(source_dir, pkg_name, auto_install, dry_run);
    perf_timer_log(&t_chk);

    PerfTimer t_excl = perf_timer_start("handle_mutual_exclusions");
    handle_mutual_exclusions(target_dir, source_dir, pkg_name, dry_run);
    perf_timer_log(&t_excl);

    PerfTimer t_conf = perf_timer_start("handle_dynamic_package_conflicts");
    handle_dynamic_package_conflicts(target_dir, source_dir, pkg_name, &pctx.pkg_files, dry_run);
    perf_timer_log(&t_conf);

    PerfTimer t_unf = perf_timer_start("unfold_directory_symlinks");
    unfold_directory_symlinks(target_dir, source_dir, &pctx.pkg_files, dry_run);
    perf_timer_log(&t_unf);

    PerfTimer t_prep = perf_timer_start("prepare_target_conflicts");
    prepare_target_conflicts(target_dir, source_dir, pkg_name, &pctx.pkg_files, dry_run);
    perf_timer_log(&t_prep);

    if (dry_run) {
        log_success(
            "[DRY-RUN] Dry run / Diff complete for package '%s'. No changes were made to disk.",
            pkg_name);
        package_context_free(&pctx);
        perf_timer_log(&t_stow);
        return 0;
    }

    for (size_t i = 0; i < pctx.pkg_files.count; i++) {
        native_link_cb(
            pctx.pkg_files.entries[i].full_path, pctx.pkg_files.entries[i].rel_path, &pctx);
    }

    int result = (pctx.errors == 0) ? 0 : -1;
    if (result == 0) {
        log_success("Successfully linked package '%s'!", pkg_name);
    } else {
        log_error("Failed to link package '%s'!", pkg_name);
    }

    package_context_free(&pctx);
    perf_timer_log(&t_stow);
    return result;
}

int unlink_package(const char *source_dir,
                   const char *target_dir,
                   const char *pkg_name,
                   bool dry_run)
{
    if (dry_run) {
        log_info("[DRY-RUN] Previewing unlink operation for package '%s'...", pkg_name);
    } else {
        log_info("Unlinking package '%s'...", pkg_name);
    }

    PackageContext pctx;
    if (!package_context_init(&pctx, source_dir, target_dir, pkg_name, false, dry_run)) {
        log_error("Package directory does not exist: %s/%s", source_dir, pkg_name);
        return -1;
    }

    for (size_t i = 0; i < pctx.pkg_files.count; i++) {
        native_unlink_cb(
            pctx.pkg_files.entries[i].full_path, pctx.pkg_files.entries[i].rel_path, &pctx);
    }

    int result = (pctx.errors == 0) ? 0 : -1;
    if (dry_run) {
        log_success(
            "[DRY-RUN] Dry run / Diff complete for package '%s'. No changes were made to disk.",
            pkg_name);
        result = 0;
    } else if (result == 0) {
        log_success("Successfully unlinked package '%s'!", pkg_name);
    } else {
        log_error("Failed to unlink package '%s'!", pkg_name);
    }

    package_context_free(&pctx);
    return result;
}

int relink_package(const char *source_dir,
                   const char *target_dir,
                   const char *pkg_name,
                   bool auto_install,
                   bool dry_run)
{
    if (dry_run) {
        log_info("[DRY-RUN] Relinking package '%s'...", pkg_name);
    } else {
        log_info("Relinking package '%s'...", pkg_name);
    }

    check_package_dependencies(source_dir, pkg_name, auto_install, dry_run);
    handle_mutual_exclusions(target_dir, source_dir, pkg_name, dry_run);
    handle_dynamic_package_conflicts(target_dir, source_dir, pkg_name, NULL, dry_run);
    unfold_directory_symlinks(target_dir, source_dir, NULL, dry_run);

    if (dry_run) {
        unlink_package(source_dir, target_dir, pkg_name, true);
        prepare_target_conflicts(target_dir, source_dir, pkg_name, NULL, true);
        log_success(
            "[DRY-RUN] Dry run / Diff complete for package '%s'. No changes were made to disk.",
            pkg_name);
        return 0;
    }

    unlink_package(source_dir, target_dir, pkg_name, false);
    prepare_target_conflicts(target_dir, source_dir, pkg_name, NULL, dry_run);

    PackageContext pctx;
    if (!package_context_init(&pctx, source_dir, target_dir, pkg_name, auto_install, dry_run)) {
        log_error("Package directory does not exist: %s/%s", source_dir, pkg_name);
        return -1;
    }

    for (size_t i = 0; i < pctx.pkg_files.count; i++) {
        native_link_cb(
            pctx.pkg_files.entries[i].full_path, pctx.pkg_files.entries[i].rel_path, &pctx);
    }

    int result = (pctx.errors == 0) ? 0 : -1;
    if (result == 0) {
        log_success("Successfully relinked package '%s'!", pkg_name);
    } else {
        log_error("Failed to relink package '%s'!", pkg_name);
    }

    package_context_free(&pctx);
    return result;
}

void link_all_packages(const char *source_dir,
                       const char *target_dir,
                       bool auto_install,
                       bool dry_run)
{
    StringArray packages;
    str_array_init(&packages);
    get_all_packages(source_dir, &packages);

    if (dry_run) {
        log_info("[DRY-RUN] Previewing link operation for ALL packages (%zu found)...",
                 packages.count);
    } else {
        log_info("Linking ALL packages (%zu found)...", packages.count);
    }

    for (size_t i = 0; i < packages.count; i++) {
        link_package(source_dir, target_dir, packages.items[i], auto_install, dry_run);
    }

    str_array_free(&packages);
}
