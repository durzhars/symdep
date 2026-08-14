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

#include "utils/env.h"
#include "utils/fs.h"
#include "utils/path.h"
#include "utils/str.h"

#include <dirent.h>
#include <limits.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

typedef struct {
    const char *env_var;
    const char *default_rel;
} XdgMapping;

static const XdgMapping XDG_TABLE[] = {
    [XDG_CONFIG] = {"XDG_CONFIG_HOME", ".config"},
    [XDG_DATA] = {"XDG_DATA_HOME", ".local/share"},
    [XDG_CACHE] = {"XDG_CACHE_HOME", ".cache"},
    [XDG_STATE] = {"XDG_STATE_HOME", ".local/state"},
};

static const char *getenv_adapter(const char *var_name, void *ctx)
{
    (void)ctx;
    return getenv(var_name);
}

void expand_env_vars(const char *src, char *out, size_t out_size)
{
    str_expand_vars(src, out, out_size, getenv_adapter, NULL);
}

static bool
resolve_xdg_path(const char *env_var, const char *default_rel, char *buf, size_t buf_size)
{
    if (!buf || buf_size == 0) {
        return false;
    }

    const char *env = getenv(env_var);
    if (env && strlen(env) > 0) {
        char expanded[STOW_PATH_LARGE];
        expand_env_vars(env, expanded, sizeof(expanded));
        if (expanded[0] == '/') {
            snprintf(buf, buf_size, "%s", expanded);
            return true;
        }
    }

    char home[STOW_PATH_MAX];
    if (get_user_home_dir(home, sizeof(home))) {
        join_path(buf, buf_size, home, default_rel);
        return true;
    }

    return false;
}

static bool get_home_from_passwd_file(uid_t target_uid, char *buf, size_t buf_size)
{
    FILE *fp = fopen("/etc/passwd", "r");
    if (!fp) {
        return false;
    }

    char line[1024];
    bool found = false;

    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\r\n")] = '\0';
        if (line[0] == '#' || line[0] == '\0') {
            continue;
        }

        char *line_ptr = line;
        char *username = strsep(&line_ptr, ":");
        char *password = strsep(&line_ptr, ":");
        char *uid_str = strsep(&line_ptr, ":");
        char *gid_str = strsep(&line_ptr, ":");
        char *gecos = strsep(&line_ptr, ":");
        char *homedir = strsep(&line_ptr, ":");

        (void)username;
        (void)password;
        (void)gid_str;
        (void)gecos;

        if (!uid_str || !homedir) {
            continue;
        }

        uid_t uid = (uid_t)strtoul(uid_str, NULL, 10);
        if (uid == target_uid && verify_path_sanity(homedir) == PATH_VALID) {
            snprintf(buf, buf_size, "%s", homedir);
            found = true;
            break;
        }
    }

    fclose(fp);
    return found;
}

bool get_user_home_dir(char *buf, size_t buf_size)
{
    if (!buf || buf_size == 0) {
        return false;
    }

    const char *home = getenv("HOME");
    if (home && *home != '\0') {
        if (verify_path_sanity(home) == PATH_VALID) {
            snprintf(buf, buf_size, "%s", home);
            return true;
        }
    }

#if defined(NO_NSS_FALLBACK)
    if (get_home_from_passwd_file(getuid(), buf, buf_size)) {
        return true;
    }
#else
    struct passwd pwd;
    struct passwd *result = NULL;
    char pwbuf[1024];

    if (getpwuid_r(getuid(), &pwd, pwbuf, sizeof(pwbuf), &result) == 0 && result != NULL) {
        if (pwd.pw_dir && verify_path_sanity(pwd.pw_dir) == PATH_VALID) {
            snprintf(buf, buf_size, "%s", pwd.pw_dir);
            return true;
        }
    }

    if (get_home_from_passwd_file(getuid(), buf, buf_size)) {
        return true;
    }
#endif

    buf[0] = '\0';
    return false;
}

bool get_xdg_dir(XdgDirType type, char *buf, size_t buf_size)
{
    if (type < XDG_CONFIG || type > XDG_STATE) {
        return false;
    }

    const XdgMapping *spec = &XDG_TABLE[type];
    return resolve_xdg_path(spec->env_var, spec->default_rel, buf, buf_size);
}

bool get_xdg_config_home(char *buf, size_t buf_size)
{
    return get_xdg_dir(XDG_CONFIG, buf, buf_size);
}

bool get_xdg_data_home(char *buf, size_t buf_size)
{
    return get_xdg_dir(XDG_DATA, buf, buf_size);
}

bool get_xdg_cache_home(char *buf, size_t buf_size)
{
    return get_xdg_dir(XDG_CACHE, buf, buf_size);
}

bool get_xdg_state_home(char *buf, size_t buf_size)
{
    return get_xdg_dir(XDG_STATE, buf, buf_size);
}

static void
get_xdg_colon_separated_dirs(const char *env_var, const char *default_val, StringArray *dirs)
{
    const char *env = getenv(env_var);
    if (!env || *env == '\0') {
        env = default_val;
    }

    StringArray raw_tokens;
    str_array_init(&raw_tokens);
    str_split_delim(env, ":", &raw_tokens);

    for (size_t i = 0; i < raw_tokens.count; i++) {
        char expanded[STOW_PATH_LARGE];
        expand_env_vars(raw_tokens.items[i], expanded, sizeof(expanded));
        str_array_append(dirs, expanded);
    }

    str_array_free(&raw_tokens);
}

