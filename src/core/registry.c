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

#include "core/registry.h"
#include "core/file_collector.h"

#include "utils/env.h"
#include "utils/fs.h"
#include "utils/mem.h"
#include "utils/path.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void get_all_packages(const char *source_dir, StringArray *packages)
{
    DIR *dir = opendir(source_dir);
    if (!dir) {
        return;
    }

    StringArray ignore_patterns;
    str_array_init(&ignore_patterns);
    get_default_stowignore(&ignore_patterns);
    parse_stowignore_raw(source_dir, &ignore_patterns);

    struct dirent *entry;
    char path[STOW_PATH_LARGE];
    size_t source_len = strlen(source_dir);

    if (source_len < sizeof(path) - 1) {
        memcpy(path, source_dir, source_len);
        if (source_len > 0 && path[source_len - 1] != '/') {
            path[source_len++] = '/';
        }
    }

    while ((entry = readdir(dir)) != NULL) {
        const char *name = entry->d_name;
        if (name[0] == '.' && (name[1] == '\0' || (name[1] == '.' && name[2] == '\0'))) {
            continue;
        }

        if (!is_path_ignored(name, &ignore_patterns)) {
            // Leverage d_type to avoid unnecessary stat calls when available
            if (entry->d_type == DT_DIR) {
                str_array_append(packages, name);
            } else if (entry->d_type == DT_UNKNOWN || entry->d_type == DT_LNK) {
                size_t name_len = strlen(name);
                if (source_len + name_len < sizeof(path)) {
                    memcpy(path + source_len, name, name_len + 1);
                    if (is_dir(path) && !is_symlink(path)) {
                        str_array_append(packages, name);
                    }
                }
            }
        }
    }

    str_array_free(&ignore_patterns);
    closedir(dir);
}

typedef struct {
    char key[128];
    char value[512];
} CachedRegistryEntry;

typedef struct {
    CachedRegistryEntry *entries;
    size_t count;
    size_t capacity;
    char source_dir[STOW_PATH_LARGE];
    bool loaded;
} RegistryCache;

static RegistryCache g_registry_cache = {0};

static void registry_cache_free(void)
{
    if (g_registry_cache.entries) {
        free(g_registry_cache.entries);
        g_registry_cache.entries = NULL;
    }
    g_registry_cache.count = 0;
    g_registry_cache.capacity = 0;
    g_registry_cache.loaded = false;
    g_registry_cache.source_dir[0] = '\0';
}

static FILE *open_registry_file(const char *source_dir)
{
    if (source_dir && *source_dir != '\0') {
        char path[STOW_PATH_LARGE];
        snprintf(path, sizeof(path), "%s/symdep.registry", source_dir);
        FILE *fp = fopen(path, "r");
        if (fp) {
            return fp;
        }

        snprintf(path, sizeof(path), "%s/.symdepregistry", source_dir);
        fp = fopen(path, "r");
        if (fp) {
            return fp;
        }

        snprintf(path, sizeof(path), "%s/stow.registry", source_dir);
        fp = fopen(path, "r");
        if (fp) {
            return fp;
        }

        snprintf(path, sizeof(path), "%s/.stowregistry", source_dir);
        fp = fopen(path, "r");
        if (fp) {
            return fp;
        }
    }

    FILE *rfp = open_resource_file("symdep.registry");
    if (!rfp) {
        rfp = open_resource_file("symdep.registry.default");
    }
    if (!rfp) {
        rfp = open_resource_file("stow.registry");
    }
    return rfp;
}

static void registry_cache_ensure(const char *source_dir)
{
    const char *src = (source_dir && *source_dir != '\0') ? source_dir : "";
    if (g_registry_cache.loaded && strcmp(g_registry_cache.source_dir, src) == 0) {
        return;
    }

    registry_cache_free();
    snprintf(g_registry_cache.source_dir, sizeof(g_registry_cache.source_dir), "%s", src);

    FILE *fp = open_registry_file(source_dir);
    if (!fp) {
        g_registry_cache.loaded = true;
        return;
    }

    g_registry_cache.capacity = 128;
    g_registry_cache.entries =
        (CachedRegistryEntry *)safe_calloc(g_registry_cache.capacity, sizeof(CachedRegistryEntry));

    char *linebuf = NULL;
    size_t linecap = 0;
    ssize_t linelen;

    while ((linelen = getline(&linebuf, &linecap, fp)) != -1) {
        (void)linelen;
        char *trimmed = trim_whitespace(linebuf);
        if (trimmed[0] == '#' || trimmed[0] == '\0') {
            continue;
        }

        char *eq = strchr(trimmed, '=');
        if (eq) {
            *eq = '\0';
            char *key = trim_whitespace(trimmed);
            char *val = trim_whitespace(eq + 1);

            bool updated = false;
            for (size_t i = 0; i < g_registry_cache.count; i++) {
                if (strcmp(g_registry_cache.entries[i].key, key) == 0) {
                    snprintf(g_registry_cache.entries[i].value,
                             sizeof(g_registry_cache.entries[i].value),
                             "%s",
                             val);
                    updated = true;
                    break;
                }
            }

            if (!updated) {
                if (g_registry_cache.count >= g_registry_cache.capacity) {
                    size_t new_cap = g_registry_cache.capacity * 2;
                    CachedRegistryEntry *new_entries = (CachedRegistryEntry *)realloc(
                        g_registry_cache.entries, new_cap * sizeof(CachedRegistryEntry));
                    if (new_entries) {
                        g_registry_cache.entries = new_entries;
                        g_registry_cache.capacity = new_cap;
                    } else {
                        break;
                    }
                }

                CachedRegistryEntry *entry = &g_registry_cache.entries[g_registry_cache.count++];
                snprintf(entry->key, sizeof(entry->key), "%s", key);
                snprintf(entry->value, sizeof(entry->value), "%s", val);
            }
        }
    }

    free(linebuf);
    fclose(fp);
    g_registry_cache.loaded = true;
}

