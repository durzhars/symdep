/*
 * Symlink & Dependency Manager (symdep)
 * CLI Dispatch Core Engine & Route Matching
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

static void print_general_help_hint(void)
{
    log_info("Hint: Pass -h or help for information.");
}

static bool is_known_group(const char *group)
{
    if (!group) {
        return false;
    }
    for (size_t i = 0; ROUTE_TABLE[i].handler != NULL; i++) {
        if (strcmp(group, ROUTE_TABLE[i].group) == 0) {
            return true;
        }
    }
    return false;
}

static void print_group_usage_help(const char *group, bool is_help_cmd)
{
    if (!is_help_cmd) {
        log_error("Missing or invalid subcommand for command group '%s'.", group);
    }
    printf("  %sAvailable subcommands for '%s':%s\n", COLOR_BOLD, group, COLOR_RESET);
    for (size_t i = 0; ROUTE_TABLE[i].handler != NULL; i++) {
        if (strcmp(group, ROUTE_TABLE[i].group) == 0 && ROUTE_TABLE[i].subcommand != NULL) {
            if (ROUTE_TABLE[i].usage) {
                printf("    %s%s%s\n", COLOR_CYAN, ROUTE_TABLE[i].usage, COLOR_RESET);
            } else {
                printf("    %ssymdep %s %s%s\n",
                       COLOR_CYAN,
                       group,
                       ROUTE_TABLE[i].subcommand,
                       COLOR_RESET);
            }
        }
    }
    printf("\n");
    if (!is_help_cmd) {
        print_general_help_hint();
    }
}

int dispatch_command(const StringArray *args, const CliOptions *opts)
{
    if (!args || args->count == 0) {
        log_error("Invalid arguments.");
        print_general_help_hint();
        return 1;
    }

    const char *token1 = args->items[0];
    const char *token2 = (args->count > 1) ? args->items[1] : NULL;

    for (size_t i = 0; ROUTE_TABLE[i].handler != NULL; i++) {
        const CommandRoute *route = &ROUTE_TABLE[i];
        bool matched = false;
        size_t consumed_tokens = 0;

        // Check group / subcommand space-separated matching
        if (strcmp(token1, route->group) == 0) {
            if (route->subcommand != NULL) {
                if (token2) {
                    if (strcmp(token2, route->subcommand) == 0) {
                        matched = true;
                        consumed_tokens = 2;
                    } else if (route->aliases) {
                        char combined[256];
                        snprintf(combined, sizeof(combined), "%s:%s", route->group, token2);
                        for (size_t a = 0; route->aliases[a] != NULL; a++) {
                            if (strcmp(token2, route->aliases[a]) == 0 ||
                                strcmp(combined, route->aliases[a]) == 0) {
                                matched = true;
                                consumed_tokens = 2;
                                break;
                            }
                        }
                    }
                }
            } else {
                matched = true;
                consumed_tokens = 1;
            }
        }

        if (!matched && route->aliases) {
            for (size_t a = 0; route->aliases[a] != NULL; a++) {
                if (strcmp(token1, route->aliases[a]) == 0) {
                    matched = true;
                    consumed_tokens = 1;
                    break;
                }
            }
        }

        if (matched) {
            size_t sub_args = args->count - consumed_tokens;
            if (sub_args < route->min_args) {
                log_error("%s", route->usage ? route->usage : "Insufficient arguments!");
                return 1;
            }

            char source_dir[STOW_PATH_LARGE] = {0};
            char global_target_dir[STOW_PATH_LARGE] = {0};

            if (strcmp(route->group, "config") != 0 && strcmp(route->group, "help") != 0) {
                get_active_config_dirs(opts->cli_source_dir,
                                       opts->cli_target_dir,
                                       source_dir,
                                       sizeof(source_dir),
                                       global_target_dir,
                                       sizeof(global_target_dir));
            }

            CommandContext ctx = {.opts = opts,
                                  .source_dir = source_dir,
                                  .global_target_dir = global_target_dir,
                                  .args = args,
                                  .arg_offset = consumed_tokens};

            return route->handler(&ctx);
        }
    }

    if (is_known_group(token1)) {
        bool is_help_req = (opts && opts->help_flag) ||
                           (token2 && (strcmp(token2, "help") == 0 || strcmp(token2, "-h") == 0 ||
                                       strcmp(token2, "--help") == 0));
        print_group_usage_help(token1, is_help_req);
        return is_help_req ? 0 : 1;
    }

    char source_dir[STOW_PATH_LARGE] = {0};
    char global_target_dir[STOW_PATH_LARGE] = {0};
    get_active_config_dirs(opts->cli_source_dir,
                           opts->cli_target_dir,
                           source_dir,
                           sizeof(source_dir),
                           global_target_dir,
                           sizeof(global_target_dir));

    bool all_valid = true;
    for (size_t i = 0; i < args->count; i++) {
        char full_pkg_path[STOW_PATH_LARGE];
        join_path(full_pkg_path, sizeof(full_pkg_path), source_dir, args->items[i]);
        if (!is_dir(full_pkg_path)) {
            all_valid = false;
            break;
        }
    }

    if (all_valid) {
        CommandContext ctx = {.opts = opts,
                              .source_dir = source_dir,
                              .global_target_dir = global_target_dir,
                              .args = args,
                              .arg_offset = 0};
        return cmd_stow(&ctx);
    }

    log_error("Unknown command: %s", token1);
    print_general_help_hint();
    return 1;
}
