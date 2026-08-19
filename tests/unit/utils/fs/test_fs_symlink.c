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

void test_fs_syscall_macros(void)
{
    char tmp_dir[PATH_MAX];
    ASSERT(create_test_tmp_dir(tmp_dir, sizeof(tmp_dir), "fs_macros") != NULL,
           "create_test_tmp_dir should succeed");

    /* 1. Test FS_MKDIR and fs_mkdir */
    char sub_dir[PATH_MAX];
    snprintf(sub_dir, sizeof(sub_dir), "%s/subdir", tmp_dir);
    ASSERT(FS_MKDIR(sub_dir, 0755) == 0, "FS_MKDIR should create directory");

    char sub_dir2[PATH_MAX];
    snprintf(sub_dir2, sizeof(sub_dir2), "%s/subdir2", tmp_dir);
    ASSERT(fs_mkdir(sub_dir2, 0755) == 0, "fs_mkdir should create directory");

    /* 2. Test FS_OPEN and fs_open */
    char file1[PATH_MAX];
    snprintf(file1, sizeof(file1), "%s/file1.txt", sub_dir);
    int fd1 = FS_OPEN(file1, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    ASSERT(fd1 >= 0, "FS_OPEN should open/create file");
    write(fd1, "macro_test_data", 15);
    close(fd1);

    char file2[PATH_MAX];
    snprintf(file2, sizeof(file2), "%s/file2.txt", sub_dir);
    int fd2 = fs_open(file2, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    ASSERT(fd2 >= 0, "fs_open should open/create file");
    write(fd2, "macro_test_data_2", 17);
    close(fd2);

    /* 3. Test FS_ACCESS and fs_access */
    ASSERT(FS_ACCESS(file1, R_OK) == 0, "FS_ACCESS should verify read permission");
    ASSERT(fs_access(file2, R_OK | W_OK) == 0, "fs_access should verify read/write permission");

    /* 4. Test FS_STAT and fs_stat */
    struct stat st1, st2;
    ASSERT(FS_STAT(file1, &st1) == 0, "FS_STAT should stat file1");
    ASSERT(S_ISREG(st1.st_mode), "file1 should be regular file");
    ASSERT(fs_stat(sub_dir, &st2) == 0, "fs_stat should stat directory");
    ASSERT(S_ISDIR(st2.st_mode), "sub_dir should be directory");

    /* 5. Test FS_SYMLINK and fs_symlink */
    char sym1[PATH_MAX];
    snprintf(sym1, sizeof(sym1), "%s/sym1.lnk", tmp_dir);
    ASSERT(FS_SYMLINK(file1, sym1) == 0, "FS_SYMLINK should create symlink");

    char sym2[PATH_MAX];
    snprintf(sym2, sizeof(sym2), "%s/sym2.lnk", tmp_dir);
    ASSERT(fs_symlink(file2, sym2) == 0, "fs_symlink should create symlink");

    /* 6. Test FS_LSTAT and fs_lstat */
    struct stat lst1, lst2;
    ASSERT(FS_LSTAT(sym1, &lst1) == 0, "FS_LSTAT should lstat sym1");
    ASSERT(S_ISLNK(lst1.st_mode), "sym1 must be detected as symlink via FS_LSTAT");
    ASSERT(fs_lstat(sym2, &lst2) == 0, "fs_lstat should lstat sym2");
    ASSERT(S_ISLNK(lst2.st_mode), "sym2 must be detected as symlink via fs_lstat");

    /* 7. Test FS_READLINK and fs_readlink */
    char read_buf1[PATH_MAX];
    ssize_t rlen1 = FS_READLINK(sym1, read_buf1, sizeof(read_buf1) - 1);
    ASSERT(rlen1 > 0, "FS_READLINK should read symlink target");
    read_buf1[rlen1] = '\0';
    ASSERT_STR_EQ(read_buf1, file1);

    char read_buf2[PATH_MAX];
    ssize_t rlen2 = fs_readlink(sym2, read_buf2, sizeof(read_buf2) - 1);
    ASSERT(rlen2 > 0, "fs_readlink should read symlink target");
    read_buf2[rlen2] = '\0';
    ASSERT_STR_EQ(read_buf2, file2);

    /* 8. Test FS_RENAME and fs_rename */
    char renamed_file[PATH_MAX];
    snprintf(renamed_file, sizeof(renamed_file), "%s/file1_renamed.txt", sub_dir);
    ASSERT(FS_RENAME(file1, renamed_file) == 0, "FS_RENAME should rename file");
    ASSERT(file_exists(renamed_file), "renamed_file should exist");
    ASSERT(!file_exists(file1), "original file1 should not exist after rename");

    char renamed_sym[PATH_MAX];
    snprintf(renamed_sym, sizeof(renamed_sym), "%s/sym2_renamed.lnk", tmp_dir);
    ASSERT(fs_rename(sym2, renamed_sym) == 0, "fs_rename should rename symlink");
    ASSERT(is_symlink(renamed_sym), "renamed_sym should exist as symlink");

    /* 9. Test FS_CHMOD and fs_chmod */
    ASSERT(FS_CHMOD(renamed_file, 0600) == 0, "FS_CHMOD should set permissions");
    ASSERT(fs_chmod(renamed_file, 0644) == 0, "fs_chmod should set permissions");

    /* 10. Test FS_UNLINK, fs_unlink, FS_RMDIR, fs_rmdir */
    ASSERT(FS_UNLINK(sym1) == 0, "FS_UNLINK should remove symlink");
    ASSERT(fs_unlink(renamed_sym) == 0, "fs_unlink should remove symlink");
    ASSERT(FS_UNLINK(renamed_file) == 0, "FS_UNLINK should remove regular file");
    ASSERT(fs_unlink(file2) == 0, "fs_unlink should remove regular file");
    ASSERT(FS_RMDIR(sub_dir) == 0, "FS_RMDIR should remove empty directory");
    ASSERT(fs_rmdir(sub_dir2) == 0, "fs_rmdir should remove empty directory");

    cleanup_test_dir(tmp_dir);
}
