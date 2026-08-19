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

#ifndef SYMDEP_REGISTRY_H
#define SYMDEP_REGISTRY_H

#include "utils/str.h"

/**
 * @brief Retrieve binary aliases for a virtual tool name from symdep.registry.
 *
 * @param source_dir Path to active source repository.
 * @param tool       Virtual tool name (e.g., "bat").
 * @param aliases    Output StringArray populated with alias strings.
 */
void registry_get_aliases(const char *source_dir, const char *tool, StringArray *aliases);

/**
 * @brief Map a virtual tool name to a distro-specific package manager name.
 *
 * @param source_dir Path to active source repository.
 * @param tool       Virtual tool name (e.g., "bat").
 * @param distro     Active distro identifier (e.g., "ubuntu").
 * @param out        Output buffer for distro package name.
 * @param out_size   Size of output buffer.
 */
void registry_get_distro_pkg(const char *source_dir,
                             const char *tool,
                             const char *distro,
                             char *out,
                             size_t out_size);

/**
 * @brief Get all registered tool names from symdep.registry.
 *
 * @param source_dir Path to active source repository.
 * @param tools      Output StringArray populated with registered tool names.
 */
void registry_get_all_tools(const char *source_dir, StringArray *tools);

/**
 * @brief Append a new tool entry to symdep.registry.
 *
 * @param source_dir Path to active source repository.
 * @param tool       Tool string or alias rule to add.
 */
void registry_add_tool(const char *source_dir, const char *tool);

/**
 * @brief Append or update a distro-specific package mapping in symdep.registry.
 *
 * @param source_dir Path to active source repository.
 * @param tool       Virtual tool name (e.g., "fd").
 * @param distro     Distro identifier (e.g., "ubuntu").
 * @param pkg_name   Target package manager package name (e.g., "fd-find").
 */
void registry_add_distro_mapping(const char *source_dir,
                                 const char *tool,
                                 const char *distro,
                                 const char *pkg_name);

/**
 * @brief Check if a tool or plugin is installed using registry rules and $PATH.
 *
 * @param source_dir Path to active source repository.
 * @param tool       Tool or plugin name/rule string.
 * @return true if executable binary or plugin directory exists.
 */
bool is_tool_installed_dynamic(const char *source_dir, const char *tool);

/**
 * @brief List all valid package directory names in source repository.
 *
 * @param source_dir Path to active source repository.
 * @param packages   Output StringArray populated with discovered package names.
 */
void get_all_packages(const char *source_dir, StringArray *packages);

#endif /* SYMDEP_REGISTRY_H */
