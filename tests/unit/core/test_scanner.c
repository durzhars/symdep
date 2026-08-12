/*
 * Dotfiles Stow Manager (stow-manager)
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
#include "core/manifest.h"
#include "core/scanner.h"

#include <fcntl.h>
#include <unistd.h>

void test_scan_package(void)
{
    char tmp_dotfiles[PATH_MAX];
    ASSERT(create_test_tmp_dir(tmp_dotfiles, sizeof(tmp_dotfiles), "scan_pkg") != NULL,
           "Should create temporary dotfiles directory");

    char pkg_dir[PATH_MAX];
    snprintf(pkg_dir, sizeof(pkg_dir), "%s/scanpkg", tmp_dotfiles);
    ASSERT(mkdir(pkg_dir, 0755) == 0, "Should create scanpkg directory");

    char fzf_pkg_dir[PATH_MAX];
    snprintf(fzf_pkg_dir, sizeof(fzf_pkg_dir), "%s/fzf", tmp_dotfiles);
    ASSERT(mkdir(fzf_pkg_dir, 0755) == 0, "Should create fzf candidate package directory");

    char tmux_pkg_dir[PATH_MAX];
    snprintf(tmux_pkg_dir, sizeof(tmux_pkg_dir), "%s/tmux", tmp_dotfiles);
    ASSERT(mkdir(tmux_pkg_dir, 0755) == 0, "Should create tmux candidate package directory");

    char script_path[PATH_MAX];
    snprintf(script_path, sizeof(script_path), "%s/setup.sh", pkg_dir);
    FILE *fp = fopen(script_path, "w");
    ASSERT(fp != NULL, "Should open script file for writing");
    fprintf(fp, "#!/usr/bin/env bash\n");
    fprintf(fp, "# Test script\n");
    fprintf(fp, "fzf --version\n");
    fprintf(fp, "tmux new-session\n");
    fclose(fp);

    scan_package(tmp_dotfiles, "scanpkg");

    PackageManifest manifest;
    manifest_init(&manifest, "scanpkg");
    ASSERT(manifest_load(&manifest, tmp_dotfiles),
           "Generated manifest should exist and load successfully");
    ASSERT(str_array_contains(&manifest.required, "bash"),
           "Shebang should auto-detect 'bash' as required dependency");
    ASSERT(str_array_contains(&manifest.optional, "fzf"),
           "Tool invocation should auto-detect 'fzf' as optional tool");
    ASSERT(str_array_contains(&manifest.optional, "tmux"),
           "Tool invocation should auto-detect 'tmux' as optional tool");
    manifest_free(&manifest);

    cleanup_test_dir(tmp_dotfiles);
}

void test_scan_package_excludes_non_tool_packages(void)
{
    char tmp_dotfiles[PATH_MAX];
    ASSERT(create_test_tmp_dir(tmp_dotfiles, sizeof(tmp_dotfiles), "scan_non_tool") != NULL,
           "Should create temporary dotfiles directory");

    // Create package directories that are NOT system tools
    char terminal_pkg[PATH_MAX];
    snprintf(terminal_pkg, sizeof(terminal_pkg), "%s/terminal", tmp_dotfiles);
    ASSERT(mkdir(terminal_pkg, 0755) == 0, "Should create terminal package directory");

    char scripts_pkg[PATH_MAX];
    snprintf(scripts_pkg, sizeof(scripts_pkg), "%s/scripts", tmp_dotfiles);
    ASSERT(mkdir(scripts_pkg, 0755) == 0, "Should create scripts package directory");

    char misc_pkg[PATH_MAX];
    snprintf(misc_pkg, sizeof(misc_pkg), "%s/misc", tmp_dotfiles);
    ASSERT(mkdir(misc_pkg, 0755) == 0, "Should create misc package directory");

    char headless_pkg[PATH_MAX];
    snprintf(headless_pkg, sizeof(headless_pkg), "%s/headless", tmp_dotfiles);
    ASSERT(mkdir(headless_pkg, 0755) == 0, "Should create headless package directory");

    // Create target package to scan
    char scan_pkg[PATH_MAX];
    snprintf(scan_pkg, sizeof(scan_pkg), "%s/scanpkg", tmp_dotfiles);
    ASSERT(mkdir(scan_pkg, 0755) == 0, "Should create scanpkg directory");

    char conf_path[PATH_MAX];
    snprintf(conf_path, sizeof(conf_path), "%s/config.conf", scan_pkg);
    FILE *fp = fopen(conf_path, "w");
    ASSERT(fp != NULL, "Should open config file for writing");
    fprintf(fp, "# Configuration mentioning non-tool packages\n");
    fprintf(fp, "script_path = ~/.config/hypr/scripts/foo.sh\n");
    fprintf(fp, "terminal = alacritty\n");
    fprintf(fp, "misc_setting = true\n");
    fprintf(fp, "headless = false\n");
    fclose(fp);

    scan_package(tmp_dotfiles, "scanpkg");

    PackageManifest manifest;
    manifest_init(&manifest, "scanpkg");
    ASSERT(manifest_load(&manifest, tmp_dotfiles),
           "Generated manifest should exist and load successfully");
    ASSERT(!str_array_contains(&manifest.optional, "terminal"),
           "Non-tool package 'terminal' should NOT be detected as optional tool");
    ASSERT(!str_array_contains(&manifest.optional, "scripts"),
           "Non-tool package 'scripts' should NOT be detected as optional tool");
    ASSERT(!str_array_contains(&manifest.optional, "misc"),
           "Non-tool package 'misc' should NOT be detected as optional tool");
    ASSERT(!str_array_contains(&manifest.optional, "headless"),
           "Non-tool package 'headless' should NOT be detected as optional tool");
    manifest_free(&manifest);

    cleanup_test_dir(tmp_dotfiles);
}

void test_scan_package_comment_stripping_and_auto_registry(void)
{
    char tmp_dotfiles[PATH_MAX];
    ASSERT(create_test_tmp_dir(tmp_dotfiles, sizeof(tmp_dotfiles), "scan_comment_reg") != NULL,
           "Should create temporary dotfiles directory");

    char fzf_pkg_dir[PATH_MAX];
    snprintf(fzf_pkg_dir, sizeof(fzf_pkg_dir), "%s/fzf", tmp_dotfiles);
    ASSERT(mkdir(fzf_pkg_dir, 0755) == 0, "Should create fzf candidate package directory");

    char scan_pkg[PATH_MAX];
    snprintf(scan_pkg, sizeof(scan_pkg), "%s/scanpkg", tmp_dotfiles);
    ASSERT(mkdir(scan_pkg, 0755) == 0, "Should create scanpkg directory");

    char conf_path[PATH_MAX];
    snprintf(conf_path, sizeof(conf_path), "%s/config.conf", scan_pkg);
    FILE *fp = fopen(conf_path, "w");
    ASSERT(fp != NULL, "Should open config file for writing");
    fprintf(fp, "# optional: fzf --version\n");
    fprintf(fp, "; comment line with fzf\n");
    fprintf(fp, "// another comment with fzf\n");
    fprintf(fp, "-- lua comment with fzf\n");
    fprintf(fp, "exec = fzf\n");
    fclose(fp);

    scan_package(tmp_dotfiles, "scanpkg");

    PackageManifest manifest;
    manifest_init(&manifest, "scanpkg");
    ASSERT(manifest_load(&manifest, tmp_dotfiles),
           "Generated manifest should exist and load successfully");
    ASSERT(str_array_contains(&manifest.optional, "fzf"),
           "Uncommented 'exec = fzf' line should detect 'fzf' as optional tool");
    manifest_free(&manifest);

    cleanup_test_dir(tmp_dotfiles);
}

void test_scan_package_interactive_mode(void)
{
    char tmp_dotfiles[PATH_MAX];
    ASSERT(create_test_tmp_dir(tmp_dotfiles, sizeof(tmp_dotfiles), "scan_interactive") != NULL,
           "Should create temporary dotfiles directory");

    char fzf_pkg_dir[PATH_MAX];
    snprintf(fzf_pkg_dir, sizeof(fzf_pkg_dir), "%s/fzf", tmp_dotfiles);
    ASSERT(mkdir(fzf_pkg_dir, 0755) == 0, "Should create fzf candidate package directory");

    char scan_pkg[PATH_MAX];
    snprintf(scan_pkg, sizeof(scan_pkg), "%s/interpkg", tmp_dotfiles);
    ASSERT(mkdir(scan_pkg, 0755) == 0, "Should create interpkg directory");

    char conf_path[PATH_MAX];
    snprintf(conf_path, sizeof(conf_path), "%s/config.conf", scan_pkg);
    FILE *fp = fopen(conf_path, "w");
    ASSERT(fp != NULL, "Should open config file for writing");
    fprintf(fp, "exec = fzf\n");
    fclose(fp);

    int saved_stdin = dup(STDIN_FILENO);
    int null_fd = open("/dev/null", O_RDONLY);
    if (null_fd >= 0) {
        dup2(null_fd, STDIN_FILENO);
        close(null_fd);
    }

    scan_package_opts(tmp_dotfiles, "interpkg", true, true, false);

    if (saved_stdin >= 0) {
        dup2(saved_stdin, STDIN_FILENO);
        close(saved_stdin);
    }

    PackageManifest manifest;

    manifest_init(&manifest, "interpkg");
    ASSERT(manifest_load(&manifest, tmp_dotfiles),
           "Generated manifest should exist and load in interactive scan mode");
    ASSERT(str_array_contains(&manifest.optional, "fzf"),
           "Interactive scan should detect 'fzf' as optional tool candidate");
    manifest_free(&manifest);

    cleanup_test_dir(tmp_dotfiles);
}
