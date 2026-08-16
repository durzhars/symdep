# ADR-012: Flat In-Place Directory Unfolding vs. Multi-Hop Symlink Indirection

## Status
Accepted

## Date
- **Decision / Commit Date**: 2026-08-16
- **Recorded Date**: 2026-08-16

## Context
When resolving directory folding collisions (e.g. converting a folded `~/.config` symlink into a multi-package directory), there are two architectural paradigms available:

1. **Multi-Hop Container Indirection ($O(2)$ Traversal)**:
   - Create a hidden container directory in state storage (e.g. `~/.local/state/symdep/unfolded/config/`).
   - Populate child symlinks inside the container pointing to respective source repositories.
   - Replace `~/.config` with a top-level symlink pointing to the container directory.
2. **Flat In-Place Unfolding ($O(1)$ Direct Lookup)**:
   - Replace `~/.config` in-place with a concrete real directory.
   - Populate child symlinks directly inside `~/.config/` pointing to their respective source package files.

### Critical Failure Modes of Multi-Hop Indirection ($O(2)$)
While Multi-Hop Indirection allows atomic swapping on legacy POSIX systems via standard `rename(symlink, symlink)`, it introduces critical operational risks in real-world desktop and server environments:

- **Canonical Path & `realpath()` Fragility**: Many applications, shells, and runtime environments (e.g., Python `__file__`, Node.js module resolution, Neovim `stdpath('config')`, Git credential helpers, shell scripts using `$(dirname "$0")`) rely on non-recursive `readlink()` or string-prefix comparisons against `$HOME`. A two-hop symlink resolves to an intermediate state store directory (`~/.local/state/...`), confusing applications expecting direct canonical source paths.
- **Performance Overhead ($O(2)$ VFS Resolution)**: Every path lookup incurs double symlink dereferencing and multi-inode traversal across the VFS dentry cache.
- **Hidden State & Cleanup Leaks**: Requires managing external auxiliary state directories. If state directories are purged or desynchronized, target symlinks become broken dangling pointers.
- **User Ergonomics**: Running `ls -la ~` reveals confusing indirection paths to hidden state directories rather than a clean, intuitive directory hierarchy.

## Decision
Adopt **Flat In-Place Directory Unfolding ($O(1)$ Direct Lookup)** as the standard architecture in `symdep`:

1. **Flat Native Hierarchy**: Target destination directories (such as `~/.config` or `~/.local/bin`) are converted directly into real concrete directories. Child files are symlinked directly to their source packages (1-hop canonical lookup).
2. **Kernel Dentry Exchange Acceleration**:
   - On Linux ($\ge 3.15$), use raw `renameat2(AT_FDCWD, tmp_dir, AT_FDCWD, symlink_path, RENAME_EXCHANGE)` to achieve zero-window, strictly atomic physical dentry swapping in the VFS.
   - On macOS / Darwin, use `renameatx_np(..., RENAME_EXCHANGE)` on APFS/HFS+ filesystems.
3. **Fault-Tolerant Legacy POSIX Fallback**:
   - On legacy operating systems without kernel exchange primitives, `symdep` pre-populates all child symlinks in a PID-isolated staging path (`.unfold_tmp_PID`) registered with async-signal trap handlers before executing the minimal 2-syscall sequence (`unlink` $\to$ `rename`).
   - This bounds the non-existence window to a single sub-microsecond syscall boundary while preserving clean $O(1)$ flat hierarchy ergonomics and zero application path breakage.

## Alternatives Considered

### Multi-Hop Container Indirection
- **Pros**: Allows single-call `rename()` on legacy POSIX systems without kernel exchange APIs.
- **Cons**: Breaks applications with single-level `readlink` assumptions, introduces $O(2)$ VFS lookup overhead, clutters `$HOME` with indirection aliases, and creates auxiliary state management complexity.
- **Rejected**: Operational stability, $O(1)$ lookup performance, and application compatibility far outweigh legacy multi-hop workarounds.

## Consequences
- **Maximum Application Compatibility**: All user tools and scripts receive clean, canonical 1-hop path resolution without `readlink` confusion.
- **Optimal VFS Lookup Performance**: $O(1)$ direct dentry traversal without secondary inode dereferencing.
- **Zero Hidden State**: Target home directories remain self-contained with zero auxiliary cache files.
- **Strict Atomicity on Modern Systems**: Full zero-window atomic swapping on Linux ($\ge 3.15$), macOS, and FreeBSD 14+.
