# `symdep` C API Reference

This document provides a comprehensive developer reference for the internal C API modules, data structures, and function contracts defined across the header files in [`include/`](../include/).

`symdep` is written in native **ISO C17** (`-std=c17`) with zero external dependencies.

---

## Table of Contents

- [Core Engine Modules](#core-engine-modules)
  - [Linker Engine (`core/linker.h`)](#linker-engine-corelinkerh)
  - [Package Manager Engine (`core/pkg_manager.h`)](#package-manager-engine-corepkg_managerh)
  - [Dependency Scanner Engine (`core/scanner.h`)](#dependency-scanner-engine-corescannerh)
  - [Configuration Management (`core/config.h`)](#configuration-management-coreconfigh)
  - [Package Manifest (`core/manifest.h`)](#package-manifest-coremanifesth)
  - [Dependency & Health Checker (`core/checker.h`)](#dependency--health-checker-corecheckerh)
  - [Tool & Plugin Registry (`core/registry.h`)](#tool--plugin-registry-coreregistryh)
  - [File Collector & Ignore (`core/file_collector.h` & `core/ignore.h`)](#file-collector--ignore-corefile_collectorh--coreignoreh)
- [CLI Dispatch & Interface Modules](#cli-dispatch--interface-modules)
  - [CLI Option Parser (`cli/cli.h`)](#cli-option-parser-cliclih)
  - [Command Dispatch Engine (`cli/cmd_dispatch.h`)](#command-dispatch-engine-clicmd_dispatchh)
  - [Command Routing Registry & Handlers (`cli/cmd_routes.h`)](#command-routing-registry--handlers-clicmd_routesh)
  - [Interactive Help & Subcommand Manuals (`cli/help.h`)](#interactive-help--subcommand-manuals-clihelph)
- [Asynchronous & Parallel Execution Engines](#asynchronous--parallel-execution-engines)
  - [Linux `io_uring` Asynchronous Driver (`utils/io_uring_backend.h`)](#linux-io_uring-asynchronous-driver-utilsio_uring_backendh)
  - [POSIX Worker Thread Pool (`utils/thread_pool.h`)](#posix-worker-thread-pool-utilsthread_poolh)
- [System & Environment Utility Modules](#system--environment-utility-modules)
  - [Environment & XDG Resolution (`utils/env.h`)](#environment--xdg-resolution-utilsenvh)
  - [Filesystem & Safety Subsystem (`utils/fs.h`)](#filesystem--safety-subsystem-utilsfsh)
  - [Structured Logging (`utils/logger.h`)](#structured-logging-utilsloggerh)
  - [Dynamic Strings, StringArray & StrSet (`utils/str.h`)](#dynamic-strings-stringarray--strset-utilsstrh)
  - [Memory Allocation & Safe Wrappers (`utils/mem.h`)](#memory-allocation--safe-wrappers-utilsmemh)
  - [Path Normalization & Manipulation (`utils/path.h`)](#path-normalization--manipulation-utilspathh)
  - [Signal Handling & Atomic Cleanup (`utils/signal.h`)](#signal-handling--atomic-cleanup-utilssignalh)
  - [Nanosecond Performance Profiler (`utils/timer.h`)](#nanosecond-performance-profiler-utilstimerh)
  - [Global Definitions & Constants (`utils/defs.h`)](#global-definitions--constants-utilsdefsh)

---

## Core Engine Modules

### Linker Engine (`core/linker.h`)

The linker engine executes symlink deployment, unlinking, relinking, directory symlink unfolding, and mutual exclusion handling.

```c
#include "core/linker.h"
```

#### Types & Enums
- `LinkStatus`: `LINK_STATUS_UNLINKED` (0), `LINK_STATUS_PARTIAL` (1), `LINK_STATUS_LINKED` (2).
- `LinkAction`: `LINK_ACTION_LINK` (0), `LINK_ACTION_UNLINK` (1), `LINK_ACTION_RELINK` (2).

#### Key Functions

```c
// Deploy symlinks for a package with dependency and conflict handling
int link_package(const char *source_dir, const char *target_dir, const char *pkg_name, bool auto_install, bool dry_run);

// Unlink stowed symlinks for a package
int unlink_package(const char *source_dir, const char *target_dir, const char *pkg_name, bool dry_run);

// Relink (unlink then link) a package
int relink_package(const char *source_dir, const char *target_dir, const char *pkg_name, bool auto_install, bool dry_run);

// Deploy symlinks for all packages present in source repository
void link_all_packages(const char *source_dir, const char *target_dir, bool auto_install, bool dry_run);

// Unfold target directory symlinks to resolve tree folding collisions
void unfold_directory_symlinks(const char *target_dir, const char *source_dir, const PkgFileList *pkg_files, bool dry_run);

// Evaluate package link status
LinkStatus get_package_link_status(const char *target_dir, const char *source_dir, const char *pkg_name);

// Check if package is fully linked
bool is_package_linked(const char *target_dir, const char *source_dir, const char *pkg_name);

// Unlink mutually exclusive packages defined in package manifest
void handle_mutual_exclusions(const char *target_dir, const char *source_dir, const char *pkg_name, bool dry_run);

// Display deployment status table ([LINKED], [PARTIAL], [UNLINKED]) for all packages
void list_packages_status(const char *source_dir, const char *target_dir);

// Walk symlinks in target directory matching package file patterns
void walk_target_dir_symlinks_targeted(const char *target_dir, const char *source_dir, const PkgFileList *pkg_files, WalkSymlinkCallback cb, void *user_data);
```

---

### Package Manager Engine (`core/pkg_manager.h`)

Handles dynamic cross-distro package manager discovery (`pacman`, `apt`, `dnf`, `apk`, `brew`, `nix-env`, etc.), privilege elevation probing (`sudo`, `doas`, `tsu`), and non-root environment resolution.

```c
#include "core/pkg_manager.h"
```

#### Types
- `PkgManagerEntry`: Struct holding package manager `name`, `binary`, `install_cmd` template, `update_cmd`, `requires_root` flag, and `is_custom` indicator.
- `PkgManagerArray`: Dynamic list of package manager entries.

#### Key Functions

```c
// Resolve active package manager respecting CLI -> Env -> Config -> PATH precedence
bool pkg_manager_resolve(const char *source_dir, const char *cli_override, PkgManagerEntry *out_entry, bool auto_install, bool dry_run);

// Determine required privilege elevation tool (sudo, doas, tsu, or none)
void pkg_manager_get_elevation_tool(const char *source_dir, const PkgManagerEntry *mgr, char *out_tool, size_t out_tool_size, bool auto_install, bool dry_run);

// Construct full shell command string for installing package list
void pkg_manager_build_command(const PkgManagerEntry *mgr, const char *source_dir, const char *pkg_list, char *out_cmd, size_t out_cmd_size, bool auto_install, bool dry_run);

// Search package manager list by name
bool pkg_manager_find_by_name(const PkgManagerArray *list, const char *name, PkgManagerEntry *out_entry);

// Detect package managers installed on active $PATH
void pkg_manager_detect_on_path(const PkgManagerArray *list, PkgManagerArray *out_detected);

// Interactive selection prompt when multiple package managers exist
int pkg_manager_prompt_selection(const PkgManagerArray *detected, PkgManagerEntry *out_entry);
```

---

### Dependency Scanner Engine (`core/scanner.h`)

Parses package scripts, shebang lines (`#!/bin/bash`, `#!/usr/bin/env python3`), and config files to detect tool dependencies.

```c
#include "core/scanner.h"
```

#### Key Functions

```c
// Scan package in preview mode (displays detected tools without disk changes)
void scan_package(const char *source_dir, const char *pkg_name);

// Scan package with custom options (interactive wizard, auto-save manifest, dry-run)
void scan_package_opts(const char *source_dir, const char *pkg_name, bool interactive, bool write_manifest, bool dry_run);
```

---

### Configuration Management (`core/config.h`)

Manages user settings in `~/.config/symdep/config` (or `$XDG_CONFIG_HOME/symdep/config`) and resolves source/target directory paths.

```c
#include "core/config.h"
```

#### Key Functions

```c
// Initialize and load active configuration from disk
void config_load_active(Config *cfg);

// Set default target home directory
void config_set_target_dir(const char *path);

// Set primary source repository path
void config_set_source_dir(const char *path);

// Add an additional source repository path (multi-repository mode)
void config_add_source_dir(const char *path);

// Remove a source repository path from configuration
void config_remove_source_dir(const char *path);

// Set package manager and privilege elevation tool overrides
void config_set_pkg_manager(const char *mgr_name);
void config_set_elevation_tool(const char *tool_name);

// Display active configuration to stdout
void config_show(void);

// Resolve active source directory respecting CLI -> Env -> Config -> CWD precedence
void get_active_source_dir(const char *cli_override, char *buf, size_t buf_size);

// Resolve active target directory respecting CLI -> Env -> Config -> $HOME precedence
void get_active_target_dir(const char *cli_override, char *buf, size_t buf_size);

// Resolve target directory for a package (respecting TARGET="/path" in .symdeps)
void get_active_target_dir_for_pkg(const char *cli_override, const char *source_dir, const char *pkg_name, char *buf, size_t buf_size);
```

---

### Package Manifest (`core/manifest.h`)

Parses and serializes package `.symdeps` manifests.

```c
#include "core/manifest.h"
```

#### Key Functions

```c
// Initialize an empty PackageManifest
void manifest_init(PackageManifest *manifest, const char *pkg_name);

// Load and parse .symdeps manifest from package directory
bool manifest_load(PackageManifest *manifest, const char *source_dir);

// Save PackageManifest struct back to disk
bool manifest_save(const PackageManifest *manifest, const char *source_dir);

// Free heap allocations in PackageManifest
void manifest_free(PackageManifest *manifest);

// Add dependency or conflict entry to manifest
void manifest_add_dep(const char *source_dir, const char *pkg_name, const char *dep, const char *type);

// Edit existing dependency classification in manifest
void manifest_edit_dep(const char *source_dir, const char *pkg_name, const char *dep, const char *new_type);

// Remove dependency or conflict entry from manifest
void manifest_remove_dep(const char *source_dir, const char *pkg_name, const char *dep);

// Set per-package target path override
void manifest_set_target(const char *source_dir, const char *pkg_name, const char *target_path);

// Print raw manifest contents to stdout
void manifest_show(const char *source_dir, const char *pkg_name);

// Safely unlink package symlinks and remove package directory from disk
void package_remove(const char *source_dir, const char *target_dir, const char *pkg_name, bool dry_run);
```

---

### Dependency & Health Checker (`core/checker.h`)

Verifies package requirements and audits broken or orphan symlinks.

```c
#include "core/checker.h"
```

#### Key Functions

```c
// Non-blocking passive audit of required/optional dependencies (auto-installs if auto_install is true)
void check_package_dependencies(const char *source_dir, const char *target_pkg, bool auto_install, bool dry_run);

// Standalone dependency installer for package(s) via active package manager
int install_package_dependencies(const char *source_dir, const char *target_pkg, bool auto_install, bool dry_run, bool strict_no_skip);

// Scan repository and target home for broken and unmanaged orphan symlinks
void check_symlink_health(const char *source_dir, const char *target_dir);
```

---

### Tool & Plugin Registry (`core/registry.h`)

Maps virtual tool names to distribution-specific package names and shell plugin paths (`symdep.registry`).

```c
#include "core/registry.h"
```

#### Key Functions

```c
// Retrieve binary aliases for a virtual tool name
void registry_get_aliases(const char *source_dir, const char *tool, StringArray *aliases);

// Translate tool name to distro package name (e.g., bat -> batcat on Ubuntu)
void registry_get_distro_pkg(const char *source_dir, const char *tool, const char *distro, char *out, size_t out_size);

// Query all registered tool names
void registry_get_all_tools(const char *source_dir, StringArray *tools);

// Append a new tool entry or alias mapping to registry
void registry_add_tool(const char *source_dir, const char *tool);

// Append or update a distro-specific package mapping in registry
void registry_add_distro_mapping(const char *source_dir, const char *tool, const char *distro, const char *pkg_name);

// Check if tool binary or shell plugin directory exists dynamically
bool is_tool_installed_dynamic(const char *source_dir, const char *tool);

// Discover all valid package directories in source repository
void get_all_packages(const char *source_dir, StringArray *packages);
```

---

### File Collector & Ignore (`core/file_collector.h` & `core/ignore.h`)

Handles directory file traversal, pattern filtering, and `.symignore` management.

```c
#include "core/file_collector.h"
#include "core/ignore.h"
```

#### Key Functions

```c
// Recursively collect package files filtering out ignored patterns
void collect_package_files(const char *pkg_dir, const StringArray *raw_ignores, PkgFileList *list);

// Check if a relative path matches active ignore patterns
bool is_path_ignored(const char *rel_path, const StringArray *raw_ignores);

// Initialize .symignore files for package(s) or repository root
void ignore_init(const char *source_dir, const char *const *pkgs, size_t count);

// Append pattern(s) to package or global .symignore
void ignore_add_patterns(const char *source_dir, const char *pkg_name, const char *const *patterns, size_t count);

// Remove pattern(s) from package or global .symignore
void ignore_remove_patterns(const char *source_dir, const char *pkg_name, const char *const *patterns, size_t count);

// Clear/purge .symignore files
void ignore_clear(const char *source_dir, const char *const *pkgs, size_t count);

// Display active merged ignore patterns with redundancy warnings
void ignore_show(const char *source_dir, const char *const *pkgs, size_t count);
```

---

## CLI Dispatch & Interface Modules

### CLI Option Parser (`cli/cli.h`)

Parses global flags (`-d`, `-t`, `-m`, `-i`, `-y`, `-n`, `-s`, `-p`, `-h`).

```c
#include "cli/cli.h"

// Parse CLI flags and extract positional arguments
int parse_cli_options(int argc, char **argv, CliOptions *opts, StringArray *positional_args);

// Free memory associated with CliOptions
void cli_options_free(CliOptions *opts);
```

---

### Command Dispatch Engine (`cli/cmd_dispatch.h`)

Routes CLI invocations against the centralized `ROUTE_TABLE[]`.

```c
#include "cli/cmd_dispatch.h"

// Route positional arguments against command table
int dispatch_command(const StringArray *args, const CliOptions *opts);

// Match an argument against primary route keyword or alias list
bool route_matches(const CommandRoute *route, const char *arg);
```

---

### Command Routing Registry & Handlers (`cli/cmd_routes.h`)

Centralized command routing table registry and individual command handlers.

```c
#include "cli/cmd_routes.h"

// Global dispatch routing table
extern const CommandRoute ROUTE_TABLE[];

// Core command handlers:
int cmd_stow(const CommandContext *ctx);
int cmd_unstow(const CommandContext *ctx);
int cmd_restow(const CommandContext *ctx);
int cmd_all(const CommandContext *ctx);
int cmd_diff(const CommandContext *ctx);
int cmd_scan(const CommandContext *ctx);
int cmd_check(const CommandContext *ctx);
int cmd_check_symlinks(const CommandContext *ctx);
int cmd_fix_conflicts(const CommandContext *ctx);
int cmd_pkg_create(const CommandContext *ctx);
int cmd_pkg_remove(const CommandContext *ctx);
int cmd_pkg_list(const CommandContext *ctx);
int cmd_deps_add(const CommandContext *ctx);
int cmd_deps_edit(const CommandContext *ctx);
int cmd_deps_remove(const CommandContext *ctx);
int cmd_deps_show(const CommandContext *ctx);
int cmd_deps_target(const CommandContext *ctx);
int cmd_ignore_init(const CommandContext *ctx);
int cmd_ignore_add(const CommandContext *ctx);
int cmd_ignore_remove(const CommandContext *ctx);
int cmd_ignore_show(const CommandContext *ctx);
int cmd_ignore_clear(const CommandContext *ctx);
int cmd_config_show(const CommandContext *ctx);
int cmd_config_set(const CommandContext *ctx);
int cmd_config_add(const CommandContext *ctx);
int cmd_config_remove(const CommandContext *ctx);
int cmd_help(const CommandContext *ctx);
```

---

### Interactive Help & Subcommand Manuals (`cli/help.h`)

Formats and prints top-level and subcommand help manuals.

```c
#include "cli/help.h"

// Display top-level usage manual and command summary
void show_help(void);

// Display detailed manual for scan command
void show_scan_help(void);

// Display detailed manual for a specific subcommand topic (e.g. "config")
void show_subcommand_help(const char *topic);
```

---

## Asynchronous & Parallel Execution Engines

### Linux `io_uring` Asynchronous Driver (`utils/io_uring_backend.h`)

Zero-copy kernel submission queue ring buffer driver for ultra-high-throughput batch symlink operations on modern Linux kernels.

```c
#include "utils/io_uring_backend.h"

// Probe if host Linux kernel supports io_uring with required syscalls
bool io_uring_is_supported(void);

// Execute zero-copy batch symlink operations using Linux io_uring SQE ring buffer
int io_uring_link_batch(const PkgFileList *files, PackageContext *ctx);
```

---

### POSIX Worker Thread Pool (`utils/thread_pool.h`)

Zero-dependency work-stealing pthread pool for parallel multi-core symlink deployment and cross-platform execution.

```c
#include "utils/thread_pool.h"

// Create a new thread pool (num_threads=0 for auto-detecting online CPU cores)
ThreadPool *thread_pool_create(size_t num_threads);

// Enqueue a task work item
bool thread_pool_add_task(ThreadPool *pool, void (*function)(void *arg), void *arg);

// Wait for all queued and active tasks to complete
void thread_pool_wait(ThreadPool *pool);

// Destroy thread pool and release resources
void thread_pool_destroy(ThreadPool *pool);

// Check if caller is executing inside a worker thread
bool is_in_worker_thread(void);

// Query number of online CPU cores
size_t get_cpu_core_count(void);
```

---

## System & Environment Utility Modules

### Environment & XDG Resolution (`utils/env.h`)

Handles environment variable expansion, XDG Base Directory resolution, `$PATH` binary lookups with in-memory memoization, and `/etc/passwd` parsing fallback.

```c
#include "utils/env.h"

// Expand $VAR and ${VAR} in string
void expand_env_vars(const char *src, char *out, size_t out_size);

// Resolve user home directory with NSS-independent /etc/passwd fallback
bool get_user_home_dir(char *buf, size_t buf_size);

// Query XDG Base Directory paths
bool get_xdg_dir(XdgDirType type, char *buf, size_t buf_size);
bool get_xdg_config_home(char *buf, size_t buf_size);
bool get_xdg_data_home(char *buf, size_t buf_size);
bool get_xdg_cache_home(char *buf, size_t buf_size);
bool get_xdg_state_home(char *buf, size_t buf_size);

// Initialize and resolve runtime AppEnvironment against sanity checks
void app_env_init(AppEnvironment *env);
bool app_env_resolve(AppEnvironment *env, const char *cli_target_override, PathSanityResult *out_reason);

// Identify host Linux distribution ID ("arch", "ubuntu", "fedora", "alpine", etc.)
void get_distro_id(char *buf, size_t buf_size);

// Search system $PATH for binary with in-memory memoization
bool find_executable_in_path(const char *executable, char *out_path, size_t out_path_size);
```

---

### Filesystem & Safety Subsystem (`utils/fs.h`)

File verification, path sanity checks, directory recursion, and atomic operations.

```c
#include "utils/fs.h"

// Dual-Driver Filesystem & Syscall Abstraction (Modern *at vs Legacy POSIX)
// Modern (default): routes via directory-relative *at syscalls with AT_FDCWD (seccomp safe, SIGSYS 31 immune)
// Legacy (-DSYMDEP_LEGACY_SYSCALLS): routes via classic POSIX direct syscalls
FS_SYMLINK(target, linkpath)     // symlinkat(target, AT_FDCWD, linkpath) / symlink(target, linkpath)
FS_UNLINK(path)                  // unlinkat(AT_FDCWD, path, 0) / unlink(path)
FS_RMDIR(path)                   // unlinkat(AT_FDCWD, path, AT_REMOVEDIR) / rmdir(path)
FS_MKDIR(path, mode)             // mkdirat(AT_FDCWD, path, mode) / mkdir(path, mode)
FS_RENAME(oldpath, newpath)      // renameat(AT_FDCWD, oldpath, AT_FDCWD, newpath) / rename(oldpath, newpath)
FS_READLINK(path, buf, bufsiz)   // readlinkat(AT_FDCWD, path, buf, bufsiz) / readlink(path, buf, bufsiz)
FS_STAT(path, statbuf)           // fstatat(AT_FDCWD, path, statbuf, 0) / stat(path, statbuf)
FS_LSTAT(path, statbuf)          // fstatat(AT_FDCWD, path, statbuf, AT_SYMLINK_NOFOLLOW) / lstat(path, statbuf)
FS_OPEN(path, flags, ...)        // openat(AT_FDCWD, path, flags, mode) / open(path, flags, mode)
FS_ACCESS(path, mode)            // faccessat(AT_FDCWD, path, mode, 0) / access(path, mode)
FS_CHMOD(path, mode)             // fchmodat(AT_FDCWD, path, mode, 0) / chmod(path, mode)
FS_CHOWN(path, owner, group)     // fchownat(AT_FDCWD, path, owner, group, 0) / chown(path, owner, group)

// Verify path security and permissions
PathSanityResult verify_path_sanity(const char *path);
const char *path_sanity_strerror(PathSanityResult res, const char *path);

// File and path inspection
bool file_exists(const char *path);
bool is_dir(const char *path);
bool is_symlink(const char *path);
bool is_executable_in_path(const char *executable);
char *read_symlink_target(const char *path);
bool is_symlink_pointing_to(const char *symlink_path, const char *pkg_file_path, const char *real_pkg_file_path);

// Recursive directory creation (mkdir -p)
int mkdir_p(const char *path, mode_t mode);

// Directory traversal
void walk_dir_symlinks(const char *dir_path, int current_depth, int max_depth, WalkSymlinkCallback cb, void *user_data);
void walk_dir_files(const char *base_dir, const char *current_dir, WalkFileCallback cb, void *user_data);

// Clean temporary directory contents
void cleanup_temp_dir_contents(const char *dir_path);

// Open system resource file
FILE *open_resource_file(const char *filename);

// Execute shell command
int run_system_cmd(const char *cmd);

// Execute shell command and capture standard error/output
int run_system_cmd_with_capture(const char *cmd, char *out_buf, size_t out_size);
```

---

### Structured Logging (`utils/logger.h`)

Colorized, severity-tiered diagnostic logging and optional file logging.

```c
#include "utils/logger.h"

// Initialize logging threshold and optional file output
void logger_init(LogLevel level, const char *log_file_path);
void logger_set_level(LogLevel level);
void logger_close(void);

// Logging primitives
void log_debug(const char *fmt, ...);
void log_info(const char *fmt, ...);
void log_success(const char *fmt, ...);
void log_warn(const char *fmt, ...);
void log_error(const char *fmt, ...);
```

---

### Dynamic Strings, StringArray & StrSet (`utils/str.h`)

Dynamic string collections, string set deduplication, whitespace trimming, and shell escaping.

```c
#include "utils/str.h"

// StringArray operations
void str_array_init(StringArray *arr);
void str_array_append(StringArray *arr, const char *str);
bool str_array_contains(const StringArray *arr, const char *str);
void str_array_free(StringArray *arr);

// StrSet operations
void str_set_init(StrSet *set);
bool str_set_add(StrSet *set, const char *str);
bool str_set_contains(const StrSet *set, const char *str);
void str_set_free(StrSet *set);

// String helpers
char *trim_whitespace(char *str);
void escape_shell_arg(const char *src, char *dest, size_t dest_size);
void str_split_delim(const char *src, const char *delim, StringArray *out_arr);
void str_expand_vars(const char *src, char *out, size_t out_size, StrVarResolver resolver, void *ctx);
```

---

### Memory Allocation & Safe Wrappers (`utils/mem.h`)

OOM-safe allocation wrappers that exit cleanly on memory allocation failure.

```c
#include "utils/mem.h"

void *safe_malloc(size_t size);
void *safe_calloc(size_t num, size_t size);
void *safe_realloc(void *ptr, size_t size);
char *safe_strdup(const char *s);
```

---

### Path Normalization & Manipulation (`utils/path.h`)

POSIX path normalization, prefix verification, tilde expansion, and joining.

```c
#include "utils/path.h"

void normalize_path(char *path);
int collapse_path(char *path);
int join_path(char *out, size_t out_size, const char *dir, const char *rel);
int is_path_prefix(const char *path, const char *prefix);
void expand_tilde_path(const char *path, char *out, size_t out_size);
```

---

### Signal Handling & Atomic Cleanup (`utils/signal.h`)

POSIX signal handling (`sigaction`) and atomic scratch path cleanup on termination.

```c
#include "utils/signal.h"

void setup_signal_handlers(void);
void register_temp_path(const char *path);
void unregister_temp_path(const char *path);
void cleanup_temp_paths_signal_safe(void);
```

---

### Nanosecond Performance Profiler (`utils/timer.h`)

High-precision nanosecond execution timer and scoped profiling macros.

```c
#include "utils/timer.h"

// Global profiler control
void perf_profiler_set_enabled(bool enabled);
bool perf_profiler_is_enabled(void);

// Timer operations
PerfTimer perf_timer_start(const char *name);
double perf_timer_stop(PerfTimer *timer);
double perf_timer_elapsed_ms(const PerfTimer *timer);
double perf_timer_elapsed_us(const PerfTimer *timer);
void perf_timer_log(const PerfTimer *timer);
void perf_timer_log_force(const PerfTimer *timer);

// Scoped measurement macros
#define PERF_PROFILE_START(label) PerfTimer _timer_##label = perf_timer_start(#label)
#define PERF_PROFILE_END(label)           \
    do {                                  \
        perf_timer_stop(&_timer_##label); \
        perf_timer_log(&_timer_##label);  \
    } while (0)
```

---

### Global Definitions & Constants (`utils/defs.h`)

Standard deterministic path limits, stringification macros, and ANSI color codes.

```c
#include "utils/defs.h"

#define STOW_PATH_MAX   4096
#define STOW_PATH_LARGE 8192
#define STOW_PATH_HUGE  16384

#define COLOR_RED     "\033[0;31m"
#define COLOR_GREEN   "\033[0;32m"
#define COLOR_YELLOW  "\033[1;33m"
#define COLOR_BLUE    "\033[0;34m"
#define COLOR_CYAN    "\033[0;36m"
#define COLOR_WHITE   "\033[1;37m"
#define COLOR_BOLD    "\033[1m"
#define COLOR_RESET   "\033[0m"
```
