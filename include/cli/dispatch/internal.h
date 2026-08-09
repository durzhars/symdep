/*
 * Symlink & Dependency Manager (symdep)
 * CLI Dispatcher Internal Module Header
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

#ifndef SYMDEP_DISPATCH_INTERNAL_H
#define SYMDEP_DISPATCH_INTERNAL_H

#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <string.h>

#include "cli/cmd_dispatch.h"
#include "cli/cmd_routes.h"
#include "cli/help.h"

#include "core/checker.h"
#include "core/config.h"
#include "core/ignore.h"
#include "core/linker.h"
#include "core/manifest.h"
#include "core/registry.h"
#include "core/scanner.h"

#include "utils/defs.h"
#include "utils/fs.h"
#include "utils/logger.h"
#include "utils/path.h"

typedef int (*PackageActionFn)(const char *source_dir,
                               const char *target_dir,
                               const char *pkg_name,
                               const CommandContext *ctx);

int foreach_package(const CommandContext *ctx, PackageActionFn action);

bool parse_ignore_args(const CommandContext *ctx,
                              const char **out_pkg,
                              const char *const **out_patterns,
                              size_t *out_count);

#endif /* SYMDEP_DISPATCH_INTERNAL_H */
