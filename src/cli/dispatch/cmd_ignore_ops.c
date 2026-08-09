/*
 * Symlink & Dependency Manager (symdep)
 * CLI Ignore Rules Action Handlers Submodule
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

bool parse_ignore_args(const CommandContext *ctx,
                       const char **out_pkg,
                       const char *const **out_patterns,
                       size_t *out_count)
{
    *out_pkg = NULL;
    *out_patterns = NULL;
    *out_count = 0;

    if (ctx->args->count <= ctx->arg_offset) {
        return false;
    }

    size_t pattern_start = ctx->arg_offset;
    bool force_global = false;

    // Support -g / --global flag override
    const char *first_arg = ctx->args->items[pattern_start];
    if (strcmp(first_arg, "-g") == 0 || strcmp(first_arg, "--global") == 0) {
        force_global = true;
        pattern_start++;
        if (pattern_start >= ctx->args->count) {
            log_error("Option '%s' specified, but no patterns provided!", first_arg);
            return false;
        }
        first_arg = ctx->args->items[pattern_start];
    }

    if (!force_global) {
        char candidate_pkg[STOW_PATH_LARGE];
        join_path(candidate_pkg, sizeof(candidate_pkg), ctx->source_dir, first_arg);

        if (is_dir(candidate_pkg)) {
            if (ctx->args->count > pattern_start + 1) {
                *out_pkg = first_arg;
                pattern_start++;
            } else {
                const char *action_cmd =
                    (ctx->arg_offset >= 2) ? ctx->args->items[1] : ctx->args->items[0];

                log_error("Package '%s' specified, but no patterns provided!", first_arg);
                log_info("Hint: Use 'symdep %s %s %s <pattern...>' or "
                         "'symdep ignore -g %s'",
                         (ctx->arg_offset >= 2) ? "ignore" : "",
                         action_cmd,
                         first_arg,
                         first_arg);
                return false;
            }
        }
    }

    *out_count = ctx->args->count - pattern_start;
    if (*out_count > 0) {
        *out_patterns = (const char *const *)&ctx->args->items[pattern_start];
        return true;
    }

    return false;
}

int cmd_ignore_init(const CommandContext *ctx)
{
    size_t count = ctx->args->count - ctx->arg_offset;
    const char *const *pkgs =
        (count > 0) ? (const char *const *)&ctx->args->items[ctx->arg_offset] : NULL;
    ignore_init(ctx->source_dir, pkgs, count);
    return 0;
}

int cmd_ignore_add(const CommandContext *ctx)
{
    const char *pkg = NULL;
    const char *const *patterns = NULL;
    size_t count = 0;

    if (!parse_ignore_args(ctx, &pkg, &patterns, &count)) {
        if (!pkg) {
            log_error("Usage: symdep ignore add [pkg] <pattern...>");
        }
        return 1;
    }

    ignore_add_patterns(ctx->source_dir, pkg, patterns, count);
    return 0;
}

int cmd_ignore_remove(const CommandContext *ctx)
{
    const char *pkg = NULL;
    const char *const *patterns = NULL;
    size_t count = 0;

    if (!parse_ignore_args(ctx, &pkg, &patterns, &count)) {
        if (!pkg) {
            log_error("Usage: symdep ignore remove [pkg] <pattern...>");
        }
        return 1;
    }

    ignore_remove_patterns(ctx->source_dir, pkg, patterns, count);
    return 0;
}

int cmd_ignore_clear(const CommandContext *ctx)
{
    size_t count = ctx->args->count - ctx->arg_offset;
    const char *const *pkgs =
        (count > 0) ? (const char *const *)&ctx->args->items[ctx->arg_offset] : NULL;
    ignore_clear(ctx->source_dir, pkgs, count);
    return 0;
}

int cmd_ignore_show(const CommandContext *ctx)
{
    size_t count = ctx->args->count - ctx->arg_offset;
    const char *const *pkgs =
        (count > 0) ? (const char *const *)&ctx->args->items[ctx->arg_offset] : NULL;
    ignore_show(ctx->source_dir, pkgs, count);
    return 0;
}
