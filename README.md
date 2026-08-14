# Symlink & Dependency Manager (`symdep`)

[![ISO C17](https://img.shields.io/badge/C-ISO%20C17-blue.svg)](https://en.wikipedia.org/wiki/C17_(C_standard_revision))
[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](LICENSE)

A high-performance, zero-dependency symlink manager and cross-distro package dependency resolver written in native ISO C17.

`symdep` automates source package deployment, cross-distro package manager dependency installation (`pacman`, `apt`, `dnf`, `apk`, `brew`), mutual exclusion conflict resolution (e.g., `terminal` vs `headless`), directory symlink folding collision handling (`fix-conflicts`), multi-repository management, AST/shebang script dependency scanning (`scan`), and broken/orphan symlink health auditing (`check-symlinks`).

---

## Table of Contents

- [Quick Start](#quick-start)
- [Features](#features)
- [Architecture & Design Rationale](#architecture--design-rationale)
  - [Architectural Decision Records (ADRs)](#architectural-decision-records-adrs)
  - [Target Directory Precedence](#target-directory-precedence)
  - [Source Repository Precedence](#source-repository-precedence)
- [Build & Installation](#build--installation)
  - [Standard Build Targets](#standard-build-targets)
  - [Clang Optimization & Sanitizer Profiles](#clang-optimization--sanitizer-profiles)
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
  - [Tool & Plugin Registry (`symdep.registry`)](#tool--plugin-registry-symdepregistry)
  - [Global User Configuration (`~/.config/symdep/config`)](#global-user-configuration-configsymdepconfig)
- [Environment Variables](#environment-variables)
- [Contributing](#contributing)
- [License](#license)

---

## Quick Start

```bash
# Clone and build symdep binary (output: bin/symdep)
git clone https://github.com/durzhars/symdep.git
cd symdep
make

# Run full test suite (unit + feature tests)
make test
make test-feature

# Scaffold a new dotfile package manifest
./bin/symdep pkg create hyprland

# Auto-scan script dependencies inside package and save to manifest
./bin/symdep scan hyprland -y

# Link package and auto-install any missing dependencies via active system package manager
./bin/symdep link hyprland -y
```

---

## Features

- **Zero-Dependency ISO C17 Architecture**: Lightweight, sub-millisecond C binary compiled with `-std=c17`. Manages symlinks directly with zero external tool or interpreter requirements.
- **Symlink Unfolding & Collision Prevention (`fix`)**: Automatically detects and unfolds directory symlinks in target directories to resolve directory folding collisions cleanly.
- **Cross-Distro Dependency Resolution (`deps`, `scan`)**: Auto-detects missing tools across Linux and macOS package managers (`pacman`, `apt`, `dnf`, `apk`, `brew`) and shell plugins (`symdep.registry`).
- **AST & Shebang Code Analysis Scanner (`scan`)**: Recursively parses package scripts and configs for shebang interpreters and command invocations to auto-generate `.symdeps` manifests.
- **Conflict & Mutual Exclusion Management**: Auto-unlinks conflicting source packages (defined in `.symdeps` `CONFLICTS` or detected dynamically when target paths collide) prior to linking.
- **Standardized Command Namespaces**: Intuitive CRUD command namespaces for package management (`pkg`), dependencies (`deps`), file filtering (`ignore`), and system settings (`config`), supporting space-separated, colon-separated, and legacy Stow syntaxes.
- **Multi-Repository & Per-Package Target Configuration**: Manage multiple source repositories simultaneously and assign custom target directories per package (e.g., `/etc` or custom system paths).
- **Global & Package File Filtering (`ignore`)**: Manage `.symignore` glob patterns at repository root or package level with inheritance and redundant pattern detection.
- **Symlink Health & Integrity Audit (`check-symlinks`)**: Scans repository for broken symlinks and target home for unmanaged orphan symlinks.
- **Safety & Signal Cleanups**: Built-in signal handlers (`SIGINT`/`Ctrl+C`) perform atomic temp directory cleanups on unexpected exits.
- **Nanosecond Performance Profiling (`-p`, `--profile`)**: High-precision nanosecond execution profiling for benchmarking deployment steps.

---

## Architecture & Design Rationale

### Architectural Decision Records (ADRs)

Key technical decisions in `symdep` are documented in detail within [`docs/decisions/`](docs/decisions/):

- **[ADR-001: Zero-Dependency ISO C17 Architecture](docs/decisions/ADR-001-zero-dependency-c17-architecture.md)** — Rationale for choosing ISO C17 over Perl (GNU Stow), Python, or Rust/Go for execution speed and portability in minimal system environments.
- **[ADR-002: Dynamic Symlink Unfolding Engine](docs/decisions/ADR-002-symlink-unfolding-and-collision-engine.md)** — Design of the automatic directory unfolding mechanism to prevent symlink tree folding collisions.
- **[ADR-003: Cross-Distro Package & Plugin Registry Engine](docs/decisions/ADR-003-cross-distro-package-and-plugin-registry.md)** — Multi-distro package manager detection, privilege elevation abstraction (`sudo`, `doas`, `tsu`), and custom tool registry mapping.
- **[ADR-004: AST / Shebang Code-Analysis Dependency Scanner Engine](docs/decisions/ADR-004-static-analysis-dependency-scanner.md)** — Static code parsing for shebangs and tool calls to eliminate manual manifest creation.
- **[ADR-005: Unified Command Dispatch Table](docs/decisions/ADR-005-unified-command-dispatch-table.md)** — Centralized routing architecture supporting multi-namespace, colon-separated, and legacy GNU Stow command syntaxes.
- **[ADR-006: Hierarchical Ignore Rule Engine](docs/decisions/ADR-006-hierarchical-ignore-rule-engine.md)** — Glob pattern matching, inheritance across global and package `.symignore` files, and redundancy warnings.
- **[ADR-007: Dual-Driver Asynchronous Symlink Execution Engine](docs/decisions/ADR-007-dual-driver-asynchronous-symlink-execution-engine.md)** — Linux `io_uring` kernel submission queue driver & POSIX pthread work-stealing pool adaptive execution engine (optimized for Ext4 single-directory mutex locking vs Btrfs/XFS concurrent metadata batch writes).
- **[ADR-008: TOCTOU-Safe Atomic Symlink Replacement](docs/decisions/ADR-008-toctou-safe-atomic-symlink-replacement.md)** — Atomic temporary symlink creation and `renameat` replacement to eliminate TOCTOU race conditions.
- **[ADR-009: Targeted Traversal & Two-Pass Directory Creation](docs/decisions/ADR-009-targeted-traversal-and-two-pass-directory-creation.md)** — Pre-creation of target directory tree hierarchy upfront to eliminate thread lock contention and redundant per-file `mkdir_p` syscalls.
- **[ADR-010: Process-Level Lookup Caching Engine](docs/decisions/ADR-010-process-level-lookup-caching-engine.md)** — In-memory caching for `$PATH` binary lookups and registry configuration memoization.

For complete module-by-module developer function signatures, data structures, and contract specifications, see the **[C API Reference Guide](docs/API.md)**.

### Target Directory Precedence

When resolving the target destination for symlink deployment, `symdep` checks sources in strict order of precedence:

1. **CLI Flag**: `-t, --target-dir <path>`
2. **Package Manifest**: `TARGET="/path"` entry in `.symdeps` (when evaluating a specific package)
3. **Environment Variable**: `SYMDEP_TARGET_DIR` or `TARGET_DIR`
4. **Configuration File**: `TARGET_DIR` set via `symdep config set --target <path>`
5. **Fallback Environment**: `$HOME`

### Source Repository Precedence

When locating the active source repository directory:

1. **CLI Flag**: `-d, --source-dir <path>` (aliases: `--src-dir`, `--dotfiles-dir`)
2. **Environment Variable**: `SYMDEP_SOURCE_DIR` or `SOURCE_DIR`
3. **Working Directory Marker**: Current working directory if `symdep.registry` or `.symdepregistry` is present
4. **Configuration File**: Primary entry in `SOURCE_DIRS` set via `symdep config set --source <path>`
5. **Fallback Environment**: Current working directory (`getcwd`)

---

## Build & Installation

### Standard Build Targets

```bash
# Build release binary (bin/symdep)
make

# Run unit test suite (79 unit tests)
make test

# Run end-to-end integration feature tests (6 test suites)
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

### Clang Optimization & Sanitizer Profiles

The build system includes pre-configured targets for performance tuning, sanitizer instrumentation, and static analysis:

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

| Flag | Long Option / Aliases | Description |
| :--- | :--- | :--- |
| `-d` | `--source-dir`, `--src-dir`, `--dotfiles-dir` | Set source repository directory for current command. |
| `-t` | `--target-dir` | Set target home directory for current command (e.g., `-t ~/`). |
| `-m` | `--manager`, `--pkg-mgr`, `--package-manager` | Override active package manager for current command (e.g., `-m yay`). |
| `-i` | `--interactive` | Launch interactive wizard for scanner dependency confirmation. |
| `-y` | `--install` | Auto-confirm installation of missing dependencies & optional plugins without prompting. |
| `-n` | `--dry-run` | Preview disk changes, symlink creations, backups, and actions without modifying disk. |
| `-s` | `--save` | Save command-line directory overrides (`-d`/`-t`) directly to user configuration file. |
| `-p` | `--profile`, `--perf`, `--performance`, `--profiler` | Enable nanosecond execution profiler logging (also enabled via `SYMDEP_PROFILE=1`). |
| `-h` | `--help` | Display comprehensive help manual. |

---

### Symlink & Deployment Commands

```bash
# Link / deploy one or multiple packages (with automatic dependency & conflict handling)
symdep link <pkg...>
# Aliases: stow, deploy

# Shorthand invocation (omitting command keyword defaults to linking valid packages)
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

Namespace: `pkg` (aliases: `package`). Supports space-separated (`pkg create`), colon-separated (`pkg:create`), and top-level shorthand syntaxes.

```bash
# Scaffold a new package directory & initialize a default .symdeps manifest
symdep pkg create <name>
# Aliases: pkg:create, package:create, make:pkg

# Safely unlink and remove package directory from disk
symdep pkg remove <name...>
# Aliases: pkg:remove, package:remove, pkg:rm, remove, delete

# List all packages with active symlink status ([LINKED], [PARTIAL], [UNLINKED])
symdep pkg list
# Aliases: pkg:list, package:list, pkg:show, list, ls
```

---

### Dependency Management (`deps`)

Namespace: `deps`. Supports space-separated (`deps add`) and colon-separated (`deps:add`) syntaxes.

```bash
# Add a dependency or conflict entry to package .symdeps manifest
symdep deps add <pkg> <dep> [--required | --optional | --conflict]
# Aliases: deps:add (default classification: --optional)

# Edit existing dependency classification
symdep deps edit <pkg> <dep> <type>
# Aliases: deps:edit, deps:set (type: --required, --optional, or --conflict)

# Remove a dependency or conflict entry from package manifest
symdep deps remove <pkg> <dep>
# Aliases: deps:remove, deps:rm, remove, delete

# Display raw .symdeps manifest contents for a package
symdep deps show <pkg>
# Aliases: deps:show, deps:list

# Set per-package target directory override in package manifest
symdep deps target <pkg> <path>
# Aliases: deps:target

# Recursively scan package scripts/configs to auto-detect missing tools & plugins (AST scanner)
symdep scan [pkg...] [-i | -n | -y]
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
# Aliases: ignore:remove, ignore:rm, delete

# Purge .symignore file(s) for package(s) or repository root
symdep ignore clear [pkg...]
# Aliases: ignore:clear, ignore:purge

# Display active .symignore rules with redundancy warnings
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

Namespace: `config`. Manages settings in `~/.config/symdep/config`.

```bash
# Display active configuration, source repositories, and target directory
symdep config show
# Aliases: config:show, config:list, config:get, show, list, get

# Set primary source repository, default target directory, package manager, or elevation tool
symdep config set --manager <name>
symdep config set --elevation <tool>
symdep config set --target <path>
symdep config set --source <path>
# Aliases: config:set, config:target, config:source

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

Located inside individual package directories (e.g. `~/dotfiles/hyprland/.symdeps`). Legacy `.stowdeps` files are supported as fallbacks.

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

### Tool & Plugin Registry (`symdep.registry`)

Optionally placed in source repository root (`symdep.registry` or `.symdepregistry`, fallback `stow.registry`) to map tool names to distro package manager names and shell plugin locations.

```ini
# Distro package name overrides (tool@distro=package_name)
neovim@arch=neovim
neovim@ubuntu=neovim
fd@ubuntu=fd-find
bat@ubuntu=batcat
ripgrep@debian=ripgrep

# Tool alias & shell plugin path mapping (tool=alias1|alias2|plugin:~/.zsh/plugins/tool)
zsh-autosuggestions=plugin:~/.zsh/plugins/zsh-autosuggestions
bat=bat|batcat
```

---

### Global User Configuration (`~/.config/symdep/config`)

Manages global settings in `~/.config/symdep/config` (or `$XDG_CONFIG_HOME/symdep/config`).

```ini
TARGET_DIR=/home/user
SOURCE_DIRS=/home/user/dotfiles:/home/user/dotfiles-work
PACKAGE_MANAGER=pacman
ELEVATION_TOOL=sudo
```

---

## Environment Variables

| Variable | Description |
| :--- | :--- |
| `SYMDEP_SOURCE_DIR` / `SOURCE_DIR` / `DOTFILES_DIR` | Override active source repository directory path. |
| `SYMDEP_TARGET_DIR` / `TARGET_DIR` | Override target destination home directory path. |
| `SYMDEP_PROFILE` / `PROFILE` / `STOW_PROFILE` | Set to non-empty string to enable execution profiling. |
| `HOME` | Default target home directory when no override is configured. |
| `XDG_CONFIG_HOME` | Primary directory for `symdep/config` (`~/.config`). |
| `XDG_CONFIG_DIRS` | System-wide search directories for `symdep/config`. |
| `NO_COLOR` | Disable ANSI color codes in terminal output. |

---

## Contributing

Contributions are welcome! Please read [`CONTRIBUTING.md`](CONTRIBUTING.md) for details on code style (ISO C17, `clang-format`), testing procedures (`make test`, `make test-feature`), static analysis (`make tidy`), and pull request guidelines.

---

## License

Licensed under the [GNU General Public License v3.0](LICENSE).
