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
#include "core/file_collector.h"
#include "core/ignore.h"

void test_stowignore(void)
{
    char tmp_dir[PATH_MAX];
    ASSERT(create_test_tmp_dir(tmp_dir, sizeof(tmp_dir), "ignore") != NULL,
           "Should create temporary directory for stowignore test");

    char ignore_file[PATH_MAX * 2];
    snprintf(ignore_file, sizeof(ignore_file), "%s/.stowignore", tmp_dir);
    FILE *fp = fopen(ignore_file, "w");
    ASSERT(fp != NULL, "Should create .stowignore file");
    fprintf(fp, "# Comment line\nREADME.md\n*.bak\n");
    fclose(fp);

    StringArray patterns;
    str_array_init(&patterns);
    parse_stowignore(tmp_dir, &patterns);

    ASSERT(str_array_contains(&patterns, "README\\.md"),
           ".stowignore should contain escaped README\\.md");
    ASSERT(str_array_contains(&patterns, ".*\\.bak"),
           ".stowignore should contain escaped .*\\.bak");
    str_array_free(&patterns);

    cleanup_test_dir(tmp_dir);
}

void test_ignore_add_and_remove_patterns(void)
{
    char tmp_dir[PATH_MAX];
    ASSERT(create_test_tmp_dir(tmp_dir, sizeof(tmp_dir), "ign_pat") != NULL,
           "Should create temporary test directory");

    char pkg_dir[PATH_MAX];
    snprintf(pkg_dir, sizeof(pkg_dir), "%s/testpkg", tmp_dir);
    ASSERT(mkdir(pkg_dir, 0755) == 0, "Should create testpkg directory");

    const char *add_pats[] = {"*.tmp", "cache/", "build.log"};
    ignore_add_patterns(tmp_dir, "testpkg", add_pats, 3);

    char pkg_ignore[PATH_MAX];
    snprintf(pkg_ignore, sizeof(pkg_ignore), "%s/.symignore", pkg_dir);
    if (!file_exists(pkg_ignore)) {
        snprintf(pkg_ignore, sizeof(pkg_ignore), "%s/.stowignore", pkg_dir);
    }
    ASSERT(file_exists(pkg_ignore), "Ignore file should be created automatically");

    StringArray raw_patterns;
    str_array_init(&raw_patterns);
    parse_stowignore_raw(pkg_dir, &raw_patterns);
    ASSERT(str_array_contains(&raw_patterns, "*.tmp"), "Should contain *.tmp pattern");
    ASSERT(str_array_contains(&raw_patterns, "cache/"), "Should contain cache/ pattern");
    ASSERT(str_array_contains(&raw_patterns, "build.log"), "Should contain build.log pattern");
    str_array_free(&raw_patterns);

    ignore_add_patterns(tmp_dir, "testpkg", add_pats, 1);
    str_array_init(&raw_patterns);
    parse_stowignore_raw(pkg_dir, &raw_patterns);
    size_t tmp_count = 0;
    for (size_t i = 0; i < raw_patterns.count; i++) {
        if (strcmp(raw_patterns.items[i], "*.tmp") == 0) {
            tmp_count++;
        }
    }
    ASSERT(tmp_count == 1, "*.tmp should not be duplicated in ignore file");
    str_array_free(&raw_patterns);

    const char *rem_pats[] = {"cache/", "non_existent_pattern"};
    ignore_remove_patterns(tmp_dir, "testpkg", rem_pats, 2);

    str_array_init(&raw_patterns);
    parse_stowignore_raw(pkg_dir, &raw_patterns);
    ASSERT(str_array_contains(&raw_patterns, "*.tmp"), "Should still contain *.tmp");
    ASSERT(!str_array_contains(&raw_patterns, "cache/"), "cache/ should be removed");
    str_array_free(&raw_patterns);

    const char *global_pats[] = {"*.bak"};
    ignore_add_patterns(tmp_dir, NULL, global_pats, 1);
    char root_ignore[PATH_MAX];
    snprintf(root_ignore, sizeof(root_ignore), "%s/.symignore", tmp_dir);
    if (!file_exists(root_ignore)) {
        snprintf(root_ignore, sizeof(root_ignore), "%s/.stowignore", tmp_dir);
    }
    ASSERT(file_exists(root_ignore), "Global ignore file should be created");

    str_array_init(&raw_patterns);
    parse_stowignore_raw(tmp_dir, &raw_patterns);
    ASSERT(str_array_contains(&raw_patterns, "*.bak"), "Global .stowignore should contain *.bak");
    str_array_free(&raw_patterns);

    ignore_add_patterns(tmp_dir, "testpkg", NULL, 0);
    ignore_remove_patterns(tmp_dir, "testpkg", NULL, 0);

    cleanup_test_dir(tmp_dir);
}
