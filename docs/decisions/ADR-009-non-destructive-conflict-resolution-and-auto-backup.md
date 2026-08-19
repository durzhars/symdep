# ADR-009: Non-Destructive Conflict Resolution & Automated Backup Strategy

## Status
Accepted

## Date
- **Decision / Commit Date**: 2026-08-12 (Commit `4753a55`, `5db3173`)
- **Recorded Date**: 2026-08-16

## Context
When deploying dotfiles to target directories (typically `$HOME`), target destinations frequently contain pre-existing regular files or directories created by distribution installers, desktop environments, or prior manual edits (e.g. `~/.bashrc`, `~/.config/mimeapps.list`).

Existing tools handle destination file collisions in two polarizing ways:
1. **Hard Abort (GNU Stow)**: GNU Stow aborts immediately with an unrecoverable error (`stow: cannot stow ... existing target is not a symlink`), forcing the user to manually inspect, rename, or delete every conflicting file one by one.
2. **Destructive Force-Overwrite (Dotbot / `ln -sf`)**: Overwrite flags blindly clobber existing regular files, permanently deleting uncommitted user modifications, API tokens, or local customizations without recourse.

Key requirements for `symdep`:
- Zero data loss: Existing user configuration files must never be clobbered or permanently deleted.
- Non-interactive friction reduction: First-time dotfile adoption on clean or existing OS installs must succeed automatically without requiring manual file renaming.
- Auditable and previewable: Users must be able to inspect pending backups before applying them (`symdep diff`, `symdep -n link`).
- Re-ingestion prevention: Backup files must not be accidentally tracked or re-deployed during future stow passes.

## Decision
Implement an automated, **Non-Destructive Conflict Resolution and Timestamped Backup Engine** in `src/core/linker/linker_conflicts.c` and `src/core/linker/linker_ops.c`:

1. **Target Conflict Classification**:
   - **Self-Pointing Symlink**: If target is already a symlink pointing to the incoming package source file, it is marked unchanged and skipped.
   - **Stale / Foreign Managed Symlink**: If target is a symlink pointing to an older version or another managed package, it is atomically replaced via temporary symlink (`.symdep_tmp_PID`) and `renameat()` to eliminate race conditions.
   - **Non-Symlink Regular File / Directory Collision**: If target is a regular file or directory, `symdep` automatically backs it up before proceeding.
2. **Unique Timestamped Backup Path Construction (`build_unique_backup_path`)**:
   - Backs up conflicting target files to `.symdep_backup_YYYYMMDD_HHMMSS` (or incremental counter `.symdep_backup_YYYYMMDD_HHMMSS.N` if collisions occur within the same second).
   - Preserves all file metadata and permissions.
3. **Dry-Run & Diff Preview**:
   - `symdep diff` and `symdep -n link` emit warning diagnostics detailing exact backup destinations without modifying filesystem state:
     `[DRY-RUN] Conflict! Would backup file: ~/.bashrc -> ~/.bashrc.symdep_backup_20260816_161500`
4. **Template-Based Ignore Rule Integration**:
   - Initialized `.symignore` templates ([ADR-006](ADR-006-hierarchical-ignore-rule-engine.md)) include `*.symdep_backup_*`, preventing backup artifacts from being re-ingested into package trees and enabling modular `.symignore` parser reuse across backup lifecycle tools.

## Alternatives Considered

### Aborting on Conflict (GNU Stow Paradigm)
- **Pros**: Strict safety by doing nothing.
- **Cons**: High user friction during onboarding or dotfile restructuring.
- **Rejected**: Forces tedious manual intervention.

### Prompting Interactively per Conflicting File
- **Pros**: Explicit user confirmation for every file.
- **Cons**: Breaks automated non-interactive deployment scripts (e.g. post-install shell scripts, NixOS provisioning, cloud-init).
- **Rejected**: Auto-backup provides equal safety while preserving automated script execution.

## Consequences
- Guaranteed zero data loss during dotfile deployment.
- Seamless onboarding experience on fresh or existing OS installations.
- Users can easily diff or restore original configurations from `.symdep_backup_*` files.

## Future Architectural Extensions

To provide complete end-to-end lifecycle management for backups, the following dedicated subcommands are planned under the `backup` namespace:

1. **`backup list [pkg...]`**:
   - Discovers and audits all `.symdep_backup_*` files across the target directory, grouped by package ownership or target subdirectory.
2. **`backup restore <pkg>`**:
   - Non-destructive rollback: Removes active symlinks for `<pkg>` and atomically restores the most recent `.symdep_backup_*` artifact to its original target location.
3. **`backup prune [pkg...]`**:
   - Audits and purges stale backup artifacts (with optional `--keep <N>` or `--older-than <days>` retention flags) to prevent cumulative disk clutter.
