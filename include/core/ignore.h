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

#ifndef SYMDEP_IGNORE_H
#define SYMDEP_IGNORE_H

#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Initialize global or package-level .symignore file(s).
 *
 * @param source_dir Path to active source repository.
 * @param pkgs       Array of package names (NULL or empty count for global repository init).
 * @param count      Number of package names in pkgs slice.
 */
void ignore_init(const char *source_dir, const char *const *pkgs, size_t count);

/**
 * @brief Clear/purge global or package-level .symignore file(s).
 *
 * @param source_dir Path to active source repository.
 * @param pkgs       Array of package names (NULL or empty count for global repository purge).
 * @param count      Number of package names in pkgs slice.
 */
void ignore_clear(const char *source_dir, const char *const *pkgs, size_t count);

/**
 * @brief Display active .symignore rules with redundancy warnings.
 *
 * @param source_dir Path to active source repository.
 * @param pkgs       Array of package names (NULL or empty count for global rules display).
 * @param count      Number of package names in pkgs slice.
 */
void ignore_show(const char *source_dir, const char *const *pkgs, size_t count);

/**
 * @brief Append glob pattern(s) to package or global .symignore file.
 *
 * @param source_dir Path to active source repository.
 * @param pkg_name   Package name (NULL or empty for global root .symignore).
 * @param patterns   Array of glob pattern strings to add.
 * @param count      Number of pattern strings.
 */
void ignore_add_patterns(const char *source_dir,
                         const char *pkg_name,
                         const char *const *patterns,
                         size_t count);

/**
 * @brief Remove glob pattern(s) from package or global .symignore file.
 *
 * @param source_dir Path to active source repository.
 * @param pkg_name   Package name (NULL or empty for global root .symignore).
 * @param patterns   Array of glob pattern strings to remove.
 * @param count      Number of pattern strings.
 */
void ignore_remove_patterns(const char *source_dir,
                            const char *pkg_name,
                            const char *const *patterns,
                            size_t count);

#endif /* SYMDEP_IGNORE_H */
