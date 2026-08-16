/*
 * Symlink & Dependency Manager (symdep)
 * Internal Linker Submodule Interface
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

#ifndef SYMDEP_LINKER_INTERNAL_H
#define SYMDEP_LINKER_INTERNAL_H

#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

#include "core/checker.h"
#include "core/file_collector.h"
#include "core/linker.h"
#include "core/manifest.h"
#include "core/registry.h"
#include "utils/defs.h"
#include "utils/fs.h"
#include "utils/logger.h"
#include "utils/path.h"
#include "utils/signal.h"
#include "utils/timer.h"

#include <dirent.h>
#include <errno.h>
#include <fnmatch.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/**
 * @struct PackageContext
 * @brief Unified package context structure shared across linker submodules.
 */
typedef struct {
    const char *source_dir;             /**< Active source dotfiles repository path */
    const char *target_dir;             /**< Resolved destination target directory */
    const char *pkg_name;               /**< Name of package being processed */
    char pkg_dir[STOW_PATH_LARGE];      /**< Full path to package directory */
    char real_pkg_dir[STOW_PATH_LARGE]; /**< Canonical resolved path to package directory */
    StringArray raw_ignores;            /**< Merged global and package ignore rules */
    PkgFileList pkg_files;              /**< List of collected files belonging to package */
    bool dry_run;                       /**< Preview mode without disk changes */
    bool auto_install;                  /**< Auto-confirm missing dependency installations */
    atomic_int errors;                  /**< Atomic count of errors encountered */
    atomic_size_t created_count;        /**< Atomic count of created symlinks */
    size_t unlinked_count;              /**< Count of unlinked symlinks */
} PackageContext;

/**
 * @enum TargetResolveResult
 * @brief Result code when resolving a target path conflict or atomic replacement.
 */
typedef enum {
    TARGET_RESOLVE_PROCEED_LINK = 0, /**< Target is clear or backed up; proceed with link */
    TARGET_RESOLVE_ALREADY_LINKED,   /**< Target symlink already points to expected file */
    TARGET_RESOLVE_REPLACED,         /**< Stale symlink was atomically replaced via rename */
    TARGET_RESOLVE_ERROR             /**< Conflict backup or replacement failed */
} TargetResolveResult;

/** Initialize ignore rules for a package */
void init_package_ignores(StringArray *raw_ignores, const char *source_dir, const char *pkg_dir);

/** Initialize PackageContext structure and collect package files */
bool package_context_init(PackageContext *ctx,
                          const char *source_dir,
                          const char *target_dir,
                          const char *pkg_name,
                          bool auto_install,
                          bool dry_run);

/** Free internal allocations within PackageContext */
void package_context_free(PackageContext *ctx);

/** Generate unique timestamped/counter backup filename for conflicting target files */
void build_unique_backup_path(const char *target_path, char *out_buf, size_t out_size);

/** Determine package owning a symlink in the target directory */
bool get_symlink_owner_package(const char *symlink_path,
                               const char *source_dir,
                               char *owner_pkg_buf,
                               size_t buf_size);

/** Detect and unlink conflicting packages whose files collide with incoming package */
void handle_dynamic_package_conflicts(const char *target_dir,
                                      const char *source_dir,
                                      const char *pkg_name,
                                      const PkgFileList *pkg_files_param,
                                      bool dry_run);

/** Atomically resolve stale symlink replacements or back up non-symlink file collisions */
TargetResolveResult resolve_target_conflict_or_replace(const char *target_path,
                                                       const char *pkg_file_path,
                                                       const char *rel_path);

#endif /* SYMDEP_LINKER_INTERNAL_H */
