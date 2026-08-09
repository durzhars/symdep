/*
 * Dotfiles Stow Manager (stow-manager)
 * Filesystem Validation & Inspection Submodule
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

#include "utils/fs/internal.h"

bool file_exists(const char *path)
{
    struct stat st;
    return (lstat(path, &st) == 0);
}

bool is_dir(const char *path)
{
    struct stat st;
    if (stat(path, &st) == 0) {
        return S_ISDIR(st.st_mode);
    }
    return false;
}

bool is_symlink(const char *path)
{
    struct stat st;
    if (lstat(path, &st) == 0) {
        return S_ISLNK(st.st_mode);
    }
    return false;
}

PathSanityResult verify_path_sanity(const char *path)
{
    if (!path || *path == '\0') {
        return ERR_PATH_EMPTY;
    }

    if (path[0] != '/') {
        return ERR_NOT_ABSOLUTE;
    }

    struct stat st;
    if (stat(path, &st) != 0) {
        return ERR_INSUFFICIENT_PERMS;
    }

    if (!S_ISDIR(st.st_mode)) {
        return ERR_NOT_A_DIRECTORY;
    }

    if (getuid() != 0 && st.st_uid != getuid()) {
        return ERR_NOT_OWNED_BY_USER;
    }

    if ((st.st_mode & S_IWOTH) && !(st.st_mode & S_ISVTX)) {
        return ERR_WORLD_WRITABLE;
    }

    if (access(path, R_OK | W_OK | X_OK) != 0) {
        return ERR_INSUFFICIENT_PERMS;
    }

    return PATH_VALID;
}

const char *path_sanity_strerror(PathSanityResult res, const char *path)
{
    static _Thread_local char buf[512];
    const char *p = (path && *path) ? path : "<empty>";

    struct stat st;
    int has_stat = (path && stat(path, &st) == 0);

    switch (res) {
    case PATH_VALID:
        snprintf(buf, sizeof(buf), "path '%s' is valid", p);
        break;
    case ERR_PATH_EMPTY:
        snprintf(buf, sizeof(buf), "path string is empty or NULL");
        break;
    case ERR_NOT_ABSOLUTE:
        snprintf(buf, sizeof(buf), "path '%s' is not absolute (must start with '/')", p);
        break;
    case ERR_NOT_A_DIRECTORY:
        snprintf(buf, sizeof(buf), "'%s' is not a directory", p);
        break;
    case ERR_NOT_OWNED_BY_USER:
        if (has_stat) {
            snprintf(buf,
                     sizeof(buf),
                     "owner UID %u of '%s' does not match running UID %u",
                     st.st_uid,
                     p,
                     getuid());
        } else {
            snprintf(
                buf, sizeof(buf), "directory owner UID does not match running UID %u", getuid());
        }
        break;
    case ERR_WORLD_WRITABLE:
        if (has_stat) {
            snprintf(buf,
                     sizeof(buf),
                     "'%s' permissions (%04o) are world-writable (security violation)",
                     p,
                     st.st_mode & 07777);
        } else {
            snprintf(buf, sizeof(buf), "'%s' is world-writable (security violation)", p);
        }
        break;
    case ERR_INSUFFICIENT_PERMS:
        snprintf(buf, sizeof(buf), "insufficient permissions for '%s' (rwx access required)", p);
        break;
    default:
        snprintf(buf, sizeof(buf), "unknown path sanity error for '%s'", p);
        break;
    }

    return buf;
}
