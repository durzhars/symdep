/*
 * Symlink & Dependency Manager (symdep)
 * CLI Stow / Link Action Handlers Submodule
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

int foreach_package(const CommandContext *ctx, PackageActionFn action)
{
    int status = 0;
    for (size_t i = ctx->arg_offset; i < ctx->args->count; i++) {
        const char *pkg_name = ctx->args->items[i];
        char target_dir[STOW_PATH_LARGE];
        get_active_target_dir_for_pkg(
            ctx->opts->cli_target_dir, ctx->source_dir, pkg_name, target_dir, sizeof(target_dir));
        int res = action(ctx->source_dir, target_dir, pkg_name, ctx);
        if (res != 0) {
            status = res;
        }
    }
    return status;
}

static int
action_stow(const char *source, const char *target, const char *pkg, const CommandContext *ctx)
{
    return link_package(source, target, pkg, ctx->opts->auto_install, ctx->opts->dry_run);
}

static int
action_unstow(const char *source, const char *target, const char *pkg, const CommandContext *ctx)
{
    return unlink_package(source, target, pkg, ctx->opts->dry_run);
}

static int
action_restow(const char *source, const char *target, const char *pkg, const CommandContext *ctx)
{
    return relink_package(source, target, pkg, ctx->opts->auto_install, ctx->opts->dry_run);
}

static int
action_diff(const char *source, const char *target, const char *pkg, const CommandContext *ctx)
{
    return link_package(source, target, pkg, ctx->opts->auto_install, true);
}

int cmd_stow(const CommandContext *ctx)
{
    return foreach_package(ctx, action_stow);
}

int cmd_unstow(const CommandContext *ctx)
{
    return foreach_package(ctx, action_unstow);
}

int cmd_restow(const CommandContext *ctx)
{
    return foreach_package(ctx, action_restow);
}

int cmd_all(const CommandContext *ctx)
{
    link_all_packages(
        ctx->source_dir, ctx->global_target_dir, ctx->opts->auto_install, ctx->opts->dry_run);
    return 0;
}

int cmd_diff(const CommandContext *ctx)
{
    if (ctx->args->count > ctx->arg_offset) {
        return foreach_package(ctx, action_diff);
    }
    link_all_packages(ctx->source_dir, ctx->global_target_dir, ctx->opts->auto_install, true);
    return 0;
}

int cmd_scan(const CommandContext *ctx)
{
    if (ctx->args->count > ctx->arg_offset) {
        for (size_t i = ctx->arg_offset; i < ctx->args->count; i++) {
            scan_package(ctx->source_dir, ctx->args->items[i]);
        }
    } else {
        StringArray pkgs;
        str_array_init(&pkgs);
        get_all_packages(ctx->source_dir, &pkgs);
        for (size_t i = 0; i < pkgs.count; i++) {
            scan_package(ctx->source_dir, pkgs.items[i]);
        }
        str_array_free(&pkgs);
    }
    return 0;
}

int cmd_check(const CommandContext *ctx)
{
    if (ctx->args->count > ctx->arg_offset &&
        strcmp(ctx->args->items[ctx->arg_offset], "symlinks") == 0) {
        check_symlink_health(ctx->source_dir, ctx->global_target_dir);
        return 0;
    }

    if (ctx->args->count > ctx->arg_offset) {
        for (size_t i = ctx->arg_offset; i < ctx->args->count; i++) {
            check_package_dependencies(ctx->source_dir,
                                       ctx->args->items[i],
                                       ctx->opts->auto_install,
                                       ctx->opts->dry_run);
        }
    } else {
        check_package_dependencies(
            ctx->source_dir, NULL, ctx->opts->auto_install, ctx->opts->dry_run);
    }
    check_symlink_health(ctx->source_dir, ctx->global_target_dir);
    return 0;
}

int cmd_check_symlinks(const CommandContext *ctx)
{
    check_symlink_health(ctx->source_dir, ctx->global_target_dir);
    return 0;
}

int cmd_fix_conflicts(const CommandContext *ctx)
{
    unfold_directory_symlinks(ctx->global_target_dir, ctx->source_dir, NULL, ctx->opts->dry_run);
    return 0;
}
