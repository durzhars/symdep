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
#include "core/file_collector.h"
#include "utils/str.h"

void test_default_stowignore(void)
{
    StringArray defaults;
    str_array_init(&defaults);
    get_default_stowignore(&defaults);

    ASSERT(defaults.count > 0, "Default stowignore patterns should be loaded");
    ASSERT(str_array_contains(&defaults, ".gitignore"), "Should contain .gitignore pattern");
    ASSERT(str_array_contains(&defaults, ".git"), "Should contain .git pattern");

    ASSERT(is_path_ignored(".config/nvim_lazyvim_backup/.gitignore", &defaults),
           "Subdirectory .gitignore must be ignored by default patterns");
    ASSERT(is_path_ignored("README.md", &defaults),
           "README.md must be ignored by default patterns");
    ASSERT(is_path_ignored("nested/README.md", &defaults),
           "nested/README.md must be ignored by default patterns");
    ASSERT(!is_path_ignored(".config/nvim/init.lua", &defaults),
           "Normal config file must not be ignored");

    str_array_free(&defaults);
}

void test_pkg_file_list_ops(void)
{
    PkgFileList list;
    pkg_file_list_init(&list);
    ASSERT(list.count == 0, "Initial PkgFileList count should be 0");

    pkg_file_list_append(&list, "config/init.lua", "/tmp/pkg/config/init.lua", false);
    ASSERT(list.count == 1, "PkgFileList count should be 1 after append");
    ASSERT_STR_EQ(list.entries[0].rel_path, "config/init.lua");

    pkg_file_list_free(&list);
    ASSERT(list.count == 0, "PkgFileList count should be 0 after free");
}

void test_collect_package_files(void)
{
    char tmp_dir[PATH_MAX];
    ASSERT(create_test_tmp_dir(tmp_dir, sizeof(tmp_dir), "coll_pkg") != NULL,
           "Should create temporary test directory");

    char pkg_dir[PATH_MAX];
    snprintf(pkg_dir, sizeof(pkg_dir), "%s/mypkg", tmp_dir);
    mkdir(pkg_dir, 0755);

    char file1[PATH_MAX];
    snprintf(file1, sizeof(file1), "%s/app.conf", pkg_dir);
    FILE *fp = fopen(file1, "w");
    if (fp) {
        fprintf(fp, "config\n");
        fclose(fp);
    }

    char ignore_file[PATH_MAX];
    snprintf(ignore_file, sizeof(ignore_file), "%s/.symignore", pkg_dir);
    FILE *fign = fopen(ignore_file, "w");
    if (fign) {
        fprintf(fign, "*.log # comment\n");
        fclose(fign);
    }

    StringArray raw_ignores;
    str_array_init(&raw_ignores);
    get_default_ignore_patterns(&raw_ignores);
    parse_ignore_file_raw(pkg_dir, &raw_ignores);
    ASSERT(raw_ignores.count > 1, "raw_ignores should contain default and parsed patterns");

    StringArray parsed_patterns;
    str_array_init(&parsed_patterns);
    parse_ignore_file(pkg_dir, &parsed_patterns);
    ASSERT(parsed_patterns.count == 1, "parse_ignore_file should parse 1 escaped pattern");

    PkgFileList list;
    collect_package_files(pkg_dir, &raw_ignores, &list);
    ASSERT(list.count == 1, "Should collect 1 file");
    ASSERT_STR_EQ(list.entries[0].rel_path, "app.conf");

    pkg_file_list_free(&list);
    str_array_free(&raw_ignores);
    str_array_free(&parsed_patterns);
    cleanup_test_dir(tmp_dir);
}
