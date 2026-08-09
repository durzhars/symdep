/*
 * Dotfiles Stow Manager (stow-manager)
 * Directory Walking & Creation Submodule
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

#include "utils/fs/internal.h"

int mkdir_p(const char *path, mode_t mode)
{
    if (!path || *path == '\0') {
        errno = EINVAL;
        return -1;
    }

    char tmp[STOW_PATH_LARGE];
    size_t len = strlen(path);
    if (len >= sizeof(tmp)) {
        errno = ENAMETOOLONG;
        return -1;
    }
    memcpy(tmp, path, len + 1);

    if (len > 1 && tmp[len - 1] == '/') {
        tmp[--len] = '\0';
    }

    for (char *p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';

            if (mkdir(tmp, mode) != 0) {
                if (errno == EEXIST) {
                    struct stat st;
                    if (stat(tmp, &st) != 0 || !S_ISDIR(st.st_mode)) {
                        errno = ENOTDIR;
                        return -1;
                    }
                } else {
                    return -1;
                }
            }
            *p = '/';
        }
    }

    if (mkdir(tmp, mode) != 0) {
        if (errno == EEXIST) {
            struct stat st;
            if (stat(tmp, &st) != 0 || !S_ISDIR(st.st_mode)) {
                errno = ENOTDIR;
                return -1;
            }
            return 0;
        }
        return -1;
    }

    return 0;
}

void walk_dir_symlinks(const char *dir_path,
                       int current_depth,
                       int max_depth,
                       WalkSymlinkCallback cb,
                       void *user_data)
{
    if (current_depth > max_depth) {
        return;
    }

    DIR *dir = opendir(dir_path);
    if (!dir) {
        return;
    }

    char path[STOW_PATH_LARGE];
    size_t dir_len = strlen(dir_path);
    if (dir_len < sizeof(path) - 1) {
        memcpy(path, dir_path, dir_len);
        if (dir_len > 0 && path[dir_len - 1] != '/') {
            path[dir_len++] = '/';
        }
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        const char *name = entry->d_name;
        if (name[0] == '.' && (name[1] == '\0' || (name[1] == '.' && name[2] == '\0'))) {
            continue;
        }

        size_t name_len = strlen(name);
        if (dir_len + name_len >= sizeof(path)) {
            continue;
        }
        memcpy(path + dir_len, name, name_len + 1);

        if (entry->d_type == DT_LNK) {
            cb(path, user_data);
        } else if (entry->d_type == DT_DIR) {
            walk_dir_symlinks(path, current_depth + 1, max_depth, cb, user_data);
        } else if (entry->d_type == DT_UNKNOWN) {
            if (is_symlink(path)) {
                cb(path, user_data);
            } else if (is_dir(path)) {
                walk_dir_symlinks(path, current_depth + 1, max_depth, cb, user_data);
            }
        }
    }

    closedir(dir);
}

#define MAX_DIR_RECURSION_DEPTH 64

typedef struct {
    const char *base_dir;
    char full_buf[STOW_PATH_LARGE];
    char rel_buf[STOW_PATH_LARGE];
    WalkFileCallback cb;
    void *user_data;
    int depth;
} WalkFilesState;

static void walk_dir_files_recursive(WalkFilesState *state)
{
    if (state->depth >= MAX_DIR_RECURSION_DEPTH) {
        return;
    }

    DIR *dir = opendir(state->full_buf);
    if (!dir) {
        return;
    }

    state->depth++;
    size_t base_full_len = strlen(state->full_buf);
    size_t base_rel_len = strlen(state->rel_buf);

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        const char *name = entry->d_name;
        if (name[0] == '.' && (name[1] == '\0' || (name[1] == '.' && name[2] == '\0'))) {
            continue;
        }

        size_t name_len = strlen(name);

        char *full_p = state->full_buf + base_full_len;
        if (base_full_len > 0 && state->full_buf[base_full_len - 1] != '/') {
            *full_p++ = '/';
        }
        if ((size_t)(full_p - state->full_buf) + name_len < sizeof(state->full_buf)) {
            memcpy(full_p, name, name_len + 1);
        }

        char *rel_p = state->rel_buf + base_rel_len;
        if (base_rel_len > 0 && state->rel_buf[base_rel_len - 1] != '/') {
            *rel_p++ = '/';
        }
        if ((size_t)(rel_p - state->rel_buf) + name_len < sizeof(state->rel_buf)) {
            memcpy(rel_p, name, name_len + 1);
        }

        if (entry->d_type == DT_DIR) {
            walk_dir_files_recursive(state);
        } else if (entry->d_type == DT_REG || entry->d_type == DT_LNK) {
            state->cb(state->full_buf, state->rel_buf, state->user_data);
        } else {
            if (is_dir(state->full_buf) && !is_symlink(state->full_buf)) {
                walk_dir_files_recursive(state);
            } else {
                state->cb(state->full_buf, state->rel_buf, state->user_data);
            }
        }

        state->full_buf[base_full_len] = '\0';
        state->rel_buf[base_rel_len] = '\0';
    }

    closedir(dir);
    state->depth--;
}

void walk_dir_files(const char *base_dir,
                    const char *current_dir,
                    WalkFileCallback cb,
                    void *user_data)
{
    WalkFilesState state;
    state.base_dir = base_dir;
    state.cb = cb;
    state.user_data = user_data;
    state.depth = 0;

    if (current_dir && *current_dir != '\0') {
        join_path(state.full_buf, sizeof(state.full_buf), base_dir, current_dir);
        snprintf(state.rel_buf, sizeof(state.rel_buf), "%s", current_dir);
    } else {
        snprintf(state.full_buf, sizeof(state.full_buf), "%s", base_dir);
        state.rel_buf[0] = '\0';
    }

    walk_dir_files_recursive(&state);
}

void cleanup_temp_dir_contents(const char *dir_path)
{
    if (!dir_path || *dir_path == '\0') {
        return;
    }

    DIR *dir = opendir(dir_path);
    if (!dir) {
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        const char *name = entry->d_name;
        if (name[0] == '.' && (name[1] == '\0' || (name[1] == '.' && name[2] == '\0'))) {
            continue;
        }

        char child_path[STOW_PATH_LARGE];
        join_path(child_path, sizeof(child_path), dir_path, name);

        if (is_dir(child_path) && !is_symlink(child_path)) {
            cleanup_temp_dir_contents(child_path);
            rmdir(child_path);
        } else {
            unlink(child_path);
        }
    }

    closedir(dir);
}
