# ADR-008: Targeted Traversal & Two-Pass Parallel Directory Hierarchy Pre-Creation

## Status
Accepted

## Date
- **Decision / Commit Date**: 2026-08-13 (Commit `a5d57c5`, `8f6cd50`, `3cc101c`)
- **Recorded Date**: 2026-08-13

## Context
When linking dotfile packages into multi-layered destination directories (e.g. `~/.config/hypr/scripts/`), invoking directory creation (`mkdir_p`) on a per-file basis inside concurrent threadpool workers causes thread contention, lock collisions, and redundant `stat()` / `mkdir()` system call overhead across worker threads.

Furthermore, scanning the entire target home directory (`readdir()`) for every package deployment scales poorly when target home folders contain tens of thousands of unrelated files.

## Decision
Implement a **Two-Pass Parallel Linker Engine & Targeted Traversal**:

1. **Pass 1: Directory Hierarchy Pre-Creation**:
   - Before launching parallel symlink worker tasks, `symdep` performs a fast single-threaded pre-pass (`relink_package` & `link_package`) that collects all unique target subdirectories required by the incoming package and creates the directory tree hierarchy upfront (`mkdir_p`).
2. **Pass 2: Parallel Concurrent File Symlinking**:
   - Worker tasks execute symlink operations concurrently in parallel across pre-created target directories, completely eliminating per-file `mkdir_p` calls, directory creation locks, and cross-thread directory creation race conditions (`a5d57c5`).
3. **Targeted Traversal (`walk_target_dir_symlinks_targeted`)**:
   - During targeted package deployment, `symdep` skips scanning the entire target home directory. Instead, it probes only the relative paths owned by the specific package, drastically reducing directory traversal overhead (`8f6cd50`).

## Alternatives Considered

### Mutex-Protected `mkdir_p` in Worker Threads
- **Pros**: Handles missing directories on demand.
- **Cons**: High lock contention; worker threads frequently block waiting for directory creation locks.
- **Rejected**: Two-pass pre-creation separates directory structure setup from file linking, maximizing parallelism.

## Consequences
- Completely eliminates directory creation race conditions in parallel threadpool workers ([ADR-007](ADR-007-dual-driver-asynchronous-symlink-execution-engine.md)).
- Integrates seamlessly with dynamic unfolding ([ADR-002](ADR-002-symlink-unfolding-and-collision-engine.md)) and flat in-place dentry exchange ([ADR-012](ADR-012-flat-in-place-unfolding-vs-multi-hop-indirection.md)).
- Eliminates thousands of redundant filesystem checks and locks during deployment.
- Maximizes I/O throughput for multi-core parallel linking workloads.
