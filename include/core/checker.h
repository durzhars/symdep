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

#ifndef SYMDEP_CHECKER_H
#define SYMDEP_CHECKER_H

#include <stdbool.h>

/**
 * @brief Audit required and optional dependencies for a target package.
 *
 * Verifies that required binaries, plugins, and optional dependencies declared
 * in .symdeps exist on $PATH or in expected plugin locations. If auto_install is set,
 * invokes the system package manager to install missing dependencies.
 *
 * @param source_dir   Path to active source repository.
 * @param target_pkg   Name of target package (NULL for all packages).
 * @param auto_install Auto-install missing dependencies via system package manager.
 * @param dry_run      Preview mode without modifying disk or running installers.
 */
void check_package_dependencies(const char *source_dir,
                                const char *target_pkg,
                                bool auto_install,
                                bool dry_run);

/**
 * @brief Perform a comprehensive symlink integrity and health audit.
 *
 * Scans the source repository for dangling/broken symlinks and checks the
 * target home directory for unmanaged orphan symlinks pointing into repository.
 *
 * @param source_dir Path to active source repository.
 * @param target_dir Destination target home directory.
 */
void check_symlink_health(const char *source_dir, const char *target_dir);

#endif /* SYMDEP_CHECKER_H */
