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

typedef struct {
    int file_count;
} WalkTestContext;

static void count_files_cb(const char *file_path, const char *rel_path, void *user_data)
{
    (void)file_path;
    (void)rel_path;
    WalkTestContext *ctx = (WalkTestContext *)user_data;
    ctx->file_count++;
}

static void symlink_cb(const char *symlink_path, void *user_data)
{
    (void)symlink_path;
    WalkTestContext *ctx = (WalkTestContext *)user_data;
    ctx->file_count++;
}

void test_walk_dir_files_and_cleanup(void)
{
    char tmp_dir[PATH_MAX];
    ASSERT(create_test_tmp_dir(tmp_dir, sizeof(tmp_dir), "walk_t") != NULL,
           "Should create temporary directory for walk test");

    char sub_dir[PATH_MAX];
    snprintf(sub_dir, sizeof(sub_dir), "%s/subdir", tmp_dir);
    mkdir(sub_dir, 0755);

    char f1[PATH_MAX];
    char f2[PATH_MAX];
    snprintf(f1, sizeof(f1), "%s/file1.txt", tmp_dir);
    snprintf(f2, sizeof(f2), "%s/file2.txt", sub_dir);

    FILE *fp1 = fopen(f1, "w");
    if (fp1) {
        fprintf(fp1, "f1\n");
        fclose(fp1);
    }
    FILE *fp2 = fopen(f2, "w");
    if (fp2) {
        fprintf(fp2, "f2\n");
        fclose(fp2);
    }

    WalkTestContext ctx = {0};
    walk_dir_files(tmp_dir, NULL, count_files_cb, &ctx);
    ASSERT(ctx.file_count == 2, "walk_dir_files should count 2 regular files");

    char sym_link[PATH_MAX];
    snprintf(sym_link, sizeof(sym_link), "%s/link.sym", tmp_dir);
    symlink(f1, sym_link);

    WalkTestContext sym_ctx = {0};
    walk_dir_symlinks(tmp_dir, 1, 3, symlink_cb, &sym_ctx);
    ASSERT(sym_ctx.file_count == 1, "walk_dir_symlinks should count 1 symlink");

    cleanup_temp_dir_contents(tmp_dir);
    ASSERT(!file_exists(f1), "f1 should be deleted after cleanup_temp_dir_contents");
    ASSERT(!file_exists(f2), "f2 should be deleted after cleanup_temp_dir_contents");
    ASSERT(!is_dir(sub_dir), "subdir should be deleted after cleanup_temp_dir_contents");

    cleanup_test_dir(tmp_dir);
}
