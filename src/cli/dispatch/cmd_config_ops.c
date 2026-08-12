/*
 * Symlink & Dependency Manager (symdep)
 * CLI Configuration Action Handlers Submodule
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

#include "cli/dispatch/internal.h"

int cmd_config_show(const CommandContext *ctx)
{
    (void)ctx;
    config_show();
    return 0;
}

int cmd_config_set(const CommandContext *ctx)
{
    const CliOptions *opts = ctx->opts;
    bool handled_flags = false;

    if (opts) {
        if (opts->pkg_mgr_override && *opts->pkg_mgr_override != '\0') {
            config_set_pkg_manager(opts->pkg_mgr_override);
            handled_flags = true;
        }
    }

    size_t count = ctx->args->count - ctx->arg_offset;
    if (count == 0) {
        if (handled_flags) {
            return 0;
        }
        log_error("Missing required options for 'config set'.");
        show_subcommand_help("config");
        return 1;
    }

    for (size_t i = ctx->arg_offset; i < ctx->args->count; i++) {
        const char *arg = ctx->args->items[i];
        const char *next_arg = (i + 1 < ctx->args->count) ? ctx->args->items[i + 1] : NULL;

        if (arg[0] == '-') {
            if (strcmp(arg, "-m") == 0 || strcmp(arg, "--manager") == 0 ||
                strcmp(arg, "--pkg-manager") == 0 || strcmp(arg, "--pkg-mgr") == 0) {
                if (!next_arg) {
                    log_error("Option '%s' requires a package manager name argument", arg);
                    show_subcommand_help("config");
                    return 1;
                }
                config_set_pkg_manager(next_arg);
                i++;
            } else if (strcmp(arg, "-e") == 0 || strcmp(arg, "--elevation") == 0 ||
                       strcmp(arg, "--elevation-tool") == 0) {
                if (!next_arg) {
                    log_error("Option '%s' requires an elevation tool argument (e.g. sudo, tsu, "
                              "doas, none)",
                              arg);
                    show_subcommand_help("config");
                    return 1;
                }
                config_set_elevation_tool(next_arg);
                i++;
            } else if (strcmp(arg, "-t") == 0 || strcmp(arg, "--target") == 0 ||
                       strcmp(arg, "--target-dir") == 0) {
                if (!next_arg) {
                    log_error("Option '%s' requires a directory path argument", arg);
                    show_subcommand_help("config");
                    return 1;
                }
                config_set_target_dir(next_arg);
                i++;
            } else if (strcmp(arg, "-d") == 0 || strcmp(arg, "--source") == 0 ||
                       strcmp(arg, "--source-dir") == 0 || strcmp(arg, "--src-dir") == 0 ||
                       strcmp(arg, "--dotfiles-dir") == 0) {
                if (!next_arg) {
                    log_error("Option '%s' requires a directory path argument", arg);
                    show_subcommand_help("config");
                    return 1;
                }
                config_set_source_dir(next_arg);
                i++;
            } else {
                log_error("Unknown option '%s' for 'config set'.", arg);
                show_subcommand_help("config");
                return 1;
            }
        } else {
            config_set_source_dir(arg);
        }
    }
    return 0;
}

int cmd_config_add(const CommandContext *ctx)
{
    config_add_source_dir(ctx->args->items[ctx->arg_offset]);
    return 0;
}

int cmd_config_remove(const CommandContext *ctx)
{
    config_remove_source_dir(ctx->args->items[ctx->arg_offset]);
    return 0;
}

#include "cli/help.h"

int cmd_help(const CommandContext *ctx)
{
    if (ctx->args->count > ctx->arg_offset) {
        const char *topic = ctx->args->items[ctx->arg_offset];
        show_subcommand_help(topic);
        return 0;
    }
    show_help();
    return 0;
}
