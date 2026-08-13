# ADR-010: Process-Level Environment & Tool Lookup Caching Engine

## Status
Accepted

## Date
- **Decision / Commit Date**: 2026-08-13 (Commit `7ebe025`, `a65fb32`)
- **Recorded Date**: 2026-08-13

## Context
Dependency verification (`symdep check`, `symdep scan`) and package manager resolution probe `$PATH` directories and system registries for dozens of binary executables and plugins. When evaluating multiple packages or dependencies in a single run, repeated directory scanning (`find_executable_in_path`) and configuration file parsing incur hundreds of redundant `stat()` and `access()` system calls, degrading performance.

## Decision
Implement a **Process-Level Caching Engine**:

1. **PATH Executable Location Caching (`utils/env.c`)**:
   - Caches discovered binary paths in process memory upon first lookup. Subsequent requests for the same binary (e.g. checking `bash` across 10 packages) return cached results instantly without issuing disk I/O syscalls.
2. **Registry File & Package Manager Memoization (`core/registry.c` & `core/pkg_manager.c`)**:
   - Memoizes resolved `symdep.registry` file paths and package manager configurations, eliminating repeated file reads and text parsing during multi-package operations (`a65fb32`).

## Alternatives Considered

### Un-cached Disk Probing
- **Pros**: Zero state maintained in memory.
- **Cons**: High I/O overhead; issuing duplicate `stat()` calls for common binaries (`bash`, `git`, `zsh`) across multiple package checks.
- **Rejected**: In-memory caching provides sub-microsecond lookup times for repeated queries.

## Consequences
- Eliminates hundreds of failing or duplicate disk system calls during multi-package scanning and dependency checking.
- Sub-microsecond response times for cached executable and registry queries.
- Clean process-lifetime memory management with explicit cache cleanup upon process completion.
