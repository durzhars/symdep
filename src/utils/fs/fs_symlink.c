/*
 * Dotfiles Stow Manager (stow-manager)
 * Symlink Resolution & Inspection Submodule
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

char *read_symlink_target(const char *path)
{
    if (!path || *path == '\0' || !is_symlink(path)) {
        return NULL;
    }

    char target[STOW_PATH_LARGE];
    ssize_t len = readlink(path, target, sizeof(target) - 1);
    if (len == -1) {
        return NULL;
    }
    target[len] = '\0';

    char norm_path[STOW_PATH_LARGE];
    if (target[0] != '/') {
        const char *last_slash = strrchr(path, '/');
        int formatted_len = 0;
        if (last_slash) {
            size_t parent_len = (size_t)(last_slash - path);
            if (parent_len >= STOW_PATH_MAX) {
                return NULL;
            }
            formatted_len =
                snprintf(norm_path, sizeof(norm_path), "%.*s/%s", (int)parent_len, path, target);
        } else {
            formatted_len = snprintf(norm_path, sizeof(norm_path), "./%s", target);
        }

        if (formatted_len < 0 || (size_t)formatted_len >= sizeof(norm_path)) {
            return NULL;
        }
    } else {
        snprintf(norm_path, sizeof(norm_path), "%s", target);
    }

    collapse_path(norm_path);
    normalize_path(norm_path);

    return safe_strdup(norm_path);
}

bool is_symlink_pointing_to(const char *symlink_path,
                            const char *pkg_file_path,
                            const char *real_pkg_file_path)
{
    if (!symlink_path || !pkg_file_path) {
        return false;
    }

    char raw_link[STOW_PATH_MAX];
    ssize_t len = readlink(symlink_path, raw_link, sizeof(raw_link) - 1);
    if (len == -1) {
        return false;
    }
    raw_link[len] = '\0';

    char one_level_target[STOW_PATH_MAX];
    if (raw_link[0] == '/') {
        memcpy(one_level_target, raw_link, (size_t)len + 1);
    } else {
        const char *last_slash = strrchr(symlink_path, '/');
        if (last_slash) {
            size_t parent_len = (size_t)(last_slash - symlink_path);
            if (parent_len + 1 + (size_t)len >= sizeof(one_level_target)) {
                return false;
            }
            memcpy(one_level_target, symlink_path, parent_len);
            one_level_target[parent_len] = '/';
            memcpy(one_level_target + parent_len + 1, raw_link, (size_t)len + 1);
        } else {
            if ((size_t)len + 3 >= sizeof(one_level_target)) {
                return false;
            }
            one_level_target[0] = '.';
            one_level_target[1] = '/';
            memcpy(one_level_target + 2, raw_link, (size_t)len + 1);
        }
    }
    collapse_path(one_level_target);
    normalize_path(one_level_target);

    char norm_pkg_file[STOW_PATH_MAX];
    size_t pkg_len = strlen(pkg_file_path);
    if (pkg_len < sizeof(norm_pkg_file)) {
        memcpy(norm_pkg_file, pkg_file_path, pkg_len + 1);
        normalize_path(norm_pkg_file);
        if (strcmp(one_level_target, norm_pkg_file) == 0) {
            return true;
        }
    }

    if (real_pkg_file_path && *real_pkg_file_path != '\0') {
        if (strcmp(one_level_target, real_pkg_file_path) == 0) {
            return true;
        }
        char norm_real_pkg_file[STOW_PATH_MAX];
        size_t real_len = strlen(real_pkg_file_path);
        if (real_len < sizeof(norm_real_pkg_file)) {
            memcpy(norm_real_pkg_file, real_pkg_file_path, real_len + 1);
            normalize_path(norm_real_pkg_file);
            if (strcmp(one_level_target, norm_real_pkg_file) == 0) {
                return true;
            }
        }
    }

    // Only invoke realpath if one_level_target is itself a symlink (multi-hop chain)
    if (is_symlink(one_level_target)) {
        char real_symlink[STOW_PATH_MAX];
        if (realpath(symlink_path, real_symlink) != NULL) {
            if (real_pkg_file_path && strcmp(real_symlink, real_pkg_file_path) == 0) {
                return true;
            }
            char norm_real_symlink[STOW_PATH_MAX];
            size_t rlen = strlen(real_symlink);
            if (rlen < sizeof(norm_real_symlink)) {
                memcpy(norm_real_symlink, real_symlink, rlen + 1);
                normalize_path(norm_real_symlink);
                if (real_pkg_file_path && strcmp(norm_real_symlink, real_pkg_file_path) == 0) {
                    return true;
                }
            }
        }
    }

    return false;
}

#ifdef __linux__
#include <fcntl.h>
#include <sys/syscall.h>
#ifndef SYS_renameat2
#define SYS_renameat2 316
#endif
#ifndef RENAME_EXCHANGE
#define RENAME_EXCHANGE (1 << 1)
#endif
#endif

bool fs_atomic_swap_or_replace(const char *src_tmp,
                               const char *dst_target,
                               bool is_dir_over_symlink)
{
    if (!src_tmp || !dst_target) {
        return false;
    }

    if (!is_dir_over_symlink) {
        /* Standard non-directory (symlink-to-symlink, file-to-file) replacement */
        return (rename(src_tmp, dst_target) == 0);
    }

#if defined(__linux__) && defined(SYS_renameat2)
    /* Tier 1: Linux raw kernel dentry exchange (0-window physical atomicity) */
    if (syscall(SYS_renameat2, AT_FDCWD, src_tmp, AT_FDCWD, dst_target, RENAME_EXCHANGE) == 0) {
        unlink(src_tmp); /* Clean up old symlink swapped into src_tmp */
        return true;
    }
#elif defined(__APPLE__)
    /* Tier 1: Darwin / macOS APFS atomic dentry exchange */
    if (renameatx_np(AT_FDCWD, src_tmp, AT_FDCWD, dst_target, RENAME_EXCHANGE) == 0) {
        unlink(src_tmp);
        return true;
    }
#endif

    /* Tier 2: Generic POSIX fallback (unlink + rename) */
    unlink(dst_target);
    return (rename(src_tmp, dst_target) == 0);
}
