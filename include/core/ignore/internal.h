/*
 * Symlink & Dependency Manager (symdep)
 * Ignore Subsystem Internal Header
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

#ifndef SYMDEP_IGNORE_INTERNAL_H
#define SYMDEP_IGNORE_INTERNAL_H

#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core/file_collector.h"
#include "core/ignore.h"
#include "core/registry.h"
#include "utils/defs.h"
#include "utils/fs.h"
#include "utils/logger.h"
#include "utils/path.h"
#include "utils/str.h"

/* Shared Internal Function Declarations */
void get_stowignore_path(const char *source_dir,
                          const char *pkg_name,
                          char *out_path,
                          size_t out_size);

void ignore_init_single(const char *source_dir, const char *pkg_name);
void ignore_clear_single(const char *source_dir, const char *pkg_name);

#endif /* SYMDEP_IGNORE_INTERNAL_H */
