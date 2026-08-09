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

void test_cmd_ignore_ops(void)
{
    char tmp_dotfiles[PATH_MAX];
    char tmp_target[PATH_MAX];
    ASSERT(create_test_tmp_dir(tmp_dotfiles, sizeof(tmp_dotfiles), "ign_ops_dot") != NULL,
           "Should create temporary dotfiles directory");
    ASSERT(create_test_tmp_dir(tmp_target, sizeof(tmp_target), "ign_ops_tgt") != NULL,
           "Should create temporary target directory");

    char pkg_dir[PATH_MAX];
    snprintf(pkg_dir, sizeof(pkg_dir), "%s/ignpkg", tmp_dotfiles);
    ASSERT(mkdir(pkg_dir, 0755) == 0, "Should create package directory");

    CliOptions opts = {.auto_install = false,
                       .dry_run = true,
                       .save_flag = false,
                       .cli_source_dir = tmp_dotfiles,
                       .cli_target_dir = tmp_target};

    StringArray args;
    str_array_init(&args);
    str_array_append(&args, "ignore");
    str_array_append(&args, "add");
    str_array_append(&args, "ignpkg");
    str_array_append(&args, "*.tmp");

    CommandContext ctx = {.opts = &opts,
                          .source_dir = tmp_dotfiles,
                          .global_target_dir = tmp_target,
                          .args = &args,
                          .arg_offset = 2};

    int res_init = cmd_ignore_init(&ctx);
    ASSERT(res_init == 0, "cmd_ignore_init should return 0 success");

    int res_add = cmd_ignore_add(&ctx);
    ASSERT(res_add == 0, "cmd_ignore_add should return 0 success");

    int res_show = cmd_ignore_show(&ctx);
    ASSERT(res_show == 0, "cmd_ignore_show should return 0 success");

    int res_remove = cmd_ignore_remove(&ctx);
    ASSERT(res_remove == 0, "cmd_ignore_remove should return 0 success");

    int res_clear = cmd_ignore_clear(&ctx);
    ASSERT(res_clear == 0, "cmd_ignore_clear should return 0 success");

    str_array_free(&args);
    cleanup_test_dir(tmp_dotfiles);
    cleanup_test_dir(tmp_target);
}