void get_xdg_data_dirs(StringArray *dirs)
{
    get_xdg_colon_separated_dirs("XDG_DATA_DIRS", "/usr/local/share:/usr/share", dirs);
}

void get_xdg_config_dirs(StringArray *dirs)
{
    get_xdg_colon_separated_dirs("XDG_CONFIG_DIRS", "/etc/xdg", dirs);
}

void app_env_init(AppEnvironment *env)
{
    if (!env) {
        return;
    }
    memset(env, 0, sizeof(*env));
}

bool app_env_resolve(AppEnvironment *env,
                     const char *cli_target_override,
                     PathSanityResult *out_reason)
{
    if (!env) {
        return false;
    }
    app_env_init(env);

    if (out_reason) {
        *out_reason = PATH_VALID;
    }

    if (cli_target_override && strlen(cli_target_override) > 0) {
        expand_tilde_path(cli_target_override, env->target_dir, sizeof(env->target_dir));
        normalize_path(env->target_dir);
        env->is_target_override = true;
    }

    env->is_home_validated = get_user_home_dir(env->home_dir, sizeof(env->home_dir));

    if (!env->is_home_validated) {
        if (!env->is_target_override) {
            const char *raw_home = getenv("HOME");
            PathSanityResult reason = verify_path_sanity(raw_home);
            if (reason != PATH_VALID) {
                if (out_reason) {
                    *out_reason = reason;
                }
                //                log_error("Fatal: Invalid or missing $HOME directory (%s).
                //                Exiting.",
                //                         path_sanity_strerror(reason, raw_home));
                return false;
            }
            snprintf(env->target_dir, sizeof(env->target_dir), "%s", raw_home ? raw_home : "");
        }
    } else if (!env->is_target_override) {
        snprintf(env->target_dir, sizeof(env->target_dir), "%s", env->home_dir);
    }

    get_xdg_config_home(env->xdg_config_home, sizeof(env->xdg_config_home));
    get_xdg_data_home(env->xdg_data_home, sizeof(env->xdg_data_home));
    get_xdg_cache_home(env->xdg_cache_home, sizeof(env->xdg_cache_home));
    get_xdg_state_home(env->xdg_state_home, sizeof(env->xdg_state_home));

    return true;
}

void get_distro_id(char *buf, size_t buf_size)
{
    if (!buf || buf_size == 0) {
        return;
    }
    buf[0] = '\0';

    FILE *fp = fopen("/etc/os-release", "r");
    if (!fp) {
        fp = fopen("/usr/lib/os-release", "r");
    }
    if (fp) {
        char line[256];
        while (fgets(line, sizeof(line), fp)) {
            if (strncmp(line, "ID=", 3) == 0) {
                char *val = line + 3;
                if (*val == '"' || *val == '\'') {
                    val++;
                }
                char *trimmed = trim_whitespace(val);
                size_t len = strlen(trimmed);
                if (len > 0 && (trimmed[len - 1] == '"' || trimmed[len - 1] == '\'')) {
                    trimmed[len - 1] = '\0';
                }
                snprintf(buf, buf_size, "%s", trimmed);
                fclose(fp);
                return;
            }
        }
        fclose(fp);
    }

    snprintf(buf, buf_size, "unix");
}

static StrSet g_path_executables_cache;
static bool g_path_cache_initialized = false;

static void init_path_executables_cache(void)
{
    if (g_path_cache_initialized) {
        return;
    }
    str_set_init(&g_path_executables_cache);

    const char *path_env = getenv("PATH");
    if (!path_env || *path_env == '\0') {
        g_path_cache_initialized = true;
        return;
    }

    const char *p = path_env;
    while (*p != '\0') {
        const char *next = strchr(p, ':');
        size_t dir_len = next ? (size_t)(next - p) : strlen(p);
        if (dir_len > 0) {
            char dir_path[STOW_PATH_LARGE];
            if (dir_len < sizeof(dir_path)) {
                memcpy(dir_path, p, dir_len);
                dir_path[dir_len] = '\0';
                DIR *d = opendir(dir_path);
                if (d) {
                    struct dirent *entry;
                    while ((entry = readdir(d)) != NULL) {
                        if (entry->d_name[0] != '.') {
                            str_set_add(&g_path_executables_cache, entry->d_name);
                        }
                    }
                    closedir(d);
                }
            }
        }
        if (!next) {
            break;
        }
        p = next + 1;
    }
    g_path_cache_initialized = true;
}

bool find_executable_in_path(const char *executable, char *out_path, size_t out_path_size)
{
    if (!executable || *executable == '\0' || !out_path || out_path_size == 0) {
        return false;
    }
    out_path[0] = '\0';

    if (strchr(executable, '/') != NULL) {
        if (access(executable, X_OK) == 0) {
            snprintf(out_path, out_path_size, "%s", executable);
            return true;
        }
        return false;
    }

    init_path_executables_cache();

    if (str_set_contains(&g_path_executables_cache, executable)) {
        snprintf(out_path, out_path_size, "%s", executable);
        return true;
    }

    return false;
}

bool is_executable_in_path(const char *executable)
{
    char dummy[STOW_PATH_LARGE];
    return find_executable_in_path(executable, dummy, sizeof(dummy));
}

int run_system_cmd(const char *cmd)
{
    return system(cmd);
}
