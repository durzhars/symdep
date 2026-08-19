# ADR-002: Dynamic Symlink Unfolding and Collision Resolution Engine

## Status
Accepted

## Date
- **Decision / Commit Date**: 2026-08-02 (Commit `f42dad0`)
- **Recorded Date**: 2026-08-12

## Context
Standard symlink deployment tools (like GNU Stow) create top-level directory symlinks when linking target folders (e.g., `~/.config -> ~/dotfiles/hyprland/.config`). This behavior, known as "directory folding", creates critical conflicts when multiple packages install into subdirectories of the same target path (e.g. `hyprland` installing `~/.config/hypr` and `waybar` installing `~/.config/waybar`).

If `~/.config` is folded as a single symlink to `hyprland/.config`, linking `waybar` will either fail with a collision error or write directly into `hyprland`'s repository source directory.

## Decision
Implement a dynamic **Symlink Unfolding Engine** in `symdep` (`unfold_directory_symlinks` and `symdep fix-conflicts`). When `symdep` detects an existing directory symlink in the target destination that needs to accept stowed files from another package:
1. It automatically replaces the top-level directory symlink in target with a real directory.
2. It recreates individual symlinks for existing files pointing back to their original source repository paths.
3. It creates new file symlinks for the incoming package.

## Alternatives Considered

### Strict Rejection on Collision (GNU Stow default)
- **Pros**: Simple logic; leaves disk untouched.
- **Cons**: Forces manual user intervention (`mkdir`, manual un-stowing, manual re-stowing) every time a new package shares a common parent directory like `~/.config` or `~/.local/bin`.
- **Rejected**: Terrible user experience for modern desktop dotfile management.

### Always Tree Folding (Deep File Symlinking Only)
- **Pros**: Prevents directory folding entirely.
- **Cons**: Creates hundreds of individual file symlinks unnecessarily when only one package resides in a target folder.
- **Rejected**: Less clean target filesystem layout when top-level directory folding is safe.

## Consequences
- **Automated Collision Recovery**: `symdep link` dynamically resolves tree folding collisions during deployment.
- **Manual Maintenance Tool**: `symdep fix-conflicts` (and standalone `symdep fix`) provides explicit user control to scan and unfold directory symlinks on demand.
- **Atomic Operations & Flat Hierarchy**: Unfolding operations utilize flat in-place directory unfolding with kernel dentry exchange acceleration ([ADR-012](ADR-012-flat-in-place-unfolding-vs-multi-hop-indirection.md)) and upfront directory pre-creation ([ADR-008](ADR-008-targeted-traversal-and-two-pass-directory-creation.md)) to ensure zero file loss, lock-free directory traversal, and atomic state transitions.
