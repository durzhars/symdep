/*
 * Symlink & Dependency Manager (symdep)
 * CLI Flags & Global Options Parser Header
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

#ifndef SYMDEP_CLI_H
#define SYMDEP_CLI_H

#include <stdbool.h>

#include "utils/str.h"

/**
 * @struct CliOptions
 * @brief Holds global command-line options and overrides.
 */
typedef struct {
    bool auto_install;       /**< -y, --install: Auto-confirm dependency installation */
    bool dry_run;            /**< -n, --dry-run: Preview disk operations */
    bool save_flag;          /**< -s, --save: Save CLI directory overrides to config */
    bool profile;            /**< -p, --profile: Enable performance profiler */
    bool interactive;        /**< -i, --interactive: Launch interactive scanner wizard */
    bool help_flag;          /**< -h, --help: Display help manual */
    const char *cli_source_dir; /**< -d, --source-dir: Source directory override */
    const char *cli_target_dir; /**< -t, --target-dir: Target directory override */
    const char *pkg_mgr_override; /**< -m, --manager: Package manager override */
} CliOptions;

/**
 * @brief Parse global CLI options and separate positional arguments.
 *
 * @param argc            Argument count from main.
 * @param argv            Argument array from main.
 * @param opts            Output CliOptions struct.
 * @param positional_args Output StringArray for subcommands and package names.
 * @return 0 on success, -1 on help display, 1 on error.
 */
int parse_cli_options(int argc, char **argv, CliOptions *opts, StringArray *positional_args);

#endif /* SYMDEP_CLI_H */
