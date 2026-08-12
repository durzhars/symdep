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

void test_config_add_remove_dotfiles_dir(void)
{
    char tmp_dir[PATH_MAX];
    ASSERT(create_test_tmp_dir(tmp_dir, sizeof(tmp_dir), "cfg_dirs") != NULL,
           "Should create temporary directory for config dirs test");

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

    char dir_a[PATH_MAX];
    char dir_b[PATH_MAX];
    snprintf(dir_a, sizeof(dir_a), "%s/source_a", tmp_dir);
    snprintf(dir_b, sizeof(dir_b), "%s/source_b", tmp_dir);
    ASSERT(mkdir(dir_a, 0755) == 0, "Should create source_a");
    ASSERT(mkdir(dir_b, 0755) == 0, "Should create source_b");

    config_add_source_dir(dir_a);

    Config cfg;
    config_init(&cfg);
    ASSERT(config_load(&cfg), "Config should load");
    ASSERT(str_array_contains(&cfg.source_dirs, dir_a), "Config should contain dir_a");
    config_free(&cfg);

    config_add_source_dir(dir_b);
    config_init(&cfg);
    config_load(&cfg);
    ASSERT(str_array_contains(&cfg.source_dirs, dir_b), "Config should contain dir_b");

    // Duplicate addition handling
    config_add_source_dir(dir_a);
    config_free(&cfg);

    config_init(&cfg);
    config_load(&cfg);
    size_t count_a = 0;
    for (size_t i = 0; i < cfg.source_dirs.count; i++) {
        if (strcmp(cfg.source_dirs.items[i], dir_a) == 0) {
            count_a++;
        }
    }
    ASSERT(count_a == 1, "dir_a should not be duplicated in config");
    config_free(&cfg);

    // Remove path
    config_remove_source_dir(dir_a);
    config_init(&cfg);
    config_load(&cfg);
    ASSERT(!str_array_contains(&cfg.source_dirs, dir_a), "Config should no longer contain dir_a");
    ASSERT(str_array_contains(&cfg.source_dirs, dir_b), "Config should still contain dir_b");
    config_free(&cfg);

    // Remove non-registered path
    config_remove_source_dir("/nonexistent/path/for/test");
    config_init(&cfg);
    config_load(&cfg);
    ASSERT(cfg.source_dirs.count == 1,
           "Config count should remain 1 after removing non-registered path");
    config_free(&cfg);

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

void test_config_sanity_checks(void)
{
    char tmp_dir[PATH_MAX];
    ASSERT(create_test_tmp_dir(tmp_dir, sizeof(tmp_dir), "cfg_san") != NULL,
           "Should create temporary test directory");

    const char *old_xdg = getenv("XDG_CONFIG_HOME");
    const char *old_symdep = getenv("SYMDEP_CONFIG_FILE");
    char old_symdep_buf[PATH_MAX] = {0};
    if (old_symdep) {
        snprintf(old_symdep_buf, sizeof(old_symdep_buf), "%s", old_symdep);
    }
    unsetenv("SYMDEP_CONFIG_FILE");
    setenv("XDG_CONFIG_HOME", tmp_dir, 1);

    config_set_target_dir("/nonexistent/invalid/target/path/xyz");

    Config cfg;
    config_init(&cfg);
    config_load(&cfg);
    ASSERT(cfg.target_dir[0] == '\0', "Target dir should not be set when sanity check fails");
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
