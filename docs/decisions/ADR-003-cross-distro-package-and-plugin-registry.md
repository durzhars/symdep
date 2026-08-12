# ADR-003: Cross-Distro Package Manager & Shell Plugin Registry Engine

## Status
Accepted

## Date
- **Decision / Commit Date**: 2026-08-12 (Commit `8af3b6f`)
- **Recorded Date**: 2026-08-12

## Context
System package names for CLI utilities and tools vary across Linux distributions and package managers:
- `bat` on Arch Linux is named `batcat` on Debian/Ubuntu.
- `fd` on Arch Linux is named `fd-find` on Debian/Ubuntu.
- Package managers (`pacman`, `apt`, `dnf`, `apk`, `brew`) require different CLI flags for non-interactive dependency installation (e.g. `pacman -S --noconfirm`, `apt-get install -y`, `dnf install -y`, `apk add`, `brew install`).
- Privilege elevation mechanisms differ between Linux environments (`sudo`, `doas`) and Android/Termux environments (`tsu`, `none`).
- Non-package manager dependencies (e.g. shell plugins like `zsh-autosuggestions`) exist in custom filesystem paths rather than package manager databases.

## Decision
Design a multi-layered **Package Manager & Registry Abstraction Layer**:
1. **Dynamic Package Manager Discovery**: Built-in detection for `pacman`, `apt`, `dnf`, `apk`, `brew`, `nix-env`, `yay`, `paru`, and custom managers, complete with non-interactive flags.
2. **Non-Root & Unprivileged Environment Handling**:
   - **Root Elevation Probe**: Before invoking package management commands, `symdep` checks if privilege elevation is actually required using POSIX context (`geteuid() == 0`, `mgr->requires_root`, or `is_binary_writable_by_user()`).
   - **Unprivileged Managers**: User-space package managers (`brew`, `nix-env`, `yay`, `paru`, `flatpak`) execute directly as the current user without requesting elevation.
   - **Writable Prefixes & Termux**: If binary paths or install directories are writable by the active unprivileged user (such as Termux on Android, user-local prefix `/home/user/.local/bin`, or custom chroots), elevation tool prefixing is automatically bypassed.
   - **Elevation Tool Detection**: When root permissions are strictly required, `symdep` dynamically detects available elevation helpers (`sudo`, `tsu`, `doas`, `su`) or respects user config (`symdep config set --elevation <tool>`).
   - **Elevation Failure Fallback**: If elevation tools are unavailable or fail, `symdep` aborts execution gracefully and prints the exact manual command line required for the user to execute.
3. **Tool Registry (`symdep.registry`)**: Support for root-level tool registries that map virtual tool names to distribution-specific package names (e.g., `bat@ubuntu=batcat`, `fd@ubuntu=fd-find`).
4. **Plugin Path Mapping**: Support for `plugin:~/.zsh/plugins/tool` entries to verify non-package-manager shell dependencies.

## Alternatives Considered

### Per-Distribution Manifest Files
- **Pros**: Explicit separation.
- **Cons**: Multiplies maintenance burden (`.symdeps.arch`, `.symdeps.ubuntu`, `.symdeps.macos`).
- **Rejected**: Breaks portable dotfile repositories.

### Hardcoded Package Name Translations
- **Pros**: Zero configuration files needed.
- **Cons**: Inflexible; cannot support custom user package repos or obscure distros.
- **Rejected**: Centralized tool registry `symdep.registry` allows both defaults and custom overrides.

## Consequences
- Single `.symdeps` manifest works across Arch Linux, Ubuntu, Fedora, Alpine, macOS, Termux, and unprivileged user environments.
- `-y` / `--install` flag enables unattended environment bootstrapping across all supported operating systems.
- Unprivileged users can deploy packages without requiring `sudo` privileges when using user-space package managers or writable system prefixes.
- Shell plugins in custom directories are recognized and checked alongside binary executables.
