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
#include "cli/cmd_routes.h"

void test_cmd_config_ops(void)
{
    char tmp_dotfiles[PATH_MAX];
    char tmp_target[PATH_MAX];
    ASSERT(create_test_tmp_dir(tmp_dotfiles, sizeof(tmp_dotfiles), "cfg_ops_dot") != NULL,
           "Should create temporary dotfiles directory");
    ASSERT(create_test_tmp_dir(tmp_target, sizeof(tmp_target), "cfg_ops_tgt") != NULL,
           "Should create temporary target directory");

    CliOptions opts = {.auto_install = false,
                       .dry_run = true,
                       .save_flag = false,
                       .cli_source_dir = tmp_dotfiles,
                       .cli_target_dir = tmp_target};

    StringArray args;
    str_array_init(&args);
    str_array_append(&args, "config");
    str_array_append(&args, "show");

    CommandContext ctx = {.opts = &opts,
                          .source_dir = tmp_dotfiles,
                          .global_target_dir = tmp_target,
                          .args = &args,
                          .arg_offset = 2};

    int res_show = cmd_config_show(&ctx);
    ASSERT(res_show == 0, "cmd_config_show should return 0 success");

    StringArray set_args;
    str_array_init(&set_args);
    str_array_append(&set_args, "config");
    str_array_append(&set_args, "set");
    str_array_append(&set_args, "--target");
    str_array_append(&set_args, tmp_target);
    CommandContext set_ctx = {.opts = &opts,
                              .source_dir = tmp_dotfiles,
                              .global_target_dir = tmp_target,
                              .args = &set_args,
                              .arg_offset = 2};
    int res_set = cmd_config_set(&set_ctx);
    ASSERT(res_set == 0, "cmd_config_set --target should return 0 success");
    str_array_free(&set_args);

    StringArray mgr_args;
    str_array_init(&mgr_args);
    str_array_append(&mgr_args, "config");
    str_array_append(&mgr_args, "set");
    str_array_append(&mgr_args, "--manager");
    str_array_append(&mgr_args, "yay");
    CommandContext mgr_ctx = {.opts = &opts,
                              .source_dir = tmp_dotfiles,
                              .global_target_dir = tmp_target,
                              .args = &mgr_args,
                              .arg_offset = 2};
    int res_mgr = cmd_config_set(&mgr_ctx);
    ASSERT(res_mgr == 0, "cmd_config_set --manager should return 0 success");
    str_array_free(&mgr_args);

    // Invalid/Unknown flag test (--m) should fail with 1
    StringArray invalid_args;
    str_array_init(&invalid_args);
    str_array_append(&invalid_args, "config");
    str_array_append(&invalid_args, "set");
    str_array_append(&invalid_args, "--m");
    str_array_append(&invalid_args, "pacman");
    CommandContext invalid_ctx = {.opts = &opts,
                                  .source_dir = tmp_dotfiles,
                                  .global_target_dir = tmp_target,
                                  .args = &invalid_args,
                                  .arg_offset = 2};
    int res_invalid = cmd_config_set(&invalid_ctx);
    ASSERT(res_invalid == 1, "cmd_config_set with unknown flag --m should return 1 error");
    str_array_free(&invalid_args);

    StringArray add_args;
    str_array_init(&add_args);
    str_array_append(&add_args, "config");
    str_array_append(&add_args, "add");
    str_array_append(&add_args, tmp_dotfiles);
    CommandContext add_ctx = {.opts = &opts,
                              .source_dir = tmp_dotfiles,
                              .global_target_dir = tmp_target,
                              .args = &add_args,
                              .arg_offset = 2};
    int res_add = cmd_config_add(&add_ctx);
    ASSERT(res_add == 0, "cmd_config_add should return 0 success");
    str_array_free(&add_args);

    StringArray rm_args;
    str_array_init(&rm_args);
    str_array_append(&rm_args, "config");
    str_array_append(&rm_args, "remove");
    str_array_append(&rm_args, tmp_dotfiles);
    CommandContext rm_ctx = {.opts = &opts,
                             .source_dir = tmp_dotfiles,
                             .global_target_dir = tmp_target,
                             .args = &rm_args,
                             .arg_offset = 2};
    int res_rm = cmd_config_remove(&rm_ctx);
    ASSERT(res_rm == 0, "cmd_config_remove should return 0 success");
    str_array_free(&rm_args);

    str_array_free(&args);
    cleanup_test_dir(tmp_dotfiles);
    cleanup_test_dir(tmp_target);
}
