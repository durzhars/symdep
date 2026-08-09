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

#include "../test_framework.h"
#include "utils/defs.h"
#include "utils/env.h"
#include "utils/path.h"
#include <errno.h>

void test_normalize_path(void)
{
    // NULL & Empty string handling
    normalize_path(NULL);

    char p0[PATH_MAX] = "";
    normalize_path(p0);
    ASSERT_STR_EQ(p0, "");

    // Collapsing duplicate slashes
    char p1[PATH_MAX] = "///home//user///.config";
    normalize_path(p1);
    ASSERT_STR_EQ(p1, "/home/user/.config");

    // Stripping trailing slashes on non-root paths
    char p2[PATH_MAX] = "/var/log/";
    normalize_path(p2);
    ASSERT_STR_EQ(p2, "/var/log");

    // Preserving single root slash
    char p3[PATH_MAX] = "/";
    normalize_path(p3);
    ASSERT_STR_EQ(p3, "/");

    // Collapsing multiple root slashes to a single root slash
    char p4[PATH_MAX] = "///";
    normalize_path(p4);
    ASSERT_STR_EQ(p4, "/");
}

void test_collapse_path(void)
{
    // NULL & Empty string handling
    collapse_path(NULL);

    char p0[PATH_MAX] = "";
    collapse_path(p0);
    ASSERT_STR_EQ(p0, "");

    // Resolving .. relative path segments
    char p1[PATH_MAX] = "/a/b/../c";
    collapse_path(p1);
    ASSERT_STR_EQ(p1, "/a/c");

    // Resolving . relative path segments
    char p2[PATH_MAX] = "/a/b/./c";
    collapse_path(p2);
    ASSERT_STR_EQ(p2, "/a/b/c");

    // Multiple relative segments
    char p3[PATH_MAX] = "/a/b/c/../../d";
    collapse_path(p3);
    ASSERT_STR_EQ(p3, "/a/d");

    // Complex sequence with dot and dot-dot
    char p4[PATH_MAX] = "/a/./b/../c";
    collapse_path(p4);
    ASSERT_STR_EQ(p4, "/a/c");

    // Absolute path cannot collapse past root
    char p5[PATH_MAX] = "/a/../../b";
    collapse_path(p5);
    ASSERT_STR_EQ(p5, "/b");

    // Relative path with leading dot-dot
    char p6[PATH_MAX] = "a/b/../../../c";
    collapse_path(p6);
    ASSERT_STR_EQ(p6, "../c");
}

void test_escape_shell_arg(void)
{
    char dest[256];

    // Simple single-word argument wrapping
    escape_shell_arg("foo", dest, sizeof(dest));
    ASSERT_STR_EQ(dest, "'foo'");

    // Escaping embedded single quotes
    escape_shell_arg("it's", dest, sizeof(dest));
    ASSERT_STR_EQ(dest, "'it'\\''s'");

    // Empty string handling
    escape_shell_arg("", dest, sizeof(dest));
    ASSERT_STR_EQ(dest, "''");

    // Buffer boundary limits (Must fail closed to empty string state on overflow)
    char small_buf[5] = "XXXX";
    escape_shell_arg("hello", small_buf, sizeof(small_buf));
    ASSERT(small_buf[0] == '\0',
           "escape_shell_arg must fail closed to empty string on buffer overflow");
}

void test_expand_tilde_path(void)
{
    char out[PATH_MAX];
    const char *orig_home = getenv("HOME");

    char mock_home[PATH_MAX];
    ASSERT(create_test_tmp_dir(mock_home, sizeof(mock_home), "test_tilde") != NULL,
           "create_test_tmp_dir should create temporary mock HOME directory");

    // 1. With a valid HOME directory
    setenv("HOME", mock_home, 1);

    char expected_config[PATH_MAX];
    snprintf(expected_config, sizeof(expected_config), "%s/config", mock_home);

    expand_tilde_path("~/config", out, sizeof(out));
    ASSERT_STR_EQ(out, expected_config);

    expand_tilde_path("~", out, sizeof(out));
    ASSERT_STR_EQ(out, mock_home);

    cleanup_test_dir(mock_home);

    // 2. Non-tilde paths remaining untouched
    expand_tilde_path("/var/log", out, sizeof(out));
    ASSERT_STR_EQ(out, "/var/log");

    expand_tilde_path("relative/path", out, sizeof(out));
    ASSERT_STR_EQ(out, "relative/path");

    // Restore original HOME
    if (orig_home) {
        setenv("HOME", orig_home, 1);
    } else {
        unsetenv("HOME");
    }
}

