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
#include "core/manifest.h"
#include "utils/fs.h"

void test_handle_mutual_exclusions(void)
{
    char tmp_dotfiles[PATH_MAX];
    char tmp_target[PATH_MAX];
    ASSERT(create_test_tmp_dir(tmp_dotfiles, sizeof(tmp_dotfiles), "mut_dot") != NULL,
           "Should create temporary dotfiles directory");
    ASSERT(create_test_tmp_dir(tmp_target, sizeof(tmp_target), "mut_tgt") != NULL,
           "Should create temporary target directory");

    char pkg_a_dir[PATH_MAX];
    char pkg_b_dir[PATH_MAX];
    snprintf(pkg_a_dir, sizeof(pkg_a_dir), "%s/pkgA", tmp_dotfiles);
    snprintf(pkg_b_dir, sizeof(pkg_b_dir), "%s/pkgB", tmp_dotfiles);
    ASSERT(mkdir(pkg_a_dir, 0755) == 0, "Should create pkgA dir");
    ASSERT(mkdir(pkg_b_dir, 0755) == 0, "Should create pkgB dir");

    char file_a[PATH_MAX];
    char file_b[PATH_MAX];
    snprintf(file_a, sizeof(file_a), "%s/.fileA", pkg_a_dir);
    snprintf(file_b, sizeof(file_b), "%s/.fileB", pkg_b_dir);

    FILE *fa = fopen(file_a, "w");
    if (fa) {
        fprintf(fa, "content A\n");
        fclose(fa);
    }
    FILE *fb = fopen(file_b, "w");
    if (fb) {
        fprintf(fb, "content B\n");
        fclose(fb);
    }

    manifest_add_dep(tmp_dotfiles, "pkgA", "pkgB", "--conflict");

    int res_b = stow_package(tmp_dotfiles, tmp_target, "pkgB", false, false);
    ASSERT(res_b == 0, "Stowing pkgB should succeed");
    ASSERT(is_package_stowed(tmp_target, tmp_dotfiles, "pkgB"), "pkgB should be stowed");

    handle_mutual_exclusions(tmp_target, tmp_dotfiles, "pkgA", false);

    ASSERT(!is_package_stowed(tmp_target, tmp_dotfiles, "pkgB"),
           "pkgB should be unstowed after mutual exclusion handling");

    cleanup_test_dir(tmp_dotfiles);
    cleanup_test_dir(tmp_target);
}

void test_native_stow_broken_symlink_conflict(void)
{
    char tmp_dotfiles[PATH_MAX];
    char tmp_target[PATH_MAX];
    ASSERT(create_test_tmp_dir(tmp_dotfiles, sizeof(tmp_dotfiles), "brk_dot") != NULL,
           "Should create temporary dotfiles directory");
    ASSERT(create_test_tmp_dir(tmp_target, sizeof(tmp_target), "brk_tgt") != NULL,
           "Should create temporary target directory");

    char pkg_dir[PATH_MAX * 2];
    snprintf(pkg_dir, sizeof(pkg_dir), "%s/brkpkg", tmp_dotfiles);
    ASSERT(mkdir(pkg_dir, 0755) == 0, "Should create brkpkg dir");

    char pkg_file[PATH_MAX * 2];
    snprintf(pkg_file, sizeof(pkg_file), "%s/.configfile", pkg_dir);
    FILE *fp = fopen(pkg_file, "w");
    if (fp) {
        fprintf(fp, "valid content\n");
        fclose(fp);
    }

    char target_link[PATH_MAX * 2];
    snprintf(target_link, sizeof(target_link), "%s/.configfile", tmp_target);
    ASSERT(symlink("/nonexistent/file/path/xyz", target_link) == 0,
           "Should create broken symlink in target");
    ASSERT(is_symlink(target_link), "target_link should be a symlink");

    int res = stow_package(tmp_dotfiles, tmp_target, "brkpkg", false, false);
    ASSERT(res == 0, "stow_package should succeed over broken symlink");

    ASSERT(is_symlink(target_link), "target_link should still be a symlink");
    char *sym_target = read_symlink_target(target_link);
    ASSERT(sym_target != NULL, "symlink target should be readable");
    ASSERT_STR_EQ(sym_target, pkg_file);
    free(sym_target);

    cleanup_test_dir(tmp_dotfiles);
    cleanup_test_dir(tmp_target);
}

void test_dynamic_package_conflicts(void)
{
    char tmp_dotfiles[PATH_MAX];
    char tmp_target[PATH_MAX];
    ASSERT(create_test_tmp_dir(tmp_dotfiles, sizeof(tmp_dotfiles), "dyn_dot") != NULL,
           "Should create temporary dotfiles directory");
    ASSERT(create_test_tmp_dir(tmp_target, sizeof(tmp_target), "dyn_tgt") != NULL,
           "Should create temporary target directory");

    char pkg1_dir[PATH_MAX], pkg2_dir[PATH_MAX];
    snprintf(pkg1_dir, sizeof(pkg1_dir), "%s/nvim", tmp_dotfiles);
    snprintf(pkg2_dir, sizeof(pkg2_dir), "%s/nvim-headless", tmp_dotfiles);
    ASSERT(mkdir(pkg1_dir, 0755) == 0, "Should create nvim dir");
    ASSERT(mkdir(pkg2_dir, 0755) == 0, "Should create nvim-headless dir");

    char cfg1[PATH_MAX], cfg2[PATH_MAX];
    snprintf(cfg1, sizeof(cfg1), "%s/.config", pkg1_dir);
    snprintf(cfg2, sizeof(cfg2), "%s/.config", pkg2_dir);
    mkdir(cfg1, 0755);
    mkdir(cfg2, 0755);

    char file1[PATH_MAX], file2[PATH_MAX];
    snprintf(file1, sizeof(file1), "%s/.config/init.lua", pkg1_dir);
    snprintf(file2, sizeof(file2), "%s/.config/init.lua", pkg2_dir);

    FILE *f1 = fopen(file1, "w");
    if (f1) {
        fprintf(f1, "full nvim config\n");
        fclose(f1);
    }
    FILE *f2 = fopen(file2, "w");
    if (f2) {
        fprintf(f2, "headless nvim config\n");
        fclose(f2);
    }

    PkgFileList list;
    pkg_file_list_init(&list);
    pkg_file_list_append(&list, ".config/init.lua", file1, false);

    prepare_target_conflicts(tmp_target, tmp_dotfiles, "nvim", &list, true);

    pkg_file_list_free(&list);

    ASSERT(stow_package(tmp_dotfiles, tmp_target, "nvim", false, false) == 0,
           "Stow nvim should succeed");
    ASSERT(is_package_stowed(tmp_target, tmp_dotfiles, "nvim"), "nvim package should be stowed");

    ASSERT(stow_package(tmp_dotfiles, tmp_target, "nvim-headless", false, false) == 0,
           "Stow nvim-headless should succeed");
    ASSERT(!is_package_stowed(tmp_target, tmp_dotfiles, "nvim"),
           "nvim should be automatically unstowed on collision");
    ASSERT(is_package_stowed(tmp_target, tmp_dotfiles, "nvim-headless"),
           "nvim-headless should now be stowed");

    cleanup_test_dir(tmp_dotfiles);
    cleanup_test_dir(tmp_target);
}
