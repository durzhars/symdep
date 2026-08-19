# ADR-011: Multi-Repository Federation & Hierarchical Cascading Precedence

## Status
Accepted

## Date
- **Decision / Commit Date**: 2026-08-11 (Commit `877c857`, `b176f3d`)
- **Recorded Date**: 2026-08-16

## Context
Complex dotfile setups often diverge across organizational boundaries and filesystem access levels:
1. **Multi-Repository Workflows**: Developers frequently maintain multiple separate dotfile repositories (e.g. `~/dotfiles-personal` for personal desktop customizations and `~/dotfiles-work` for employer-specific VPN, SSH, and proxy configs).
2. **System vs. User Destinations**: While most dotfile packages target `$HOME` (e.g. `hyprland`, `neovim`, `zsh`), system-level configurations require deployment directly to `/etc` or custom system directories (e.g. `/etc/kanata/`, `/etc/udev/rules.d/`, `/etc/systemd/system/`).
3. **Ambiguity in Source and Target Locations**: When running commands without explicit `-d` or `-t` flags, the tool must deterministically locate source repositories and target directories without prompting.

Key requirements:
- Support managing multiple independent dotfiles repositories concurrently without manual directory hopping.
- Allow individual packages to specify custom target paths (e.g. `/etc/custom`) within their package manifest (`.symdeps`).
- Provide an unambiguous, deterministic precedence order for directory resolution across CLI flags, manifests, environment variables, configuration files, and defaults.

## Decision
Implement a **Multi-Repository Federation and Hierarchical Precedence Engine** in `src/core/config/config_active.c` and `src/core/config/config_ops.c`:

1. **Two-Dimensional Precedence Model**:

   #### Source Repository Resolution Cascade (`get_active_source_dir`):
   1. **CLI Flag**: `-d, --source-dir <path>` (aliases: `--src-dir`, `--dotfiles-dir`).
   2. **Environment Variable**: `SYMDEP_SOURCE_DIR` / `SOURCE_DIR` / `DOTFILES_DIR`.
   3. **Current Working Directory Marker**: `getcwd()` if containing `symdep.registry` or `.symdepregistry`.
   4. **User Configuration**: Primary entry in `SOURCE_DIRS` list in `~/.config/symdep/config`.
   5. **Fallback Default**: `~/dotfiles` (or current working directory).

   #### Target Destination Directory Resolution Cascade (`get_active_target_dir_for_pkg`):
   1. **CLI Flag**: `-t, --target-dir <path>`.
   2. **Package Manifest**: `TARGET="/path"` entry in package `.symdeps`.
   3. **Environment Variable**: `SYMDEP_TARGET_DIR` / `TARGET_DIR`.
   4. **User Configuration**: `TARGET_DIR` in `~/.config/symdep/config`.
   5. **Fallback Default**: `$HOME` (resolved via `src/utils/env.c` / [ADR-001](ADR-001-zero-dependency-c17-architecture.md); enforces a clean fatal exit if `$HOME` is unset without an explicit `-t` target override).

2. **Multi-Repository Federation (`SOURCE_DIRS`)**:
   - `symdep config add <path>` and `symdep config remove <path>` maintain a colon-separated list of active repository paths in the user configuration.
   - Commands such as `symdep all` or `symdep link <pkg>` search across federated repositories sequentially, enabling unified management of personal, work, and shared packages.
3. **Save-On-Command Persistence (`-s`, `--save`)**:
   - Passing `-s` / `--save` alongside `-d` or `-t` flags automatically persists the resolved paths into `~/.config/symdep/config`, updating the default configuration without needing an explicit `symdep config set` invocation.

## Alternatives Considered

### Single Repository Restriction (GNU Stow Model)
- **Pros**: Simpler directory discovery.
- **Cons**: Forces users into monolithic dotfile repositories or running multiple Stow commands with nested `-d` arguments.
- **Rejected**: Multi-repository setups are a core requirement for enterprise and multi-role developers.

### Symlinks to System Targets via `sudo stow` Only
- **Pros**: Reuses same home directory logic.
- **Cons**: Requires executing entire tool as root, misplacing user configuration files into `/root/.config`.
- **Rejected**: Manifest-level `TARGET="/etc/..."` enables per-package target routing without compromising user-space configuration storage.

## Consequences
- Clean separation of personal and work dotfiles repositories under unified CLI management.
- Per-package system destination overrides without running user configuration management as root.
- Deterministic, auditable directory resolution behavior across all execution contexts.
