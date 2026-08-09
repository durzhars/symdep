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

void test_cmd_pkg_ops(void)
{
    char tmp_dotfiles[PATH_MAX];
    char tmp_target[PATH_MAX];
    ASSERT(create_test_tmp_dir(tmp_dotfiles, sizeof(tmp_dotfiles), "pkg_ops_dot") != NULL,
           "Should create temporary dotfiles directory");
    ASSERT(create_test_tmp_dir(tmp_target, sizeof(tmp_target), "pkg_ops_tgt") != NULL,
           "Should create temporary target directory");

    CliOptions opts = {.auto_install = false,
                       .dry_run = true,
                       .save_flag = false,
                       .cli_source_dir = tmp_dotfiles,
                       .cli_target_dir = tmp_target};

    StringArray args;
    str_array_init(&args);
    str_array_append(&args, "pkg");
    str_array_append(&args, "create");
    str_array_append(&args, "testpkg");

    CommandContext ctx = {.opts = &opts,
                          .source_dir = tmp_dotfiles,
                          .global_target_dir = tmp_target,
                          .args = &args,
                          .arg_offset = 2};

    int res_create = cmd_pkg_create(&ctx);
    ASSERT(res_create == 0, "cmd_pkg_create should return 0 success");

    int res_list = cmd_pkg_list(&ctx);
    ASSERT(res_list == 0, "cmd_pkg_list should return 0 success");

    int res_scan = cmd_scan(&ctx);
    ASSERT(res_scan == 0, "cmd_scan should return 0 success");

    int res_remove = cmd_pkg_remove(&ctx);
    ASSERT(res_remove == 0, "cmd_pkg_remove should return 0 success");

    str_array_free(&args);
    cleanup_test_dir(tmp_dotfiles);
    cleanup_test_dir(tmp_target);
}
