/*
 * Symlink & Dependency Manager (symdep)
 * CLI Package Management Action Handlers Submodule
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

static int
action_remove(const char *source, const char *target, const char *pkg, const CommandContext *ctx)
{
    package_remove(source, target, pkg, ctx->opts->dry_run);
    return 0;
}

int cmd_pkg_create(const CommandContext *ctx)
{
    const char *pkg = ctx->args->items[ctx->arg_offset];
    PackageManifest manifest;
    manifest_init(&manifest, pkg);
    manifest_save(&manifest, ctx->source_dir);
    log_success("Created package directory & manifest for '%s'.", pkg);
    manifest_free(&manifest);
    return 0;
}

int cmd_pkg_remove(const CommandContext *ctx)
{
    return foreach_package(ctx, action_remove);
}

int cmd_pkg_list(const CommandContext *ctx)
{
    list_packages_status(ctx->source_dir, ctx->global_target_dir);
    return 0;
}
