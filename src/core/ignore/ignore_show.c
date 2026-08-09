/*
 * Symlink & Dependency Manager (symdep)
 * Ignore Rules Formatting & Display Submodule
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

static void print_ignore_file_lines(const char *path, const StringArray *global_patterns)
{
    FILE *fp = fopen(path, "r");
    if (!fp) {
        return;
    }

    char line[512];
    while (fgets(line, sizeof(line), fp)) {
        size_t len = strlen(line);
        while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
            line[--len] = '\0';
        }

        char *trimmed = trim_whitespace(line);

        /* Strip inline or full-line comments */
        char *hash = strchr(trimmed, '#');
        if (hash) {
            *hash = '\0';
            trimmed = trim_whitespace(trimmed);
        }

        /* Skip blank lines / comment-only lines */
        if (*trimmed == '\0') {
            continue;
        }

        bool is_redundant = (bool)(global_patterns && str_array_contains(global_patterns, trimmed));

        if (is_redundant) {
            printf("  %s (redundant: covered by global)%s\n", trimmed, COLOR_RESET);
        } else {
            printf("  %s\n", trimmed);
        }
    }
    fclose(fp);
}

static void ignore_show_global(const char *source_dir)
{
    char global_path[STOW_PATH_MAX];
    get_stowignore_path(source_dir, NULL, global_path, sizeof(global_path));

    if (!file_exists(global_path)) {
        log_warn("No global '.symignore' file found at repository root.");
        return;
    }

    printf("\n%s%s=== Global Ignore Rules [.symignore] ===%s\n\n",
           COLOR_CYAN,
           COLOR_BOLD,
           COLOR_RESET);
    print_ignore_file_lines(global_path, NULL);
    printf("\n");
}

static void ignore_show_package(const char *source_dir, const char *pkg_name)
{
    char pkg_path[STOW_PATH_MAX];
    get_stowignore_path(source_dir, pkg_name, pkg_path, sizeof(pkg_path));

    if (!file_exists(pkg_path)) {
        log_warn("No '.symignore' file found for package '%s'.", pkg_name);
        return;
    }

    printf("\n%s%s=== Ignore Rules [.symignore] for Package '%s' ===%s\n\n",
           COLOR_CYAN,
           COLOR_BOLD,
           pkg_name,
           COLOR_RESET);

    /* Load raw global ignore patterns to mark redundant declarations */
    StringArray global_patterns;
    str_array_init(&global_patterns);
    parse_stowignore_raw(source_dir, &global_patterns);

    print_ignore_file_lines(pkg_path, &global_patterns);
    printf("\n");

    str_array_free(&global_patterns);
}

static void ignore_show_all(const char *source_dir)
{
    ignore_show_global(source_dir);

    StringArray packages;
    str_array_init(&packages);
    get_all_packages(source_dir, &packages);

    for (size_t i = 0; i < packages.count; i++) {
        char pkg_path[STOW_PATH_MAX];
        get_stowignore_path(source_dir, packages.items[i], pkg_path, sizeof(pkg_path));
        if (file_exists(pkg_path)) {
            ignore_show_package(source_dir, packages.items[i]);
        }
    }

    str_array_free(&packages);
}

void ignore_show(const char *source_dir, const char *const *pkgs, size_t count)
{
    if (!pkgs || count == 0) {
        ignore_show_all(source_dir);
        return;
    }

    for (size_t i = 0; i < count; i++) {
        if (strcmp(pkgs[i], "all") == 0 || strcmp(pkgs[i], "--all") == 0) {
            ignore_show_all(source_dir);
        } else {
            ignore_show_package(source_dir, pkgs[i]);
        }
    }
}
