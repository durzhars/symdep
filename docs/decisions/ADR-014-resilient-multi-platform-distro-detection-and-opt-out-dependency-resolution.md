# ADR-014: Resilient Multi-Platform OS Detection, Ecosystem Family Aliasing, and Opt-Out Dependency Resolution

## Status
Accepted

## Date
- **Decision Date**: 2026-08-20
- **Recorded Date**: 2026-08-20

## Context
Cross-platform dotfiles management spans heterogeneous UNIX operating systems, including standard Linux distributions (Debian, Ubuntu, Arch, Fedora, Alpine), non-standard filesystems (Android/Termux, NixOS), BSDs (FreeBSD, OpenBSD, NetBSD), and macOS. 

Historically, dependency resolution faced several edge cases:
1. **Brittle OS Detection via `/etc/os-release`**:
   - Non-FHS environments (e.g. Termux at `$PREFIX/etc/os-release`), BSDs, and macOS lack `/etc/os-release`.
   - Stripped container images, chroots, or environments with tampered/modified `os-release` files caused OS detection to silently fail or misidentify the active platform.
2. **Namespace Divergence Across Distro Repositories**:
   - Package managers (e.g., `apt`) are decoupled from repository package namespaces. A package may be named `fd-find` on Debian/Ubuntu, `fd` on Arch, and `fdfind` elsewhere.
   - Flat aliasing risked false-positive cross-repo contamination between upstream and downstream forks.
3. **Prompt Fatigue in Interactive Dependency Installation**:
   - The interactive installer previously operated on an allowlist (defaulting to `none`), requiring users to manually select every package to install.
   - When a user explicitly typed a corrected package name in interactive mode, asking a redundant secondary confirmation to save the rule caused prompt fatigue.

## Decision

### 1. 6-Tier Resilient Multi-Platform OS Detection Cascade
Implement a multi-tier fallback in `get_distro_id()` in `src/utils/env.c`:
1. **Environment Variable Override**: Inspect `$SYMDEP_DISTRO` and `$DISTRO_ID` (for CI/CD and container overrides).
2. **Prefix & Android/Termux Detection**: Parse `$PREFIX/etc/os-release`, detect `com.termux` in `$PREFIX`, and probe `$ANDROID_ROOT`/`/system/bin/sh`.
3. **Freedesktop / Systemd Standard Paths**: Check `/etc/os-release`, `/usr/lib/os-release`, `/usr/share/os-release`, and `/etc/initrd-release`.
4. **Legacy Linux Distribution Markers**: Probe `/etc/arch-release`, `/etc/debian_version`, `/etc/fedora-release`, `/etc/alpine-release`, `/etc/void-release`, `/etc/gentoo-release`, `/etc/SuSE-release`, and `/etc/slackware-version`.
5. **Native Preprocessors & POSIX `uname()` Fallback**: Evaluate `__APPLE__` (`macos`), `__FreeBSD__` (`freebsd`), `__OpenBSD__` (`openbsd`), `__NetBSD__` (`netbsd`), and `uname(&utsname).sysname`.
6. **Generic Catch-All**: Fallback to `"unix"`.

### 2. Ecosystem Family Alias Resolution & Executable Grounding
Bridge the package manager engine (`pkg_manager.c`) and registry engine (`registry.c`):
- **Executable Grounding**: Resolution is anchored in the executable binary present on `$PATH` (`apt`, `pacman`, `dnf`, `apk`, `brew`, `pkg`), ensuring that tampered `os-release` files cannot mislead package manager execution.
- **Upstream Ancestry Family Traversal**: Define directed ecosystem families:
  - `apt` / `apt-get` $\rightarrow$ `debian`, `ubuntu`, `pop`, `mint`, `kali`, `raspbian`
  - `pacman` / `yay` / `paru` $\rightarrow$ `arch`, `manjaro`, `endeavouros`, `artix`
  - `dnf` / `yum` $\rightarrow$ `fedora`, `rhel`, `centos`, `rocky`, `almalinux`
  - `apk` $\rightarrow$ `alpine`
  - `xbps-install` $\rightarrow$ `xbps`, `void`
  - `brew` $\rightarrow$ `homebrew`, `macos`, `darwin`
  - `pkg` $\rightarrow$ `termux`, `android`
  - `pkg_add` / `pkgin` $\rightarrow$ `freebsd`, `openbsd`, `netbsd`, `dragonfly`
- **Lookup Precedence**:
  1. Exact Distro Tag (`tool@<distro_id>`, e.g., `fd@ubuntu`)
  2. Family Tag Match (`tool@<family_alias>`, e.g., `fd@debian`)
  3. System Distro Probe Match (`tool@<sys_distro>`)
  4. Raw Universal Tool Name (`tool`, e.g., `fd`)

### 3. Opt-Out (Exclusion-Based) Interactive Menu
Reorient `symdep deps install` / `symdep install` interactive prompts:
- **Default Assumption**: Assume the user wants to install all missing dependencies (`[all]`).
- **Exclusion Syntax**: Users press `Enter` to install all, or enter items to exclude (`1,3`, `!fzf`, `-1`, `except 2`, `none`).
- **Inclusion Override**: Explicit `only 1,2` or `include 1,2` syntax remains supported.

### 4. Targeted Error Isolation & Direct Interactive Auto-Registration
- **Targeted Error Isolation**: When a batch package manager command fails, `symdep` captures output and matches failed package tokens against `.symdeps` entries. Prompts are presented **strictly for the specific packages that failed** rather than badgering the user for every attempted tool.
- **Direct Interactive Auto-Registration**:
  - When the user types a replacement package name (e.g. `fd-find`), `symdep` immediately writes `<tool>@<distro> = <pkg_name>` to `symdep.registry` (and the package manager alias) and retries installation without redundant confirmation prompts.
  - In standard mode, pressing `Enter` without input skips registering that specific tool.
- **Strict / No-Skip Mode (`-S` / `--no-skip` / `--strict`)**:
  - In `--no-skip` mode, pressing `Enter` on a prompt assumes the default tool name is what the user intended for that distro, and records `<tool>@<distro> = <tool>` (with duplicate-skipping).
- **Headless / Non-Interactive Safety**: Non-interactive (CI/headless) executions remain strictly non-blocking, non-interactive, and non-mutating.

## Consequences

- **Universal UNIX Portability**: Seamless operation across Linux, Android 16/Termux, macOS, BSDs, and minimal container images without crashing or skipping OS resolution.
- **Immunity to Modified `os-release`**: Package execution is bound to verified binaries on `$PATH`, eliminating false-positive resolution on custom or bare-metal environments.
- **Frictionless Interactive Bootstrap**: Users install all packages by default with single-keypress confirmation (`Enter`) and natural exclusion filters.
- **Self-Documenting In-Repo Knowledge**: Interactive package name corrections persist directly into the version-controlled `symdep.registry`, enabling automated zero-touch setup on subsequent machine clones.
