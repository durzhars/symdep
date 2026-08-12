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

void test_config_system(void)
{
    char tmp_dir[PATH_MAX];
    ASSERT(create_test_tmp_dir(tmp_dir, sizeof(tmp_dir), "cfg_test") != NULL,
           "Should create temporary test directory");

    const char *old_xdg = getenv("XDG_CONFIG_HOME");
    const char *old_symdep = getenv("SYMDEP_CONFIG_FILE");
    char old_xdg_buf[PATH_MAX] = {0};
    char old_symdep_buf[PATH_MAX] = {0};
    if (old_xdg) {
        snprintf(old_xdg_buf, sizeof(old_xdg_buf), "%s", old_xdg);
    }
    if (old_symdep) {
        snprintf(old_symdep_buf, sizeof(old_symdep_buf), "%s", old_symdep);
    }
    unsetenv("SYMDEP_CONFIG_FILE");
    setenv("XDG_CONFIG_HOME", tmp_dir, 1);

    Config cfg;
    config_init(&cfg);
    str_array_append(&cfg.source_dirs, tmp_dir);
    config_save(&cfg);
    config_free(&cfg);

    Config loaded;
    config_init(&loaded);
    ASSERT(config_load(&loaded), "Should load config file");
    ASSERT(str_array_contains(&loaded.source_dirs, tmp_dir),
           "Config should contain saved source_dir");
    config_free(&loaded);

    if (old_xdg_buf[0] != '\0') {
        setenv("XDG_CONFIG_HOME", old_xdg_buf, 1);
    } else {
        unsetenv("XDG_CONFIG_HOME");
    }
    if (old_symdep_buf[0] != '\0') {
        setenv("SYMDEP_CONFIG_FILE", old_symdep_buf, 1);
    }

    cleanup_test_dir(tmp_dir);
}

void test_config_save_disclaimer(void)
{
    char tmp_dir[PATH_MAX];
    ASSERT(create_test_tmp_dir(tmp_dir, sizeof(tmp_dir), "cfg_hdr") != NULL,
           "Should create temporary test directory");

    const char *old_xdg = getenv("XDG_CONFIG_HOME");
    const char *old_symdep = getenv("SYMDEP_CONFIG_FILE");
    char old_symdep_buf[PATH_MAX] = {0};
    if (old_symdep) {
        snprintf(old_symdep_buf, sizeof(old_symdep_buf), "%s", old_symdep);
    }
    unsetenv("SYMDEP_CONFIG_FILE");
    setenv("XDG_CONFIG_HOME", tmp_dir, 1);

    Config cfg;
    config_init(&cfg);
    str_array_append(&cfg.source_dirs, tmp_dir);
    config_save(&cfg);

    char cfg_path[PATH_MAX];
    snprintf(cfg_path, sizeof(cfg_path), "%s/symdep/config", tmp_dir);
    if (!file_exists(cfg_path)) {
        snprintf(cfg_path, sizeof(cfg_path), "%s/stow-manager/config", tmp_dir);
    }

    FILE *fp = fopen(cfg_path, "r");
    ASSERT(fp != NULL, "Saved config file should exist");

    char buffer[1024] = {0};
    fread(buffer, 1, sizeof(buffer) - 1, fp);
    fclose(fp);

    ASSERT(strstr(buffer, "Configuration (Auto-Generated)") != NULL,
           "Config file should contain auto-generated disclaimer header");

    config_free(&cfg);

    if (old_xdg)
        setenv("XDG_CONFIG_HOME", old_xdg, 1);
    else
        unsetenv("XDG_CONFIG_HOME");

    if (old_symdep_buf[0] != '\0') {
        setenv("SYMDEP_CONFIG_FILE", old_symdep_buf, 1);
    }

    cleanup_test_dir(tmp_dir);
}
