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

#define _GNU_SOURCE
#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L

#include "../../test_framework.h"
#include "core/config.h"

void test_get_active_dotfiles_dir_cascade(void)
{
    char tmp_dir[PATH_MAX];
    ASSERT(create_test_tmp_dir(tmp_dir, sizeof(tmp_dir), "cfg_casc") != NULL,
           "Should create temporary directory for cascade test");

    const char *old_stow_dir = getenv("SYMDEP_SOURCE_DIR");
    char old_stow_buf[PATH_MAX] = {0};
    if (old_stow_dir) {
        snprintf(old_stow_buf, sizeof(old_stow_buf), "%s", old_stow_dir);
    }

    const char *old_dot_dir = getenv("SOURCE_DIR");
    char old_dot_buf[PATH_MAX] = {0};
    if (old_dot_dir) {
        snprintf(old_dot_buf, sizeof(old_dot_buf), "%s", old_dot_dir);
    }

    const char *old_xdg = getenv("XDG_CONFIG_HOME");
    char old_xdg_buf[PATH_MAX] = {0};
    if (old_xdg) {
        snprintf(old_xdg_buf, sizeof(old_xdg_buf), "%s", old_xdg);
    }

    setenv("XDG_CONFIG_HOME", tmp_dir, 1);

    char cli_dir[PATH_MAX];
    char env_dir[PATH_MAX];
    char cfg_dir[PATH_MAX];
    snprintf(cli_dir, sizeof(cli_dir), "%s/cli_source", tmp_dir);
    snprintf(env_dir, sizeof(env_dir), "%s/env_source", tmp_dir);
    snprintf(cfg_dir, sizeof(cfg_dir), "%s/cfg_source", tmp_dir);

    ASSERT(mkdir(cli_dir, 0755) == 0, "Should create cli_source");
    ASSERT(mkdir(env_dir, 0755) == 0, "Should create env_source");
    ASSERT(mkdir(cfg_dir, 0755) == 0, "Should create cfg_source");

    config_add_source_dir(cfg_dir);
    setenv("SYMDEP_SOURCE_DIR", env_dir, 1);

    char buf[PATH_MAX];

    // Priority 1: CLI override > ENV > Config
    get_active_source_dir(cli_dir, buf, sizeof(buf));
    ASSERT_STR_EQ(buf, cli_dir);

    // Priority 2: ENV > Config (CLI is NULL)
    get_active_source_dir(NULL, buf, sizeof(buf));
    ASSERT_STR_EQ(buf, env_dir);

    // Priority 3: Config file (CLI is NULL, ENV is unset)
    unsetenv("SYMDEP_SOURCE_DIR");
    unsetenv("SOURCE_DIR");
    unsetenv("STOW_DOTFILES_DIR");
    unsetenv("DOTFILES_DIR");
    get_active_source_dir(NULL, buf, sizeof(buf));
    ASSERT_STR_EQ(buf, cfg_dir);

    // Priority 4: Fallback CWD
    char cfg_file[PATH_MAX];
    snprintf(cfg_file, sizeof(cfg_file), "%s/symdep/config", tmp_dir);
    remove(cfg_file);
    snprintf(cfg_file, sizeof(cfg_file), "%s/stow-manager/config", tmp_dir);
    remove(cfg_file);

    get_active_source_dir(NULL, buf, sizeof(buf));
    ASSERT(strlen(buf) > 0, "Fallback active source dir should not be empty");

    if (old_stow_buf[0] != '\0') {
        setenv("SYMDEP_SOURCE_DIR", old_stow_buf, 1);
    } else {
        unsetenv("SYMDEP_SOURCE_DIR");
    }

    if (old_dot_buf[0] != '\0') {
        setenv("SOURCE_DIR", old_dot_buf, 1);
    } else {
        unsetenv("SOURCE_DIR");
    }

    if (old_xdg_buf[0] != '\0') {
        setenv("XDG_CONFIG_HOME", old_xdg_buf, 1);
    } else {
        unsetenv("XDG_CONFIG_HOME");
    }

    cleanup_test_dir(tmp_dir);
}
