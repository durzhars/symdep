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

typedef struct {
    char *package_name;
    char *target_path;
    StringArray required;
    StringArray optional;
    StringArray conflicts;
} PackageManifest;

void manifest_init(PackageManifest *manifest, const char *pkg_name);
bool manifest_load(PackageManifest *manifest, const char *source_dir);
bool manifest_save(const PackageManifest *manifest, const char *source_dir);
void manifest_free(PackageManifest *manifest);

void manifest_add_dep(const char *source_dir,
                      const char *pkg_name,
                      const char *dep,
                      const char *type);
void manifest_edit_dep(const char *source_dir,
                       const char *pkg_name,
                       const char *dep,
                       const char *new_type);
void manifest_remove_dep(const char *source_dir, const char *pkg_name, const char *dep);
void manifest_set_target(const char *source_dir, const char *pkg_name, const char *target_path);
void manifest_show(const char *source_dir, const char *pkg_name);

void package_remove(const char *source_dir,
                    const char *target_dir,
                    const char *pkg_name,
                    bool dry_run);

#endif /* SYMDEP_MANIFEST_H */
