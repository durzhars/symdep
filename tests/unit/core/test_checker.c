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
#include "core/checker.h"
#include "core/manifest.h"
#include "utils/fs.h"

void test_check_package_dependencies(void)
{
    char tmp_dotfiles[PATH_MAX];
    ASSERT(create_test_tmp_dir(tmp_dotfiles, sizeof(tmp_dotfiles), "chk_dep") != NULL,
           "Should create temporary dotfiles directory");

    char pkg_dir[PATH_MAX];
    int snp_res = snprintf(pkg_dir, sizeof(pkg_dir), "%s/chkpkg", tmp_dotfiles);
    ASSERT(snp_res > 0 && (size_t)snp_res < sizeof(pkg_dir),
           "snprintf should successfully format pkg_dir without truncation");
    ASSERT(mkdir(pkg_dir, 0755) == 0, "Should create chkpkg directory");

    manifest_add_dep(tmp_dotfiles, "chkpkg", "sh", "--required");
    manifest_add_dep(tmp_dotfiles, "chkpkg", "nonexistent_tool_xyz_99", "--optional");

    audit_package_dependencies_brief(tmp_dotfiles, "chkpkg");
    check_package_dependencies(tmp_dotfiles, "chkpkg", false, true);
    check_package_dependencies(tmp_dotfiles, "all", false, true);

    cleanup_test_dir(tmp_dotfiles);
}

void test_symlink_health_check(void)
{
    char tmp_dotfiles[PATH_MAX];
    char tmp_target[PATH_MAX];
    ASSERT(create_test_tmp_dir(tmp_dotfiles, sizeof(tmp_dotfiles), "sym_dot") != NULL,
           "Should create temporary dotfiles directory");
    ASSERT(create_test_tmp_dir(tmp_target, sizeof(tmp_target), "sym_tgt") != NULL,
           "Should create temporary target directory");

    char broken_link[PATH_MAX * 4];
    snprintf(broken_link, sizeof(broken_link), "%s/broken.symlink", tmp_dotfiles);
    symlink("/nonexistent/file/path", broken_link);

    char del_pkg[PATH_MAX * 4];
    snprintf(del_pkg, sizeof(del_pkg), "%s/delpkg", tmp_dotfiles);
    mkdir(del_pkg, 0755);
    char del_file[PATH_MAX * 4];
    snprintf(del_file, sizeof(del_file), "%s/.dummy", del_pkg);

    char orphan_link[PATH_MAX * 4];
    snprintf(orphan_link, sizeof(orphan_link), "%s/orphan.symlink", tmp_target);
    symlink(del_file, orphan_link);

    check_symlink_health(tmp_dotfiles, tmp_target);

    cleanup_test_dir(tmp_dotfiles);
    cleanup_test_dir(tmp_target);
}

void test_symlink_health_check_clean(void)
{
    char tmp_dotfiles[PATH_MAX];
    char tmp_target[PATH_MAX];
    ASSERT(create_test_tmp_dir(tmp_dotfiles, sizeof(tmp_dotfiles), "sym_clean_dot") != NULL,
           "Should create temporary dotfiles directory");
    ASSERT(create_test_tmp_dir(tmp_target, sizeof(tmp_target), "sym_clean_tgt") != NULL,
           "Should create temporary target directory");

    char pkg_dir[PATH_MAX];
    snprintf(pkg_dir, sizeof(pkg_dir), "%s/mypkg", tmp_dotfiles);
    mkdir(pkg_dir, 0755);
    char real_file[PATH_MAX];
    snprintf(real_file, sizeof(real_file), "%s/valid.conf", pkg_dir);
    FILE *fp = fopen(real_file, "w");
    if (fp) {
        fprintf(fp, "valid\n");
        fclose(fp);
    }

    char valid_symlink[PATH_MAX];
    snprintf(valid_symlink, sizeof(valid_symlink), "%s/valid.conf", tmp_target);
    symlink(real_file, valid_symlink);

    // Health check on valid setup
    check_symlink_health(tmp_dotfiles, tmp_target);

    cleanup_test_dir(tmp_dotfiles);
    cleanup_test_dir(tmp_target);
}

void test_install_package_dependencies(void)
{
    char tmp_dotfiles[PATH_MAX];
    ASSERT(create_test_tmp_dir(tmp_dotfiles, sizeof(tmp_dotfiles), "install_dep") != NULL,
           "Should create temporary dotfiles directory");

    char pkg_dir[PATH_MAX];
    snprintf(pkg_dir, sizeof(pkg_dir), "%s/mypkg", tmp_dotfiles);
    ASSERT(mkdir(pkg_dir, 0755) == 0, "Should create mypkg directory");

    manifest_add_dep(tmp_dotfiles, "mypkg", "nonexistent_tool_1", "--required");
    manifest_add_dep(tmp_dotfiles, "mypkg", "nonexistent_tool_2", "--optional");

    // Dry run install should identify dependencies and succeed
    int res = install_package_dependencies(tmp_dotfiles, "mypkg", false, true);
    ASSERT(res == 0, "install_package_dependencies dry-run should succeed");

    // Install on package with zero missing dependencies should succeed immediately
    manifest_remove_dep(tmp_dotfiles, "mypkg", "nonexistent_tool_1");
    manifest_remove_dep(tmp_dotfiles, "mypkg", "nonexistent_tool_2");
    res = install_package_dependencies(tmp_dotfiles, "mypkg", false, true);
    ASSERT(res == 0, "install_package_dependencies with 0 missing should return 0");

    cleanup_test_dir(tmp_dotfiles);
}
