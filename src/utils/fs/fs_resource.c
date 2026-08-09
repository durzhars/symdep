/*
 * Dotfiles Stow Manager (stow-manager)
 * Application Resource File Resolution Submodule
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

#include "utils/fs/internal.h"

static inline const char *get_system_datadir(void)
{
#ifdef DATADIR
    return STR(DATADIR);
#else
    return NULL;
#endif
}

static FILE *try_open_resource(const char *base_dir, const char *app, const char *filename)
{
    if (!base_dir || *base_dir == '\0') {
        return NULL;
    }
    char p[STOW_PATH_LARGE];
    join_path(p, sizeof(p), base_dir, app);
    char full_p[STOW_PATH_LARGE];
    join_path(full_p, sizeof(full_p), p, filename);
    if (file_exists(full_p)) {
        return fopen(full_p, "r");
    }
    return NULL;
}

FILE *open_resource_file(const char *filename)
{
    if (!filename || *filename == '\0') {
        return NULL;
    }

    FILE *fp = NULL;

    // 1. CWD project resources
    char p_res[STOW_PATH_LARGE];
    join_path(p_res, sizeof(p_res), "resources", filename);
    if (file_exists(p_res) && (fp = fopen(p_res, "r")) != NULL) {
        return fp;
    }

    char data_home[STOW_PATH_MAX] = {0};
    char config_home[STOW_PATH_MAX] = {0};
    bool has_data = get_xdg_data_home(data_home, sizeof(data_home));
    bool has_config = get_xdg_config_home(config_home, sizeof(config_home));

    // 2. XDG data home (symdep) & config home (symdep)
    if (has_data && (fp = try_open_resource(data_home, "symdep", filename)) != NULL) {
        return fp;
    }
    if (has_config && (fp = try_open_resource(config_home, "symdep", filename)) != NULL) {
        return fp;
    }

    // 3. XDG data dirs (symdep)
    StringArray data_dirs;
    str_array_init(&data_dirs);
    get_xdg_data_dirs(&data_dirs);
    for (size_t i = 0; i < data_dirs.count; i++) {
        if ((fp = try_open_resource(data_dirs.items[i], "symdep", filename)) != NULL) {
            str_array_free(&data_dirs);
            return fp;
        }
    }
    str_array_free(&data_dirs);

    // 4. System DATADIR (symdep)
    const char *sys_datadir = get_system_datadir();
    if (sys_datadir && (fp = try_open_resource(sys_datadir, "symdep", filename)) != NULL) {
        return fp;
    }

    // 5. Legacy fallbacks (stow-manager)
    if (has_data && (fp = try_open_resource(data_home, "stow-manager", filename)) != NULL) {
        return fp;
    }
    if (has_config && (fp = try_open_resource(config_home, "stow-manager", filename)) != NULL) {
        return fp;
    }

    return NULL;
}
