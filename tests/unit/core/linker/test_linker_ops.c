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

void test_native_unstow_edited_user_file(void)
{
    char tmp_dotfiles[PATH_MAX];
    char tmp_target[PATH_MAX];
    ASSERT(create_test_tmp_dir(tmp_dotfiles, sizeof(tmp_dotfiles), "usr_dot") != NULL,
           "Should create temporary dotfiles directory");
    ASSERT(create_test_tmp_dir(tmp_target, sizeof(tmp_target), "usr_tgt") != NULL,
           "Should create temporary target directory");

    char pkg_dir[PATH_MAX * 2];
    snprintf(pkg_dir, sizeof(pkg_dir), "%s/usrpkg", tmp_dotfiles);
    ASSERT(mkdir(pkg_dir, 0755) == 0, "Should create usrpkg dir");

    char pkg_file[PATH_MAX * 2];
    snprintf(pkg_file, sizeof(pkg_file), "%s/.usrconfig", pkg_dir);
    FILE *fp = fopen(pkg_file, "w");
    if (fp) {
        fprintf(fp, "original pkg content\n");
        fclose(fp);
    }

    ASSERT(stow_package(tmp_dotfiles, tmp_target, "usrpkg", false, false) == 0,
           "Stow should succeed");

    char target_file[PATH_MAX * 2];
    snprintf(target_file, sizeof(target_file), "%s/.usrconfig", tmp_target);
    ASSERT(is_symlink(target_file), "target_file should be a symlink initially");

    unlink(target_file);
    FILE *fusr = fopen(target_file, "w");
    if (fusr) {
        fprintf(fusr, "custom user file\n");
        fclose(fusr);
    }

    ASSERT(unstow_package(tmp_dotfiles, tmp_target, "usrpkg", false) == 0, "Unstow should succeed");

    ASSERT(file_exists(target_file), "User file should be preserved during unstow");
    ASSERT(!is_symlink(target_file), "Preserved file should not be a symlink");

    cleanup_test_dir(tmp_dotfiles);
    cleanup_test_dir(tmp_target);
}

void test_native_stow_ignore_patterns(void)
{
    char tmp_dotfiles[PATH_MAX];
    char tmp_target[PATH_MAX];
    ASSERT(create_test_tmp_dir(tmp_dotfiles, sizeof(tmp_dotfiles), "ign_dot") != NULL,
           "Should create temporary dotfiles directory");
    ASSERT(create_test_tmp_dir(tmp_target, sizeof(tmp_target), "ign_tgt") != NULL,
           "Should create temporary target directory");

    char pkg_dir[PATH_MAX * 2];
    snprintf(pkg_dir, sizeof(pkg_dir), "%s/ignpkg", tmp_dotfiles);
    ASSERT(mkdir(pkg_dir, 0755) == 0, "Should create ignpkg dir");

    char ignore_file[PATH_MAX * 2];
    snprintf(ignore_file, sizeof(ignore_file), "%s/.stowignore", pkg_dir);
    FILE *fign = fopen(ignore_file, "w");
    if (fign) {
        fprintf(fign, "*.log\nignored_dir/\n");
        fclose(fign);
    }

    char normal_file[PATH_MAX * 2];
    snprintf(normal_file, sizeof(normal_file), "%s/normal.conf", pkg_dir);
    FILE *fnorm = fopen(normal_file, "w");
    if (fnorm) {
        fprintf(fnorm, "normal\n");
        fclose(fnorm);
    }

    char log_file[PATH_MAX * 2];
    snprintf(log_file, sizeof(log_file), "%s/app.log", pkg_dir);
    FILE *flog = fopen(log_file, "w");
    if (flog) {
        fprintf(flog, "log\n");
        fclose(flog);
    }

    char ign_subdir[PATH_MAX * 2];
    snprintf(ign_subdir, sizeof(ign_subdir), "%s/ignored_dir", pkg_dir);
    mkdir(ign_subdir, 0755);
    char secret_file[PATH_MAX * 2];
    snprintf(secret_file, sizeof(secret_file), "%s/secret.key", ign_subdir);
    FILE *fsec = fopen(secret_file, "w");
    if (fsec) {
        fprintf(fsec, "secret\n");
        fclose(fsec);
    }

    ASSERT(stow_package(tmp_dotfiles, tmp_target, "ignpkg", false, false) == 0,
           "Stow should succeed");

    char target_normal[PATH_MAX * 2];
    char target_log[PATH_MAX * 2];
    char target_secret[PATH_MAX * 2];
    snprintf(target_normal, sizeof(target_normal), "%s/normal.conf", tmp_target);
    snprintf(target_log, sizeof(target_log), "%s/app.log", tmp_target);
    snprintf(target_secret, sizeof(target_secret), "%s/ignored_dir/secret.key", tmp_target);

    ASSERT(is_symlink(target_normal), "normal.conf should be stowed");
    ASSERT(!file_exists(target_log), "app.log should be ignored and not created");
    ASSERT(!file_exists(target_secret), "ignored_dir/secret.key should be ignored and not created");

    cleanup_test_dir(tmp_dotfiles);
    cleanup_test_dir(tmp_target);
}

