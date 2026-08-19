/*
 * Symlink & Dependency Manager (symdep)
 * Linux io_uring Asynchronous Batch Symlink Backend Implementation
 * Copyright (C) 2026 durzhars
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include "utils/io_uring_backend.h"

#include <errno.h>
#include <fcntl.h>
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "utils/fs.h"
#include "utils/logger.h"
#include "utils/mem.h"
#include "utils/path.h"

#ifdef __linux__

#if defined(__has_include)
#if __has_include(<linux/io_uring.h>)
#include <linux/io_uring.h>
#define HAVE_LINUX_IO_URING_H 1
#endif
#endif

#ifndef HAVE_LINUX_IO_URING_H
struct io_sqring_offsets {
    uint32_t head;
    uint32_t tail;
    uint32_t ring_mask;
    uint32_t ring_entries;
    uint32_t flags;
    uint32_t dropped;
    uint32_t array;
    uint32_t resv1;
    uint64_t resv2;
};

struct io_cqring_offsets {
    uint32_t head;
    uint32_t tail;
    uint32_t ring_mask;
    uint32_t ring_entries;
    uint32_t overflow;
    uint32_t cqes;
    uint32_t flags;
    uint32_t resv1;
    uint64_t resv2;
};

struct io_uring_params {
    uint32_t sq_entries;
    uint32_t cq_entries;
    uint32_t flags;
    uint32_t sq_thread_cpu;
    uint32_t sq_thread_idle;
    uint32_t features;
    uint32_t wq_fd;
    uint32_t resv[3];
    struct io_sqring_offsets sq_off;
    struct io_cqring_offsets cq_off;
};

struct io_uring_sqe {
    uint8_t opcode;
    uint8_t flags;
    uint16_t ioprio;
    int32_t fd;
    union {
        uint64_t off;
        uint64_t addr2;
    };
    union {
        uint64_t addr;
        uint64_t splice_off_in;
    };
    uint32_t len;
    union {
        uint32_t rw_flags;
        uint32_t fsync_flags;
        uint16_t poll_events;
        uint32_t poll32_events;
        uint32_t sync_range_flags;
        uint32_t msg_flags;
        uint32_t timeout_flags;
        uint32_t accept_flags;
        uint32_t cancel_flags;
        uint32_t open_flags;
        uint32_t statx_flags;
        uint32_t fadvise_advice;
        uint32_t splice_flags;
        uint32_t rename_flags;
        uint32_t unlink_flags;
        uint32_t hardlink_flags;
        uint32_t xattr_flags;
        uint32_t msg_ring_flags;
        uint32_t uring_cmd_flags;
    };
    uint64_t user_data;
    union {
        struct {
            union {
                uint16_t buf_index;
                uint16_t buf_group;
            };
            uint16_t personality;
            int32_t splice_fd_in;
        };
        uint64_t __pad2[3];
    };
};

struct io_uring_cqe {
    uint64_t user_data;
    int32_t res;
    uint32_t flags;
};
#endif

#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/syscall.h>

#ifndef __NR_io_uring_setup
#define __NR_io_uring_setup 425
#endif

#ifndef __NR_io_uring_enter
#define __NR_io_uring_enter 426
#endif

#ifndef __NR_io_uring_register
#define __NR_io_uring_register 427
#endif

#ifndef IORING_OP_SYMLINKAT
#define IORING_OP_SYMLINKAT 38
#endif

#ifndef IORING_ENTER_GETEVENTS
#define IORING_ENTER_GETEVENTS (1U << 0)
#endif

#ifndef IORING_OFF_SQ_RING
#define IORING_OFF_SQ_RING 0ULL
#endif

#ifndef IORING_OFF_CQ_RING
#define IORING_OFF_CQ_RING 0x8000000ULL
#endif

#ifndef IORING_OFF_SQES
#define IORING_OFF_SQES 0x10000000ULL
#endif

static sigjmp_buf g_probe_sigsys_jmp;
static volatile sig_atomic_t g_probe_sigsys_caught = 0;

static void probe_sigsys_handler(int signo)
{
    (void)signo;
    g_probe_sigsys_caught = 1;
    siglongjmp(g_probe_sigsys_jmp, 1);
}

static bool is_running_on_android(void)
{
#if defined(__ANDROID__)
    return true;
#else
    if (getenv("ANDROID_ROOT") || getenv("ANDROID_DATA") || getenv("ANDROID_STORAGE")) {
        return true;
    }
    const char *prefix = getenv("PREFIX");
    if (prefix && strstr(prefix, "com.termux")) {
        return true;
    }
    if (access("/system/bin/sh", F_OK) == 0 && access("/system/etc/seccomp_policy", F_OK) == 0) {
        return true;
    }
    return false;
#endif
}

typedef struct {
    int ring_fd;
    void *sq_ptr;
    void *cq_ptr;
    struct io_uring_sqe *sqes;
    size_t sq_size;
    size_t cq_size;
    size_t sqes_size;

    uint32_t *sq_head;
    uint32_t *sq_tail;
    uint32_t *sq_ring_mask;
    uint32_t *sq_array;

    uint32_t *cq_head;
    uint32_t *cq_tail;
    uint32_t *cq_ring_mask;
    struct io_uring_cqe *cqes;
    uint32_t entries;
} IoUringRing;

static inline size_t page_align_size(size_t size)
{
    long page_size = sysconf(_SC_PAGESIZE);
    if (page_size <= 0) {
        page_size = 4096;
    }
    return (size + (size_t)page_size - 1) & ~((size_t)page_size - 1);
}

static bool init_io_uring(IoUringRing *ring, uint32_t entries)
{
    memset(ring, 0, sizeof(*ring));
    struct io_uring_params p;
    memset(&p, 0, sizeof(p));

    int ring_fd = (int)syscall(__NR_io_uring_setup, entries, &p);
    if (ring_fd < 0) {
        return false;
    }

    ring->ring_fd = ring_fd;
    ring->entries = p.sq_entries;

    ring->sq_size = page_align_size(p.sq_off.array + p.sq_entries * sizeof(uint32_t));
    ring->cq_size = page_align_size(p.cq_off.cqes + p.cq_entries * sizeof(struct io_uring_cqe));
    ring->sqes_size = page_align_size(p.sq_entries * sizeof(struct io_uring_sqe));

    ring->sq_ptr =
        mmap(0, ring->sq_size, PROT_READ | PROT_WRITE, MAP_SHARED, ring_fd, IORING_OFF_SQ_RING);
    ring->cq_ptr =
        mmap(0, ring->cq_size, PROT_READ | PROT_WRITE, MAP_SHARED, ring_fd, IORING_OFF_CQ_RING);
    ring->sqes = (struct io_uring_sqe *)mmap(
        0, ring->sqes_size, PROT_READ | PROT_WRITE, MAP_SHARED, ring_fd, IORING_OFF_SQES);

    if (ring->sq_ptr == MAP_FAILED || ring->cq_ptr == MAP_FAILED || ring->sqes == MAP_FAILED) {
        if (ring->sq_ptr != MAP_FAILED)
            munmap(ring->sq_ptr, ring->sq_size);
        if (ring->cq_ptr != MAP_FAILED)
            munmap(ring->cq_ptr, ring->cq_size);
        if (ring->sqes != MAP_FAILED)
            munmap(ring->sqes, ring->sqes_size);
        close(ring_fd);
        return false;
    }

    ring->sq_head = (uint32_t *)((char *)ring->sq_ptr + p.sq_off.head);
    ring->sq_tail = (uint32_t *)((char *)ring->sq_ptr + p.sq_off.tail);
    ring->sq_ring_mask = (uint32_t *)((char *)ring->sq_ptr + p.sq_off.ring_mask);
    ring->sq_array = (uint32_t *)((char *)ring->sq_ptr + p.sq_off.array);

    ring->cq_head = (uint32_t *)((char *)ring->cq_ptr + p.cq_off.head);
    ring->cq_tail = (uint32_t *)((char *)ring->cq_ptr + p.cq_off.tail);
    ring->cq_ring_mask = (uint32_t *)((char *)ring->cq_ptr + p.cq_off.ring_mask);
    ring->cqes = (struct io_uring_cqe *)((char *)ring->cq_ptr + p.cq_off.cqes);

    return true;
}

static void free_io_uring(IoUringRing *ring)
{
    if (ring->ring_fd >= 0) {
        if (ring->sq_ptr && ring->sq_ptr != MAP_FAILED)
            munmap(ring->sq_ptr, ring->sq_size);
        if (ring->cq_ptr && ring->cq_ptr != MAP_FAILED)
            munmap(ring->cq_ptr, ring->cq_size);
        if (ring->sqes && ring->sqes != MAP_FAILED)
            munmap(ring->sqes, ring->sqes_size);
        close(ring->ring_fd);
        ring->ring_fd = -1;
    }
}

bool io_uring_is_supported(void)
{
    if (is_running_on_android()) {
        /* Android App / Zygote Seccomp policy strictly blocks io_uring with SIGSYS */
        return false;
    }

    struct sigaction sa, old_sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = probe_sigsys_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    g_probe_sigsys_caught = 0;
    if (sigaction(SIGSYS, &sa, &old_sa) != 0) {
        return false;
    }

    volatile bool supported = false;
    if (sigsetjmp(g_probe_sigsys_jmp, 1) == 0) {
        IoUringRing ring;
        if (init_io_uring(&ring, 4)) {
            const char *tmp = getenv("TMPDIR");
            static char termux_tmp[STOW_PATH_LARGE];
            if (!tmp || *tmp == '\0') {
                tmp = getenv("PREFIX");
                if (tmp) {
                    join_path(termux_tmp, sizeof(termux_tmp), tmp, "tmp");
                    tmp = termux_tmp;
                }
            }
            if (!tmp || access(tmp, W_OK) != 0) {
                tmp = "/tmp";
            }

            char test_src[STOW_PATH_LARGE];
            char test_tgt[STOW_PATH_LARGE];
            char fname_src[64];
            char fname_tgt[64];
            snprintf(fname_src, sizeof(fname_src), ".symdep_uring_probe_src_%d", (int)getpid());
            snprintf(fname_tgt, sizeof(fname_tgt), ".symdep_uring_probe_tgt_%d", (int)getpid());
            join_path(test_src, sizeof(test_src), tmp, fname_src);
            join_path(test_tgt, sizeof(test_tgt), tmp, fname_tgt);

            FS_UNLINK(test_tgt);
            FS_UNLINK(test_src);

            int fd = FS_OPEN(test_src, O_CREAT | O_WRONLY, 0644);
            if (fd >= 0) {
                close(fd);

                struct io_uring_sqe *sqe = &ring.sqes[0];
                memset(sqe, 0, sizeof(*sqe));
                sqe->opcode = IORING_OP_SYMLINKAT;
                sqe->fd = AT_FDCWD;
                sqe->addr = (uint64_t)test_src;
                sqe->addr2 = (uint64_t)test_tgt;

                ring.sq_array[0] = 0;
                atomic_store_explicit((_Atomic uint32_t *)ring.sq_tail, 1, memory_order_release);

                int ret = (int)syscall(
                    __NR_io_uring_enter, ring.ring_fd, 1, 1, IORING_ENTER_GETEVENTS, NULL, 0);
                if (ret >= 0 && *ring.cq_head != *ring.cq_tail) {
                    struct io_uring_cqe *cqe = &ring.cqes[*ring.cq_head & *ring.cq_ring_mask];
                    if (cqe->res == 0) {
                        supported = true;
                    }
                }

                FS_UNLINK(test_src);
                FS_UNLINK(test_tgt);
            }
            free_io_uring(&ring);
        }
    } else {
        /* SIGSYS was caught during probe -> seccomp blocked io_uring */
        supported = false;
    }

    sigaction(SIGSYS, &old_sa, NULL);
    return supported && !g_probe_sigsys_caught;
}

