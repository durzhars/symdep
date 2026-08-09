/*
 * Symlink & Dependency Manager (symdep)
 * Ignore Pattern Add & Remove Operations Submodule
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

void ignore_add_patterns(const char *source_dir,
                         const char *pkg_name,
                         const char *const *patterns,
                         size_t count)
{
    if (!patterns || count == 0) {
        log_error("No patterns specified to add!");
        return;
    }

    int is_package = (pkg_name && *pkg_name != '\0');

    StringArray global_patterns;
    str_array_init(&global_patterns);
    if (is_package) {
        parse_stowignore_raw(source_dir, &global_patterns);
    }

    char path[STOW_PATH_HUGE];
    get_stowignore_path(source_dir, pkg_name, path, sizeof(path));

    if (!file_exists(path)) {
        ignore_init_single(source_dir, pkg_name);
    }

    StringArray existing;
    str_array_init(&existing);

    FILE *rfp = fopen(path, "r");
    if (rfp) {
        char linebuf[512];
        while (fgets(linebuf, sizeof(linebuf), rfp)) {
            char *trimmed = trim_whitespace(linebuf);
            if (*trimmed != '\0') {
                str_array_append(&existing, trimmed);
            }
        }
        fclose(rfp);
    }

    FILE *afp = fopen(path, "a");
    if (!afp) {
        log_error("Failed to open '.symignore' for writing: %s", path);
        str_array_free(&existing);
        str_array_free(&global_patterns);
        return;
    }

    for (size_t i = 0; i < count; i++) {
        const char *pat = patterns[i];
        if (!pat || *pat == '\0') {
            continue;
        }

        if (is_package && str_array_contains(&global_patterns, pat)) {
            log_warn("Pattern '%s' is already in global '.symignore' (skipping duplicate).", pat);
            continue;
        }

        if (str_array_contains(&existing, pat)) {
            log_warn("Pattern '%s' already exists in '.symignore' (skipping duplicate).", pat);
            continue;
        }

        fprintf(afp, "%s\n", pat);
        str_array_append(&existing, pat);

        if (is_package) {
            log_success("Added pattern '%s' to '.symignore' for package '%s'.", pat, pkg_name);
        } else {
            log_success("Added pattern '%s' to global '.symignore'.", pat);
        }
    }

    fclose(afp);
    str_array_free(&existing);
    str_array_free(&global_patterns);
}

void ignore_remove_patterns(const char *source_dir,
                            const char *pkg_name,
                            const char *const *patterns,
                            size_t count)
{
    if (!patterns || count == 0) {
        log_error("No patterns specified to remove!");
        return;
    }

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

    FILE *fp = fopen(path, "r");
    if (!fp) {
        log_error("Failed to open '.symignore' for reading: %s", path);
        return;
    }

    StringArray lines;
    str_array_init(&lines);

    char linebuf[512];
    while (fgets(linebuf, sizeof(linebuf), fp)) {
        size_t len = strlen(linebuf);
        if (len > 0 && (linebuf[len - 1] == '\n' || linebuf[len - 1] == '\r')) {
            linebuf[len - 1] = '\0';
        }
        str_array_append(&lines, linebuf);
    }
    fclose(fp);

    bool file_modified = false;

    for (size_t p = 0; p < count; p++) {
        const char *pat = patterns[p];
        bool found = false;

        StringArray filtered;
        str_array_init(&filtered);

        for (size_t l = 0; l < lines.count; l++) {
            char *trimmed = trim_whitespace(lines.items[l]);
            if (strcmp(trimmed, pat) == 0) {
                found = true;
                file_modified = true;
            } else {
                str_array_append(&filtered, lines.items[l]);
            }
        }

        str_array_free(&lines);
        lines = filtered;

        if (found) {
            if (pkg_name && *pkg_name != '\0') {
                log_success("Removed pattern '%s' from '.symignore' for package '%s'.", pat, pkg_name);
            } else {
                log_success("Removed pattern '%s' from global '.symignore'.", pat);
            }
        } else {
            log_warn("Pattern '%s' was not found in '.symignore'.", pat);
        }
    }

    if (file_modified) {
        FILE *wfp = fopen(path, "w");
        if (wfp) {
            for (size_t i = 0; i < lines.count; i++) {
                fprintf(wfp, "%s\n", lines.items[i]);
            }
            fclose(wfp);
        } else {
            log_error("Failed to write updated '.symignore': %s", path);
        }
    }

    str_array_free(&lines);
}