void test_restow_package(void)
{
    char tmp_dotfiles[PATH_MAX];
    char tmp_target[PATH_MAX];
    ASSERT(create_test_tmp_dir(tmp_dotfiles, sizeof(tmp_dotfiles), "rst_dot") != NULL,
           "Should create temporary dotfiles directory");
    ASSERT(create_test_tmp_dir(tmp_target, sizeof(tmp_target), "rst_tgt") != NULL,
           "Should create temporary target directory");

    char pkg_dir[PATH_MAX];
    snprintf(pkg_dir, sizeof(pkg_dir), "%s/rstpkg", tmp_dotfiles);
    ASSERT(mkdir(pkg_dir, 0755) == 0, "Should create rstpkg directory");

    char file1[PATH_MAX];
    snprintf(file1, sizeof(file1), "%s/.file1", pkg_dir);
    FILE *fp = fopen(file1, "w");
    if (fp) {
        fprintf(fp, "version 1\n");
        fclose(fp);
    }

    ASSERT(stow_package(tmp_dotfiles, tmp_target, "rstpkg", false, false) == 0,
           "Initial stow should succeed");
    ASSERT(is_package_stowed(tmp_target, tmp_dotfiles, "rstpkg"), "rstpkg should be stowed");

    ASSERT(restow_package(tmp_dotfiles, tmp_target, "rstpkg", false, false) == 0,
           "Restow operation should succeed");
    ASSERT(is_package_stowed(tmp_target, tmp_dotfiles, "rstpkg"),
           "rstpkg should remain stowed after restow");

    cleanup_test_dir(tmp_dotfiles);
    cleanup_test_dir(tmp_target);
}

void test_link_all_packages(void)
{
    char tmp_dotfiles[PATH_MAX];
    char tmp_target[PATH_MAX];
    ASSERT(create_test_tmp_dir(tmp_dotfiles, sizeof(tmp_dotfiles), "all_dot") != NULL,
           "Should create temporary dotfiles directory");
    ASSERT(create_test_tmp_dir(tmp_target, sizeof(tmp_target), "all_tgt") != NULL,
           "Should create temporary target directory");

    char pkg1[PATH_MAX];
    snprintf(pkg1, sizeof(pkg1), "%s/pkg1", tmp_dotfiles);
    mkdir(pkg1, 0755);
    char f1[PATH_MAX];
    snprintf(f1, sizeof(f1), "%s/.f1", pkg1);
    FILE *fp1 = fopen(f1, "w");
    if (fp1) {
        fprintf(fp1, "1\n");
        fclose(fp1);
    }

    link_all_packages(tmp_dotfiles, tmp_target, false, true);

    cleanup_test_dir(tmp_dotfiles);
    cleanup_test_dir(tmp_target);
}