void test_is_path_prefix(void)
{
    // True prefix matches
    ASSERT(is_path_prefix("/home/user/dotfiles/bash", "/home/user/dotfiles"),
           "/home/user/dotfiles should be a prefix of /home/user/dotfiles/bash");

    ASSERT(is_path_prefix("/home/user/dotfiles", "/home/user/dotfiles"),
           "Identical path should be a valid prefix of itself");

    // False positive boundaries
    ASSERT(!is_path_prefix("/home/user/dotfiles-other", "/home/user/dotfiles"),
           "/home/user/dotfiles should NOT be a prefix of /home/user/dotfiles-other");

    ASSERT(!is_path_prefix("/etc/passwd", "/home/user"),
           "/home/user should NOT be a prefix of /etc/passwd");
}

void test_join_path(void)
{
    char out[PATH_MAX];

    // Standard successful joining
    int r1 = join_path(out, sizeof(out), "/home/user/dotfiles", "hyprland");
    ASSERT(r1 == 0, "join_path should return 0 on success");
    ASSERT_STR_EQ(out, "/home/user/dotfiles/hyprland");

    int r2 = join_path(out, sizeof(out), "/home/user/dotfiles/", "hyprland");
    ASSERT(r2 == 0, "join_path should return 0 on success");
    ASSERT_STR_EQ(out, "/home/user/dotfiles/hyprland");

    int r3 = join_path(out, sizeof(out), "", "hyprland");
    ASSERT(r3 == 0, "join_path should return 0 when dir is empty");
    ASSERT_STR_EQ(out, "hyprland");

    int r4 = join_path(out, sizeof(out), "/home/user", "");
    ASSERT(r4 == 0, "join_path should return 0 when rel is empty");
    ASSERT_STR_EQ(out, "/home/user");

    // NULL parameter defense
    ASSERT(join_path(NULL, 100, "dir", "rel") == -1, "join_path(NULL, ...) should return -1");
    ASSERT(errno == EINVAL, "join_path(NULL, ...) should set errno to EINVAL");

    ASSERT(join_path(out, 0, "dir", "rel") == -1, "join_path(..., out_size=0) should return -1");
    ASSERT(errno == EINVAL, "join_path(..., out_size=0) should set errno to EINVAL");

    // Defensive Fail-Closed on Buffer Overflow (NO silent truncation allowed!)
    char small_buf[10];
    small_buf[0] = 'X';
    errno = 0;
    int err_res =
        join_path(small_buf, sizeof(small_buf), "/home/user/very_long_directory_name", "subfile");
    ASSERT(err_res == -1, "join_path must fail closed (-1) when buffer is too small");
    ASSERT(errno == ENAMETOOLONG, "join_path must set errno to ENAMETOOLONG on buffer overflow");
    ASSERT(small_buf[0] == '\0',
           "join_path must fail closed by zeroing output buffer state on error");

    // Overlapping buffer safety (out == dir)
    char overlap_buf[PATH_MAX] = "/home/user";
    int r_overlap1 = join_path(overlap_buf, sizeof(overlap_buf), overlap_buf, ".config");
    ASSERT(r_overlap1 == 0, "join_path should handle out == dir overlapping buffer safely");
    ASSERT_STR_EQ(overlap_buf, "/home/user/.config");

    // Overlapping buffer safety (out == rel)
    char overlap_buf2[PATH_MAX] = "nvim";
    int r_overlap2 =
        join_path(overlap_buf2, sizeof(overlap_buf2), "/home/user/.config", overlap_buf2);
    ASSERT(r_overlap2 == 0, "join_path should handle out == rel overlapping buffer safely");
    ASSERT_STR_EQ(overlap_buf2, "/home/user/.config/nvim");
}

