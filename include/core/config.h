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

typedef struct {
    char config_file_path[STOW_PATH_MAX];
    StringArray source_dirs;
    char target_dir[STOW_PATH_MAX];
} Config;

void config_init(Config *cfg);
void config_free(Config *cfg);
void get_config_file_path(char *buf, size_t buf_size);
bool config_load(Config *cfg);
bool config_save(const Config *cfg);

void config_set_source_dir(const char *path);
void config_add_source_dir(const char *path);
void config_remove_source_dir(const char *path);
void config_set_target_dir(const char *path);
void config_show(void);

void get_active_source_dir(const char *cli_override, char *buf, size_t buf_size);
void get_active_target_dir(const char *cli_override, char *buf, size_t buf_size);
void get_active_target_dir_for_pkg(const char *cli_override,
                                   const char *source_dir,
                                   const char *pkg_name,
                                   char *buf,
                                   size_t buf_size);

#endif /* SYMDEP_CONFIG_H */

