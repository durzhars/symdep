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

#ifndef SYMDEP_MANIFEST_H
#define SYMDEP_MANIFEST_H

#include "utils/str.h"

/**
 * @struct PackageManifest
 * @brief Represents a parsed package .symdeps manifest.
 */
typedef struct {
    char *package_name;  /**< Name of the package */
    char *target_path;   /**< Custom target path override (or NULL) */
    StringArray required;/**< List of required CLI binary dependencies */
    StringArray optional;/**< List of optional plugins or secondary utilities */
    StringArray conflicts;/**< List of mutually exclusive conflicting packages */
} PackageManifest;

/**
 * @brief Initialize an empty PackageManifest instance.
 *
 * @param manifest Pointer to uninitialized manifest struct.
 * @param pkg_name Name of package.
 */
void manifest_init(PackageManifest *manifest, const char *pkg_name);

/**
 * @brief Load and parse a package's .symdeps (or legacy .stowdeps) manifest from disk.
 *
 * @param manifest   Pointer to initialized manifest struct.
 * @param source_dir Path to active source repository.
 * @return true on success, false if file does not exist or parsing failed.
 */
bool manifest_load(PackageManifest *manifest, const char *source_dir);

/**
 * @brief Write a PackageManifest struct back to disk as INI format.
 *
 * @param manifest   Pointer to manifest struct to serialize.
 * @param source_dir Path to active source repository.
 * @return true on success, false on write error.
 */
bool manifest_save(const PackageManifest *manifest, const char *source_dir);

/**
 * @brief Free heap allocations associated with a PackageManifest.
 *
 * @param manifest Pointer to manifest struct.
 */
void manifest_free(PackageManifest *manifest);

/**
 * @brief Add a dependency or conflict entry to package .symdeps manifest.
 *
 * @param source_dir Path to active source repository.
 * @param pkg_name   Package name.
 * @param dep        Dependency tool name or conflicting package name.
 * @param type       Classification type ("--required", "--optional", or "--conflict").
 */
void manifest_add_dep(const char *source_dir,
                      const char *pkg_name,
                      const char *dep,
                      const char *type);

/**
 * @brief Edit existing dependency classification in package .symdeps manifest.
 *
 * @param source_dir Path to active source repository.
 * @param pkg_name   Package name.
 * @param dep        Dependency tool name.
 * @param new_type   New classification ("--required", "--optional", or "--conflict").
 */
void manifest_edit_dep(const char *source_dir,
                       const char *pkg_name,
                       const char *dep,
                       const char *new_type);

/**
 * @brief Remove a dependency or conflict entry from package manifest.
 *
 * @param source_dir Path to active source repository.
 * @param pkg_name   Package name.
 * @param dep        Dependency or conflict entry to remove.
 */
void manifest_remove_dep(const char *source_dir, const char *pkg_name, const char *dep);

/**
 * @brief Set custom per-package target path override in package manifest.
 *
 * @param source_dir  Path to active source repository.
 * @param pkg_name    Package name.
 * @param target_path Target directory override path.
 */
void manifest_set_target(const char *source_dir, const char *pkg_name, const char *target_path);

/**
 * @brief Print raw contents of package .symdeps manifest.
 *
 * @param source_dir Path to active source repository.
 * @param pkg_name   Package name.
 */
void manifest_show(const char *source_dir, const char *pkg_name);

/**
 * @brief Safely unlink package symlinks and remove package directory from disk.
 *
 * @param source_dir Path to active source repository.
 * @param target_dir Destination target home directory.
 * @param pkg_name   Package name to remove.
 * @param dry_run    If true, preview changes without deleting directory.
 */
void package_remove(const char *source_dir,
                    const char *target_dir,
                    const char *pkg_name,
                    bool dry_run);

#endif /* SYMDEP_MANIFEST_H */
