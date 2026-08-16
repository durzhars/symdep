# ADR-010: Dynamic Privilege Elevation & Writable-Prefix Probing

## Status
Accepted

## Date
- **Decision / Commit Date**: 2026-08-12 (Commit `b457c2f`, `c885331`)
- **Recorded Date**: 2026-08-16

## Context
Package managers on UNIX systems operate under heterogeneous security paradigms:
1. **System-Level Package Managers (`pacman`, `apt`, `dnf`, `apk`, `zypper`)**: Require superuser privileges (`root` / `sudo`) to modify system root directories (`/usr`, `/var/lib`).
2. **User-Space Package Managers (Homebrew, Nix User Profile, Termux `pkg`)**: Manage user-owned prefix directories (e.g. `/home/linuxbrew/.linuxbrew`, `~/.nix-profile`, `/data/data/com.termux/files/usr`). In fact, running Homebrew under `sudo` is explicitly prohibited by Homebrew and results in fatal execution errors.
3. **Containerized / Embedded Root Environments**: Environments running directly as `root` (e.g. Docker containers, rescue chroots) do not have or require `sudo`.

Relying on hardcoded OS or distro checks (e.g. `#ifdef __APPLE__` or checking `ID=ubuntu` in `/etc/os-release`) fails in hybrid environments, such as running Homebrew on Ubuntu, Termux on Android, or unprivileged chroot environments.

Key requirements:
- Automatically determine whether privilege escalation is required without hardcoding OS assumptions.
- Prevent executing user-space package managers (like Homebrew) under `sudo`.
- Support multiple privilege elevation tools (`sudo`, `tsu`, `doas`, `su`, or `none`).
- Ensure elevation commands are properly escaped and safe against format string vulnerabilities (`c69bfc9`).

## Decision
Implement a **Dynamic Privilege Elevation and Writable-Prefix Probing Architecture** in `src/core/pkg_manager.c` (`pkg_manager_get_elevation_tool`):

1. **Permission-Based Root Detection Cascade**:
   - **Step 1 (Explicit Configuration)**: If user configured `SYMDEP_ELEVATION_TOOL` or `config set --elevation <tool>` (or set it to `none`), respect the user choice immediately.
   - **Step 2 (Active EUID Check)**: If `geteuid() == 0` (already executing as root), no elevation tool prefix is added.
   - **Step 3 (Metadata Flag)**: If the package manager definition specifies `requires_root == false`, elevation is bypassed.
   - **Step 4 (Writable-Prefix Probing `is_binary_writable_by_user`)**: Resolves the executable path of the package manager binary on `$PATH` and tests write access on its parent directory using `access(parent_dir, W_OK)`. If the current user has write access to the binary prefix (e.g. `~/.linuxbrew/bin`), root elevation is bypassed automatically.
2. **Multi-Tool Elevation Resolver**:
   - When elevation is required, `symdep` probes `$PATH` in order: `sudo` -> `tsu` (Termux root) -> `doas` (OpenBSD / Alpine) -> `su`.
   - In interactive mode, `symdep` presents a selection menu allowing users to choose their preferred elevation method or run unprivileged.
3. **Compound Command Elevation & Format String Safety**:
   - For multi-step installer definitions (e.g. repository refresh followed by install), each sub-command is individually elevated or enclosed in single-quoted subshells to prevent privilege drops across piped commands.

## Alternatives Considered

### Hardcoding OS / Distro Flags
- **Pros**: Simple conditional checks.
- **Cons**: Breaks Linuxbrew on Linux, rootless Nix, and Termux on Android.
- **Rejected**: Brittle across modern user-space package management ecosystems.

### Blindly Prepending `sudo` to All Package Managers
- **Pros**: Minimal code.
- **Cons**: Fatal error with Homebrew (`Error: Running Homebrew as root is extremely dangerous...`).
- **Rejected**: Incompatible with user-space package managers.

## Consequences
- Transparent execution across Linux, macOS, BSD, Termux, and Docker containers.
- Zero configuration required for Homebrew, Nix, and custom user prefixes.
- Clean integration with alternative elevation utilities (`doas`, `tsu`).
