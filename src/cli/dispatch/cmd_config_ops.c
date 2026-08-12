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
    const char *key = ctx->args->items[ctx->arg_offset];
    const char *val = ctx->args->items[ctx->arg_offset + 1];

    if (strcmp(key, "target") == 0) {
        config_set_target_dir(val);
    } else {
        config_set_source_dir(val);
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
        if (strcmp(topic, "scan") == 0) {
            show_scan_help();
            return 0;
        }
    }
    show_help();
    return 0;
}
