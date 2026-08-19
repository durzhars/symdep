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
 * in .symdeps exist on $PATH or in expected plugin locations. During link operations,
 * audits non-destructively without blocking unless auto_install (-y) is enabled.
 *
 * @param source_dir   Path to active source repository.
 * @param target_pkg   Name of target package (NULL or "all" for all packages).
 * @param auto_install Auto-install missing dependencies via system package manager.
 * @param dry_run      Preview mode without modifying disk or running installers.
 */
void check_package_dependencies(const char *source_dir,
                                const char *target_pkg,
                                bool auto_install,
                                bool dry_run);

/**
 * @brief Standalone dependency installer for package(s).
 *
 * Resolves missing required and optional dependencies for target package(s)
 * and invokes the system package manager without creating symlinks.
 *
 * @param source_dir   Path to active source repository.
 * @param target_pkg   Name of target package (NULL or "all" for all packages).
 * @param auto_install Auto-confirm installation without prompting.
 * @param dry_run      Preview installation command without executing.
 * @param strict_no_skip Enforce strict registration on skipped prompts.
 * @return 0 on success (or dependencies already satisfied), non-zero on failure.
 */
int install_package_dependencies(const char *source_dir,
                                 const char *target_pkg,
                                 bool auto_install,
                                 bool dry_run,
                                 bool strict_no_skip);

/**
 * @brief Concise numerical dependency audit for a single package.
 *
 * Checks required and optional dependencies in .symdeps for pkg_name,
 * and outputs a single-line numerical summary (satisfied vs missing counts).
 * Non-blocking, never pauses on stdin or prompts.
 *
 * @param source_dir Path to active source repository.
 * @param pkg_name   Name of package to audit.
 */
void audit_package_dependencies_brief(const char *source_dir, const char *pkg_name);

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
