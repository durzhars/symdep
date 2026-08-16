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
#include "utils/fs.h"
#include "utils/path.h"
#include "utils/signal.h"
#include <stdio.h>
#include <unistd.h>

void test_is_executable_in_path(void)
{
    ASSERT(is_executable_in_path("sh"), "sh should be in PATH");
    ASSERT(!is_executable_in_path("nonexistent_binary_xyz_12345"),
           "nonexistent binary should not be in PATH");
    ASSERT(!is_executable_in_path(""), "Empty string should not be in PATH");
}

void test_temp_path_registration(void)
{
    // 1. NULL / empty string guards
    register_temp_path(NULL);
    register_temp_path("");
    unregister_temp_path(NULL);
    unregister_temp_path("");

    char tmp_sandbox[PATH_MAX];
    ASSERT(create_test_tmp_dir(tmp_sandbox, sizeof(tmp_sandbox), "sig_reg") != NULL,
           "Should create temp sandbox");

    char f1[PATH_MAX], f2[PATH_MAX], d1[PATH_MAX], sym1[PATH_MAX];
    snprintf(f1, sizeof(f1), "%s/file1.tmp", tmp_sandbox);
    snprintf(f2, sizeof(f2), "%s/file2.tmp", tmp_sandbox);
    snprintf(d1, sizeof(d1), "%s/dir1.tmp", tmp_sandbox);
    snprintf(sym1, sizeof(sym1), "%s/sym1.tmp", tmp_sandbox);

    FILE *fp1 = fopen(f1, "w");
    if (fp1) {
        fprintf(fp1, "temp 1\n");
        fclose(fp1);
    }
    FILE *fp2 = fopen(f2, "w");
    if (fp2) {
        fprintf(fp2, "temp 2\n");
        fclose(fp2);
    }
    ASSERT(mkdir(d1, 0755) == 0, "Should create d1");
    char nested[PATH_MAX];
    snprintf(nested, sizeof(nested), "%s/nested.txt", d1);
    FILE *fpn = fopen(nested, "w");
    if (fpn) {
        fprintf(fpn, "nested\n");
        fclose(fpn);
    }
    ASSERT(symlink(f1, sym1) == 0, "Should create symlink sym1");

    // Register all
    register_temp_path(f1);
    register_temp_path(f1); // duplicate registration
    register_temp_path(f2);
    register_temp_path(d1);
    register_temp_path(sym1);

    // Unregister f2 and a nonexistent path
    unregister_temp_path(f2);
    unregister_temp_path("/tmp/nonexistent_xyz_987");

    cleanup_temp_paths();

    // f1, d1, sym1 should be cleaned up
    ASSERT(!file_exists(f1), "f1 should be deleted by cleanup_temp_paths");
    ASSERT(!file_exists(d1), "d1 should be deleted by cleanup_temp_paths");
    ASSERT(!is_symlink(sym1), "sym1 should be deleted by cleanup_temp_paths");

    // f2 was unregistered, so it should still exist
    ASSERT(file_exists(f2), "f2 was unregistered, should still exist");
    unlink(f2);

    cleanup_test_dir(tmp_sandbox);
}

void test_signal_safe_temp_cleanup(void)
{
    char tmp_sandbox[PATH_MAX];
    ASSERT(create_test_tmp_dir(tmp_sandbox, sizeof(tmp_sandbox), "sig_safe") != NULL,
           "Should create temp sandbox for signal safe test");

    char dir_safe[PATH_MAX];
    snprintf(dir_safe, sizeof(dir_safe), "%s/safedir", tmp_sandbox);
    ASSERT(mkdir(dir_safe, 0755) == 0, "Should create safedir");

    char file_inside[PATH_MAX];
    snprintf(file_inside, sizeof(file_inside), "%s/file.txt", dir_safe);
    FILE *fp = fopen(file_inside, "w");
    if (fp) {
        fprintf(fp, "inside\n");
        fclose(fp);
    }

    register_temp_path(dir_safe);

    cleanup_temp_paths_signal_safe();

    ASSERT(!file_exists(file_inside), "file inside safedir should be removed");
    ASSERT(!file_exists(dir_safe), "safedir should be removed");

    cleanup_test_dir(tmp_sandbox);
}

void test_signal_handlers_setup(void)
{
    setup_signal_handlers();
}
