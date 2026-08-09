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

void test_ignore_show(void)
{
    char tmp_dir[PATH_MAX];
    ASSERT(create_test_tmp_dir(tmp_dir, sizeof(tmp_dir), "ign_show") != NULL,
           "Should create temporary test directory");

    char pkg_dir[PATH_MAX];
    snprintf(pkg_dir, sizeof(pkg_dir), "%s/showpkg", tmp_dir);
    ASSERT(mkdir(pkg_dir, 0755) == 0, "Should create showpkg directory");

    ignore_show(tmp_dir, NULL, 0);
    const char *pkgs[] = {"showpkg"};
    ignore_show(tmp_dir, pkgs, 1);

    ignore_init(tmp_dir, pkgs, 1);
    ignore_show(tmp_dir, pkgs, 1);

    cleanup_test_dir(tmp_dir);
}
