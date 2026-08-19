/*
 * Symlink & Dependency Manager (symdep)
 * Filesystem Validation, Traversal & Symlink Safety Subsystem Header
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
#ifndef SYMDEP_UTILS_FS_H
#define SYMDEP_UTILS_FS_H

#include <fcntl.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "utils/defs.h"
#include "utils/str.h"

#ifndef AT_FDCWD
#define AT_FDCWD (-100)
#endif
#ifndef AT_SYMLINK_NOFOLLOW
#define AT_SYMLINK_NOFOLLOW 0x100
#endif
#ifndef AT_REMOVEDIR
#define AT_REMOVEDIR 0x200
#endif
#ifndef AT_EACCESS
#define AT_EACCESS 0x200
#endif

#include <stdarg.h>

/**
 * @name Dual-Driver Filesystem & Syscall Abstraction Macros
 *
 * Modern seccomp kernels and 64-bit architectures (AArch64, RISC-V 64) omit
 * or filter legacy direct filesystem syscalls (failing with SIGSYS / error 31).
 * On modern kernels, operations route via directory-relative '*at' syscalls
 * with AT_FDCWD (symlinkat, unlinkat, mkdirat, renameat, readlinkat, fstatat,
 * openat, faccessat, fchmodat, fchownat).
 *
 * On legacy kernels or when SYMDEP_LEGACY_SYSCALLS / USE_LEGACY_SYSCALLS is
 * defined, operations route via classic POSIX syscalls.
 * @{
 */
#if defined(SYMDEP_LEGACY_SYSCALLS) || defined(USE_LEGACY_SYSCALLS) || defined(NO_AT_SYSCALLS)

/* Legacy kernel filesystem syscalls */
#define FS_SYMLINK(target, linkpath) symlink((target), (linkpath))
#define FS_UNLINK(path) unlink(path)
#define FS_RMDIR(path) rmdir(path)
#define FS_MKDIR(path, mode) mkdir((path), (mode))
#define FS_RENAME(oldpath, newpath) rename((oldpath), (newpath))
#define FS_READLINK(path, buf, bufsiz) readlink((path), (buf), (bufsiz))
#define FS_STAT(path, statbuf) stat((path), (statbuf))
#define FS_LSTAT(path, statbuf) lstat((path), (statbuf))
#define FS_ACCESS(path, mode) access((path), (mode))
#define FS_CHMOD(path, mode) chmod((path), (mode))
#define FS_CHOWN(path, owner, group) chown((path), (owner), (group))

static inline int fs_open(const char *path, int flags, ...)
{
    mode_t mode = 0;
    if (flags & O_CREAT) {
        va_list args;
        va_start(args, flags);
        mode = (mode_t)va_arg(args, int);
        va_end(args);
    }
    return open(path, flags, mode);
}

#else

/* Modern kernel *at filesystem syscalls (seccomp & modern ABI safe) */
#define FS_SYMLINK(target, linkpath) symlinkat((target), AT_FDCWD, (linkpath))
#define FS_UNLINK(path) unlinkat(AT_FDCWD, (path), 0)
#define FS_RMDIR(path) unlinkat(AT_FDCWD, (path), AT_REMOVEDIR)
#define FS_MKDIR(path, mode) mkdirat(AT_FDCWD, (path), (mode))
#define FS_RENAME(oldpath, newpath) renameat(AT_FDCWD, (oldpath), AT_FDCWD, (newpath))
#define FS_READLINK(path, buf, bufsiz) readlinkat(AT_FDCWD, (path), (buf), (bufsiz))
#define FS_STAT(path, statbuf) fstatat(AT_FDCWD, (path), (statbuf), 0)
#define FS_LSTAT(path, statbuf) fstatat(AT_FDCWD, (path), (statbuf), AT_SYMLINK_NOFOLLOW)
#define FS_ACCESS(path, mode) faccessat(AT_FDCWD, (path), (mode), 0)
#define FS_CHMOD(path, mode) fchmodat(AT_FDCWD, (path), (mode), 0)
#define FS_CHOWN(path, owner, group) fchownat(AT_FDCWD, (path), (owner), (group), 0)

static inline int fs_open(const char *path, int flags, ...)
{
    mode_t mode = 0;
    if (flags & O_CREAT) {
        va_list args;
        va_start(args, flags);
        mode = (mode_t)va_arg(args, int);
        va_end(args);
    }
    return openat(AT_FDCWD, path, flags, mode);
}

#endif

#define FS_OPEN fs_open

/* Lowercase aliases for convenience */
#define fs_symlink(target, linkpath) FS_SYMLINK(target, linkpath)
#define fs_unlink(path) FS_UNLINK(path)
#define fs_rmdir(path) FS_RMDIR(path)
#define fs_mkdir(path, mode) FS_MKDIR(path, mode)
#define fs_rename(oldpath, newpath) FS_RENAME(oldpath, newpath)
#define fs_readlink(path, buf, bufsiz) FS_READLINK(path, buf, bufsiz)
#define fs_stat(path, statbuf) FS_STAT(path, statbuf)
#define fs_lstat(path, statbuf) FS_LSTAT(path, statbuf)
#define fs_access(path, mode) FS_ACCESS(path, mode)
#define fs_chmod(path, mode) FS_CHMOD(path, mode)
#define fs_chown(path, owner, group) FS_CHOWN(path, owner, group)
/** @} */

