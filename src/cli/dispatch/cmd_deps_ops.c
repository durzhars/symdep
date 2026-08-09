/*
 * Symlink & Dependency Manager (symdep)
 * CLI Dependency Management Action Handlers Submodule
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

int cmd_deps_add(const CommandContext *ctx)
{
    const char *pkg = ctx->args->items[ctx->arg_offset];
    const char *dep = ctx->args->items[ctx->arg_offset + 1];
    const char *type = (ctx->args->count > ctx->arg_offset + 2)
                           ? ctx->args->items[ctx->arg_offset + 2]
                           : "--optional";
    manifest_add_dep(ctx->source_dir, pkg, dep, type);
    return 0;
}

int cmd_deps_edit(const CommandContext *ctx)
{
    manifest_edit_dep(ctx->source_dir,
                      ctx->args->items[ctx->arg_offset],
                      ctx->args->items[ctx->arg_offset + 1],
                      ctx->args->items[ctx->arg_offset + 2]);
    return 0;
}

int cmd_deps_remove(const CommandContext *ctx)
{
    manifest_remove_dep(ctx->source_dir,
                        ctx->args->items[ctx->arg_offset],
                        ctx->args->items[ctx->arg_offset + 1]);
    return 0;
}

int cmd_deps_show(const CommandContext *ctx)
{
    manifest_show(ctx->source_dir, ctx->args->items[ctx->arg_offset]);
    return 0;
}

int cmd_deps_target(const CommandContext *ctx)
{
    manifest_set_target(ctx->source_dir,
                        ctx->args->items[ctx->arg_offset],
                        ctx->args->items[ctx->arg_offset + 1]);
    return 0;
}