static inline void fast_path_join(char *out, const char *dir, size_t dlen, const char *rel)
{
    memcpy(out, dir, dlen);
    char *p = out + dlen;
    if (dlen > 0 && p[-1] != '/') {
        *p++ = '/';
    }
    size_t rlen = strlen(rel);
    memcpy(p, rel, rlen + 1);
}

static _Thread_local IoUringRing g_tls_ring;
static _Thread_local bool g_tls_ring_inited = false;
static _Thread_local bool g_tls_ring_failed = false;

typedef struct {
    char target_path[STOW_PATH_LARGE];
    char pkg_file_path[STOW_PATH_LARGE];
    const char *rel_path;
} PendingLink;

static _Thread_local PendingLink *g_tls_batch = NULL;

int io_uring_link_batch(const PkgFileList *files, PackageContext *ctx)
{
    if (!files || files->count == 0) {
        return 0;
    }

    if (g_tls_ring_failed) {
        return -1;
    }

    uint32_t queue_depth = 256;
    if (!g_tls_ring_inited) {
        if (init_io_uring(&g_tls_ring, queue_depth)) {
            g_tls_ring_inited = true;
        } else {
            g_tls_ring_failed = true;
            return -1;
        }
    }
    IoUringRing *ring = &g_tls_ring;

    if (!g_tls_batch) {
        g_tls_batch = (PendingLink *)safe_calloc(queue_depth, sizeof(PendingLink));
    }
    PendingLink *batch = g_tls_batch;

    int target_dfd = FS_OPEN(ctx->target_dir, O_RDONLY | O_DIRECTORY);
    if (target_dfd < 0) {
        target_dfd = AT_FDCWD;
    }

    size_t target_dir_len = strlen(ctx->target_dir);
    size_t pkg_dir_len = strlen(ctx->pkg_dir);

    size_t total = files->count;
    size_t file_idx = 0;

    while (file_idx < total) {
        uint32_t queued = 0;

        while (file_idx < total && queued < queue_depth) {
            const char *rel_path = files->entries[file_idx].rel_path;
            file_idx++;

            if (is_path_ignored(rel_path, &ctx->raw_ignores)) {
                continue;
            }

            PendingLink *link = &batch[queued];
            link->rel_path = rel_path;
            fast_path_join(link->target_path, ctx->target_dir, target_dir_len, rel_path);
            fast_path_join(link->pkg_file_path, ctx->pkg_dir, pkg_dir_len, rel_path);

            TargetResolveResult res = resolve_target_conflict_or_replace(
                link->target_path, link->pkg_file_path, link->rel_path);
            if (res == TARGET_RESOLVE_ALREADY_LINKED) {
                continue;
            }
            if (res == TARGET_RESOLVE_REPLACED) {
                atomic_fetch_add_explicit(&ctx->created_count, 1, memory_order_relaxed);
                continue;
            }
            if (res == TARGET_RESOLVE_ERROR) {
                atomic_fetch_add_explicit(&ctx->errors, 1, memory_order_relaxed);
                continue;
            }

            // Enqueue SQE
            uint32_t tail = *ring->sq_tail;
            uint32_t index = tail & *ring->sq_ring_mask;
            struct io_uring_sqe *sqe = &ring->sqes[index];
            memset(sqe, 0, sizeof(*sqe));
            sqe->opcode = IORING_OP_SYMLINKAT;
            sqe->fd = target_dfd;
            sqe->addr = (uint64_t)link->pkg_file_path;
            sqe->addr2 = (uint64_t)link->target_path;
            sqe->user_data = queued;

            ring->sq_array[index] = index;
            atomic_store_explicit(
                (_Atomic uint32_t *)ring->sq_tail, tail + 1, memory_order_release);
            queued++;
        }

        if (queued == 0) {
            continue;
        }

        // Single kernel syscall submission for the whole batch!
        int submitted = (int)syscall(
            __NR_io_uring_enter, ring->ring_fd, queued, queued, IORING_ENTER_GETEVENTS, NULL, 0);
        if (submitted < 0) {
            log_error("io_uring_enter failed: %s", strerror(errno));
            atomic_fetch_add_explicit(&ctx->errors, (int)queued, memory_order_relaxed);
            break;
        }

        // Process completions
        uint32_t head = *ring->cq_head;
        uint32_t cq_tail = *ring->cq_tail;
        while (head != cq_tail) {
            struct io_uring_cqe *cqe = &ring->cqes[head & *ring->cq_ring_mask];
            if (cqe->res == 0) {
                atomic_fetch_add_explicit(&ctx->created_count, 1, memory_order_relaxed);
            } else {
                size_t b_idx = (size_t)cqe->user_data;
                log_error("io_uring symlink failed (target=%s -> pkg=%s) cqe_res=%d: %s",
                          batch[b_idx].target_path,
                          batch[b_idx].pkg_file_path,
                          cqe->res,
                          strerror(-cqe->res));
                atomic_fetch_add_explicit(&ctx->errors, 1, memory_order_relaxed);
            }
            head++;
        }
        *ring->cq_head = head;
    }

    if (target_dfd >= 0 && target_dfd != AT_FDCWD) {
        close(target_dfd);
    }
    return (ctx->errors == 0) ? 0 : -1;
}

#else

bool io_uring_is_supported(void)
{
    return false;
}

int io_uring_link_batch(const PkgFileList *files, PackageContext *ctx)
{
    (void)files;
    (void)ctx;
    return -1;
}

#endif
