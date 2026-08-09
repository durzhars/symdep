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

void test_mkdir_p(void)
{
    char tmp_dir[PATH_MAX];
    ASSERT(create_test_tmp_dir(tmp_dir, sizeof(tmp_dir), "mkdir") != NULL,
           "Should create temporary directory for mkdir_p test");

    char deep_path[PATH_MAX];
    snprintf(deep_path, sizeof(deep_path), "%s/a/b/c/d", tmp_dir);
    int res = mkdir_p(deep_path, 0755);
    ASSERT(res == 0, "mkdir_p should return 0 on success");
    ASSERT(is_dir(deep_path), "Deeply nested directory should exist");

    int res_existing = mkdir_p(deep_path, 0755);
    ASSERT(res_existing == 0, "mkdir_p on existing directory should return 0");

    cleanup_test_dir(tmp_dir);
}

void test_mkdir_p_file_collision(void)
{
    char tmp_dir[PATH_MAX];
    ASSERT(create_test_tmp_dir(tmp_dir, sizeof(tmp_dir), "mkdir_col") != NULL,
           "Should create temporary test directory");

    char file_path[PATH_MAX];
    snprintf(file_path, sizeof(file_path), "%s/blocking_file", tmp_dir);
    FILE *fp = fopen(file_path, "w");
    if (fp) {
        fprintf(fp, "I am a file, not a directory\n");
        fclose(fp);
    }

    char invalid_dir[PATH_MAX];
    snprintf(invalid_dir, sizeof(invalid_dir), "%s/blocking_file/sub_dir", tmp_dir);
    int res = mkdir_p(invalid_dir, 0755);
    ASSERT(res != 0, "mkdir_p should fail when intermediate component is a regular file");

    cleanup_test_dir(tmp_dir);
}