void test_path_sanity_strerror(void)
{
    const char *p = "/fake/path";
    char expected_uid_msg[256];

    snprintf(expected_uid_msg,
             sizeof(expected_uid_msg),
             "directory owner UID does not match running UID %u",
             getuid());

    ASSERT_STR_EQ(path_sanity_strerror(PATH_VALID, p), "path '/fake/path' is valid");
    ASSERT_STR_EQ(path_sanity_strerror(ERR_PATH_EMPTY, NULL), "path string is empty or NULL");
    ASSERT_STR_EQ(path_sanity_strerror(ERR_NOT_ABSOLUTE, "dotfiles"),
                  "path 'dotfiles' is not absolute (must start with '/')");
    ASSERT_STR_EQ(path_sanity_strerror(ERR_NOT_A_DIRECTORY, "/dev/null"),
                  "'/dev/null' is not a directory");
    ASSERT_STR_EQ(path_sanity_strerror(ERR_NOT_OWNED_BY_USER, p), expected_uid_msg);
    ASSERT_STR_EQ(path_sanity_strerror(ERR_WORLD_WRITABLE, p),
                  "'/fake/path' is world-writable (security violation)");
    ASSERT_STR_EQ(path_sanity_strerror(ERR_INSUFFICIENT_PERMS, p),
                  "insufficient permissions for '/fake/path' (rwx access required)");
    ASSERT_STR_EQ(path_sanity_strerror((PathSanityResult)999, p),
                  "unknown path sanity error for '/fake/path'");
}