/**
 * @enum PathSanityResult
 * @brief Path validation and security sanity check outcomes.
 */
typedef enum {
    PATH_VALID,            /**< Path exists, is absolute, and satisfies permissions */
    ERR_PATH_EMPTY,        /**< Path string is empty */
    ERR_NOT_ABSOLUTE,      /**< Path is not an absolute path */
    ERR_NOT_A_DIRECTORY,   /**< Path is not a directory */
    ERR_NOT_OWNED_BY_USER, /**< Path is not owned by the current user */
    ERR_WORLD_WRITABLE,    /**< Path has world-writable permissions (security risk) */
    ERR_INSUFFICIENT_PERMS /**< Process lacks read/write/execute permissions on path */
} PathSanityResult;

/**
 * @brief Verify security and sanity constraints on a directory path.
 *
 * @param path Directory path to validate.
 * @return PATH_VALID on success, or specific error enum value on failure.
 */
PathSanityResult verify_path_sanity(const char *path);

/**
 * @brief Return a human-readable error description for a PathSanityResult.
 *
 * @param res  PathSanityResult enum code.
 * @param path Path string associated with validation.
 * @return Static or formatted error message string.
 */
const char *path_sanity_strerror(PathSanityResult res, const char *path);

/** Check if a regular file exists at path */
bool file_exists(const char *path);

/** Open an installed system resource file (e.g. templates, help files) */
FILE *open_resource_file(const char *filename);

/** Check if path exists and is a directory */
bool is_dir(const char *path);

/** Check if path exists and is a symbolic link */
bool is_symlink(const char *path);

/** Check if an executable binary exists on system $PATH */
bool is_executable_in_path(const char *executable);

/** Read target destination path of a symbolic link (caller frees memory) */
char *read_symlink_target(const char *path);

/** Check if a symbolic link points to the expected package source file */
bool is_symlink_pointing_to(const char *symlink_path,
                            const char *pkg_file_path,
                            const char *real_pkg_file_path);

/**
 * @brief Recursively create directories along path (equivalent to `mkdir -p`).
 *
 * @param path Directory path to construct.
 * @param mode POSIX file mode permissions (e.g. 0755).
 * @return 0 on success, -1 on failure.
 */
int mkdir_p(const char *path, mode_t mode);

/** Callback type for symlink directory walk traversal */
typedef void (*WalkSymlinkCallback)(const char *symlink_path, void *user_data);

/** Callback type for recursive file traversal */
typedef void (*WalkFileCallback)(const char *file_path, const char *rel_path, void *user_data);

/** Clean up all files and subdirectories inside a temporary directory */
void cleanup_temp_dir_contents(const char *dir_path);

/**
 * @brief Traverse directory recursively discovering symbolic links.
 *
 * @param dir_path      Starting directory path.
 * @param current_depth Current recursion depth.
 * @param max_depth     Maximum allowed recursion depth (-1 for unlimited).
 * @param cb            Callback invoked for each discovered symlink.
 * @param user_data     Opaque pointer passed to callback.
 */
void walk_dir_symlinks(const char *dir_path,
                       int current_depth,
                       int max_depth,
                       WalkSymlinkCallback cb,
                       void *user_data);

/**
 * @brief Recursively walk files under a base directory.
 *
 * @param base_dir    Base root directory path.
 * @param current_dir Current relative/child directory path.
 * @param cb          Callback invoked for each discovered file.
 * @param user_data   Opaque pointer passed to callback.
 */
void walk_dir_files(const char *base_dir,
                    const char *current_dir,
                    WalkFileCallback cb,
                    void *user_data);

/** Execute a system shell command safely */
int run_system_cmd(const char *cmd);

/** Execute a system shell command and capture standard error/output */
int run_system_cmd_with_capture(const char *cmd, char *out_buf, size_t out_size);

/**
 * @brief Atomically swap or replace a temporary staging path with target destination.
 *
 * For directory-over-symlink unfolding:
 * - On Linux (>= 3.15), uses raw `renameat2(..., RENAME_EXCHANGE)` for zero-window physical
 * atomicity.
 * - On macOS / Darwin, uses `renameatx_np(..., RENAME_EXCHANGE)` on APFS/HFS+.
 * - On generic POSIX, falls back to staging `unlink()` + `rename()` transition with registered
 * cleanup traps.
 *
 * For non-directory replacement (symlink-to-symlink, file-to-file):
 * - Uses standard POSIX `rename()` which is natively atomic in the kernel VFS.
 *
 * @param src_tmp Staging temporary path.
 * @param dst_target Destination path to replace.
 * @param is_dir_over_symlink True if src_tmp is a directory replacing a symlink.
 * @return true on success, false on failure.
 */
bool fs_atomic_swap_or_replace(const char *src_tmp,
                               const char *dst_target,
                               bool is_dir_over_symlink);

#endif /* SYMDEP_UTILS_FS_H */
