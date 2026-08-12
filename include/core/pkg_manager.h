/*
 * Symlink & Dependency Manager (symdep)
 * Dynamic UNIX Package Manager Engine & Registry Header
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

#ifndef SYMDEP_PKG_MANAGER_H
#define SYMDEP_PKG_MANAGER_H

#include "utils/str.h"
#include <stdbool.h>
#include <stddef.h>

#define PKG_MGR_NAME_MAX 64
#define PKG_MGR_CMD_MAX 256

/**
 * @struct PkgManagerEntry
 * @brief Represents a system package manager definition entry.
 */
typedef struct {
    char name[PKG_MGR_NAME_MAX];       /**< Package manager identifier (e.g. "pacman") */
    char binary[PKG_MGR_NAME_MAX];     /**< Binary executable name on $PATH */
    char install_cmd[PKG_MGR_CMD_MAX]; /**< Install command template (uses %s for packages) */
    char update_cmd[PKG_MGR_CMD_MAX];  /**< Repository update command */
    bool requires_root;                /**< Whether root privilege elevation is needed */
    bool is_custom;                    /**< Whether entry is user-defined */
} PkgManagerEntry;

/** Dynamic array list of PkgManagerEntry items */
typedef struct {
    PkgManagerEntry *items;
    size_t count;
    size_t capacity;
} PkgManagerArray;

void pkg_manager_array_init(PkgManagerArray *arr);
void pkg_manager_array_append(PkgManagerArray *arr, const PkgManagerEntry *entry);
void pkg_manager_array_free(PkgManagerArray *arr);

/** Load built-in package managers (pacman, apt, dnf, apk, brew, etc.) */
void pkg_manager_get_builtins(PkgManagerArray *out_arr);

/** Load custom user package managers from configuration */
void pkg_manager_load_custom_config(PkgManagerArray *out_arr, const char *source_dir);

/** Get all available package manager entries */
void pkg_manager_get_all(PkgManagerArray *out_arr, const char *source_dir);

/** Search package manager list by name */
bool pkg_manager_find_by_name(const PkgManagerArray *list,
                              const char *name,
                              PkgManagerEntry *out_entry);

/** Filter package manager list for binaries present on active $PATH */
void pkg_manager_detect_on_path(const PkgManagerArray *list, PkgManagerArray *out_detected);

/** Interactive menu to prompt user when multiple package managers exist on $PATH */
int pkg_manager_prompt_selection(const PkgManagerArray *detected, PkgManagerEntry *out_entry);

/** Fallback prompt when no registered package manager is on $PATH */
bool pkg_manager_prompt_fallback(PkgManagerEntry *out_entry, const char *source_dir);

/** Determine required privilege elevation tool (sudo, doas, tsu, or none) */
void pkg_manager_get_elevation_tool(const char *source_dir,
                                    const PkgManagerEntry *mgr,
                                    char *out_tool,
                                    size_t out_tool_size,
                                    bool auto_install,
                                    bool dry_run);

/** Construct shell execution command string for installing package list */
void pkg_manager_build_command(const PkgManagerEntry *mgr,
                               const char *source_dir,
                               const char *pkg_list,
                               char *out_cmd,
                               size_t out_cmd_size,
                               bool auto_install,
                               bool dry_run);

/** Resolve active package manager respecting CLI -> Env -> Config -> PATH probing */
bool pkg_manager_resolve(const char *source_dir,
                         const char *cli_override,
                         PkgManagerEntry *out_entry,
                         bool auto_install,
                         bool dry_run);

#endif /* SYMDEP_PKG_MANAGER_H */
