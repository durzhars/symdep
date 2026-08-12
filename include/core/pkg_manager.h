/*
 * Symlink & Dependency Manager (symdep)
 * Copyright (C) 2026 durzhars
 *
 * Dynamic UNIX Package Manager Engine & Registry Header
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

#ifndef SYMDEP_PKG_MANAGER_H
#define SYMDEP_PKG_MANAGER_H

#include "utils/str.h"
#include <stdbool.h>
#include <stddef.h>

#define PKG_MGR_NAME_MAX 64
#define PKG_MGR_CMD_MAX 256

typedef struct {
    char name[PKG_MGR_NAME_MAX];
    char binary[PKG_MGR_NAME_MAX];
    char install_cmd[PKG_MGR_CMD_MAX];
    char update_cmd[PKG_MGR_CMD_MAX];
    bool requires_root;
    bool is_custom;
} PkgManagerEntry;

typedef struct {
    PkgManagerEntry *items;
    size_t count;
    size_t capacity;
} PkgManagerArray;

void pkg_manager_array_init(PkgManagerArray *arr);
void pkg_manager_array_append(PkgManagerArray *arr, const PkgManagerEntry *entry);
void pkg_manager_array_free(PkgManagerArray *arr);

void pkg_manager_get_builtins(PkgManagerArray *out_arr);
void pkg_manager_load_custom_config(PkgManagerArray *out_arr, const char *source_dir);
void pkg_manager_get_all(PkgManagerArray *out_arr, const char *source_dir);

bool pkg_manager_find_by_name(const PkgManagerArray *list,
                              const char *name,
                              PkgManagerEntry *out_entry);

void pkg_manager_detect_on_path(const PkgManagerArray *list, PkgManagerArray *out_detected);

int pkg_manager_prompt_selection(const PkgManagerArray *detected, PkgManagerEntry *out_entry);
bool pkg_manager_prompt_fallback(PkgManagerEntry *out_entry, const char *source_dir);

void pkg_manager_get_elevation_tool(const char *source_dir,
                                    const PkgManagerEntry *mgr,
                                    char *out_tool,
                                    size_t out_tool_size,
                                    bool auto_install,
                                    bool dry_run);

void pkg_manager_build_command(const PkgManagerEntry *mgr,
                               const char *source_dir,
                               const char *pkg_list,
                               char *out_cmd,
                               size_t out_cmd_size,
                               bool auto_install,
                               bool dry_run);

bool pkg_manager_resolve(const char *source_dir,
                         const char *cli_override,
                         PkgManagerEntry *out_entry,
                         bool auto_install,
                         bool dry_run);

#endif /* SYMDEP_PKG_MANAGER_H */
