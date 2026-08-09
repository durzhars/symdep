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
#include "core/ignore.h"

void test_ignore_init_and_clear(void)
{
    char tmp_dir[PATH_MAX];
    ASSERT(create_test_tmp_dir(tmp_dir, sizeof(tmp_dir), "ign_init") != NULL,
           "Should create temporary test directory");

    char pkg_dir[PATH_MAX];
    snprintf(pkg_dir, sizeof(pkg_dir), "%s/mypkg", tmp_dir);
    ASSERT(mkdir(pkg_dir, 0755) == 0, "Should create package directory");

    ignore_init(tmp_dir, NULL, 0);
    char root_ignore[PATH_MAX];
    snprintf(root_ignore, sizeof(root_ignore), "%s/.symignore", tmp_dir);
    if (!file_exists(root_ignore)) {
        snprintf(root_ignore, sizeof(root_ignore), "%s/.stowignore", tmp_dir);
    }
    ASSERT(file_exists(root_ignore), "Root ignore file should exist after ignore_init");

    ignore_init(tmp_dir, NULL, 0);
    ASSERT(file_exists(root_ignore), "Root ignore file should still exist");

    const char *pkgs[] = {"mypkg"};
    ignore_init(tmp_dir, pkgs, 1);
    char pkg_ignore[PATH_MAX];
    snprintf(pkg_ignore, sizeof(pkg_ignore), "%s/.symignore", pkg_dir);
    if (!file_exists(pkg_ignore)) {
        snprintf(pkg_ignore, sizeof(pkg_ignore), "%s/.stowignore", pkg_dir);
    }
    ASSERT(file_exists(pkg_ignore), "Package ignore file should exist after ignore_init");

    ignore_clear(tmp_dir, pkgs, 1);
    ASSERT(!file_exists(pkg_ignore), "Package ignore file should be deleted after ignore_clear");

    ignore_clear(tmp_dir, NULL, 0);
    ASSERT(!file_exists(root_ignore), "Root ignore file should be deleted after ignore_clear");

    ignore_clear(tmp_dir, pkgs, 1);

    cleanup_test_dir(tmp_dir);
}
