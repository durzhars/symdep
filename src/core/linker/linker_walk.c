/*
 * Symlink & Dependency Manager (symdep)
 * Linker Target Directory Walking & Unfolding Submodule
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
    StrSet visited_paths;
    WalkSymlinkCallback cb;
    void *user_data;
} TargetedWalkState;

static void check_and_notify_symlink(const char *path, TargetedWalkState *state)
{
    if (path && *path != '\0' && str_set_add(&state->visited_paths, path) && is_symlink(path)) {
        state->cb(path, state->user_data);
    }
}

static void add_rel_path_and_parents(StrSet *set, const char *rel_path)
{
    if (!rel_path || *rel_path == '\0') {
        return;
    }
    const char *p = rel_path;
    while (*p != '\0') {
        if (*p == '/') {
            size_t len = (size_t)(p - rel_path);
            if (len > 0) {
                char parent[STOW_PATH_LARGE];
                if (len < sizeof(parent)) {
                    memcpy(parent, rel_path, len);
                    parent[len] = '\0';
                    str_set_add(set, parent);
                }
            }
        }
        p++;
    }
    str_set_add(set, rel_path);
}

void walk_target_dir_symlinks_targeted(const char *target_dir,
                                       const char *source_dir,
                                       const PkgFileList *pkg_files,
                                       WalkSymlinkCallback cb,
                                       void *user_data)
{
    if (!target_dir || !source_dir || !cb) {
        return;
    }

    PerfTimer t = perf_timer_start("walk_target_dir_symlinks_targeted");
    TargetedWalkState state;
    str_set_init(&state.visited_paths);
    state.cb = cb;
    state.user_data = user_data;

    // 1. Scan top-level entries directly inside target_dir (depth 1)
    DIR *dir = opendir(target_dir);
    if (dir) {
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            const char *name = entry->d_name;
            if (name[0] == '.' && (name[1] == '\0' || (name[1] == '.' && name[2] == '\0'))) {
                continue;
            }
            if (entry->d_type == DT_LNK || entry->d_type == DT_UNKNOWN) {
                char path[STOW_PATH_LARGE];
                join_path(path, sizeof(path), target_dir, name);
                check_and_notify_symlink(path, &state);
            }
        }
        closedir(dir);
    }

    // 2. Collect relative paths across target package or all packages in source_dir
    StrSet rel_paths;
    str_set_init(&rel_paths);

    if (pkg_files && pkg_files->count > 0) {
        for (size_t k = 0; k < pkg_files->count; k++) {
            add_rel_path_and_parents(&rel_paths, pkg_files->entries[k].rel_path);
        }
    } else {
        StringArray packages;
        str_array_init(&packages);
        get_all_packages(source_dir, &packages);

        for (size_t i = 0; i < packages.count; i++) {
            char pkg_dir[STOW_PATH_LARGE];
            join_path(pkg_dir, sizeof(pkg_dir), source_dir, packages.items[i]);
            if (is_dir(pkg_dir)) {
                StringArray raw_ignores;
                init_package_ignores(&raw_ignores, source_dir, pkg_dir);

                PkgFileList fetched_files;
                collect_package_files(pkg_dir, &raw_ignores, &fetched_files);
                for (size_t k = 0; k < fetched_files.count; k++) {
                    add_rel_path_and_parents(&rel_paths, fetched_files.entries[k].rel_path);
                }
                pkg_file_list_free(&fetched_files);
                str_array_free(&raw_ignores);
            }
        }
        str_array_free(&packages);
    }

    // 3. For each relative path, check target_dir/rel_path and its immediate children if it's a
    // directory
    for (size_t i = 0; i < rel_paths.capacity; i++) {
        if (!rel_paths.keys || !rel_paths.keys[i]) {
            continue;
        }
        const char *rel = rel_paths.keys[i];
        char target_path[STOW_PATH_LARGE];
        join_path(target_path, sizeof(target_path), target_dir, rel);

        struct stat st;
        if (lstat(target_path, &st) == 0) {
            if (S_ISLNK(st.st_mode)) {
                if (str_set_add(&state.visited_paths, target_path)) {
                    state.cb(target_path, state.user_data);
                }
            } else if (S_ISDIR(st.st_mode)) {
                DIR *tdir = opendir(target_path);
                if (tdir) {
                    struct dirent *entry;
                    while ((entry = readdir(tdir)) != NULL) {
                        const char *name = entry->d_name;
                        if (name[0] == '.' &&
                            (name[1] == '\0' || (name[1] == '.' && name[2] == '\0'))) {
                            continue;
                        }
                        if (entry->d_type == DT_LNK) {
                            char child_path[STOW_PATH_LARGE];
                            join_path(child_path, sizeof(child_path), target_path, name);
                            if (str_set_add(&state.visited_paths, child_path)) {
                                state.cb(child_path, state.user_data);
                            }
                        } else if (entry->d_type == DT_UNKNOWN) {
                            char child_path[STOW_PATH_LARGE];
                            join_path(child_path, sizeof(child_path), target_path, name);
                            check_and_notify_symlink(child_path, &state);
                        }
                    }
                    closedir(tdir);
                }
            }
        }
    }

    str_set_free(&rel_paths);
    str_set_free(&state.visited_paths);
    perf_timer_log(&t);
}

typedef struct {
    const char *source_dir;
    bool dry_run;
    int unfolded_count;
} UnfoldContext;

static void unfold_symlink_cb(const char *symlink_path, void *user_data)
{
    UnfoldContext *ctx = (UnfoldContext *)user_data;
    if (!is_dir(symlink_path)) {
        return;
    }

    char *target = read_symlink_target(symlink_path);
    if (!target) {
        return;
    }

    if (is_path_prefix(target, ctx->source_dir)) {
        if (ctx->dry_run) {
            log_warn("[DRY-RUN] Would unfold directory symlink: %s -> %s", symlink_path, target);
        } else {
            log_warn("Unfolding directory symlink: %s -> %s", symlink_path, target);

            struct stat target_st;
            mode_t target_mode = 0755;
            if (stat(target, &target_st) == 0) {
                target_mode = target_st.st_mode & 0777;
            }

            char tmp_dir[STOW_PATH_HUGE];
            snprintf(tmp_dir, sizeof(tmp_dir), "%s.unfold_tmp_%d", symlink_path, (int)getpid());

            register_temp_path(tmp_dir);

            if (mkdir_p(tmp_dir, target_mode) == 0) {
                DIR *tdir = opendir(target);
                bool copy_success = true;

                if (tdir) {
                    struct dirent *entry;
                    while ((entry = readdir(tdir)) != NULL) {
                        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                            continue;
                        }
                        char child_src[STOW_PATH_LARGE];
                        char child_dst[STOW_PATH_LARGE];
                        join_path(child_src, sizeof(child_src), target, entry->d_name);
                        join_path(child_dst, sizeof(child_dst), tmp_dir, entry->d_name);

                        if (symlink(child_src, child_dst) != 0) {
                            log_error("Failed to symlink unfolded child '%s'", child_dst);
                            copy_success = false;
                            break;
                        }
                    }
                    closedir(tdir);

                    if (copy_success) {
                        if (rename(tmp_dir, symlink_path) != 0) {
                            unlink(symlink_path);
                            if (rename(tmp_dir, symlink_path) != 0) {
                                log_error("Failed to rename unfolded directory '%s': %s",
                                          tmp_dir,
                                          strerror(errno));
                            }
                        }
                    } else {
                        cleanup_temp_dir_contents(tmp_dir);
                        rmdir(tmp_dir);
                    }
                } else {
                    rmdir(tmp_dir);
                }
            }
            unregister_temp_path(tmp_dir);
        }
        ctx->unfolded_count++;
    }

    free(target);
}

void unfold_directory_symlinks(const char *target_dir,
                               const char *source_dir,
                               const PkgFileList *pkg_files,
                               bool dry_run)
{
    log_info("%sScanning for directory symlinks that cause folding conflicts...",
             dry_run ? "[DRY-RUN] " : "");
    UnfoldContext ctx = {source_dir, dry_run, 0};
    walk_target_dir_symlinks_targeted(target_dir, source_dir, pkg_files, unfold_symlink_cb, &ctx);
    if (ctx.unfolded_count == 0) {
        log_info("No directory symlinks required unfolding.");
    }
}
