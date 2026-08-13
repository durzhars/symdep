# ADR-007: Dual-Driver Asynchronous Symlink Execution Engine

## Status
Accepted

## Date
- **Decision / Commit Date**: 2026-08-13 (Commit `c08b2ff`, `2d6c1b2`)
- **Recorded Date**: 2026-08-13

## Context
Deploying large dotfile packages (e.g. complex Neovim/Emacs setups, custom fonts, or desktop environment configs with hundreds or thousands of files) sequentially via single-threaded POSIX `symlink()` calls creates severe I/O bottlenecks and high system call latency overhead.

Key requirements for performance enhancement:
- Asynchronous batch submission of symlink operations to eliminate kernel syscall overhead per file on Linux.
- Portable, zero-allocation parallel worker execution on non-Linux POSIX platforms (macOS, BSD, embedded POSIX).
- Zero external library dependencies (must not require `liburing` or external threading libraries beyond standard `pthreads`).
- Fallback capability for containerized or unprivileged environments where asynchronous features may be restricted.

## Decision
Implement a **Dual-Driver Asynchronous Execution Engine** in `symdep`:

1. **Linux `io_uring` Async Driver (`utils/io_uring_backend.h` & `src/utils/io_uring_backend.c`)**:
   - Uses direct Linux raw system calls (`sys_io_uring_setup`, `sys_io_uring_enter`) using `IORING_OP_SYMLINKAT` opcodes without linking external `liburing`.
   - Submits symlink creation requests in batches to the Linux kernel ring buffer, processing completions asynchronously.
   - Utilizes thread-local ring buffer reuse and static batch memory structures to avoid memory allocation overhead and mmap tearing (`d5c605e`).
2. **POSIX Pthread Work-Stealing Pool Driver (`utils/thread_pool.h` & `src/utils/thread_pool.c`)**:
   - Provides a zero-allocation, lock-free/atomic work-stealing pthread pool for non-Linux operating systems or environments without `io_uring` support.
   - Distributes package linking tasks across available CPU cores concurrently.
3. **Runtime Driver Fallback**:
   - Automatically probes Linux `io_uring` kernel capability at runtime; falls back seamlessly to POSIX worker thread pool or single-threaded POSIX execution if unsupported or restricted.

## Alternatives Considered

### Relying on External `liburing`
- **Pros**: Simplifies low-level ring setup.
- **Cons**: Introduces a mandatory external shared library dependency.
- **Rejected**: Violates zero-dependency architecture principles (ADR-001).

### OpenMP / POSIX Threads Only
- **Pros**: Standard cross-platform threading.
- **Cons**: Does not eliminate per-file syscall overhead on Linux kernels when deploying thousands of tiny symlinks.
- **Rejected**: Dual-driver approach provides maximum performance on Linux via `io_uring` while preserving POSIX fallback portability.

## Consequences
- Benchmark workloads demonstrate multi-fold execution speedups for large dotfile deployments.
- Maintained strict zero-dependency ISO C17 architecture.
- Seamless performance fallback across Linux, macOS, BSD, and containerized systems.
