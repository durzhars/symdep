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

#ifndef SYMDEP_SCANNER_H
#define SYMDEP_SCANNER_H

#include <stdbool.h>

void scan_package(const char *source_dir, const char *pkg_name);
void scan_package_opts(const char *source_dir,
                       const char *pkg_name,
                       bool interactive,
                       bool write_manifest,
                       bool dry_run);

#endif /* SYMDEP_SCANNER_H */
