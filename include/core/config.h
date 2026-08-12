/*
 * Symlink & Dependency Manager (symdep)
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

#ifndef SYMDEP_CONFIG_H
#define SYMDEP_CONFIG_H

#include "utils/defs.h"
#include "utils/str.h"

/**
 * @struct Config
 * @brief Global user configuration state (~/.config/symdep/config).
 */
typedef struct {
    char config_file_path[STOW_PATH_MAX]; /**< Absolute path to active config file */
    StringArray source_dirs;               /**< Multi-repository source directories */
    char target_dir[STOW_PATH_MAX];       /**< Default target home directory */
    char pkg_manager[64];                  /**< Configured package manager override */
    char elevation_tool[64];               /**< Configured privilege elevation tool */
} Config;

/** Initialize an empty Config struct */
void config_init(Config *cfg);

/** Free allocations associated with a Config struct */
void config_free(Config *cfg);

/** Load active configuration from disk into cfg struct */
void config_load_active(Config *cfg);

/** Get absolute path to user configuration file (~/.config/symdep/config) */
void get_config_file_path(char *buf, size_t buf_size);

/** Load config file from disk */
bool config_load(Config *cfg);

/** Save config struct to disk */
bool config_save(const Config *cfg);

/** Set primary source repository path */
void config_set_source_dir(const char *path);

/** Add an additional source repository path (multi-repo mode) */
void config_add_source_dir(const char *path);

/** Remove a source repository path from config */
void config_remove_source_dir(const char *path);

/** Set default target home directory path */
void config_set_target_dir(const char *path);

/** Set default package manager override */
void config_set_pkg_manager(const char *mgr_name);

/** Set default privilege elevation tool override */
void config_set_elevation_tool(const char *tool_name);

/** Display active configuration settings to stdout */
void config_show(void);

/** Resolve active source repository directory respecting CLI -> Env -> Config -> CWD precedence */
void get_active_source_dir(const char *cli_override, char *buf, size_t buf_size);

/** Resolve active target home directory respecting CLI -> Env -> Config -> $HOME precedence */
void get_active_target_dir(const char *cli_override, char *buf, size_t buf_size);

/** Resolve both source and target directories in a single pass */
void get_active_config_dirs(const char *cli_source_override,
                            const char *cli_target_override,
                            char *src_buf,
                            size_t src_size,
                            char *tgt_buf,
                            size_t tgt_size);

/** Resolve target directory for a specific package respecting per-package TARGET in .symdeps */
void get_active_target_dir_for_pkg(const char *cli_override,
                                   const char *source_dir,
                                   const char *pkg_name,
                                   char *buf,
                                   size_t buf_size);

#endif /* SYMDEP_CONFIG_H */