void test_path_parsing_traversal_and_naming_edge_cases(void)
{
    char p[PATH_MAX];

    // --- Category 1: Traversal Vectors & Relative Path Parsing ---
    snprintf(p, sizeof(p), "/a/b/..");
    collapse_path(p);
    ASSERT_STR_EQ(p, "/a");

    snprintf(p, sizeof(p), "a/b/..");
    collapse_path(p);
    ASSERT_STR_EQ(p, "a");

    snprintf(p, sizeof(p), "a/b/c/../..");
    collapse_path(p);
    ASSERT_STR_EQ(p, "a");

    snprintf(p, sizeof(p), "a/..");
    collapse_path(p);
    ASSERT_STR_EQ(p, ".");

    snprintf(p, sizeof(p), "/a/..");
    collapse_path(p);
    ASSERT_STR_EQ(p, "/");

    snprintf(p, sizeof(p), "a/b/../../c");
    collapse_path(p);
    ASSERT_STR_EQ(p, "c");

    snprintf(p, sizeof(p), "a/b/../../../c");
    collapse_path(p);
    ASSERT_STR_EQ(p, "../c");

    snprintf(p, sizeof(p), "a/b/../../../../c");
    collapse_path(p);
    ASSERT_STR_EQ(p, "../../c");

    snprintf(p, sizeof(p), "./a/./b/.././c");
    collapse_path(p);
    ASSERT_STR_EQ(p, "a/c");

    // Traversal vectors on absolute paths (cannot escape root /)
    snprintf(p, sizeof(p), "/a/b/../../..");
    collapse_path(p);
    ASSERT_STR_EQ(p, "/");

    snprintf(p, sizeof(p), "/a/../../../../etc/passwd");
    collapse_path(p);
    ASSERT_STR_EQ(p, "/etc/passwd");

    snprintf(p, sizeof(p), "/./a/../b/../c");
    collapse_path(p);
    ASSERT_STR_EQ(p, "/c");

    // --- Category 2: Multi-Dot Directory Naming & Directory Edge Cases ---
    snprintf(p, sizeof(p), "a/.../b");
    collapse_path(p);
    ASSERT_STR_EQ(p, "a/.../b");

    snprintf(p, sizeof(p), "/a/..../b/../c");
    collapse_path(p);
    ASSERT_STR_EQ(p, "/a/..../c");

    // Directory names with spaces and special characters
    snprintf(p, sizeof(p), "/home/user/My Documents/./.config/../.local/share");
    collapse_path(p);
    ASSERT_STR_EQ(p, "/home/user/My Documents/.local/share");

    // Directory names starting with dot
    snprintf(p, sizeof(p), ".config/.stowignore");
    collapse_path(p);
    ASSERT_STR_EQ(p, ".config/.stowignore");

    // --- Category 3: Absolute Path Checking & Slash Normalization ---
    snprintf(p, sizeof(p), "////usr///local////bin///");
    normalize_path(p);
    ASSERT_STR_EQ(p, "/usr/local/bin");

    snprintf(p, sizeof(p), "//");
    normalize_path(p);
    ASSERT_STR_EQ(p, "/");

    // --- Category 4: Path Prefix Match Edge Cases ---
    ASSERT(!is_path_prefix("/home/user/dotfiles_backup", "/home/user/dotfiles"),
           "Prefix must match directory component boundary, not partial string prefix");
    ASSERT(!is_path_prefix("/home/user/dotfiles-2/file", "/home/user/dotfiles"),
           "Prefix check must not match 'dotfiles-2' for prefix 'dotfiles'");
    ASSERT(is_path_prefix("/home/user/dotfiles/pkg/file", "/home/user/dotfiles"),
           "Prefix check should match valid subdirectory path");
    ASSERT(is_path_prefix("/home/user/dotfiles", "/home/user/dotfiles"),
           "Prefix check should match exact path");
    ASSERT(is_path_prefix("/anything", "/"), "Root '/' prefix should match any absolute path");

    // --- Category 5: join_path & Path Assembly Edge Cases ---
    char joined[PATH_MAX];
    join_path(joined, sizeof(joined), "/home/user", "../.config");
    ASSERT_STR_EQ(joined, "/home/user/../.config");

    collapse_path(joined);
    ASSERT_STR_EQ(joined, "/home/.config");

    join_path(joined, sizeof(joined), "dotfiles", "./hypr/./hyprland.conf");
    ASSERT_STR_EQ(joined, "dotfiles/./hypr/./hyprland.conf");
    collapse_path(joined);
    ASSERT_STR_EQ(joined, "dotfiles/hypr/hyprland.conf");

    // --- Category 6: Fail-Closed Buffer Bounds & Path Depth Limits ---
    char deep[PATH_MAX * 2];
    size_t pos = 0;
    for (int i = 0; i < 260; i++) {
        int w = snprintf(deep + pos, sizeof(deep) - pos, "/dir%d", i);
        if (w > 0)
            pos += (size_t)w;
    }
    int res = collapse_path(deep);
    ASSERT(res == -1,
           "collapse_path should return -1 when path component depth exceeds MAX_PATH_DEPTH");
    ASSERT(errno == ENAMETOOLONG, "collapse_path must set errno to ENAMETOOLONG");
    ASSERT(deep[0] == '\0', "collapse_path must fail closed by zeroing output string state");

    char tiny[4] = "abc";
    errno = 0;
    int res_tiny = join_path(tiny, sizeof(tiny), "/usr/local", "bin");
    ASSERT(res_tiny == -1, "join_path must fail closed (-1) when buffer is too small");
    ASSERT(errno == ENAMETOOLONG, "join_path must set errno to ENAMETOOLONG");
    ASSERT(tiny[0] == '\0', "join_path must fail closed by setting output[0] to '\\0'");

    char single[1] = {'x'};
    errno = 0;
    int res_single = join_path(single, sizeof(single), "/path", "file");
    ASSERT(res_single == -1, "join_path with 1-byte buffer must return -1 on overflow");
    ASSERT(errno == ENAMETOOLONG, "join_path must set errno to ENAMETOOLONG");
    ASSERT(single[0] == '\0', "1-byte buffer must fail closed to empty string");
}
