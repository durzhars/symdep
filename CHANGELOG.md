# Changelog

All notable changes to the `symdep` project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.0] - 2026-08-15

### Core Architecture & Performance
- **Zero-Dependency ISO C17 Engine**: Native C implementation with zero external library or runtime dependencies.
- **Linux `io_uring` Asynchronous Kernel Backend**: SQE ring-buffer submission for ultra-high-throughput batch symlink creation on modern Linux kernels.
- **TOCTOU Race Condition Safety**: Atomic symlink replacement and path validation to guarantee security against Time-of-Check to Time-of-Action race conditions.
- **Multi-Level System Caching**: Fast in-memory caching of `PATH` executables, package manager states, and registry files to eliminate hundreds of redundant disk `stat` syscalls.
- **Nanosecond Performance Profiler (`-p`, `--profile`)**: High-precision execution timing for debugging deployment workloads and benchmarking execution passes.
- **Sterilized RAMDisk Benchmark Suite**: Benchmarking suite evaluating performance up to `--mega` scale (100,000 symlinks) against GNU Stow and Dotbot.

### Complete CLI Command Matrix

#### Core Lifecycle Commands
- `link <pkg...>` *(aliases: `stow`, `deploy`)*: Deploy package symlinks into target directories.
- `unlink <pkg...>` *(alias: `unstow`)*: Safely remove package symlinks.
- `relink <pkg...>` *(alias: `restow`)*: Perform non-destructive atomic relinking (unlink followed by link).
- `all`: Deploy all package dotfiles available in the source repository.
- `diff [pkg...]`: Dry-run preview of pending symlinks, file backups, and missing dependencies.
- `scan [pkg...]`: Static analysis engine scanning scripts, shebangs, and config files to auto-detect required tools and plugins.

#### Package Management (`pkg`)
- `pkg create <name>`: Scaffold a new package directory with default `.symdeps` manifest template.
- `pkg remove <name...>` *(alias: `remove`)*: Safely unstow and delete package directories.
- `pkg list`: Display tabular status of packages (`[LINKED]`, `[PARTIAL]`, `[UNLINKED]`).

#### Dependency Management (`deps`)
- `deps add <pkg> <dep>`: Register package dependencies (`--required`, `--optional`, `--conflict`).
- `deps edit <pkg> <dep>`: Modify existing dependency classification or requirements.
- `deps remove <pkg> <dep>` *(alias: `deps rm`)*: Remove dependency or conflict rules.
- `deps show <pkg>`: Inspect raw `.symdeps` manifest declaration.
- `deps target <pkg> <path>`: Set per-package target home directory override in `.symdeps`.

#### File Filtering & Ignore Rules (`ignore`)
- `ignore init [pkg...]`: Scaffold global (`~/.symignore`) or package-level `.symignore` pattern files.
- `ignore add [pkg] <pat...>`: Append glob ignore patterns to package or global `.symignore`.
- `ignore remove [pkg] <pat>`: Remove glob pattern from `.symignore` (`-g` for global).
- `ignore clear [pkg...]`: Purge `.symignore` files for specified packages or global repository root.
- `ignore show [pkg...]` *(alias: `ignore list`)*: Display active merged ignore patterns.

#### Diagnostics & Collision Repair
- `check [pkg...]`: Verify dependency resolution and symlink health for packages.
- `check symlinks`: Audit target directory for broken symlinks or unmanaged orphan files.
- `fix` *(alias: `fix-conflicts`)*: Unfold target directory symlinks to resolve folding collisions automatically.

#### Configuration Management (`config`)
- `config show`: View active runtime configuration and package manager preferences.
- `config set [OPTIONS]`: Update settings (`-m/--manager`, `-e/--elevation`, `-t/--target`, `-d/--source`).
- `config add <path>`: Register an additional source dotfiles repository (multi-repo support).
- `config remove <path>`: Unregister a source repository from global configuration.

### Global CLI Flags & Switches
- `-d, --source-dir <path>`: Override primary source repository directory (`--src-dir`, `--dotfiles-dir`).
- `-t, --target-dir <path>`: Override target home directory.
- `-m, --manager <name>`: Override package manager (`pacman`, `apt`, `dnf`, `apk`, `brew`).
- `-i, --interactive`: Interactive terminal selection menu for auto-discovered package dependencies.
- `-y, --install`: Automatically confirm installation of missing system dependencies.
- `-n, --dry-run`: Preview disk operations without modifying filesystem state.
- `-s, --save`: Persist CLI directory overrides (`-d`/`-t`) to configuration.
- `-p, --profile`: Print execution profiler diagnostics and nanosecond metrics.

### System Integration & Portable Build Profiles
- **Cross-Distro Package & Tool Registry (`symdep.registry`)**: Map virtual dependency names to package manager names (`bat@ubuntu=batcat`) and shell plugin paths (`plugin:~/.zsh/plugins/tool`).
- **Privilege Elevation Abstraction**: Native support for `sudo`, `doas`, `tsu`, or unprivileged execution.
- **Ultra-Small Musl Static Build (`make static-musl`)**: Generates ~230KB static binary with direct `/etc/passwd` parsing fallback (`-DNO_NSS_FALLBACK`).
- **Release Distribution Packaging (`make dist`)**: Automated creation of release `.tar.gz` distribution tarballs and SHA256 verification hashes.
- **Advanced Compiler Profiles**: Makefile targets for ThinLTO (`build-clang-opt`), 2-stage PGO (`build-pgo`), AddressSanitizer (`build-sanitize`), and size optimization (`build-size`).
