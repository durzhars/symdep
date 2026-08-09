/*
 * Symlink & Dependency Manager (symdep)
 * Ignore File Initialization, Pathing & Lifecycle Submodule
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

#include "core/ignore/internal.h"

void get_stowignore_path(const char *source_dir,
                         const char *pkg_name,
                         char *out_path,
                         size_t out_size)
{
    if (pkg_name && *pkg_name != '\0') {
        char pkg_dir[STOW_PATH_LARGE];
        join_path(pkg_dir, sizeof(pkg_dir), source_dir, pkg_name);
        mkdir_p(pkg_dir, 0755);
        join_path(out_path, out_size, pkg_dir, ".symignore");
        if (!file_exists(out_path)) {
            char legacy_path[STOW_PATH_HUGE];
            join_path(legacy_path, sizeof(legacy_path), pkg_dir, ".stowignore");
            if (file_exists(legacy_path)) {
                snprintf(out_path, out_size, "%s", legacy_path);
            }
        }
    } else {
        join_path(out_path, out_size, source_dir, ".symignore");
        if (!file_exists(out_path)) {
            char legacy_path[STOW_PATH_HUGE];
            join_path(legacy_path, sizeof(legacy_path), source_dir, ".stowignore");
            if (file_exists(legacy_path)) {
                snprintf(out_path, out_size, "%s", legacy_path);
            }
        }
    }
}

void ignore_init_single(const char *source_dir, const char *pkg_name)
{
    char path[STOW_PATH_HUGE];
    get_stowignore_path(source_dir, pkg_name, path, sizeof(path));

    if (file_exists(path)) {
        if (pkg_name && *pkg_name != '\0') {
            log_warn("'.symignore' already exists for package '%s'.", pkg_name);
        } else {
            log_warn("'.symignore' already exists at repository root.");
        }
        return;
    }

    FILE *fp = fopen(path, "w");
    if (!fp) {
        log_error("Failed to create '.symignore' at: %s", path);
        return;
    }

    if (pkg_name && *pkg_name != '\0') {
        fprintf(fp, "# .symignore for package '%s'\n", pkg_name);
    } else {
        fprintf(fp, "# Global .symignore for source repository\n");
    }

    FILE *tmpl = open_resource_file("symignore.template");
    if (!tmpl) {
        tmpl = open_resource_file("stowignore.template");
    }
    if (tmpl) {
        char line[512];
        while (fgets(line, sizeof(line), tmpl)) {
            fputs(line, fp);
        }
        fclose(tmpl);
    } else {
        fprintf(fp,
                "# Syntax matches standard glob/gitignore pattern rules\n\n"
                "# Build & Runtime Cache Artifacts\n"
                "*.zwc\n"
                "*.pyc\n"
                "*.symdep_backup_*\n\n"
                "# OS & Editor Metadata\n"
                ".DS_Store\n"
                "Thumbs.db\n"
                ".idea/\n"
                ".vscode/\n");
    }

    fclose(fp);

    if (pkg_name && *pkg_name != '\0') {
        log_success("Initialized '.symignore' for package '%s'.", pkg_name);
    } else {
        log_success("Initialized global '.symignore' at repository root.");
    }
}

void ignore_clear_single(const char *source_dir, const char *pkg_name)
{
    char path[STOW_PATH_HUGE];
    get_stowignore_path(source_dir, pkg_name, path, sizeof(path));

    if (!file_exists(path)) {
        if (pkg_name && *pkg_name != '\0') {
            log_warn("No '.symignore' file found for package '%s'.", pkg_name);
        } else {
            log_warn("No global '.symignore' file found at repository root.");
        }
        return;
    }

    if (remove(path) == 0) {
        if (pkg_name && *pkg_name != '\0') {
            log_success("Cleared '.symignore' file for package '%s'.", pkg_name);
        } else {
            log_success("Cleared global '.symignore' file at repository root.");
        }
    } else {
        log_error("Failed to delete '.symignore' file at: %s", path);
    }
}

void ignore_init(const char *source_dir, const char *const *pkgs, size_t count)
{
    if (!pkgs || count == 0) {
        ignore_init_single(source_dir, NULL);
        return;
    }
    for (size_t i = 0; i < count; i++) {
        ignore_init_single(source_dir, pkgs[i]);
    }
}

void ignore_clear(const char *source_dir, const char *const *pkgs, size_t count)
{
    if (!pkgs || count == 0) {
        ignore_clear_single(source_dir, NULL);
        return;
    }
    for (size_t i = 0; i < count; i++) {
        ignore_clear_single(source_dir, pkgs[i]);
    }
}
