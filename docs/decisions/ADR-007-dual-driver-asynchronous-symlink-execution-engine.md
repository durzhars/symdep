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
2. **POSIX Pthread Worker Pool Driver (`utils/thread_pool.h` & `src/utils/thread_pool.c`)**:
   - Manages a pool of worker threads using a mutex-and-condition-variable-synchronized FIFO task queue.
   - Executes the primary `AtomicLinkBatch` pipeline (`linker_ops.c`), where worker threads atomically claim file items via lock-free `atomic_fetch_add(&batch->current_index, 1)` with zero per-file heap allocation or queue contention.
3. **Runtime Driver Fallback & VFS Pipeline Abstraction**:
   - The file and directory linking loop serves as the core atomic execution pipeline, while `io_uring` (vectorized ring submissions on Linux) and the `pthread` pool (atomic batch workers across POSIX platforms) act as interchangeable drivers targeting the underlying Virtual Filesystem (VFS).
   - Automatically probes Linux `io_uring` kernel capability at runtime; falls back seamlessly to the POSIX worker pool or single-threaded POSIX execution if unsupported or restricted.
   - **Atomic Concurrency Guarantee**: In concurrent worker execution, stale or colliding symlinks are replaced via temporary atomic symlinks (`.symdep_tmp_PID`) and `renameat()` to eliminate race conditions and broken intermediate states.
   - **Filesystem Metadata Locking Characteristics**:
     - *Ext4 Single-Directory Mutex*: On traditional Ext4 filesystems with single-directory write locking (`ext4_add_entry` `i_rwsem`), POSIX ThreadPool with pre-created directory hierarchies ([ADR-008](ADR-008-targeted-traversal-and-two-pass-directory-creation.md)) minimizes kernel `io-wq` thread lock contention.
     - *Modern Concurrent Filesystems (Btrfs, XFS, F2FS)*: On filesystems with lock-free/concurrent B-tree metadata allocation, `io_uring` delivers maximum throughput by vectorizing submission queue entries without per-file syscall context switches.

## Alternatives Considered

### Relying on External `liburing`
- **Pros**: Simplifies low-level ring setup.
- **Cons**: Introduces a mandatory external shared library dependency.
- **Rejected**: Violates zero-dependency architecture principles (ADR-001).

### Per-File Task Queue Allocation vs. Atomic Batch Index Distribution
- **Pros**: Pushing individual file tasks into a general-purpose thread pool queue handles arbitrary task units.
- **Cons**: Creates severe mutex contention and dynamic heap allocation (`malloc`/`free`) overhead for every individual file when deploying thousands of tiny symlinks.
- **Rejected**: Instead of queuing individual file tasks, `symdep` spawns 1 task per worker thread and uses `AtomicLinkBatch` with `atomic_fetch_add(&batch->current_index, 1)` in `linker_ops.c` to achieve zero per-file allocations and zero task-queue locks during batch deployment.

### OpenMP / POSIX Threads Only
- **Pros**: Standard cross-platform threading.
- **Cons**: Does not eliminate per-file syscall overhead on Linux kernels when deploying thousands of tiny symlinks.
- **Rejected**: Dual-driver approach provides maximum performance on Linux via `io_uring` while preserving POSIX fallback portability.

## Consequences
- Benchmark workloads demonstrate multi-fold execution speedups for large dotfile deployments.
- Maintained strict zero-dependency ISO C17 architecture.
- Seamless performance fallback across Linux, macOS, BSD, and containerized systems.
