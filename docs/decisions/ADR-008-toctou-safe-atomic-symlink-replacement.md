# ADR-008: TOCTOU-Safe Atomic Symlink Replacement & Directory Traversal

## Status
Accepted

## Date
- **Decision / Commit Date**: 2026-08-13 (Commit `6d1fa9f`)
- **Recorded Date**: 2026-08-13

## Context
Filesystem operations that separate checking for path state (`lstat`, `access`) from modifying the path (`symlink`, `unlink`) suffer from Time-of-Check to Time-of-Use (TOCTOU) race conditions. In multi-threaded execution or shared directory environments, concurrent processes or malicious actors could swap target symlinks or parent directories mid-operation, causing operations to hijack target locations, corrupt files, or overwrite arbitrary files outside the target directory.

## Decision
Enforce **TOCTOU-Safe Atomic Symlink Operations**:

1. **Unique Temporary File Symlinks**:
   - Symlinks are initially constructed under unique temporary filenames (`.symdep_tmp_XXXXXX`) inside the target directory using POSIX `symlinkat()`.
2. **Atomic Path Replacement (`renameat`)**:
   - The temporary symlink is atomically renamed over the destination target path using POSIX `renameat()` (or `RENAME_EXCHANGE` / `RENAME_NOREPLACE` flags where supported).
   - This ensures the target file path transitions instantly from its old state to the new symlink in a single atomic filesystem kernel transaction.
3. **Descriptor-Based Safe Traversal**:
   - Operations on parent and child paths utilize POSIX file descriptor functions (`openat`, `fstatat`, `unlinkat`, `symlinkat`) to prevent path component swapping during traversal.

## Alternatives Considered

### Direct `unlink()` followed by `symlink()`
- **Pros**: Simple 2-step sequence.
- **Cons**: Creates a window of time where the destination target path does not exist, leaving the target path vulnerable to symlink race condition hijacking or broken application lookups mid-deployment.
- **Rejected**: Non-atomic and vulnerable to TOCTOU race conditions in multi-threaded environments.

## Consequences
- Guaranteed zero-window atomic path replacement during symlink deployment.
- Protection against symlink hijacking and path traversal vulnerabilities in multi-threaded parallel execution.
- Safe concurrent deployment even under multi-core parallel workloads.
