/*
 * Symlink & Dependency Manager (symdep)
 * Dynamic UNIX Package Manager Engine & Registry
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

#include "core/pkg_manager.h"
#include "utils/env.h"
#include "utils/fs.h"
#include "utils/logger.h"
#include "utils/path.h"
#include "utils/str.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static const PkgManagerEntry BUILTIN_PKG_MANAGERS[] = {
    {"pacman", "pacman", "sudo pacman -S --needed %s", "sudo pacman -Sy", false},
    {"yay", "yay", "yay -S --needed %s", "yay -Sy", false},
    {"paru", "paru", "paru -S --needed %s", "paru -Sy", false},
    {"apt", "apt", "sudo apt update && sudo apt install -y %s", "sudo apt update", false},
    {"apt-get",
     "apt-get",
     "sudo apt-get update && sudo apt-get install -y %s",
     "sudo apt-get update",
     false},
    {"dnf", "dnf", "sudo dnf install -y %s", "sudo dnf check-update", false},
    {"yum", "yum", "sudo yum install -y %s", "sudo yum check-update", false},
    {"zypper", "zypper", "sudo zypper install -y %s", "sudo zypper refresh", false},
    {"apk", "apk", "sudo apk add %s", "sudo apk update", false},
    {"xbps-install", "xbps-install", "sudo xbps-install -Sy %s", "sudo xbps-install -S", false},
    {"emerge", "emerge", "sudo emerge --ask=n %s", "sudo emerge --sync", false},
    {"eopkg", "eopkg", "sudo eopkg install -y %s", "sudo eopkg ur", false},
    {"slackpkg", "slackpkg", "sudo slackpkg install %s", "sudo slackpkg update", false},
    {"pkgtool", "pkgtool", "sudo installpkg %s", "", false},
    {"brew", "brew", "brew install %s", "brew update", false},
    {"port", "port", "sudo port install %s", "sudo port selfupdate", false},
    {"pkg", "pkg", "pkg install -y %s", "pkg update", false},
    {"pkg_add", "pkg_add", "sudo pkg_add %s", "", false},
    {"pkgin", "pkgin", "sudo pkgin -y install %s", "sudo pkgin update", false},
    {"nix-env", "nix-env", "nix-env -iA %s", "", false},
    {"flatpak", "flatpak", "flatpak install -y %s", "", false},
    {"snap", "snap", "sudo snap install %s", "", false},
};

static const size_t BUILTIN_PKG_MANAGERS_COUNT =
    sizeof(BUILTIN_PKG_MANAGERS) / sizeof(BUILTIN_PKG_MANAGERS[0]);

void pkg_manager_array_init(PkgManagerArray *arr)
{
    if (!arr) {
        return;
    }
    arr->items = NULL;
    arr->count = 0;
    arr->capacity = 0;
}

void pkg_manager_array_append(PkgManagerArray *arr, const PkgManagerEntry *entry)
{
    if (!arr || !entry) {
        return;
    }
    if (arr->count >= arr->capacity) {
        size_t new_cap = (arr->capacity == 0) ? 8 : arr->capacity * 2;
        PkgManagerEntry *new_items = realloc(arr->items, new_cap * sizeof(PkgManagerEntry));
        if (!new_items) {
            log_error("Out of memory in pkg_manager_array_append");
            return;
        }
        arr->items = new_items;
        arr->capacity = new_cap;
    }
    arr->items[arr->count++] = *entry;
}

void pkg_manager_array_free(PkgManagerArray *arr)
{
    if (!arr) {
        return;
    }
    free(arr->items);
    arr->items = NULL;
    arr->count = 0;
    arr->capacity = 0;
}

void pkg_manager_get_builtins(PkgManagerArray *out_arr)
{
    if (!out_arr) {
        return;
    }
    for (size_t i = 0; i < BUILTIN_PKG_MANAGERS_COUNT; i++) {
        pkg_manager_array_append(out_arr, &BUILTIN_PKG_MANAGERS[i]);
    }
}

static void get_user_pkg_config_path(char *buf, size_t buf_size)
{
    char xdg_config[STOW_PATH_MAX];
    if (get_xdg_config_home(xdg_config, sizeof(xdg_config))) {
        snprintf(buf, buf_size, "%s/symdep/pkg_managers.conf", xdg_config);
    } else {
        buf[0] = '\0';
    }
}

void pkg_manager_load_custom_config(PkgManagerArray *out_arr, const char *source_dir)
{
    if (!out_arr) {
        return;
    }

    char conf_paths[2][STOW_PATH_MAX];
    size_t num_paths = 0;

    get_user_pkg_config_path(conf_paths[0], sizeof(conf_paths[0]));
    if (conf_paths[0][0] != '\0') {
        num_paths++;
    }

    if (source_dir && *source_dir != '\0') {
        snprintf(conf_paths[num_paths],
                 sizeof(conf_paths[num_paths]),
                 "%s/.symdep/pkg_managers.conf",
                 source_dir);
        num_paths++;
    }

    for (size_t i = 0; i < num_paths; i++) {
        if (!file_exists(conf_paths[i])) {
            continue;
        }

        FILE *fp = fopen(conf_paths[i], "r");
        if (!fp) {
            continue;
        }

        char *linebuf = NULL;
        size_t linecap = 0;
        ssize_t linelen;

        while ((linelen = getline(&linebuf, &linecap, fp)) != -1) {
            (void)linelen;
            char *trimmed = trim_whitespace(linebuf);
            if (trimmed[0] == '#' || trimmed[0] == '\0') {
                continue;
            }

            // Expected format: name=binary=install_cmd[=update_cmd]
            char *first_eq = strchr(trimmed, '=');
            if (!first_eq) {
                continue;
            }
            *first_eq = '\0';
            char *name = trim_whitespace(trimmed);
            char *rest = first_eq + 1;

            char *second_eq = strchr(rest, '=');
            if (!second_eq) {
                continue;
            }
            *second_eq = '\0';
            char *binary = trim_whitespace(rest);
            char *rest2 = second_eq + 1;

            char *third_eq = strchr(rest2, '=');
            char *install_cmd = NULL;
            char *update_cmd = "";

            if (third_eq) {
                *third_eq = '\0';
                install_cmd = trim_whitespace(rest2);
                update_cmd = trim_whitespace(third_eq + 1);
            } else {
                install_cmd = trim_whitespace(rest2);
            }

            if (*name != '\0' && *binary != '\0' && *install_cmd != '\0') {
                PkgManagerEntry custom_entry;
                memset(&custom_entry, 0, sizeof(custom_entry));
                snprintf(custom_entry.name, sizeof(custom_entry.name), "%s", name);
                snprintf(custom_entry.binary, sizeof(custom_entry.binary), "%s", binary);
                snprintf(
                    custom_entry.install_cmd, sizeof(custom_entry.install_cmd), "%s", install_cmd);
                snprintf(
                    custom_entry.update_cmd, sizeof(custom_entry.update_cmd), "%s", update_cmd);
                custom_entry.is_custom = true;

                pkg_manager_array_append(out_arr, &custom_entry);
            }
        }

        free(linebuf);
        fclose(fp);
    }
}

void pkg_manager_get_all(PkgManagerArray *out_arr, const char *source_dir)
{
    if (!out_arr) {
        return;
    }
    pkg_manager_array_init(out_arr);
    pkg_manager_load_custom_config(out_arr, source_dir);
    pkg_manager_get_builtins(out_arr);
}

bool pkg_manager_find_by_name(const PkgManagerArray *list,
                              const char *name,
                              PkgManagerEntry *out_entry)
{
    if (!list || !name || !out_entry) {
        return false;
    }

    for (size_t i = 0; i < list->count; i++) {
        if (strcasecmp(list->items[i].name, name) == 0 ||
            strcasecmp(list->items[i].binary, name) == 0) {
            *out_entry = list->items[i];
            return true;
        }
    }
    return false;
}

void pkg_manager_detect_on_path(const PkgManagerArray *list, PkgManagerArray *out_detected)
{
    if (!list || !out_detected) {
        return;
    }
    pkg_manager_array_init(out_detected);

    for (size_t i = 0; i < list->count; i++) {
        // Prevent duplicate binary probes
        bool already_detected = false;
        for (size_t j = 0; j < out_detected->count; j++) {
            if (strcmp(out_detected->items[j].binary, list->items[i].binary) == 0) {
                already_detected = true;
                break;
            }
        }
        if (already_detected) {
            continue;
        }

        if (is_executable_in_path(list->items[i].binary)) {
            pkg_manager_array_append(out_detected, &list->items[i]);
        }
    }
}

int pkg_manager_prompt_selection(const PkgManagerArray *detected, PkgManagerEntry *out_entry)
{
    if (!detected || detected->count == 0 || !out_entry) {
        return -1;
    }

    printf("\n%sMultiple package managers detected on your PATH:%s\n", COLOR_BOLD, COLOR_RESET);
    for (size_t i = 0; i < detected->count; i++) {
        printf("  %s[%zu]%s %s%-14s%s (%s)\n",
               COLOR_CYAN,
               i + 1,
               COLOR_RESET,
               COLOR_BOLD,
               detected->items[i].name,
               COLOR_RESET,
               detected->items[i].install_cmd);
    }
    printf("\nSelect package manager [1-%zu]: ", detected->count);
    fflush(stdout);

    char input[64];
    if (!fgets(input, sizeof(input), stdin)) {
        *out_entry = detected->items[0];
        return 0;
    }

    int choice = atoi(input);
    if (choice < 1 || choice > (int)detected->count) {
        log_warn("Invalid selection. Defaulting to '%s'.", detected->items[0].name);
        *out_entry = detected->items[0];
        return 0;
    }

    *out_entry = detected->items[choice - 1];
    return choice - 1;
}

bool pkg_manager_prompt_fallback(PkgManagerEntry *out_entry, const char *source_dir)
{
    (void)source_dir;
    printf("\n%sNo registered package managers were detected on your PATH.%s\n",
           COLOR_BOLD,
           COLOR_RESET);
    printf("  %s[1]%s Register a custom package manager CLI command\n", COLOR_CYAN, COLOR_RESET);
    printf("  %s[2]%s Skip package manager setup (install/compile dependencies manually)\n",
           COLOR_CYAN,
           COLOR_RESET);
    printf("\nSelect option [1-2]: ");
    fflush(stdout);

    char input[64];
    if (!fgets(input, sizeof(input), stdin)) {
        return false;
    }

    int choice = atoi(input);
    if (choice != 1) {
        log_warn("Skipping package manager detection. Dependencies must be installed manually.");
        return false;
    }

    char name_buf[PKG_MGR_NAME_MAX] = {0};
    char cmd_buf[PKG_MGR_CMD_MAX] = {0};

    printf("\nEnter package manager binary/name (e.g. mypm): ");
    fflush(stdout);
    if (!fgets(name_buf, sizeof(name_buf), stdin)) {
        return false;
    }
    char *name = trim_whitespace(name_buf);

    printf("Enter install command template (use %%s for packages, e.g. 'mypm install %%s'): ");
    fflush(stdout);
    if (!fgets(cmd_buf, sizeof(cmd_buf), stdin)) {
        return false;
    }
    char *cmd = trim_whitespace(cmd_buf);

    if (*name == '\0' || *cmd == '\0') {
        log_error("Invalid package manager name or command string provided.");
        return false;
    }

    memset(out_entry, 0, sizeof(*out_entry));
    snprintf(out_entry->name, sizeof(out_entry->name), "%s", name);
    snprintf(out_entry->binary, sizeof(out_entry->binary), "%s", name);
    snprintf(out_entry->install_cmd, sizeof(out_entry->install_cmd), "%s", cmd);
    out_entry->is_custom = true;

    // Save to user config file
    char conf_path[STOW_PATH_MAX];
    get_user_pkg_config_path(conf_path, sizeof(conf_path));
    if (conf_path[0] != '\0') {
        char dir_path[STOW_PATH_MAX];
        snprintf(dir_path, sizeof(dir_path), "%s", conf_path);
        char *last_slash = strrchr(dir_path, '/');
        if (last_slash) {
            *last_slash = '\0';
            mkdir_p(dir_path, 0755);
        }

        FILE *fp = fopen(conf_path, "a");
        if (fp) {
            fprintf(fp, "%s=%s=%s\n", name, name, cmd);
            fclose(fp);
            log_info("Saved custom package manager '%s' to %s", name, conf_path);
        }
    }

    return true;
}

bool pkg_manager_resolve(const char *source_dir,
                         const char *cli_override,
                         PkgManagerEntry *out_entry,
                         bool auto_install,
                         bool dry_run)
{
    (void)dry_run;
    if (!out_entry) {
        return false;
    }
    memset(out_entry, 0, sizeof(*out_entry));

    PkgManagerArray all_managers;
    pkg_manager_get_all(&all_managers, source_dir);

    // Precedence 1: Explicit CLI override or environment variable
    const char *configured = cli_override;
    if (!configured || *configured == '\0') {
        configured = getenv("SYMDEP_PKG_MANAGER");
    }

    if (configured && *configured != '\0') {
        if (pkg_manager_find_by_name(&all_managers, configured, out_entry)) {
            pkg_manager_array_free(&all_managers);
            return true;
        }
        log_warn("Configured package manager '%s' not recognized in registry.", configured);
    }

    // Precedence 2: $PATH binary probing
    PkgManagerArray detected;
    pkg_manager_detect_on_path(&all_managers, &detected);

    bool resolved = false;

    if (detected.count == 1) {
        *out_entry = detected.items[0];
        resolved = true;
    } else if (detected.count > 1) {
        if (auto_install || dry_run || !isatty(STDIN_FILENO)) {
            *out_entry = detected.items[0];
            resolved = true;
        } else {
            int sel = pkg_manager_prompt_selection(&detected, out_entry);
            (void)sel;
            resolved = true;
        }
    } else {
        // Count == 0: No registered package manager on $PATH
        if (auto_install || dry_run || !isatty(STDIN_FILENO)) {
            resolved = false;
        } else {
            resolved = pkg_manager_prompt_fallback(out_entry, source_dir);
        }
    }

    pkg_manager_array_free(&detected);
    pkg_manager_array_free(&all_managers);

    return resolved;
}
