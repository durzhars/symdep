/*
 * Symlink & Dependency Manager (symdep)
 * Path Manipulation & Resolution Helper Utilities
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
#ifndef UTILS_PATH_H
#define UTILS_PATH_H

#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>

/**
 * @brief In-place path normalization (collapses duplicate slashes and strips trailing slashes).
 *
 * @param path Path string to normalize.
 */
void normalize_path(char *path);

/**
 * @brief Collapse relative dot components (. and ..) in path in-place.
 *
 * @param path Path string to collapse.
 * @return 0 on success, non-zero on error.
 */
int collapse_path(char *path);

/**
 * @brief Safely join a directory path and relative path into an absolute path buffer.
 *
 * @param out      Output buffer.
 * @param out_size Size of output buffer.
 * @param dir      Parent directory.
 * @param rel      Relative child path.
 * @return 0 on success, non-zero if buffer bounds would be exceeded.
 */
int join_path(char *out, size_t out_size, const char *dir, const char *rel);

/**
 * @brief Check if prefix path is a parent directory prefix of target path.
 *
 * @param path   Target path string.
 * @param prefix Prefix directory path string.
 * @return 1 if prefix is a parent prefix, 0 otherwise.
 */
int is_path_prefix(const char *path, const char *prefix);

/**
 * @brief Expand leading tilde (~) in path string to home directory.
 *
 * @param path     Input path string starting with '~'.
 * @param out      Output buffer for expanded path.
 * @param out_size Size of output buffer.
 */
void expand_tilde_path(const char *path, char *out, size_t out_size);

/**
 * @brief Resolve active dotfiles/source repository directory path.
 *
 * @param buf      Output buffer.
 * @param buf_size Size of output buffer.
 */
void get_dotfiles_dir(char *buf, size_t buf_size);

/**
 * @brief Resolve active target home directory path.
 *
 * @param buf      Output buffer.
 * @param buf_size Size of output buffer.
 * @return true if target home directory path was resolved successfully.
 */
bool get_target_dir(char *buf, size_t buf_size);

#endif /* UTILS_PATH_H */
