/*
 * Symlink & Dependency Manager (symdep)
 * Configuration CLI Operations & Directory Mutators Submodule
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

#include "core/config/internal.h"

bool prepare_config_path(const char *input_path,
                         char *out_buf,
                         size_t buf_size,
                         const char *context,
                         bool check_sanity)
{
    if (!input_path || *input_path == '\0') {
        log_error("Invalid empty path provided for %s.", context);
        return false;
    }

    char temp[STOW_PATH_MAX];
    expand_tilde_path(input_path, temp, sizeof(temp));

    if (temp[0] != '/') {
        char cwd[STOW_PATH_MAX];
        if (getcwd(cwd, sizeof(cwd))) {
            join_path(out_buf, buf_size, cwd, temp);
        } else {
            snprintf(out_buf, buf_size, "%s", temp);
        }
    } else {
        snprintf(out_buf, buf_size, "%s", temp);
    }

    collapse_path(out_buf);
    normalize_path(out_buf);

    if (check_sanity) {
        PathSanityResult sanity = verify_path_sanity(out_buf);
        if (sanity != PATH_VALID) {
            log_error("Cannot set %s: %s", context, path_sanity_strerror(sanity, out_buf));
            return false;
        }
    }
    return true;
}

void config_set_source_dir(const char *path)
{
    char abs_path[STOW_PATH_LARGE];
    if (!prepare_config_path(path, abs_path, sizeof(abs_path), "source directory", true)) {
        return;
    }

    Config cfg;
    config_load_active(&cfg);

    StringArray new_dirs;
    str_array_init(&new_dirs);
    str_array_append(&new_dirs, abs_path);

    for (size_t i = 0; i < cfg.source_dirs.count; i++) {
        if (strcmp(cfg.source_dirs.items[i], abs_path) != 0) {
            str_array_append(&new_dirs, cfg.source_dirs.items[i]);
        }
    }

    str_array_free(&cfg.source_dirs);
    cfg.source_dirs = new_dirs;

    if (config_save(&cfg)) {
        log_success("Set primary source directory to: %s", abs_path);
    }
    config_free(&cfg);
}

void config_add_source_dir(const char *path)
{
    char abs_path[STOW_PATH_LARGE];
    if (!prepare_config_path(path, abs_path, sizeof(abs_path), "source directory", true)) {
        return;
    }

    Config cfg;
    config_load_active(&cfg);

    if (!str_array_contains(&cfg.source_dirs, abs_path)) {
        str_array_append(&cfg.source_dirs, abs_path);
        if (config_save(&cfg)) {
            log_success("Added source directory: %s", abs_path);
        }
    } else {
        log_info("Source directory already registered: %s", abs_path);
    }

    config_free(&cfg);
}

void config_remove_source_dir(const char *path)
{
    if (!path || *path == '\0') {
        return;
    }

    Config cfg;
    config_load_active(&cfg);

    bool removed = false;

    if (strcmp(path, "target") == 0) {
        if (cfg.target_dir[0] != '\0') {
            cfg.target_dir[0] = '\0';
            removed = true;
            log_success("Removed target directory override from configuration.");
        }
    } else {
        char abs_path[STOW_PATH_LARGE];
        if (prepare_config_path(path, abs_path, sizeof(abs_path), "directory", false)) {
            if (cfg.target_dir[0] != '\0' && strcmp(abs_path, cfg.target_dir) == 0) {
                cfg.target_dir[0] = '\0';
                removed = true;
                log_success("Removed target directory override from configuration: %s", abs_path);
            }

            if (str_array_contains(&cfg.source_dirs, abs_path)) {
                size_t w = 0;
                for (size_t r = 0; r < cfg.source_dirs.count; r++) {
                    if (strcmp(cfg.source_dirs.items[r], abs_path) == 0) {
                        free(cfg.source_dirs.items[r]);
                    } else {
                        cfg.source_dirs.items[w++] = cfg.source_dirs.items[r];
                    }
                }
                cfg.source_dirs.count = w;
                removed = true;
                log_success("Removed source directory: %s", abs_path);
            }
        }
    }

    if (removed) {
        config_save(&cfg);
    } else {
        log_warn("Directory or setting is not in configuration: %s", path);
    }

    config_free(&cfg);
}

void config_set_target_dir(const char *path)
{
    if (!path || *path == '\0' || strcmp(path, "none") == 0 || strcmp(path, "clear") == 0 ||
        strcmp(path, "unset") == 0 || strcmp(path, "reset") == 0 || strcmp(path, "\"\"") == 0) {
        Config cfg;
        config_load_active(&cfg);
        cfg.target_dir[0] = '\0';
        if (config_save(&cfg)) {
            log_success("Cleared custom target directory override (reverting to $HOME default).");
        }
        config_free(&cfg);
        return;
    }

    char abs_path[STOW_PATH_MAX];
    if (!prepare_config_path(path, abs_path, sizeof(abs_path), "target directory", true)) {
        return;
    }

    Config cfg;
    config_load_active(&cfg);

    snprintf(cfg.target_dir, sizeof(cfg.target_dir), "%s", abs_path);

    if (config_save(&cfg)) {
        log_success("Set target directory to: %s", abs_path);
    }
    config_free(&cfg);
}

void config_set_pkg_manager(const char *mgr_name)
{
    Config cfg;
    config_load_active(&cfg);
    if (!mgr_name || *mgr_name == '\0' || strcmp(mgr_name, "none") == 0 ||
        strcmp(mgr_name, "clear") == 0 || strcmp(mgr_name, "auto") == 0) {
        cfg.pkg_manager[0] = '\0';
        if (config_save(&cfg)) {
            log_success("Cleared package manager override (reverting to auto-detection).");
        }
    } else {
        snprintf(cfg.pkg_manager, sizeof(cfg.pkg_manager), "%s", mgr_name);
        if (config_save(&cfg)) {
            log_success("Set package manager override to: %s", mgr_name);
        }
    }
    config_free(&cfg);
}

void config_set_elevation_tool(const char *tool_name)
{
    Config cfg;
    config_load_active(&cfg);
    if (!tool_name || *tool_name == '\0' || strcmp(tool_name, "auto") == 0 ||
        strcmp(tool_name, "clear") == 0) {
        cfg.elevation_tool[0] = '\0';
        if (config_save(&cfg)) {
            log_success("Cleared privilege elevation tool override (reverting to auto-detection).");
        }
    } else {
        snprintf(cfg.elevation_tool, sizeof(cfg.elevation_tool), "%s", tool_name);
        if (config_save(&cfg)) {
            log_success("Set privilege elevation tool to: %s", tool_name);
        }
    }
    config_free(&cfg);
}

void config_show(void)
{
    Config cfg;
    config_load_active(&cfg);

    printf("\n=== Symlink & Dependency Manager Configuration ===\n\n");
    printf("  Config File Path: %s\n", cfg.config_file_path);

    printf("  Source Repositories:\n");
    if (cfg.source_dirs.count == 0) {
        printf("    (none configured - using current working directory fallback)\n");
    } else {
        for (size_t i = 0; i < cfg.source_dirs.count; i++) {
            printf(
                "    %zu. %s%s\n", i + 1, cfg.source_dirs.items[i], (i == 0) ? " (primary)" : "");
        }
    }

    if (cfg.target_dir[0] != '\0') {
        printf("  Target Directory: %s (configured)\n", cfg.target_dir);
    } else {
        const char *env_home = getenv("HOME");
        if (env_home && *env_home != '\0' && verify_path_sanity(env_home) == PATH_VALID) {
            printf("  Target Directory: %s (fallback environment $HOME)\n", env_home);
        } else {
            printf("  Target Directory: (none - $HOME environment variable is not set)\n");
        }
    }

    if (cfg.pkg_manager[0] != '\0') {
        printf("  Package Manager: %s (configured)\n", cfg.pkg_manager);
    } else {
        printf("  Package Manager: (auto-detect via $PATH probing)\n");
    }

    if (cfg.elevation_tool[0] != '\0') {
        printf("  Privilege Elevation: %s (configured)\n", cfg.elevation_tool);
    } else {
        printf("  Privilege Elevation: (auto-detect via $PATH probing)\n");
    }
    printf("\n");

    config_free(&cfg);
}