void registry_get_aliases(const char *source_dir, const char *tool, StringArray *aliases)
{
    registry_cache_ensure(source_dir);

    for (size_t i = 0; i < g_registry_cache.count; i++) {
        if (strcmp(g_registry_cache.entries[i].key, tool) == 0) {
            char val_copy[512];
            snprintf(val_copy, sizeof(val_copy), "%s", g_registry_cache.entries[i].value);
            char *saveptr = NULL;
            char *token = strtok_r(val_copy, "|", &saveptr);
            while (token) {
                char *p = trim_whitespace(token);
                if (strlen(p) > 0 && !str_array_contains(aliases, p)) {
                    str_array_append(aliases, p);
                }
                token = strtok_r(NULL, "|", &saveptr);
            }
            return;
        }
    }

    str_array_append(aliases, tool);
}

static const char *const *get_tag_family_aliases(const char *tag)
{
    if (!tag || *tag == '\0') {
        return NULL;
    }
    static const char *const apt_family[] = {
        "apt", "apt-get", "debian", "ubuntu", "pop", "mint", "kali", "raspbian", NULL};
    static const char *const pacman_family[] = {
        "pacman", "yay", "paru", "arch", "manjaro", "endeavouros", "artix", NULL};
    static const char *const dnf_family[] = {
        "dnf", "yum", "fedora", "rhel", "centos", "rocky", "almalinux", NULL};
    static const char *const apk_family[] = {"apk", "alpine", NULL};
    static const char *const xbps_family[] = {"xbps-install", "xbps", "void", NULL};
    static const char *const emerge_family[] = {"emerge", "gentoo", NULL};
    static const char *const zypper_family[] = {
        "zypper", "suse", "opensuse", "opensuse-tumbleweed", "opensuse-leap", NULL};
    static const char *const brew_family[] = {"brew", "homebrew", "macos", "darwin", NULL};
    static const char *const termux_family[] = {"pkg", "termux", "android", NULL};
    static const char *const bsd_family[] = {
        "pkg_add", "pkgin", "freebsd", "openbsd", "netbsd", "dragonfly", NULL};

    static const struct {
        const char *const *family;
    } groups[] = {{apt_family},
                  {pacman_family},
                  {dnf_family},
                  {apk_family},
                  {xbps_family},
                  {emerge_family},
                  {zypper_family},
                  {brew_family},
                  {termux_family},
                  {bsd_family},
                  {NULL}};

    for (size_t g = 0; groups[g].family != NULL; g++) {
        for (size_t i = 0; groups[g].family[i] != NULL; i++) {
            if (strcasecmp(groups[g].family[i], tag) == 0) {
                return groups[g].family;
            }
        }
    }
    return NULL;
}

