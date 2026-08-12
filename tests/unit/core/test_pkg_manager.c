/*
 * Symlink & Dependency Manager (symdep)
 * Unit Tests for Dynamic Package Manager Engine & Registry
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
#include "core/pkg_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void test_pkg_manager_builtins(void)
{
    PkgManagerArray builtins;
    pkg_manager_get_builtins(&builtins);

    ASSERT(builtins.count > 15, "Should have seeded comprehensive built-in package managers array");

    PkgManagerEntry pacman_entry;
    bool found_pacman = pkg_manager_find_by_name(&builtins, "pacman", &pacman_entry);
    ASSERT(found_pacman, "Should find 'pacman' in built-in table");
    ASSERT(strcmp(pacman_entry.binary, "pacman") == 0, "pacman binary name should match");
    ASSERT(strstr(pacman_entry.install_cmd, "pacman") != NULL,
           "pacman install_cmd should contain pacman");

    PkgManagerEntry brew_entry;
    bool found_brew = pkg_manager_find_by_name(&builtins, "brew", &brew_entry);
    ASSERT(found_brew, "Should find 'brew' in built-in table");
    ASSERT(strcmp(brew_entry.binary, "brew") == 0, "brew binary name should match");

    PkgManagerEntry yay_entry;
    bool found_yay = pkg_manager_find_by_name(&builtins, "yay", &yay_entry);
    ASSERT(found_yay, "Should find 'yay' in built-in table");

    pkg_manager_array_free(&builtins);
}

void test_pkg_manager_custom_config(void)
{
    char tmp_dir[PATH_MAX];
    ASSERT(create_test_tmp_dir(tmp_dir, sizeof(tmp_dir), "pkg_conf") != NULL,
           "Should create temporary test directory");

    char conf_dir[PATH_MAX];
    snprintf(conf_dir, sizeof(conf_dir), "%s/.symdep", tmp_dir);
    mkdir(conf_dir, 0755);

    char conf_file[PATH_MAX];
    snprintf(conf_file, sizeof(conf_file), "%s/pkg_managers.conf", conf_dir);

    FILE *fp = fopen(conf_file, "w");
    ASSERT(fp != NULL, "Should open custom config file for writing");
    fprintf(fp, "# Custom package manager config\n");
    fprintf(fp, "custom_pm=custom_pm=custom_pm install %%s=custom_pm update\n");
    fclose(fp);

    PkgManagerArray all;
    pkg_manager_get_all(&all, tmp_dir);

    PkgManagerEntry custom_entry;
    bool found_custom = pkg_manager_find_by_name(&all, "custom_pm", &custom_entry);
    ASSERT(found_custom, "Should load custom package manager from config file");
    ASSERT(custom_entry.is_custom == true, "custom_entry should have is_custom set to true");
    ASSERT(strcmp(custom_entry.install_cmd, "custom_pm install %s") == 0,
           "install_cmd should match config");

    pkg_manager_array_free(&all);
    cleanup_test_dir(tmp_dir);
}

void test_pkg_manager_resolution_precedence(void)
{
    PkgManagerEntry entry;
    bool resolved = pkg_manager_resolve(NULL, "pacman", &entry, true, true);
    ASSERT(resolved, "CLI override for 'pacman' should resolve successfully");
    ASSERT(strcmp(entry.name, "pacman") == 0, "Resolved package manager name should be 'pacman'");
}

void test_pkg_manager_writable_prefix_elevation(void)
{
    PkgManagerEntry mgr;
    memset(&mgr, 0, sizeof(mgr));
    snprintf(mgr.name, sizeof(mgr.name), "test_writable");
    snprintf(mgr.binary, sizeof(mgr.binary), "sh");
    mgr.requires_root = false;

    char tool[64] = "initial";
    pkg_manager_get_elevation_tool(NULL, &mgr, tool, sizeof(tool), true, true);
    ASSERT(tool[0] == '\0', "Elevation tool should be empty for non-root-required package manager");
}
