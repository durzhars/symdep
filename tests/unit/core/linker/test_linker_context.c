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
#include "core/checker.h"
#include "core/linker.h"
#include "core/linker/internal.h"

void test_dry_run_stow(void)
{
    char tmp_dotfiles[PATH_MAX];
    char tmp_target[PATH_MAX];
    ASSERT(create_test_tmp_dir(tmp_dotfiles, sizeof(tmp_dotfiles), "dry_dot") != NULL,
           "Should create temporary dotfiles directory");
    ASSERT(create_test_tmp_dir(tmp_target, sizeof(tmp_target), "dry_tgt") != NULL,
           "Should create temporary target directory");

    char pkg_dir[PATH_MAX * 4];
    snprintf(pkg_dir, sizeof(pkg_dir), "%s/mypkg", tmp_dotfiles);
    mkdir(pkg_dir, 0755);

    char cfg_file[PATH_MAX * 4];
    snprintf(cfg_file, sizeof(cfg_file), "%s/.configfile", pkg_dir);
    FILE *fp = fopen(cfg_file, "w");
    if (fp) {
        fprintf(fp, "test content\n");
        fclose(fp);
    }

    int res = stow_package(tmp_dotfiles, tmp_target, "mypkg", false, true);
    ASSERT(res == 0, "Dry run stow should return 0 success");

    char target_cfg[PATH_MAX * 4];
    snprintf(target_cfg, sizeof(target_cfg), "%s/.configfile", tmp_target);
    ASSERT(!file_exists(target_cfg), "Dry run stow must not modify disk or create symlinks");

    cleanup_test_dir(tmp_dotfiles);
    cleanup_test_dir(tmp_target);
}

void test_package_context_init(void)
{
    char tmp_dotfiles[PATH_MAX];
    char tmp_target[PATH_MAX];
    ASSERT(create_test_tmp_dir(tmp_dotfiles, sizeof(tmp_dotfiles), "ctx_dot") != NULL,
           "Should create temporary dotfiles directory");
    ASSERT(create_test_tmp_dir(tmp_target, sizeof(tmp_target), "ctx_tgt") != NULL,
           "Should create temporary target directory");

    char pkg_dir[PATH_MAX];
    snprintf(pkg_dir, sizeof(pkg_dir), "%s/ctxpkg", tmp_dotfiles);
    mkdir(pkg_dir, 0755);

    PackageContext ctx;
    bool ok = package_context_init(&ctx, tmp_dotfiles, tmp_target, "ctxpkg", false, true);
    ASSERT(ok, "package_context_init should succeed for valid package");
    ASSERT_STR_EQ(ctx.pkg_name, "ctxpkg");

    package_context_free(&ctx);
    cleanup_test_dir(tmp_dotfiles);
    cleanup_test_dir(tmp_target);
}
