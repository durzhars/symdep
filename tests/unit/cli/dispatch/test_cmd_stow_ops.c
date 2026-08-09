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

void test_cmd_stow_ops(void)
{
    char tmp_dotfiles[PATH_MAX];
    char tmp_target[PATH_MAX];
    ASSERT(create_test_tmp_dir(tmp_dotfiles, sizeof(tmp_dotfiles), "stow_ops_dot") != NULL,
           "Should create temporary dotfiles directory");
    ASSERT(create_test_tmp_dir(tmp_target, sizeof(tmp_target), "stow_ops_tgt") != NULL,
           "Should create temporary target directory");

    char pkg_dir[PATH_MAX];
    snprintf(pkg_dir, sizeof(pkg_dir), "%s/mypkg", tmp_dotfiles);
    ASSERT(mkdir(pkg_dir, 0755) == 0, "Should create package mypkg");

    char file1[PATH_MAX];
    snprintf(file1, sizeof(file1), "%s/.configfile", pkg_dir);
    FILE *fp = fopen(file1, "w");
    if (fp) {
        fprintf(fp, "content\n");
        fclose(fp);
    }

    CliOptions opts = {.auto_install = false,
                       .dry_run = true,
                       .save_flag = false,
                       .cli_source_dir = tmp_dotfiles,
                       .cli_target_dir = tmp_target};

    StringArray args;
    str_array_init(&args);
    str_array_append(&args, "stow");
    str_array_append(&args, "mypkg");

    CommandContext ctx = {.opts = &opts,
                          .source_dir = tmp_dotfiles,
                          .global_target_dir = tmp_target,
                          .args = &args,
                          .arg_offset = 1};

    int res_stow = cmd_stow(&ctx);
    ASSERT(res_stow == 0, "cmd_stow should return 0 success");

    int res_diff = cmd_diff(&ctx);
    ASSERT(res_diff == 0, "cmd_diff should return 0 success");

    int res_all = cmd_all(&ctx);
    ASSERT(res_all == 0, "cmd_all should return 0 success");

    int res_restow = cmd_restow(&ctx);
    ASSERT(res_restow == 0, "cmd_restow should return 0 success");

    int res_unstow = cmd_unstow(&ctx);
    ASSERT(res_unstow == 0, "cmd_unstow should return 0 success");

    str_array_free(&args);
    cleanup_test_dir(tmp_dotfiles);
    cleanup_test_dir(tmp_target);
}
