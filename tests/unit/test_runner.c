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

#include "test_framework.h"

int g_tests_run = 0;
int g_tests_failed = 0;

/* Prototypes for cli tests */
void test_parse_cli_options_flags(void);
void test_parse_cli_options_directory_overrides(void);
void test_parse_cli_options_errors_and_help(void);
void test_cmd_table_lookup(void);
void test_help_formatting(void);

/* Prototypes for cli/dispatch tests */
void test_dispatch_command_routes(void);
void test_cmd_stow_ops(void);
void test_cmd_pkg_ops(void);
void test_cmd_deps_ops(void);
void test_cmd_ignore_ops(void);
void test_cmd_config_ops(void);

/* Prototypes for core tests */
void test_check_package_dependencies(void);
void test_symlink_health_check(void);
void test_pkg_manager_builtins(void);
void test_pkg_manager_custom_config(void);
void test_pkg_manager_resolution_precedence(void);
void test_pkg_manager_writable_prefix_elevation(void);
void test_default_stowignore(void);
void test_pkg_file_list_ops(void);
void test_collect_package_files(void);
void test_manifest_load_save(void);
void test_manifest_add_and_remove_dep(void);
void test_manifest_malformed_file(void);
void test_manifest_edit_dep(void);
void test_package_remove(void);
void test_manifest_set_target(void);
void test_registry_parsing(void);
void test_get_all_packages_skips_dot_dirs(void);
void test_scan_package(void);
void test_scan_package_excludes_non_tool_packages(void);
void test_scan_package_comment_stripping_and_auto_registry(void);
void test_scan_package_interactive_mode(void);
void test_scanner_parser_token_extraction(void);

/* Prototypes for core/config tests */
void test_get_active_dotfiles_dir_cascade(void);
void test_config_system(void);
void test_config_save_disclaimer(void);
void test_config_add_remove_dotfiles_dir(void);
void test_config_sanity_checks(void);

/* Prototypes for core/ignore tests */
void test_ignore_init_and_clear(void);
void test_stowignore(void);
void test_ignore_add_and_remove_patterns(void);
void test_ignore_show(void);

/* Prototypes for core/linker tests */
void test_handle_mutual_exclusions(void);
void test_native_stow_broken_symlink_conflict(void);
void test_dynamic_package_conflicts(void);
void test_dry_run_stow(void);
void test_package_context_init(void);
void test_native_unstow_edited_user_file(void);
void test_native_stow_ignore_patterns(void);
void test_restow_package(void);
void test_package_stow_status(void);
void test_unfold_directory_symlinks(void);
void test_native_unstow_recursive_directory_cleanup(void);

/* Prototypes for utils tests */
void test_xdg_paths(void);
void test_expand_env_vars(void);
void test_degraded_env_path_resolution(void);
void test_logger_output(void);
void test_safe_allocators(void);
void test_normalize_path(void);
void test_collapse_path(void);
void test_escape_shell_arg(void);
void test_expand_tilde_path(void);
void test_is_path_prefix(void);
void test_join_path(void);
void test_path_sanity_strerror(void);
void test_path_parsing_traversal_and_naming_edge_cases(void);
void test_is_executable_in_path(void);
void test_temp_path_registration(void);
void test_trim_whitespace(void);
void test_string_array(void);
void test_str_set(void);
void test_perf_timer(void);

/* Prototypes for utils/fs tests */
void test_mkdir_p(void);
void test_mkdir_p_file_collision(void);
void test_fs_resource_management(void);
void test_symlink_helpers(void);
void test_is_symlink_pointing_to(void);
void test_walk_dir_files_and_cleanup(void);

