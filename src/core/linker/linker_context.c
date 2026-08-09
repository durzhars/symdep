/*
 * Symlink & Dependency Manager (symdep)
 * Linker Context Management Submodule
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

#include "core/linker/internal.h"

void init_package_ignores(StringArray *raw_ignores, const char *source_dir, const char *pkg_dir)
{
    str_array_init(raw_ignores);
    get_default_stowignore(raw_ignores);
    parse_stowignore_raw(source_dir, raw_ignores);
    if (pkg_dir && *pkg_dir != '\0') {
        parse_stowignore_raw(pkg_dir, raw_ignores);
    }
}

bool package_context_init(PackageContext *ctx,
                          const char *source_dir,
                          const char *target_dir,
                          const char *pkg_name,
                          bool auto_install,
                          bool dry_run)
{
    if (!ctx || !source_dir || !target_dir || !pkg_name) {
        return false;
    }

    memset(ctx, 0, sizeof(*ctx));
    ctx->source_dir = source_dir;
    ctx->target_dir = target_dir;
    ctx->pkg_name = pkg_name;
    ctx->auto_install = auto_install;
    ctx->dry_run = dry_run;

    join_path(ctx->pkg_dir, sizeof(ctx->pkg_dir), source_dir, pkg_name);

    if (!is_dir(ctx->pkg_dir)) {
        return false;
    }

    if (realpath(ctx->pkg_dir, ctx->real_pkg_dir) == NULL) {
        snprintf(ctx->real_pkg_dir, sizeof(ctx->real_pkg_dir), "%s", ctx->pkg_dir);
    }

    init_package_ignores(&ctx->raw_ignores, source_dir, ctx->pkg_dir);
    collect_package_files(ctx->pkg_dir, &ctx->raw_ignores, &ctx->pkg_files);
    return true;
}

void package_context_free(PackageContext *ctx)
{
    if (!ctx) {
        return;
    }
    pkg_file_list_free(&ctx->pkg_files);
    str_array_free(&ctx->raw_ignores);
}

static void get_timestamp_str(char *buf, size_t size)
{
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    if (t) {
        strftime(buf, size, "%Y%m%d_%H%M%S", t);
    } else {
        snprintf(buf, size, "unknown");
    }
}

void build_unique_backup_path(const char *target_path, char *out_buf, size_t out_size)
{
    char ts[64];
    get_timestamp_str(ts, sizeof(ts));
    char base_backup[STOW_PATH_HUGE];
    snprintf(base_backup, sizeof(base_backup), "%s.symdep_backup_%s", target_path, ts);

    snprintf(out_buf, out_size, "%s", base_backup);
    unsigned int counter = 1;
    while (file_exists(out_buf)) {
        snprintf(out_buf, out_size, "%s.%u", base_backup, counter++);
    }
}

bool get_symlink_owner_package(const char *symlink_path,
                               const char *source_dir,
                               char *owner_pkg_buf,
                               size_t buf_size)
{
    static char cached_source_dir[STOW_PATH_MAX] = {0};
    static char cached_real_source[STOW_PATH_MAX] = {0};

    if (!source_dir || *source_dir == '\0') {
        return false;
    }

    if (strcmp(cached_source_dir, source_dir) != 0) {
        snprintf(cached_source_dir, sizeof(cached_source_dir), "%s", source_dir);
        if (realpath(source_dir, cached_real_source) == NULL) {
            snprintf(cached_real_source, sizeof(cached_real_source), "%s", source_dir);
        }
    }
    const char *real_source = cached_real_source;

    char *target = read_symlink_target(symlink_path);
    if (!target) {
        return false;
    }

    bool found = false;
    if (is_path_prefix(target, real_source)) {
        size_t prefix_len = strlen(real_source);
        const char *rel = target + prefix_len;
        while (*rel == '/') {
            rel++;
        }

        const char *slash = strchr(rel, '/');
        if (slash) {
            size_t pkg_len = (size_t)(slash - rel);
            if (pkg_len > 0 && pkg_len < buf_size) {
                strncpy(owner_pkg_buf, rel, pkg_len);
                owner_pkg_buf[pkg_len] = '\0';
                found = true;
            }
        } else if (strlen(rel) > 0 && strlen(rel) < buf_size) {
            snprintf(owner_pkg_buf, buf_size, "%s", rel);
            found = true;
        }
    }

    free(target);
    return found;
}
