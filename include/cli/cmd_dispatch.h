/*
 * Symlink & Dependency Manager (symdep)
 * Command Dispatcher Engine Header
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

#ifndef SYMDEP_CMD_DISPATCH_H
#define SYMDEP_CMD_DISPATCH_H

#include <stdbool.h>

#include "cli.h"

/**
 * @struct CommandContext
 * @brief Context structure passed to every subcommand handler function.
 */
typedef struct {
    const CliOptions *opts;      /**< Parsed global CLI options */
    const char *source_dir;      /**< Resolved active source repository directory */
    const char *global_target_dir;/**< Resolved active target home directory */
    const StringArray *args;     /**< Positional arguments slice */
    size_t arg_offset;           /**< Index where subcommand payload arguments begin */
} CommandContext;

/**
 * @typedef CommandHandler
 * @brief Function pointer signature for all subcommand handler implementations.
 */
typedef int (*CommandHandler)(const CommandContext *ctx);

/**
 * @struct CommandRoute
 * @brief Declarative descriptor entry in CLI command routing table.
 */
typedef struct {
    const char *group;      /**< Group namespace (e.g. "config", "pkg", "deps", "ignore", "stow") */
    const char *subcommand; /**< Subcommand name (e.g. "create", "add", "show") or NULL for top-level */
    const char *const
        *aliases;           /**< NULL-terminated list of alias strings */
    size_t min_args;        /**< Minimum required payload arguments after group/subcommand */
    const char *usage;      /**< Usage string displayed on argument count failure */
    CommandHandler handler; /**< Function pointer handler */
} CommandRoute;

/**
 * @brief Route positional arguments against command table with fallback checking.
 *
 * Matches positional arguments against registered routes, subcommand aliases,
 * command group help, and implicit package linking fallbacks.
 *
 * @param args Positional argument array.
 * @param opts Parsed global options.
 * @return Exit status code (0 for success, non-zero for error).
 */
int dispatch_command(const StringArray *args, const CliOptions *opts);

#endif /* SYMDEP_CMD_DISPATCH_H */