int main(void)
{
    printf("\n=== Running Symlink & Dependency Manager (symdep) C Unit Tests ===\n\n");

    char test_cfg_dir[PATH_MAX];
    bool created_sandbox =
        (create_test_tmp_dir(test_cfg_dir, sizeof(test_cfg_dir), "symdep_test_config") != NULL);
    if (created_sandbox) {
        char test_cfg_file[PATH_MAX];
        snprintf(test_cfg_file, sizeof(test_cfg_file), "%s/config", test_cfg_dir);
        char test_home_dir[PATH_MAX];
        snprintf(test_home_dir, sizeof(test_home_dir), "%s/home", test_cfg_dir);
        (void)mkdir(test_home_dir, 0755);
        setenv("SYMDEP_CONFIG_FILE", test_cfg_file, 1);
        setenv("XDG_CONFIG_HOME", test_cfg_dir, 1);
        setenv("HOME", test_home_dir, 1);
    }

    // --- cli ---
    RUN_TEST(test_parse_cli_options_flags);
    RUN_TEST(test_parse_cli_options_directory_overrides);
    RUN_TEST(test_parse_cli_options_errors_and_help);
    RUN_TEST(test_cmd_table_lookup);
    RUN_TEST(test_help_formatting);

    // --- cli/dispatch ---
    RUN_TEST(test_dispatch_command_routes);
    RUN_TEST(test_cmd_stow_ops);
    RUN_TEST(test_cmd_pkg_ops);
    RUN_TEST(test_cmd_deps_ops);
    RUN_TEST(test_cmd_ignore_ops);
    RUN_TEST(test_cmd_config_ops);

    // --- core ---
    RUN_TEST(test_check_package_dependencies);
    RUN_TEST(test_symlink_health_check);
    RUN_TEST(test_pkg_manager_builtins);
    RUN_TEST(test_pkg_manager_custom_config);
    RUN_TEST(test_pkg_manager_resolution_precedence);
    RUN_TEST(test_pkg_manager_writable_prefix_elevation);
    RUN_TEST(test_default_stowignore);
    RUN_TEST(test_pkg_file_list_ops);
    RUN_TEST(test_collect_package_files);
    RUN_TEST(test_manifest_load_save);
    RUN_TEST(test_manifest_add_and_remove_dep);
    RUN_TEST(test_manifest_malformed_file);
    RUN_TEST(test_manifest_edit_dep);
    RUN_TEST(test_package_remove);
    RUN_TEST(test_manifest_set_target);
    RUN_TEST(test_registry_parsing);
    RUN_TEST(test_get_all_packages_skips_dot_dirs);
    RUN_TEST(test_scan_package);
    RUN_TEST(test_scan_package_excludes_non_tool_packages);
    RUN_TEST(test_scan_package_comment_stripping_and_auto_registry);
    RUN_TEST(test_scan_package_interactive_mode);
    RUN_TEST(test_scanner_parser_token_extraction);

    // --- core/config ---
    RUN_TEST(test_get_active_dotfiles_dir_cascade);
    RUN_TEST(test_config_system);
    RUN_TEST(test_config_save_disclaimer);
    RUN_TEST(test_config_add_remove_dotfiles_dir);
    RUN_TEST(test_config_sanity_checks);

    // --- core/ignore ---
    RUN_TEST(test_ignore_init_and_clear);
    RUN_TEST(test_stowignore);
    RUN_TEST(test_ignore_add_and_remove_patterns);
    RUN_TEST(test_ignore_show);

    // --- core/linker ---
    RUN_TEST(test_handle_mutual_exclusions);
    RUN_TEST(test_native_stow_broken_symlink_conflict);
    RUN_TEST(test_dynamic_package_conflicts);
    RUN_TEST(test_dry_run_stow);
    RUN_TEST(test_package_context_init);
    RUN_TEST(test_native_unstow_edited_user_file);
    RUN_TEST(test_native_stow_ignore_patterns);
    RUN_TEST(test_restow_package);
    RUN_TEST(test_package_stow_status);
    RUN_TEST(test_unfold_directory_symlinks);
    RUN_TEST(test_native_unstow_recursive_directory_cleanup);

    // --- utils ---
    RUN_TEST(test_xdg_paths);
    RUN_TEST(test_expand_env_vars);
    RUN_TEST(test_degraded_env_path_resolution);
    RUN_TEST(test_logger_output);
    RUN_TEST(test_safe_allocators);
    RUN_TEST(test_normalize_path);
    RUN_TEST(test_collapse_path);
    RUN_TEST(test_escape_shell_arg);
    RUN_TEST(test_expand_tilde_path);
    RUN_TEST(test_is_path_prefix);
    RUN_TEST(test_join_path);
    RUN_TEST(test_path_sanity_strerror);
    RUN_TEST(test_path_parsing_traversal_and_naming_edge_cases);
    RUN_TEST(test_is_executable_in_path);
    RUN_TEST(test_temp_path_registration);
    RUN_TEST(test_trim_whitespace);
    RUN_TEST(test_string_array);
    RUN_TEST(test_str_set);
    RUN_TEST(test_perf_timer);

    // --- utils/fs ---
    RUN_TEST(test_mkdir_p);
    RUN_TEST(test_mkdir_p_file_collision);
    RUN_TEST(test_fs_resource_management);
    RUN_TEST(test_symlink_helpers);
    RUN_TEST(test_is_symlink_pointing_to);
    RUN_TEST(test_walk_dir_files_and_cleanup);

    if (created_sandbox) {
        cleanup_test_dir(test_cfg_dir);
    }

    printf("\n=== Test Results: %d Passed, %d Failed ===\n\n",
           g_tests_run - g_tests_failed,
           g_tests_failed);

    return g_tests_failed == 0 ? 0 : 1;
}
