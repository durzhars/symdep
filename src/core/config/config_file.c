/*
 * Symlink & Dependency Manager (symdep)
 * Configuration File I/O & XDG Path Location Submodule
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

void config_init(Config *cfg)
{
    memset(cfg->config_file_path, 0, sizeof(cfg->config_file_path));
    str_array_init(&cfg->source_dirs);
    memset(cfg->target_dir, 0, sizeof(cfg->target_dir));
    memset(cfg->pkg_manager, 0, sizeof(cfg->pkg_manager));
    memset(cfg->elevation_tool, 0, sizeof(cfg->elevation_tool));
    get_config_file_path(cfg->config_file_path, sizeof(cfg->config_file_path));
}

void config_free(Config *cfg)
{
    str_array_free(&cfg->source_dirs);
}

void config_load_active(Config *cfg)
{
    config_init(cfg);
    (void)config_load(cfg);
}

void get_config_file_path(char *buf, size_t buf_size)
{
    const char *env_cfg = getenv("SYMDEP_CONFIG_FILE");
    if (!env_cfg || *env_cfg == '\0') {
        env_cfg = getenv("STOW_CONFIG_FILE");
    }
    if (env_cfg && *env_cfg != '\0') {
        snprintf(buf, buf_size, "%s", env_cfg);
        return;
    }

    char xdg_config[STOW_PATH_MAX];
    bool has_xdg_config = get_xdg_config_home(xdg_config, sizeof(xdg_config));

    if (has_xdg_config) {
        char primary[STOW_PATH_LARGE];
        snprintf(primary, sizeof(primary), "%s/symdep/config", xdg_config);

        if (file_exists(primary)) {
            snprintf(buf, buf_size, "%s", primary);
            return;
        }

        // Backward compatibility fallback for legacy config location
        char legacy[STOW_PATH_LARGE];
        snprintf(legacy, sizeof(legacy), "%s/stow-manager/config", xdg_config);
        if (file_exists(legacy)) {
            snprintf(buf, buf_size, "%s", legacy);
            return;
        }
    }

    StringArray config_dirs;
    str_array_init(&config_dirs);
    get_xdg_config_dirs(&config_dirs);

    for (size_t i = 0; i < config_dirs.count; i++) {
        char sys_path[STOW_PATH_LARGE];
        snprintf(sys_path, sizeof(sys_path), "%s/symdep/config", config_dirs.items[i]);
        if (file_exists(sys_path)) {
            snprintf(buf, buf_size, "%s", sys_path);
            str_array_free(&config_dirs);
            return;
        }
    }
    str_array_free(&config_dirs);

    if (has_xdg_config) {
        snprintf(buf, buf_size, "%s/symdep/config", xdg_config);
    } else {
        buf[0] = '\0';
    }
}

bool config_load(Config *cfg)
{
    if (cfg->config_file_path[0] == '\0') {
        get_config_file_path(cfg->config_file_path, sizeof(cfg->config_file_path));
    }
    str_array_init(&cfg->source_dirs);
    memset(cfg->target_dir, 0, sizeof(cfg->target_dir));

    FILE *fp = fopen(cfg->config_file_path, "r");
    if (!fp) {
        return false;
    }

    char *linebuf = NULL;
    size_t linecap = 0;
    ssize_t linelen;

    while ((linelen = getline(&linebuf, &linecap, fp)) != -1) {
        (void)linelen;
        char *trimmed = trim_whitespace(linebuf);
        if (trimmed[0] == '#' || trimmed[0] == '\0') {
            continue;
        }

        char *eq = strchr(trimmed, '=');
        if (eq) {
            *eq = '\0';
            char *key = trim_whitespace(trimmed);
            char *val = trim_whitespace(eq + 1);

            if (strcmp(key, "SOURCE_DIR") == 0 || strcmp(key, "SOURCE_DIRS") == 0 ||
                strcmp(key, "DOTFILES_DIR") == 0 || strcmp(key, "DOTFILES_DIRS") == 0) {
                char *saveptr = NULL;
                char *token = strtok_r(val, ":", &saveptr);
                while (token) {
                    char *p = trim_whitespace(token);
                    if (strlen(p) > 0 && is_dir(p) && !str_array_contains(&cfg->source_dirs, p)) {
                        str_array_append(&cfg->source_dirs, p);
                    }
                    token = strtok_r(NULL, ":", &saveptr);
                }
            } else if (strcmp(key, "TARGET_DIR") == 0) {
                snprintf(cfg->target_dir, sizeof(cfg->target_dir), "%s", val);
            } else if (strcmp(key, "PKG_MANAGER") == 0 || strcmp(key, "PACKAGE_MANAGER") == 0) {
                snprintf(cfg->pkg_manager, sizeof(cfg->pkg_manager), "%s", val);
            } else if (strcmp(key, "ELEVATION_TOOL") == 0) {
                snprintf(cfg->elevation_tool, sizeof(cfg->elevation_tool), "%s", val);
            }
        }
    }

    free(linebuf);
    fclose(fp);
    return true;
}

bool config_save(const Config *cfg)
{
    char dir_path[STOW_PATH_LARGE];
    snprintf(dir_path, sizeof(dir_path), "%s", cfg->config_file_path);
    char *last_slash = strrchr(dir_path, '/');
    if (last_slash) {
        *last_slash = '\0';
        mkdir_p(dir_path, 0755);
    }

    FILE *fp = fopen(cfg->config_file_path, "w");
    if (!fp) {
        log_error("Failed to open config file for writing: %s", cfg->config_file_path);
        return false;
    }

    fprintf(fp,
            "# =============================================================================\n"
            "# Symlink & Dependency Manager Configuration (Auto-Generated)\n"
            "#\n"
            "# NOTE: This file is automatically generated and re-serialized by CLI commands.\n"
            "# Manual edits to key-values are safe, but comments will NOT be preserved.\n"
            "# =============================================================================\n\n");

    fprintf(fp, "SOURCE_DIRS=");
    for (size_t i = 0; i < cfg->source_dirs.count; i++) {
        fprintf(fp, "%s%s", cfg->source_dirs.items[i], (i + 1 < cfg->source_dirs.count) ? ":" : "");
    }
    fprintf(fp, "\n");

    if (cfg->target_dir[0] != '\0') {
        fprintf(fp, "TARGET_DIR=%s\n", cfg->target_dir);
    }
    if (cfg->pkg_manager[0] != '\0') {
        fprintf(fp, "PKG_MANAGER=%s\n", cfg->pkg_manager);
    }
    if (cfg->elevation_tool[0] != '\0') {
        fprintf(fp, "ELEVATION_TOOL=%s\n", cfg->elevation_tool);
    }

    fclose(fp);
    return true;
}
