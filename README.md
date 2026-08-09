# Symlink & Dependency Manager (`symdep`)

[![ISO C17](https://img.shields.io/badge/C-ISO%20C17-blue.svg)](https://en.wikipedia.org/wiki/C17_(C_standard_revision))
[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](LICENSE)

A high-performance, zero-dependency symlink manager and package dependency resolver written in C.

`symdep` automates source package deployment, cross-distro package manager dependency installation (`pacman`, `apt`, `dnf`, `apk`, `brew`), mutual exclusion conflicts (e.g. `terminal` vs `headless`), directory symlink folding collisions, multi-repository management, and broken/orphan symlink integrity checking.

---

## Table of Contents

- [Features](#features)
- [Architecture & Resolution Order](#architecture--resolution-order)
  - [Target Directory Precedence](#target-directory-precedence)
  - [Source Repository Precedence](#source-repository-precedence)
- [Build & Installation](#build--installation)
  - [Standard Build Commands](#standard-build-commands)
  - [Advanced Clang Optimization & Diagnostic Profiles](#advanced-clang-optimization--diagnostic-profiles)
- [Command Line Reference](#command-line-reference)
  - [Global Options](#global-options)
  - [Symlink & Deployment Commands](#symlink--deployment-commands)
  - [Package Management (`pkg`)](#package-management-pkg)
  - [Dependency Management (`deps`)](#dependency-management-deps)
  - [File Filtering (`ignore`)](#file-filtering-ignore)
  - [Diagnostics & Repair (`check`)](#diagnostics--repair-check)
  - [Configuration (`config`)](#configuration-config)
- [Configuration & Manifest File Formats](#configuration--manifest-file-formats)
  - [Package Manifest (`.symdeps`)](#package-manifest-symdeps)
  - [Ignore Rules (`.symignore`)](#ignore-rules-symignore)
  - [Tool Registry (`symdep.registry`)](#tool-registry-symdepregistry)
- [Environment Variables](#environment-variables)
- [License](#license)

---

## Features

- **Zero-Dependency Standalone ISO C17**: Lightweight, high-performance C binary compiled with `-std=c17`. Manages symlinks directly with zero external tool dependencies.
- **Symlink Unfolding & Collision Prevention (`fix`)**: Automatically detects and unfolds directory symlinks in target directories to prevent directory folding collisions.
- **Dependency & Plugin Resolution (`deps`, `scan`)**: Auto-detects missing tools across Linux and macOS package managers (`pacman`, `apt`, `dnf`, `apk`, `brew`) and shell plugins (`symdep.registry`).
- **Automated Dependency Scanner (`scan`)**: Recursively scans package scripts and configs for shebang interpreters and command invocations to auto-generate `.symdeps` manifests.
- **Conflict & Mutual Exclusion Management**: Auto-unlinks conflicting source packages (defined in `.symdeps` `CONFLICTS` or detected dynamically when target paths collide) prior to linking.
- **Standardized Command Namespaces**: Intuitive CRUD command namespaces for package management (`pkg`), dependencies (`deps`), file filtering (`ignore`), and system settings (`config`).
- **Multi-Repository & Per-Package Target Configuration**: Manage multiple source repositories simultaneously and assign custom target directories per package (e.g., `/etc` or custom system paths).
- **Global & Package File Filtering (`ignore`)**: Manage `.symignore` glob patterns at repository root or package level with inheritance and redundant pattern detection.
- **Symlink Health & Integrity Audit (`check-symlinks`)**: Scans repository for broken symlinks and target home for unmanaged orphan symlinks.
- **Safety & Signal Cleanups**: Built-in signal handlers (`SIGINT`/`Ctrl+C`) perform atomic temp directory cleanups on unexpected exits.
- **Performance Profiling (`-p`, `--profile`)**: High-precision nanosecond execution profiling for benchmarking deployment steps.

---

## Architecture & Resolution Order

### Target Directory Precedence

When resolving the target destination for symlink deployment, `symdep` checks sources in the following strict order of precedence:

1. **CLI Flag**: `-t, --target-dir <path>`
2. **Package Manifest**: `TARGET="/path"` entry in `.symdeps` (when evaluating a specific package)
3. **Environment Variable**: `SYMDEP_TARGET_DIR` or `TARGET_DIR`
4. **Configuration File**: `TARGET_DIR` set via `symdep config set target <path>`
5. **Fallback Environment**: `$HOME`

### Source Repository Precedence

When locating the active source repository directory:

1. **CLI Flag**: `-d, --source-dir <path>` (aliases: `--src-dir`, `--dotfiles-dir`)
2. **Environment Variable**: `SYMDEP_SOURCE_DIR` or `SOURCE_DIR`
3. **Working Directory Marker**: Current working directory if `symdep.registry` or `.symdepregistry` is present
4. **Configuration File**: Primary entry in `SOURCE_DIRS` set via `symdep config set source <path>`
5. **Fallback Environment**: Current working directory (`getcwd`)

---

## Build & Installation

### Standard Build Commands

```bash
# Build release binary (bin/symdep)
make

# Run unit test suite (bin/test_runner)
make test

# Run end-to-end integration feature tests
make test-feature

# Build static binary for standalone distribution
make static

# Clean build artifacts
make clean

# Install binary and system resource files (defaults to /usr/local)
sudo make install

# Custom prefix installation (e.g. ~/.local)
make install PREFIX=$HOME/.local

# Uninstall binary and installed resource files
sudo make uninstall
```

### Advanced Clang Optimization & Diagnostic Profiles

The build system includes pre-configured Clang targets for performance tuning, sanitizer instrumentation, and static analysis:

```bash
# Build with Clang ThinLTO, -O3, and optimization remarks
make build-clang-opt

# 2-Stage Profile-Guided Optimization (PGO) build using feature test workload
make build-pgo

# Build with AddressSanitizer (ASan) & UndefinedBehaviorSanitizer (UBSan)
make build-sanitize

# Build binary optimized for size (-Oz, ThinLTO)
make build-size

# Run clang-tidy static analysis across all source and test files
make tidy

# Format source files with clang-format
make format

# Verify formatting compliance without modifying files
make format-check
```

---

## Command Line Reference

### Global Options

| Flag | Description |
| :--- | :--- |
| `-d, --source-dir <path>` | Set source repository directory for current command (aliases: `--src-dir`, `--dotfiles-dir`). |
| `-t, --target-dir <path>` | Set target home directory for current command (e.g., `-t ~/`). |
| `-y, --install` | Auto-confirm installation of missing required dependencies & optional plugins without prompting. |
| `-n, --dry-run` | Preview disk changes, symlink creations, backups, and actions without modifying disk. |
| `-s, --save` | Save command-line directory overrides (`-d`/`-t`) directly to user configuration file. |
| `-p, --profile` | Enable nanosecond execution performance profiler logging (also enabled via `SYMDEP_PROFILE=1`). |
| `-h, --help` | Display comprehensive help manual. |

---

### Symlink & Deployment Commands

```bash
# Link / deploy one or multiple packages (with automatic dependency & conflict handling)
symdep link <pkg...>
# Aliases: stow, deploy

# Short invocation (omitting command keyword defaults to linking valid packages)
symdep <pkg...>

# Unlink / remove symlinks for one or multiple packages
symdep unlink <pkg...>
# Alias: unstow

# Relink (unlink & link) one or multiple packages
symdep relink <pkg...>
# Alias: restow

# Link all packages present in source repository
symdep all

# Preview pending symlink creations, backups, and missing dependencies (dry-run)
symdep diff [pkg...]
```

---

### Package Management (`pkg`)

Namespace: `pkg` (aliases: `package`). Supports both space-separated (`pkg create`) and colon-separated (`pkg:create`) syntaxes.

```bash
# Scaffold a new package directory & initialize a default .symdeps manifest
symdep pkg create <name>
# Aliases: pkg:create, package:create, make:pkg

# Safely unlink and remove package directory from disk
symdep pkg remove <name...>
# Aliases: pkg:remove, package:remove, pkg:rm, remove

# List all packages with active symlink status ([LINKED], [PARTIAL], [UNLINKED])
symdep pkg list
# Aliases: pkg:list, package:list, pkg:show, list
```

---

### Dependency Management (`deps`)

Namespace: `deps`. Supports both space-separated (`deps add`) and colon-separated (`deps:add`) syntaxes.

```bash
# Add a dependency or conflict entry to package .symdeps manifest
symdep deps add <pkg> <dep> [--required | --optional | --conflict]
# Aliases: deps:add (default classification: --optional)

# Edit existing dependency classification
symdep deps edit <pkg> <dep> <type>
# Aliases: deps:edit, deps:set (type: --required, --optional, or --conflict)

# Remove a dependency or conflict entry from package manifest
symdep deps remove <pkg> <dep>
# Aliases: deps:remove, deps:rm

# Display raw .symdeps manifest contents for a package
symdep deps show <pkg>
# Aliases: deps:show, deps:list

# Set per-package target directory override in package manifest
symdep deps target <pkg> <path>
# Aliases: deps:target

# Recursively scan package scripts/configs to auto-detect missing tools & plugins
symdep scan [pkg...]
```

---

### File Filtering (`ignore`)

Namespace: `ignore`. Manages `.symignore` files at repository root (global) or inside individual packages.

```bash
# Scaffold global or package-level .symignore template
symdep ignore init [pkg...]
# Aliases: ignore:init, ignore:create

# Append glob pattern(s) to package or global (-g) .symignore
symdep ignore add [pkg] <pattern...>
symdep ignore add -g <pattern...>
# Aliases: ignore:add

# Remove glob pattern(s) from package or global (-g) .symignore
symdep ignore remove [pkg] <pattern...>
symdep ignore remove -g <pattern...>
# Aliases: ignore:remove, ignore:rm, ignore:delete

# Purge .symignore file(s) for package(s) or repository root
symdep ignore clear [pkg...]
# Aliases: ignore:clear, ignore:purge

# Display active .symignore rules (indicates redundant package rules covered globally)
symdep ignore show [pkg...]
# Aliases: ignore:show, ignore:list
```

---

### Diagnostics & Repair (`check`)

```bash
# Verify required/optional tools, plugins, and symlink integrity for packages
symdep check [pkg...]

# Scan repository & target home for broken symlinks and unmanaged orphan symlinks
symdep check-symlinks
# Alias: symdep check symlinks

# Unfold directory symlinks in target into real directories to resolve folding collisions
symdep fix-conflicts
# Alias: symdep fix
```

---

### Configuration (`config`)

Namespace: `config`. Manages global settings in `~/.config/symdep/config`.

```bash
# Display active configuration, source repositories, and target directory
symdep config show
# Aliases: config:show, config:list, config:get

# Set primary source repository or default target directory
symdep config set target <path>
symdep config set source <path>
# Aliases: config:set, config:target

# Add an additional source repository directory (multi-repository setup)
symdep config add <path>
# Aliases: config:add

# Remove a source repository directory from configuration
symdep config remove <path>
# Aliases: config:remove, config:rm
```

---

## Configuration & Manifest File Formats

### Package Manifest (`.symdeps`)

Located inside individual package directories (e.g. `~/src/hyprland/.symdeps`). Legacy `.stowdeps` files are supported as fallbacks.

```ini
# Package Dependency Manifest for 'hyprland'
TARGET="/home/user"
REQUIRED="hyprland waybar"
OPTIONAL="rofi dunst"
CONFLICTS="sway"
```

- **`TARGET`**: Custom destination target path for this package.
- **`REQUIRED`**: Space-separated list of required CLI executables/tools.
- **`OPTIONAL`**: Space-separated list of optional plugins or secondary utilities.
- **`CONFLICTS`**: Space-separated list of packages that must be unlinked before linking this package.

---

### Ignore Rules (`.symignore`)

Follows standard glob pattern rules. Global `.symignore` resides at repository root; package `.symignore` resides inside package directories. Legacy `.stowignore` files are supported as fallbacks.

```gitignore
# Global or package-level ignore patterns
*.zwc
*.pyc
*.symdep_backup_*
.DS_Store
Thumbs.db
.idea/
.vscode/
```

Default ignored patterns: `.symdeps`, `.symignore`, `.stowdeps`, `.stowignore`, `.git`, `.gitignore`, `.gitattributes`, `.gitmodules`, `.DS_Store`, `CVS`, `.svn`, `.hg`, `README*`, `LICENSE*`, `COPYING*`, `*~`, `#*#`, `.#*`.

---

### Tool Registry (`symdep.registry`)

Optionally placed in source repository root (`symdep.registry` or `.symdepregistry`, fallback `stow.registry`) to map tool names to distro package manager names and shell plugin locations.

```ini
# Distro package name overrides (tool@distro=package_name)
neovim@arch=neovim
neovim@ubuntu=neovim
fd@ubuntu=fd-find
ripgrep@debian=ripgrep

# Tool alias & shell plugin path mapping (tool=alias1|alias2|plugin:~/.zsh/plugins/tool)
zsh-autosuggestions=plugin:~/.zsh/plugins/zsh-autosuggestions
bat=bat|batcat
```

---

## Environment Variables

| Variable | Description |
| :--- | :--- |
| `SYMDEP_SOURCE_DIR` / `SOURCE_DIR` / `DOTFILES_DIR` | Override active source repository directory path. |
| `SYMDEP_TARGET_DIR` / `TARGET_DIR` | Override target destination home directory path. |
| `SYMDEP_PROFILE` / `PROFILE` | Set to non-empty string to enable execution profiling. |
| `HOME` | Default target home directory when no override is configured. |
| `XDG_CONFIG_HOME` | Primary directory for `symdep/config` (`~/.config`). |
| `XDG_CONFIG_DIRS` | System-wide search directories for `symdep/config`. |
| `NO_COLOR` | Disable ANSI color codes in terminal help output. |

---

## License

Licensed under the [GNU General Public License v3.0](LICENSE).