void registry_get_distro_pkg(const char *source_dir,
                             const char *tool,
                             const char *distro_id,
                             char *pkg_out,
                             size_t pkg_out_size)
{
    snprintf(pkg_out, pkg_out_size, "%s", tool);
    registry_cache_ensure(source_dir);

    // 1. Primary lookup: Match exact package manager or distro tag (e.g. "apt", "pacman", "ubuntu")
    char key_distro[256];
    snprintf(key_distro, sizeof(key_distro), "%s@%s", tool, distro_id ? distro_id : "");

    for (size_t i = 0; i < g_registry_cache.count; i++) {
        if (strcmp(g_registry_cache.entries[i].key, key_distro) == 0) {
            snprintf(pkg_out, pkg_out_size, "%s", g_registry_cache.entries[i].value);
            return;
        }
    }

    // 2. Family alias lookup: Check allied distro/package manager tags in same ecosystem
    const char *const *family = get_tag_family_aliases(distro_id);
    if (family) {
        for (size_t f = 0; family[f] != NULL; f++) {
            if (distro_id && strcasecmp(family[f], distro_id) == 0) {
                continue;
            }
            char key_fam[256];
            snprintf(key_fam, sizeof(key_fam), "%s@%s", tool, family[f]);
            for (size_t i = 0; i < g_registry_cache.count; i++) {
                if (strcmp(g_registry_cache.entries[i].key, key_fam) == 0) {
                    snprintf(pkg_out, pkg_out_size, "%s", g_registry_cache.entries[i].value);
                    return;
                }
            }
        }
    }

    // 3. System distro ID from environment / os-release probing
    char sys_distro[64];
    get_distro_id(sys_distro, sizeof(sys_distro));
    if (sys_distro[0] != '\0' && (!distro_id || strcmp(sys_distro, distro_id) != 0)) {
        char key_sys[256];
        snprintf(key_sys, sizeof(key_sys), "%s@%s", tool, sys_distro);
        for (size_t i = 0; i < g_registry_cache.count; i++) {
            if (strcmp(g_registry_cache.entries[i].key, key_sys) == 0) {
                snprintf(pkg_out, pkg_out_size, "%s", g_registry_cache.entries[i].value);
                return;
            }
        }

        const char *const *sys_family = get_tag_family_aliases(sys_distro);
        if (sys_family) {
            for (size_t f = 0; sys_family[f] != NULL; f++) {
                char key_sys_fam[256];
                snprintf(key_sys_fam, sizeof(key_sys_fam), "%s@%s", tool, sys_family[f]);
                for (size_t i = 0; i < g_registry_cache.count; i++) {
                    if (strcmp(g_registry_cache.entries[i].key, key_sys_fam) == 0) {
                        snprintf(pkg_out, pkg_out_size, "%s", g_registry_cache.entries[i].value);
                        return;
                    }
                }
            }
        }
    }
}

void registry_get_all_tools(const char *source_dir, StringArray *tools)
{
    registry_cache_ensure(source_dir);

    for (size_t i = 0; i < g_registry_cache.count; i++) {
        char key_copy[128];
        snprintf(key_copy, sizeof(key_copy), "%s", g_registry_cache.entries[i].key);
        char *at = strchr(key_copy, '@');
        if (at) {
            *at = '\0';
        }
        char *tool = trim_whitespace(key_copy);
        if (strlen(tool) > 0 && !str_array_contains(tools, tool)) {
            str_array_append(tools, tool);
        }
    }
}

void registry_add_tool(const char *source_dir, const char *tool)
{
    if (!tool || *tool == '\0') {
        return;
    }

    StringArray existing_tools;
    str_array_init(&existing_tools);
    registry_get_all_tools(source_dir, &existing_tools);

    if (str_array_contains(&existing_tools, tool)) {
        str_array_free(&existing_tools);
        return;
    }
    str_array_free(&existing_tools);

    if (!source_dir || *source_dir == '\0') {
        return;
    }

    char path[STOW_PATH_LARGE];
    snprintf(path, sizeof(path), "%s/symdep.registry", source_dir);

    FILE *fp = fopen(path, "a");
    if (!fp) {
        snprintf(path, sizeof(path), "%s/.symdepregistry", source_dir);
        fp = fopen(path, "a");
    }
    if (!fp) {
        return;
    }

    fprintf(fp, "%s=%s\n", tool, tool);
    fclose(fp);
    registry_cache_free();
}

void registry_add_distro_mapping(const char *source_dir,
                                 const char *tool,
                                 const char *distro,
                                 const char *pkg_name)
{
    if (!source_dir || *source_dir == '\0' || !tool || *tool == '\0' || !pkg_name ||
        *pkg_name == '\0') {
        return;
    }
    const char *distro_id = (distro && *distro != '\0') ? distro : "unix";

    char path[STOW_PATH_LARGE];
    snprintf(path, sizeof(path), "%s/symdep.registry", source_dir);

    FILE *fp = fopen(path, "a");
    if (!fp) {
        snprintf(path, sizeof(path), "%s/.symdepregistry", source_dir);
        fp = fopen(path, "a");
    }
    if (!fp) {
        return;
    }

    fprintf(fp, "%s@%s = %s\n", tool, distro_id, pkg_name);
    fclose(fp);
    registry_cache_free();
}

bool is_tool_installed_dynamic(const char *source_dir, const char *tool)
{
    if (!tool || *tool == '\0') {
        return false;
    }

    /* Fast Path 1: Check $PATH hash cache directly (< 100ns) */
    if (is_executable_in_path(tool)) {
        return true;
    }

    /* Path 2: Consult in-memory cached registry for plugins and binary aliases */
    StringArray aliases;
    str_array_init(&aliases);
    registry_get_aliases(source_dir, tool, &aliases);

    bool installed = false;
    for (size_t i = 0; i < aliases.count; i++) {
        const char *entry = aliases.items[i];
        if (strncmp(entry, "plugin:", 7) == 0) {
            const char *plugin_path = entry + 7;
            char expanded[STOW_PATH_LARGE];
            expand_tilde_path(plugin_path, expanded, sizeof(expanded));
            if (FS_ACCESS(expanded, R_OK) == 0) {
                installed = true;
                break;
            }
        } else {
            if (is_executable_in_path(entry)) {
                installed = true;
                break;
            }
        }
    }

    str_array_free(&aliases);
    return installed;
}
