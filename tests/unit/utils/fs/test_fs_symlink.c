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
#include "utils/fs.h"

void test_symlink_helpers(void)
{
    char tmp_dir[PATH_MAX];
    ASSERT(create_test_tmp_dir(tmp_dir, sizeof(tmp_dir), "sym_hlp") != NULL,
           "Should create temporary directory for symlink helpers test");

    char target_file[PATH_MAX];
    snprintf(target_file, sizeof(target_file), "%s/target.txt", tmp_dir);
    FILE *fp = fopen(target_file, "w");
    if (fp) {
        fprintf(fp, "test\n");
        fclose(fp);
    }

    ASSERT(file_exists(target_file), "target_file should exist");
    ASSERT(!is_symlink(target_file), "target_file should not be a symlink");
    ASSERT(!is_dir(target_file), "target_file should not be a directory");

    char link_file[PATH_MAX];
    snprintf(link_file, sizeof(link_file), "%s/link.txt", tmp_dir);
    ASSERT(symlink(target_file, link_file) == 0, "Should create symlink");

    ASSERT(is_symlink(link_file), "link_file should be a symlink");
    char *sym_target = read_symlink_target(link_file);
    ASSERT(sym_target != NULL, "read_symlink_target should return target path");
    ASSERT_STR_EQ(sym_target, target_file);
    free(sym_target);

    ASSERT(read_symlink_target(NULL) == NULL, "read_symlink_target(NULL) must return NULL");
    ASSERT(read_symlink_target("") == NULL, "read_symlink_target(\"\") must return NULL");
    ASSERT(read_symlink_target("/nonexistent_file_xyz_9999") == NULL,
           "read_symlink_target on non-symlink must return NULL");

    cleanup_test_dir(tmp_dir);
}

void test_is_symlink_pointing_to(void)
{
    char tmp_dir[PATH_MAX];
    ASSERT(create_test_tmp_dir(tmp_dir, sizeof(tmp_dir), "sym_pt") != NULL,
           "create_test_tmp_dir should succeed");

    char real_target_file[PATH_MAX];
    snprintf(real_target_file, sizeof(real_target_file), "%s/theme.conf", tmp_dir);
    FILE *fp = fopen(real_target_file, "w");
    if (fp) {
        fprintf(fp, "color=blue\n");
        fclose(fp);
    }

    char pkg_symlink_file[PATH_MAX];
    snprintf(pkg_symlink_file, sizeof(pkg_symlink_file), "%s/current-theme.conf", tmp_dir);
    symlink("theme.conf", pkg_symlink_file);

    char outer_dir[PATH_MAX];
    snprintf(outer_dir, sizeof(outer_dir), "%s/outer", tmp_dir);
    mkdir(outer_dir, 0755);

    char outer_stow_link[PATH_MAX];
    snprintf(outer_stow_link, sizeof(outer_stow_link), "%s/current-theme.conf", outer_dir);
    symlink("../current-theme.conf", outer_stow_link);

    ASSERT(is_symlink_pointing_to(outer_stow_link, pkg_symlink_file, NULL),
           "Relative symlink to internal package symlink file must match");

    cleanup_test_dir(tmp_dir);
}
