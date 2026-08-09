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

#include "core/linker.h"
#include "core/checker.h"
#include "core/file_collector.h"
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/* Unified Package Context Structure across Linker Submodules */
typedef struct {
    const char *source_dir;
    const char *target_dir;
    const char *pkg_name;
    char pkg_dir[STOW_PATH_LARGE];
    char real_pkg_dir[STOW_PATH_LARGE];
    StringArray raw_ignores;
    PkgFileList pkg_files;
    bool dry_run;
    bool auto_install;
    int errors;
    size_t created_count;
    size_t unlinked_count;
} PackageContext;

/* Shared Internal Function Declarations */
void init_package_ignores(StringArray *raw_ignores, const char *source_dir, const char *pkg_dir);

bool package_context_init(PackageContext *ctx,
                         const char *source_dir,
                         const char *target_dir,
                         const char *pkg_name,
                         bool auto_install,
                         bool dry_run);

void package_context_free(PackageContext *ctx);

void build_unique_backup_path(const char *target_path, char *out_buf, size_t out_size);

bool get_symlink_owner_package(const char *symlink_path,
                                const char *source_dir,
                                char *owner_pkg_buf,
                                size_t buf_size);

void handle_dynamic_package_conflicts(const char *target_dir,
                                       const char *source_dir,
                                       const char *pkg_name,
                                       const PkgFileList *pkg_files_param,
                                       bool dry_run);

#endif /* SYMDEP_LINKER_INTERNAL_H */
