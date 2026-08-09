/*
 * Dotfiles Stow Manager (stow-manager)
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
#include "core/ignore.h"
#include "core/file_collector.h"

void test_ignore_init_and_clear(void)
{
    char tmp_dir[PATH_MAX];
    ASSERT(create_test_tmp_dir(tmp_dir, sizeof(tmp_dir), "ign_init") != NULL,
           "Should create temporary test directory");

    char pkg_dir[PATH_MAX];
    snprintf(pkg_dir, sizeof(pkg_dir), "%s/mypkg", tmp_dir);
    ASSERT(mkdir(pkg_dir, 0755) == 0, "Should create package directory");

    // 1. Initialize root ignore file
    ignore_init(tmp_dir, NULL, 0);
    char root_ignore[PATH_MAX];
    snprintf(root_ignore, sizeof(root_ignore), "%s/.symignore", tmp_dir);
    if (!file_exists(root_ignore)) {
        snprintf(root_ignore, sizeof(root_ignore), "%s/.stowignore", tmp_dir);
    }
    ASSERT(file_exists(root_ignore), "Root ignore file should exist after ignore_init");

    // Re-init root ignore file (should warn, not overwrite)
    ignore_init(tmp_dir, NULL, 0);
    ASSERT(file_exists(root_ignore), "Root ignore file should still exist");

    // 2. Initialize package ignore file
    const char *pkgs[] = {"mypkg"};
    ignore_init(tmp_dir, pkgs, 1);
    char pkg_ignore[PATH_MAX];
    snprintf(pkg_ignore, sizeof(pkg_ignore), "%s/.symignore", pkg_dir);
    if (!file_exists(pkg_ignore)) {
        snprintf(pkg_ignore, sizeof(pkg_ignore), "%s/.stowignore", pkg_dir);
    }
    ASSERT(file_exists(pkg_ignore), "Package ignore file should exist after ignore_init");

    // 3. Clear package ignore file
    ignore_clear(tmp_dir, pkgs, 1);
    ASSERT(!file_exists(pkg_ignore), "Package ignore file should be deleted after ignore_clear");

    // 4. Clear root ignore file
    ignore_clear(tmp_dir, NULL, 0);
    ASSERT(!file_exists(root_ignore), "Root ignore file should be deleted after ignore_clear");

    // Clear non-existent file gracefully
    ignore_clear(tmp_dir, pkgs, 1);

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

    // 1. Add patterns to package ignore file (should auto-init if not exists)
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

    // 2. Prevent duplicate additions
    ignore_add_patterns(tmp_dir, "testpkg", add_pats, 1); // add *.tmp again
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

    // 3. Remove patterns
    const char *rem_pats[] = {"cache/", "non_existent_pattern"};
    ignore_remove_patterns(tmp_dir, "testpkg", rem_pats, 2);

    str_array_init(&raw_patterns);
    parse_stowignore_raw(pkg_dir, &raw_patterns);
    ASSERT(str_array_contains(&raw_patterns, "*.tmp"), "Should still contain *.tmp");
    ASSERT(!str_array_contains(&raw_patterns, "cache/"), "cache/ should be removed");
    str_array_free(&raw_patterns);

    // 4. Add patterns to global ignore file
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

    // 5. Edge case: NULL or 0 count patterns
    ignore_add_patterns(tmp_dir, "testpkg", NULL, 0);
    ignore_remove_patterns(tmp_dir, "testpkg", NULL, 0);

    cleanup_test_dir(tmp_dir);
}

void test_ignore_show(void)
{
    char tmp_dir[PATH_MAX];
    ASSERT(create_test_tmp_dir(tmp_dir, sizeof(tmp_dir), "ign_show") != NULL,
           "Should create temporary test directory");

    char pkg_dir[PATH_MAX];
    snprintf(pkg_dir, sizeof(pkg_dir), "%s/showpkg", tmp_dir);
    ASSERT(mkdir(pkg_dir, 0755) == 0, "Should create showpkg directory");

    // Show non-existent (should warn gracefully)
    ignore_show(tmp_dir, NULL, 0);
    const char *pkgs[] = {"showpkg"};
    ignore_show(tmp_dir, pkgs, 1);

    // Initialize and show
    ignore_init(tmp_dir, pkgs, 1);
    ignore_show(tmp_dir, pkgs, 1);

    cleanup_test_dir(tmp_dir);
}
