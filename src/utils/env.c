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

#include <ctype.h>
#include <dirent.h>
#include <limits.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <sys/wait.h>
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

static bool parse_os_release_id(const char *path, char *buf, size_t buf_size)
{
    if (!path || *path == '\0' || !buf || buf_size == 0) {
        return false;
    }
    FILE *fp = fopen(path, "r");
    if (!fp) {
        return false;
    }

    char line[256];
    bool found = false;
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
            if (*trimmed != '\0') {
                snprintf(buf, buf_size, "%s", trimmed);
                found = true;
                break;
            }
        }
    }
    fclose(fp);
    return found;
}

void get_distro_id(char *buf, size_t buf_size)
{
    if (!buf || buf_size == 0) {
        return;
    }
    buf[0] = '\0';

    // 1. Explicit environment variable override
    const char *env_distro = getenv("SYMDEP_DISTRO");
    if (!env_distro || *env_distro == '\0') {
        env_distro = getenv("DISTRO_ID");
    }
    if (env_distro && *env_distro != '\0') {
        snprintf(buf, buf_size, "%s", env_distro);
        return;
    }

    // 2. Prefix / Termux / Android environment
    const char *prefix = getenv("PREFIX");
    if (prefix && *prefix != '\0') {
        char prefix_os_release[STOW_PATH_MAX];
        join_path(prefix_os_release, sizeof(prefix_os_release), prefix, "etc/os-release");
        if (parse_os_release_id(prefix_os_release, buf, buf_size)) {
            return;
        }
        if (strstr(prefix, "com.termux")) {
            snprintf(buf, buf_size, "termux");
            return;
        }
    }

    if (getenv("ANDROID_ROOT") || getenv("ANDROID_DATA") ||
        (access("/system/bin/sh", F_OK) == 0 && access("/system/etc/seccomp_policy", F_OK) == 0)) {
        snprintf(buf, buf_size, "android");
        return;
    }

    // 3. Freedesktop / Systemd os-release standard paths
    static const char *const os_release_paths[] = {"/etc/os-release",
                                                   "/usr/lib/os-release",
                                                   "/usr/share/os-release",
                                                   "/etc/initrd-release",
                                                   NULL};

    for (size_t i = 0; os_release_paths[i] != NULL; i++) {
        if (parse_os_release_id(os_release_paths[i], buf, buf_size)) {
            return;
        }
    }

    // 4. Legacy Linux Distribution release files
    static const struct {
        const char *file;
        const char *distro_id;
    } legacy_distros[] = {{"/etc/arch-release", "arch"},
                          {"/etc/debian_version", "debian"},
                          {"/etc/fedora-release", "fedora"},
                          {"/etc/redhat-release", "rhel"},
                          {"/etc/centos-release", "centos"},
                          {"/etc/alpine-release", "alpine"},
                          {"/etc/void-release", "void"},
                          {"/etc/gentoo-release", "gentoo"},
                          {"/etc/SuSE-release", "suse"},
                          {"/etc/slackware-version", "slackware"},
                          {NULL, NULL}};

    for (size_t i = 0; legacy_distros[i].file != NULL; i++) {
        if (access(legacy_distros[i].file, F_OK) == 0) {
            snprintf(buf, buf_size, "%s", legacy_distros[i].distro_id);
            return;
        }
    }

    // 5. Native OS Preprocessor Macros & POSIX uname() fallback
#if defined(__APPLE__)
    snprintf(buf, buf_size, "macos");
    return;
#elif defined(__FreeBSD__)
    snprintf(buf, buf_size, "freebsd");
    return;
#elif defined(__OpenBSD__)
    snprintf(buf, buf_size, "openbsd");
    return;
#elif defined(__NetBSD__)
    snprintf(buf, buf_size, "netbsd");
    return;
#elif defined(__DragonFly__)
    snprintf(buf, buf_size, "dragonfly");
    return;
#endif

    struct utsname u;
    if (uname(&u) == 0 && u.sysname[0] != '\0') {
        char sys_lower[64];
        snprintf(sys_lower, sizeof(sys_lower), "%s", u.sysname);
        for (char *p = sys_lower; *p != '\0'; p++) {
            *p = (char)tolower((unsigned char)*p);
        }
        snprintf(buf, buf_size, "%s", sys_lower);
        return;
    }

    // 6. Catch-all fallback
    snprintf(buf, buf_size, "unix");
}

