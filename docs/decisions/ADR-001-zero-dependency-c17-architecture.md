# ADR-001: Zero-Dependency ISO C17 Architecture

## Status
Accepted

## Date
- **Decision / Commit Date**: 2026-08-01 (Commit `dc43e17`)
- **Recorded Date**: 2026-08-12

## Context
Traditional dotfile symlink management utilities (such as GNU Stow) and dotfile package managers rely on dynamic scripting languages (Perl, Python, Bash) or runtime environments. In modern system administration, container environments, and degraded systems (e.g. initial bootstrap, chroots, minimal embedded Linux, or macOS), interpreters or heavy runtimes may be unavailable, broken, or slow to initialize.

Key requirements for `symdep`:
- Zero external runtime or library dependencies (standard C library and POSIX APIs only).
- Ultra-fast startup time (<1 ms) and execution performance.
- Cross-platform portability across Linux distributions (Debian, Arch, Fedora, Alpine) and macOS.
- Low memory footprint for system deployment tasks.
- Atomic file operations and strict signal safety.

## Decision
Implement `symdep` completely in native **ISO C17** (`-std=c17`) using POSIX runtime interfaces and custom utility abstractions without external dynamic link libraries.

## Alternatives Considered

### GNU Stow (Perl)
- **Pros**: Established standard for dotfile symlinking.
- **Cons**: Requires Perl runtime; slow startup; no built-in cross-distro package dependency resolution; sensitive to directory folding collisions.
- **Rejected**: Lacks built-in dependency management and requires a full Perl installation.

### Python-Based Tool
- **Pros**: Rich ecosystem, fast prototyping, built-in string/path parsing.
- **Cons**: High startup latency (~30-50 ms vs sub-millisecond in C); requires Python interpreter and environment management.
- **Rejected**: High startup overhead and external dependency on Python environment.

### Rust / Go Binary
- **Pros**: Memory safety guarantees (Rust), built-in concurrency (Go).
- **Cons**: Larger binary sizes (Go ~10MB+), complex cross-toolchain bootstrapping in minimal environments.
- **Rejected**: ISO C17 produces binary footprints <100 KB with no runtime requirements and universal GCC/Clang compiler support.

## Consequences
- **Build System**: Requires standard C toolchain (GCC or Clang) and GNU Make.
- **Memory Management**: Explicit memory management using custom safe allocation wrappers (`safe_malloc`, `safe_realloc`) and strict leak hygiene.
- **Signal Safety & Abort Cleanups**: Strict adherence to signal safety. Process signal handlers for `SIGINT`, `SIGTERM`, and `SIGHUP` are registered via POSIX `sigaction` (`include/utils/signal.h`). Temporary directories and scratch files registered during linking operations (`register_temp_path`) are cleaned up via signal-safe handlers (`cleanup_temp_paths_signal_safe`) upon unexpected interruption (`Ctrl+C`), guaranteeing zero orphaned temporary files or corrupted symlink states on disk.
- **Standalone Static Distribution & Portability**: Supports producing standalone static binaries (`make static-musl`, ~230KB) with zero shared library dependencies. To guarantee portability in minimal containers (Docker `scratch`/Alpine) or cron jobs without `$HOME` or glibc dynamic NSS modules (`libnss_*.so`), `symdep` includes an autonomous direct `/etc/passwd` parser fallback (`src/utils/env.c`).
- **Performance & In-Memory Memoization**: Nanosecond-level execution times; sub-microsecond binary lookups via process-level memoization of `$PATH` queries and registry descriptors.
- **Portability**: Compiles cleanly with zero compiler warnings under `-Wall -Wextra -Werror -pedantic`.
