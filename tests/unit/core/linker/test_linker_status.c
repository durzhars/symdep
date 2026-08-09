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

void test_package_stow_status(void)
{
    char tmp_dotfiles[PATH_MAX];
    char tmp_target[PATH_MAX];
    ASSERT(create_test_tmp_dir(tmp_dotfiles, sizeof(tmp_dotfiles), "stat_dot") != NULL,
           "Should create temporary dotfiles directory");
    ASSERT(create_test_tmp_dir(tmp_target, sizeof(tmp_target), "stat_tgt") != NULL,
           "Should create temporary target directory");

    char pkg_dir[PATH_MAX];
    snprintf(pkg_dir, sizeof(pkg_dir), "%s/statpkg", tmp_dotfiles);
    mkdir(pkg_dir, 0755);

    char f1[PATH_MAX];
    char f2[PATH_MAX];
    char f_deps[PATH_MAX];
    snprintf(f1, sizeof(f1), "%s/.file1", pkg_dir);
    snprintf(f2, sizeof(f2), "%s/.file2", pkg_dir);
    snprintf(f_deps, sizeof(f_deps), "%s/.stowdeps", pkg_dir);

    FILE *fp1 = fopen(f1, "w");
    if (fp1) {
        fprintf(fp1, "1\n");
        fclose(fp1);
    }
    FILE *fp2 = fopen(f2, "w");
    if (fp2) {
        fprintf(fp2, "2\n");
        fclose(fp2);
    }
    FILE *fpd = fopen(f_deps, "w");
    if (fpd) {
        fprintf(fpd, "REQUIRED=\"\"\n");
        fclose(fpd);
    }

    ASSERT(get_package_stow_status(tmp_target, tmp_dotfiles, "statpkg") == STOW_STATUS_UNSTOWED,
           "Should be UNSTOWED initially");

    char tf1[PATH_MAX];
    snprintf(tf1, sizeof(tf1), "%s/.file1", tmp_target);
    symlink(f1, tf1);
    ASSERT(get_package_stow_status(tmp_target, tmp_dotfiles, "statpkg") == STOW_STATUS_PARTIAL,
           "Should be PARTIAL when 1/2 non-ignored files stowed");

    char tf2[PATH_MAX];
    snprintf(tf2, sizeof(tf2), "%s/.file2", tmp_target);
    symlink(f2, tf2);
    ASSERT(get_package_stow_status(tmp_target, tmp_dotfiles, "statpkg") == STOW_STATUS_STOWED,
           "Should be STOWED when 2/2 non-ignored files stowed");

    list_packages_status(tmp_dotfiles, tmp_target);

    cleanup_test_dir(tmp_dotfiles);
    cleanup_test_dir(tmp_target);
}
