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

#include "../test_framework.h"
#include "utils/defs.h"
#include "utils/env.h"
#include "utils/path.h"
#include "utils/str.h"

void test_xdg_paths(void)
{
    const char *orig_home = getenv("HOME");
    const char *orig_xdg_config = getenv("XDG_CONFIG_HOME");
    const char *orig_xdg_data = getenv("XDG_DATA_HOME");

    char cfg_home[PATH_MAX];
    char data_home[PATH_MAX];

    // 1. Top precedence: Explicit XDG environment variables
    setenv("XDG_CONFIG_HOME", "/custom/xdg_config", 1);
    setenv("XDG_DATA_HOME", "/custom/xdg_data", 1);
    unsetenv("HOME");

    ASSERT(get_xdg_config_home(cfg_home, sizeof(cfg_home)),
           "get_xdg_config_home should resolve explicit XDG_CONFIG_HOME");
    ASSERT_STR_EQ(cfg_home, "/custom/xdg_config");

    ASSERT(get_xdg_data_home(data_home, sizeof(data_home)),
           "get_xdg_data_home should resolve explicit XDG_DATA_HOME");
    ASSERT_STR_EQ(data_home, "/custom/xdg_data");

    // 2. Second precedence: Fallback to validated $HOME when XDG variables are unset
    unsetenv("XDG_CONFIG_HOME");
    unsetenv("XDG_DATA_HOME");

    char mock_home[PATH_MAX];
    ASSERT(create_test_tmp_dir(mock_home, sizeof(mock_home), "test_xdghome") != NULL,
           "create_test_tmp_dir should create temporary mock HOME directory");
    setenv("HOME", mock_home, 1);

    char expected_cfg[PATH_MAX];
    char expected_data[PATH_MAX];
    snprintf(expected_cfg, sizeof(expected_cfg), "%s/.config", mock_home);
    snprintf(expected_data, sizeof(expected_data), "%s/.local/share", mock_home);

    ASSERT(get_xdg_config_home(cfg_home, sizeof(cfg_home)),
           "get_xdg_config_home should fallback to $HOME/.config when XDG_CONFIG_HOME is unset");
    ASSERT_STR_EQ(cfg_home, expected_cfg);

    ASSERT(get_xdg_data_home(data_home, sizeof(data_home)),
           "get_xdg_data_home should fallback to $HOME/.local/share when XDG_DATA_HOME is unset");
    ASSERT_STR_EQ(data_home, expected_data);

    cleanup_test_dir(mock_home);

    // 3. Slow-path: Unset $HOME calls getpwuid_r(getuid())
    unsetenv("HOME");
    ASSERT(get_xdg_config_home(cfg_home, sizeof(cfg_home)),
           "get_xdg_config_home should succeed via getpwuid_r slow path when HOME is unset");

    // 4. Default Data Dirs
    StringArray data_dirs;
    str_array_init(&data_dirs);
    get_xdg_data_dirs(&data_dirs);
    ASSERT(data_dirs.count > 0, "XDG_DATA_DIRS should yield at least 1 default directory");
    str_array_free(&data_dirs);

    // Restore original environment
    if (orig_home) {
        setenv("HOME", orig_home, 1);
    } else {
        unsetenv("HOME");
    }
    if (orig_xdg_config) {
        setenv("XDG_CONFIG_HOME", orig_xdg_config, 1);
    } else {
        unsetenv("XDG_CONFIG_HOME");
    }
    if (orig_xdg_data) {
        setenv("XDG_DATA_HOME", orig_xdg_data, 1);
    } else {
        unsetenv("XDG_DATA_HOME");
    }
}

void test_expand_env_vars(void)
{
    char out[PATH_MAX];
    const char *orig_home = getenv("HOME");

    char mock_home[PATH_MAX];
    ASSERT(create_test_tmp_dir(mock_home, sizeof(mock_home), "test_env") != NULL,
           "create_test_tmp_dir should create temporary mock HOME directory");

    setenv("TEST_STOW_VAR1", "/custom/path", 1);
    setenv("TEST_STOW_VAR2", "my_app", 1);
    setenv("HOME", mock_home, 1);

    // 1. POSIX $VAR syntax
    expand_env_vars("$TEST_STOW_VAR1/sub", out, sizeof(out));
    ASSERT_STR_EQ(out, "/custom/path/sub");

    // 2. POSIX ${VAR} syntax
    expand_env_vars("${TEST_STOW_VAR2}/config", out, sizeof(out));
    ASSERT_STR_EQ(out, "my_app/config");

    // 3. Combination of tilde + env var in expand_tilde_path
    char expected_combo[PATH_MAX];
    snprintf(expected_combo, sizeof(expected_combo), "%s/my_app", mock_home);

    expand_tilde_path("~/$TEST_STOW_VAR2", out, sizeof(out));
    ASSERT_STR_EQ(out, expected_combo);

    // 5. Fail-closed buffer overflow check
    char small_env_buf[5] = "XXXX";
    expand_env_vars("$TEST_STOW_VAR1/sub", small_env_buf, sizeof(small_env_buf));
    ASSERT(small_env_buf[0] == '\0',
           "expand_env_vars must fail closed to empty string on buffer overflow");

    cleanup_test_dir(mock_home);
    unsetenv("TEST_STOW_VAR1");
    unsetenv("TEST_STOW_VAR2");

    if (orig_home) {
        setenv("HOME", orig_home, 1);
    } else {
        unsetenv("HOME");
    }
}

