# `symdep` C API Reference

This document provides a reference for the internal C API modules, data structures, and function contracts defined across the header files in [`include/`](../include/).

`symdep` is written in native **ISO C17** (`-std=c17`) with zero external dependencies.

---

## Table of Contents

- [Core Engine Modules](#core-engine-modules)
  - [Linker Engine (`core/linker.h`)](#linker-engine-corelinkerh)
  - [Package Manager Engine (`core/pkg_manager.h`)](#package-manager-engine-corepkg_managerh)
  - [Dependency Scanner (`core/scanner.h`)](#dependency-scanner-corescannerh)
  - [Configuration Management (`core/config.h`)](#configuration-management-coreconfigh)
  - [Package Manifest (`core/manifest.h`)](#package-manifest-coremanifesth)
  - [Dependency & Health Checker (`core/checker.h`)](#dependency--health-checker-corecheckerh)
  - [Tool Registry (`core/registry.h`)](#tool-registry-coreregistryh)
  - [File Collector & Ignore (`core/file_collector.h` & `core/ignore.h`)](#file-collector--ignore-corefile_collectorh--coreignoreh)
- [CLI Dispatch Modules](#cli-dispatch-modules)
  - [CLI Option Parser (`cli/cli.h`)](#cli-option-parser-cliclih)
  - [Command Dispatch Engine (`cli/cmd_dispatch.h`)](#command-dispatch-engine-clicmd_dispatchh)
- [Utility Helper Modules](#utility-helper-modules)
  - [Memory Allocation (`utils/mem.h`)](#memory-allocation-utilsmemh)
  - [Path Operations (`utils/path.h`)](#path-operations-utilspathh)
  - [Signal & Safety (`utils/signal.h`)](#signal--safety-utilssignalh)

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
```

---

### Dependency Scanner (`core/scanner.h`)

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
// Load active configuration from disk
void config_load_active(Config *cfg);

// Set default target home directory
void config_set_target_dir(const char *path);

// Set primary source repository path
void config_set_source_dir(const char *path);

// Add an additional source repository path (multi-repository mode)
void config_add_source_dir(const char *path);

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
// Load and parse .symdeps manifest from package directory
bool manifest_load(PackageManifest *manifest, const char *source_dir);

// Save PackageManifest struct back to disk
bool manifest_save(const PackageManifest *manifest, const char *source_dir);

// Add dependency or conflict entry to manifest
void manifest_add_dep(const char *source_dir, const char *pkg_name, const char *dep, const char *type);

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
// Verify required/optional tools and auto-install missing dependencies if requested
void check_package_dependencies(const char *source_dir, const char *target_pkg, bool auto_install, bool dry_run);

// Scan repository and target home for broken and unmanaged orphan symlinks
void check_symlink_health(const char *source_dir, const char *target_dir);
```

---

### Tool Registry (`core/registry.h`)

Maps virtual tool names to distribution-specific package names and shell plugin paths (`symdep.registry`).

```c
#include "core/registry.h"
```

#### Key Functions

```c
// Translate tool name to distro package name (e.g., bat -> batcat on Ubuntu)
void registry_get_distro_pkg(const char *source_dir, const char *tool, const char *distro, char *out, size_t out_size);

// Check if tool binary or shell plugin directory exists
bool is_tool_installed_dynamic(const char *source_dir, const char *tool);
```

---

### File Collector & Ignore (`core/file_collector.h` & `core/ignore.h`)

Handles directory file traversal and `.symignore` pattern filtering.

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

// Append pattern(s) to package or global .symignore
void ignore_add_patterns(const char *source_dir, const char *pkg_name, const char *const *patterns, size_t count);
```

---

## CLI Dispatch Modules

### CLI Option Parser (`cli/cli.h`)

Parses global flags (`-d`, `-t`, `-m`, `-i`, `-y`, `-n`, `-s`, `-p`, `-h`).

```c
#include "cli/cli.h"

// Parse CLI flags and extract positional arguments
int parse_cli_options(int argc, char **argv, CliOptions *opts, StringArray *positional_args);
```

---

### Command Dispatch Engine (`cli/cmd_dispatch.h`)

Routes CLI invocations against the centralized `ROUTE_TABLE[]`.

```c
#include "cli/cmd_dispatch.h"

// Route positional arguments against command table
int dispatch_command(const StringArray *args, const CliOptions *opts);
```

---

## Utility Helper Modules

### Memory Allocation (`utils/mem.h`)

OOM-safe allocation wrappers that exit cleanly on allocation failure.

```c
#include "utils/mem.h"

void *safe_malloc(size_t size);
void *safe_calloc(size_t num, size_t size);
void *safe_realloc(void *ptr, size_t size);
char *safe_strdup(const char *s);
```

---

### Path Operations (`utils/path.h`)

POSIX path normalization, joining, and expansion helpers.

```c
#include "utils/path.h"

void normalize_path(char *path);
int collapse_path(char *path);
int join_path(char *out, size_t out_size, const char *dir, const char *rel);
int is_path_prefix(const char *path, const char *prefix);
void expand_tilde_path(const char *path, char *out, size_t out_size);
```

---

### Signal & Safety (`utils/signal.h`)

POSIX signal handling (`sigaction`) and atomic scratch path cleanup.

```c
#include "utils/signal.h"

void setup_signal_handlers(void);
void register_temp_path(const char *path);
void unregister_temp_path(const char *path);
void cleanup_temp_paths_signal_safe(void);
```