static StringArray g_path_dirs;
static bool g_path_dirs_initialized = false;

static void init_path_dirs(void)
{
    if (g_path_dirs_initialized) {
        return;
    }
    str_array_init(&g_path_dirs);

    const char *path_env = getenv("PATH");
    if (!path_env || *path_env == '\0') {
        g_path_dirs_initialized = true;
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
                str_array_append(&g_path_dirs, dir_path);
            }
        }
        if (!next) {
            break;
        }
        p = next + 1;
    }
    g_path_dirs_initialized = true;
}

bool find_executable_in_path(const char *executable, char *out_path, size_t out_path_size)
{
    if (!executable || *executable == '\0' || !out_path || out_path_size == 0) {
        return false;
    }
    out_path[0] = '\0';

    if (strchr(executable, '/') != NULL) {
        if (FS_ACCESS(executable, X_OK) == 0) {
            snprintf(out_path, out_path_size, "%s", executable);
            return true;
        }
        return false;
    }

    init_path_dirs();

    char candidate[STOW_PATH_LARGE];
    for (size_t i = 0; i < g_path_dirs.count; i++) {
        int len = snprintf(candidate, sizeof(candidate), "%s/%s", g_path_dirs.items[i], executable);
        if (len > 0 && (size_t)len < sizeof(candidate)) {
            if (FS_ACCESS(candidate, X_OK) == 0) {
                snprintf(out_path, out_path_size, "%s", candidate);
                return true;
            }
        }
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

int run_system_cmd_with_capture(const char *cmd, char *out_buf, size_t out_size)
{
    if (out_buf && out_size > 0) {
        out_buf[0] = '\0';
    }
    if (!cmd || *cmd == '\0') {
        return -1;
    }

    const char *tmp = getenv("TMPDIR");
    char termux_tmp[STOW_PATH_LARGE];
    if (!tmp || *tmp == '\0') {
        tmp = getenv("PREFIX");
        if (tmp) {
            join_path(termux_tmp, sizeof(termux_tmp), tmp, "tmp");
            tmp = termux_tmp;
        }
    }
    if (!tmp || access(tmp, W_OK) != 0) {
        tmp = "/tmp";
    }

    char tmp_log[STOW_PATH_MAX];
    char tmp_stat[STOW_PATH_MAX];
    snprintf(tmp_log, sizeof(tmp_log), "%s/symdep_cmd_err_%d.log", tmp, (int)getpid());
    snprintf(tmp_stat, sizeof(tmp_stat), "%s/symdep_cmd_stat_%d.log", tmp, (int)getpid());

    char full_cmd[4096];
    if (isatty(STDIN_FILENO)) {
        snprintf(full_cmd,
                 sizeof(full_cmd),
                 "{ ( %s ) ; echo $? > \"%s\"; } 2>&1 | tee \"%s\"",
                 cmd,
                 tmp_stat,
                 tmp_log);
    } else {
        snprintf(full_cmd,
                 sizeof(full_cmd),
                 "{ ( %s ) ; echo $? > \"%s\"; } > \"%s\" 2>&1",
                 cmd,
                 tmp_stat,
                 tmp_log);
    }

    system(full_cmd);

    int status = 0;
    FILE *fstat = fopen(tmp_stat, "r");
    if (fstat) {
        if (fscanf(fstat, "%d", &status) != 1) {
            status = -1;
        }
        fclose(fstat);
    }
    unlink(tmp_stat);

    if (out_buf && out_size > 0) {
        FILE *fp = fopen(tmp_log, "r");
        if (fp) {
            size_t read_bytes = fread(out_buf, 1, out_size - 1, fp);
            out_buf[read_bytes] = '\0';
            fclose(fp);
        }
    }
    unlink(tmp_log);
    return status;
}