void test_degraded_env_path_resolution(void)
{
    // Save original environment
    const char *orig_home = getenv("HOME");
    const char *orig_xdg_cfg = getenv("XDG_CONFIG_HOME");
    const char *orig_xdg_data = getenv("XDG_DATA_HOME");
    const char *orig_xdg_cache = getenv("XDG_CACHE_HOME");
    const char *orig_xdg_state = getenv("XDG_STATE_HOME");
    const char *orig_xdg_data_dirs = getenv("XDG_DATA_DIRS");
    const char *orig_xdg_config_dirs = getenv("XDG_CONFIG_DIRS");

    char home_backup[PATH_MAX] = {0};
    char xdg_cfg_backup[PATH_MAX] = {0};
    char xdg_data_backup[PATH_MAX] = {0};
    char xdg_cache_backup[PATH_MAX] = {0};
    char xdg_state_backup[PATH_MAX] = {0};
    char xdg_data_dirs_backup[PATH_MAX] = {0};
    char xdg_config_dirs_backup[PATH_MAX] = {0};

    if (orig_home) {
        snprintf(home_backup, sizeof(home_backup), "%s", orig_home);
    }
    if (orig_xdg_cfg) {
        snprintf(xdg_cfg_backup, sizeof(xdg_cfg_backup), "%s", orig_xdg_cfg);
    }
    if (orig_xdg_data) {
        snprintf(xdg_data_backup, sizeof(xdg_data_backup), "%s", orig_xdg_data);
    }
    if (orig_xdg_cache) {
        snprintf(xdg_cache_backup, sizeof(xdg_cache_backup), "%s", orig_xdg_cache);
    }
    if (orig_xdg_state) {
        snprintf(xdg_state_backup, sizeof(xdg_state_backup), "%s", orig_xdg_state);
    }
    if (orig_xdg_data_dirs) {
        snprintf(xdg_data_dirs_backup, sizeof(xdg_data_dirs_backup), "%s", orig_xdg_data_dirs);
    }
    if (orig_xdg_config_dirs) {
        snprintf(
            xdg_config_dirs_backup, sizeof(xdg_config_dirs_backup), "%s", orig_xdg_config_dirs);
    }

    // --- Scenario 1: Invalid / Missing $HOME and Unset XDG Variables ---
    setenv("HOME", "/dev/null", 1);
    unsetenv("XDG_CONFIG_HOME");
    unsetenv("XDG_DATA_HOME");
    unsetenv("XDG_CACHE_HOME");
    unsetenv("XDG_STATE_HOME");
    unsetenv("XDG_DATA_DIRS");
    unsetenv("XDG_CONFIG_DIRS");

    char buf[PATH_MAX];

    // Invalid getenv("HOME") triggers getpwuid_r slow path fallback
    ASSERT(get_user_home_dir(buf, sizeof(buf)),
           "get_user_home_dir should fallback to getpwuid_r slow path when HOME is invalid");
    ASSERT(get_xdg_config_home(buf, sizeof(buf)),
           "get_xdg_config_home should succeed via getpwuid_r slow path when HOME is invalid");

    expand_tilde_path("~/dotfiles", buf, sizeof(buf));
    ASSERT(buf[0] == '/',
           "expand_tilde_path should resolve ~ using getpwuid_r slow path when HOME environment "
           "variable is invalid");

    StringArray dirs;
    str_array_init(&dirs);
    get_xdg_config_dirs(&dirs);
    ASSERT(dirs.count == 1, "Should have 1 default config dir");
    ASSERT_STR_EQ(dirs.items[0], "/etc/xdg");
    str_array_free(&dirs);

    str_array_init(&dirs);
    get_xdg_data_dirs(&dirs);
    ASSERT(dirs.count == 2, "Should have 2 default data dirs");
    ASSERT_STR_EQ(dirs.items[0], "/usr/local/share");
    ASSERT_STR_EQ(dirs.items[1], "/usr/share");
    str_array_free(&dirs);

    // --- Scenario 2: Empty $HOME Variable ---
    setenv("HOME", "", 1);

    ASSERT(get_user_home_dir(buf, sizeof(buf)),
           "get_user_home_dir should succeed via getpwuid_r slow path when HOME is empty");
    ASSERT(get_xdg_config_home(buf, sizeof(buf)),
           "get_xdg_config_home should succeed via getpwuid_r slow path when HOME is empty");

    // --- Scenario 3: Malformed XDG_DATA_DIRS with empty segments & trailing colons ---
    setenv("XDG_DATA_DIRS", ":::/custom/share1::/custom/share2:", 1);

    str_array_init(&dirs);
    get_xdg_data_dirs(&dirs);
    ASSERT(dirs.count == 2, "Should parse only non-empty paths from malformed XDG_DATA_DIRS");
    ASSERT_STR_EQ(dirs.items[0], "/custom/share1");
    ASSERT_STR_EQ(dirs.items[1], "/custom/share2");
    str_array_free(&dirs);

    // --- Scenario 4: Nested environment variable expansion in XDG variables ---
    setenv("CUSTOM_BASE", "/opt/stow", 1);
    setenv("XDG_CONFIG_HOME", "$CUSTOM_BASE/config", 1);

    get_xdg_config_home(buf, sizeof(buf));
    ASSERT_STR_EQ(buf, "/opt/stow/config");

    unsetenv("CUSTOM_BASE");

    // --- Scenario 5: Path Sanity Kernel Checks ---
    ASSERT(verify_path_sanity("") == ERR_PATH_EMPTY, "Empty path should return ERR_PATH_EMPTY");
    ASSERT(verify_path_sanity("relative/path") == ERR_NOT_ABSOLUTE,
           "Relative path should return ERR_NOT_ABSOLUTE");
    ASSERT(verify_path_sanity("/dev/null") == ERR_NOT_A_DIRECTORY,
           "/dev/null file should return ERR_NOT_A_DIRECTORY");

    PathSanityResult tmp_res = verify_path_sanity("/tmp");
    ASSERT(tmp_res == ERR_WORLD_WRITABLE || tmp_res == ERR_NOT_OWNED_BY_USER ||
               tmp_res == ERR_INSUFFICIENT_PERMS || tmp_res == PATH_VALID,
           "/tmp should fail sanity or return valid on systems");

    // --- Scenario 6: AppEnvironment Resolution Pipeline ---
    AppEnvironment app_env;
    setenv("HOME", "/dev/null", 1);
    ASSERT(app_env_resolve(&app_env, NULL, NULL),
           "app_env_resolve should recover valid user home via getpwuid_r when getenv(HOME) is "
           "invalid");
    ASSERT(app_env.is_home_validated, "is_home_validated should be true via getpwuid_r recovery");

    ASSERT(
        app_env_resolve(&app_env, "/custom/target", NULL),
        "app_env_resolve should succeed when CLI target override is provided despite invalid HOME");
    ASSERT(app_env.is_target_override, "is_target_override should be true");
    ASSERT_STR_EQ(app_env.target_dir, "/custom/target");

    // Restore original environment
    if (orig_home) {
        setenv("HOME", home_backup, 1);
    } else {
        unsetenv("HOME");
    }
    if (orig_xdg_cfg) {
        setenv("XDG_CONFIG_HOME", xdg_cfg_backup, 1);
    } else {
        unsetenv("XDG_CONFIG_HOME");
    }
    if (orig_xdg_data) {
        setenv("XDG_DATA_HOME", xdg_data_backup, 1);
    } else {
        unsetenv("XDG_DATA_HOME");
    }
    if (orig_xdg_cache) {
        setenv("XDG_CACHE_HOME", xdg_cache_backup, 1);
    } else {
        unsetenv("XDG_CACHE_HOME");
    }
    if (orig_xdg_state) {
        setenv("XDG_STATE_HOME", xdg_state_backup, 1);
    } else {
        unsetenv("XDG_STATE_HOME");
    }
    if (orig_xdg_data_dirs) {
        setenv("XDG_DATA_DIRS", xdg_data_dirs_backup, 1);
    } else {
        unsetenv("XDG_DATA_DIRS");
    }
    if (orig_xdg_config_dirs) {
        setenv("XDG_CONFIG_DIRS", xdg_config_dirs_backup, 1);
    } else {
        unsetenv("XDG_CONFIG_DIRS");
    }
}
