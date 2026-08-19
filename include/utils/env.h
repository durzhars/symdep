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

#ifndef SYMDEP_UTILS_ENV_H
#define SYMDEP_UTILS_ENV_H

#include "utils/defs.h"
#include "utils/fs.h"
#include "utils/str.h"

/**
 * @enum XdgDirType
 * @brief XDG Base Directory specification category types.
 */
typedef enum {
    XDG_CONFIG, /**< User-specific configuration files ($XDG_CONFIG_HOME or ~/.config) */
    XDG_DATA,   /**< User-specific data files ($XDG_DATA_HOME or ~/.local/share) */
    XDG_CACHE,  /**< User-specific non-essential data ($XDG_CACHE_HOME or ~/.cache) */
    XDG_STATE   /**< User-specific state data ($XDG_STATE_HOME or ~/.local/state) */
} XdgDirType;

/**
 * @struct AppEnvironment
 * @brief Represents the resolved runtime environment and directory paths for symdep.
 */
typedef struct {
    char home_dir[STOW_PATH_MAX];        /**< Resolved user home directory */
    char target_dir[STOW_PATH_MAX];      /**< Resolved target destination directory */
    char xdg_config_home[STOW_PATH_MAX]; /**< Active XDG config directory */
    char xdg_data_home[STOW_PATH_MAX];   /**< Active XDG data directory */
    char xdg_cache_home[STOW_PATH_MAX];  /**< Active XDG cache directory */
    char xdg_state_home[STOW_PATH_MAX];  /**< Active XDG state directory */
    bool is_home_validated;              /**< Whether home directory passed sanity checks */
    bool is_target_override;             /**< Whether target directory was overridden via CLI */
} AppEnvironment;

/**
 * @brief Expand shell environment variables (e.g. $VAR or ${VAR}) within a string.
 *
 * @param src      Input source string.
 * @param out      Output destination buffer.
 * @param out_size Size of destination buffer.
 */
void expand_env_vars(const char *src, char *out, size_t out_size);

/**
 * @brief Resolve user home directory with NSS-independent /etc/passwd fallback.
 *
 * @param buf      Output buffer for home directory path.
 * @param buf_size Size of destination buffer.
 * @return true on success, false if home directory cannot be resolved.
 */
bool get_user_home_dir(char *buf, size_t buf_size);

/**
 * @brief Retrieve path for a specific XDG Base Directory type.
 *
 * @param type     XDG directory category.
 * @param buf      Output buffer for directory path.
 * @param buf_size Size of destination buffer.
 * @return true on success, false on failure.
 */
bool get_xdg_dir(XdgDirType type, char *buf, size_t buf_size);

/** Get XDG_CONFIG_HOME path (~/.config) */
bool get_xdg_config_home(char *buf, size_t buf_size);

/** Get XDG_DATA_HOME path (~/.local/share) */
bool get_xdg_data_home(char *buf, size_t buf_size);

/** Get XDG_CACHE_HOME path (~/.cache) */
bool get_xdg_cache_home(char *buf, size_t buf_size);

/** Get XDG_STATE_HOME path (~/.local/state) */
bool get_xdg_state_home(char *buf, size_t buf_size);

/**
 * @brief Query colon-separated system XDG data search directories ($XDG_DATA_DIRS).
 *
 * @param dirs Output StringArray populated with directory paths.
 */
void get_xdg_data_dirs(StringArray *dirs);

/**
 * @brief Query colon-separated system XDG config search directories ($XDG_CONFIG_DIRS).
 *
 * @param dirs Output StringArray populated with directory paths.
 */
void get_xdg_config_dirs(StringArray *dirs);

/**
 * @brief Initialize an AppEnvironment struct with default/empty values.
 *
 * @param env Pointer to uninitialized AppEnvironment struct.
 */
void app_env_init(AppEnvironment *env);

/**
 * @brief Resolve and validate runtime environment paths against filesystem sanity checks.
 *
 * @param env                 Pointer to initialized AppEnvironment struct.
 * @param cli_target_override Optional CLI target override path (or NULL).
 * @param out_reason          Output pointer for sanity verification error code.
 * @return true on success, false if path verification fails.
 */
bool app_env_resolve(AppEnvironment *env,
                     const char *cli_target_override,
                     PathSanityResult *out_reason);

/**
 * @brief Identify the host Linux distribution ID (e.g. "arch", "ubuntu", "fedora", "alpine").
 *
 * @param buf      Output buffer for distribution identifier.
 * @param buf_size Size of destination buffer.
 */
void get_distro_id(char *buf, size_t buf_size);

/**
 * @brief Search active system $PATH for an executable binary with in-memory memoization.
 *
 * @param executable   Binary name to locate.
 * @param out_path     Output buffer for absolute executable path.
 * @param out_path_size Size of destination buffer.
 * @return true if binary was located and executable, false otherwise.
 */
bool find_executable_in_path(const char *executable, char *out_path, size_t out_path_size);

/**
 * @brief Execute a shell command and capture standard error/output.
 *
 * @param cmd      Shell command string.
 * @param out_buf  Buffer to hold captured output.
 * @param out_size Size of destination buffer.
 * @return Command exit status code.
 */
int run_system_cmd_with_capture(const char *cmd, char *out_buf, size_t out_size);

#endif /* SYMDEP_UTILS_ENV_H */
