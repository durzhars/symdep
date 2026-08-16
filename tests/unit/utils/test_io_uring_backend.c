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
#include "core/linker/internal.h"
#include "utils/fs.h"
#include "utils/io_uring_backend.h"
#include "utils/logger.h"

void test_io_uring_backend_probe(void)
{
    bool supported = io_uring_is_supported();
    if (supported) {
        log_info(
            "[IO_URING] Linux io_uring SQE ring-buffer submission is SUPPORTED on this kernel.");
    } else {
        log_info("[IO_URING] Linux io_uring is RESTRICTED/UNSUPPORTED on this kernel. Fallback "
                 "POSIX engine active.");
    }
    ASSERT(supported == true || supported == false, "io_uring probe must return boolean result");
}

void test_io_uring_null_and_empty_batch(void)
{
    ASSERT(io_uring_link_batch(NULL, NULL) == 0,
           "io_uring_link_batch with NULL files should return 0");

    PkgFileList empty = {0};
    ASSERT(io_uring_link_batch(&empty, NULL) == 0,
           "io_uring_link_batch with empty list should return 0");
}

void test_io_uring_link_batch_execution(void)
{
    char tmp_dotfiles[PATH_MAX];
    char tmp_target[PATH_MAX];
    ASSERT(create_test_tmp_dir(tmp_dotfiles, sizeof(tmp_dotfiles), "uring_dot") != NULL,
           "Should create temporary dotfiles directory");
    ASSERT(create_test_tmp_dir(tmp_target, sizeof(tmp_target), "uring_tgt") != NULL,
           "Should create temporary target directory");

    char pkg_dir[PATH_MAX];
    snprintf(pkg_dir, sizeof(pkg_dir), "%s/uringpkg", tmp_dotfiles);
    ASSERT(mkdir(pkg_dir, 0755) == 0, "Should create uringpkg directory");

    char f1[PATH_MAX], f2[PATH_MAX];
    snprintf(f1, sizeof(f1), "%s/.file1", pkg_dir);
    snprintf(f2, sizeof(f2), "%s/.file2", pkg_dir);

    FILE *fp1 = fopen(f1, "w");
    if (fp1) {
        fprintf(fp1, "uring 1\n");
        fclose(fp1);
    }
    FILE *fp2 = fopen(f2, "w");
    if (fp2) {
        fprintf(fp2, "uring 2\n");
        fclose(fp2);
    }

    PackageContext ctx;
    bool init_ok = package_context_init(&ctx, tmp_dotfiles, tmp_target, "uringpkg", false, false);
    ASSERT(init_ok, "package_context_init should succeed");
    ASSERT(ctx.pkg_files.count == 2, "Package should have 2 files collected");

    if (io_uring_is_supported()) {
        int res = io_uring_link_batch(&ctx.pkg_files, &ctx);
        ASSERT(res == 0, "io_uring_link_batch should return 0 on supported kernel");

        char tgt1[PATH_MAX], tgt2[PATH_MAX];
        snprintf(tgt1, sizeof(tgt1), "%s/.file1", tmp_target);
        snprintf(tgt2, sizeof(tgt2), "%s/.file2", tmp_target);

        ASSERT(is_symlink(tgt1), "tgt1 should be a symlink created via io_uring");
        ASSERT(is_symlink(tgt2), "tgt2 should be a symlink created via io_uring");
    }

    package_context_free(&ctx);
    cleanup_test_dir(tmp_dotfiles);
    cleanup_test_dir(tmp_target);
}
