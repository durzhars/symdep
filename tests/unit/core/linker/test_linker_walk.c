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
#include "core/linker.h"
#include "utils/fs.h"

void test_unfold_directory_symlinks(void)
{
    char tmp_dotfiles[PATH_MAX];
    char tmp_target[PATH_MAX];
    ASSERT(create_test_tmp_dir(tmp_dotfiles, sizeof(tmp_dotfiles), "unf_dot") != NULL,
           "Should create temporary dotfiles directory");
    ASSERT(create_test_tmp_dir(tmp_target, sizeof(tmp_target), "unf_tgt") != NULL,
           "Should create temporary target directory");

    char pkg_dir[PATH_MAX];
    char pkg_sub[PATH_MAX];
    char file1[PATH_MAX];
    snprintf(pkg_dir, sizeof(pkg_dir), "%s/mypkg", tmp_dotfiles);
    snprintf(pkg_sub, sizeof(pkg_sub), "%s/sub", pkg_dir);
    snprintf(file1, sizeof(file1), "%s/file1.txt", pkg_sub);

    ASSERT(mkdir(pkg_dir, 0755) == 0, "Should create pkg_dir");
    ASSERT(mkdir(pkg_sub, 0755) == 0, "Should create pkg_sub");
    FILE *f1 = fopen(file1, "w");
    if (f1) {
        fprintf(f1, "hello\n");
        fclose(f1);
    }

    char target_sub[PATH_MAX];
    snprintf(target_sub, sizeof(target_sub), "%s/sub", tmp_target);
    ASSERT(symlink(pkg_sub, target_sub) == 0, "Should create directory symlink in target");

    ASSERT(is_symlink(target_sub), "target/sub should initially be a symlink");
    ASSERT(is_dir(target_sub), "target/sub should be a directory");

    unfold_directory_symlinks(tmp_target, tmp_dotfiles, NULL, false);

    ASSERT(!is_symlink(target_sub), "target/sub should no longer be a symlink");
    ASSERT(is_dir(target_sub), "target/sub should still be a directory");

    char target_child[PATH_MAX];
    snprintf(target_child, sizeof(target_child), "%s/file1.txt", target_sub);
    ASSERT(is_symlink(target_child), "target/sub/file1.txt should be a symlink");

    char *sym_target = read_symlink_target(target_child);
    ASSERT(sym_target != NULL, "Symlink target should be readable");
    ASSERT_STR_EQ(sym_target, file1);
    free(sym_target);

    cleanup_test_dir(tmp_dotfiles);
    cleanup_test_dir(tmp_target);
}

void test_native_unstow_recursive_directory_cleanup(void)
{
    char tmp_dotfiles[PATH_MAX];
    char tmp_target[PATH_MAX];
    ASSERT(create_test_tmp_dir(tmp_dotfiles, sizeof(tmp_dotfiles), "rec_dot") != NULL,
           "Should create temporary dotfiles directory");
    ASSERT(create_test_tmp_dir(tmp_target, sizeof(tmp_target), "rec_tgt") != NULL,
           "Should create temporary target directory");

    char pkg_dir[PATH_MAX * 2];
    snprintf(pkg_dir, sizeof(pkg_dir), "%s/recpkg", tmp_dotfiles);
    char deep_dir[PATH_MAX * 2];
    snprintf(deep_dir, sizeof(deep_dir), "%s/dir1/dir2/dir3", pkg_dir);
    ASSERT(mkdir_p(deep_dir, 0755) == 0, "Should create deep package directory");

    char pkg_file[PATH_MAX * 2];
    snprintf(pkg_file, sizeof(pkg_file), "%s/deep.conf", deep_dir);
    FILE *fp = fopen(pkg_file, "w");
    if (fp) {
        fprintf(fp, "deep config\n");
        fclose(fp);
    }

    ASSERT(stow_package(tmp_dotfiles, tmp_target, "recpkg", false, false) == 0,
           "Stow should succeed");

    char target_dir1[PATH_MAX * 2];
    char target_dir2[PATH_MAX * 2];
    char target_dir3[PATH_MAX * 2];
    char target_file[PATH_MAX * 2];
    snprintf(target_dir1, sizeof(target_dir1), "%s/dir1", tmp_target);
    snprintf(target_dir2, sizeof(target_dir2), "%s/dir1/dir2", tmp_target);
    snprintf(target_dir3, sizeof(target_dir3), "%s/dir1/dir2/dir3", tmp_target);
    snprintf(target_file, sizeof(target_file), "%s/dir1/dir2/dir3/deep.conf", tmp_target);

    ASSERT(is_symlink(target_file), "deep.conf should be stowed as symlink");

    char unrelated_file[PATH_MAX * 2];
    snprintf(unrelated_file, sizeof(unrelated_file), "%s/unrelated.txt", target_dir1);
    FILE *fur = fopen(unrelated_file, "w");
    if (fur) {
        fprintf(fur, "unrelated file\n");
        fclose(fur);
    }

    ASSERT(unstow_package(tmp_dotfiles, tmp_target, "recpkg", false) == 0, "Unstow should succeed");

    ASSERT(!file_exists(target_file), "deep.conf should be removed");
    ASSERT(!is_dir(target_dir3), "dir3 should be cleaned up as empty directory");
    ASSERT(!is_dir(target_dir2), "dir2 should be cleaned up as empty directory");
    ASSERT(is_dir(target_dir1), "dir1 should NOT be cleaned up because it contains unrelated.txt");
    ASSERT(file_exists(unrelated_file), "unrelated.txt in dir1 should exist");

    cleanup_test_dir(tmp_dotfiles);
    cleanup_test_dir(tmp_target);
}
