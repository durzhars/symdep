#include "utils/io_uring_backend.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "utils/logger.h"
#include "utils/mem.h"
#include "utils/path.h"

#ifdef __linux__

#include <linux/io_uring.h>
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
#define IORING_OP_SYMLINKAT 13
#endif

typedef struct {
    int ring_fd;
    void *sq_ptr;
    size_t sq_size;
    void *cq_ptr;
    size_t cq_size;
    struct io_uring_sqe *sqes;
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

    ring->sq_size = p.sq_off.array + p.sq_entries * sizeof(uint32_t);
    ring->cq_size = p.cq_off.cqes + p.cq_entries * sizeof(struct io_uring_cqe);
    ring->sqes_size = p.sq_entries * sizeof(struct io_uring_sqe);

    ring->sq_ptr = mmap(0,
                        ring->sq_size,
                        PROT_READ | PROT_WRITE,
                        MAP_SHARED | MAP_POPULATE,
                        ring_fd,
                        IORING_OFF_SQ_RING);
    ring->cq_ptr = mmap(0,
                        ring->cq_size,
                        PROT_READ | PROT_WRITE,
                        MAP_SHARED | MAP_POPULATE,
                        ring_fd,
                        IORING_OFF_CQ_RING);
    ring->sqes = (struct io_uring_sqe *)mmap(0,
                                             ring->sqes_size,
                                             PROT_READ | PROT_WRITE,
                                             MAP_SHARED | MAP_POPULATE,
                                             ring_fd,
                                             IORING_OFF_SQES);

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
    IoUringRing ring;
    if (!init_io_uring(&ring, 4)) {
        return false;
    }

    const char *test_src = "/tmp/.symdep_uring_probe_src";
    const char *test_tgt = "/tmp/.symdep_uring_probe_tgt";
    unlink(test_tgt);
    unlink(test_src);

    int fd = open(test_src, O_CREAT | O_WRONLY, 0644);
    if (fd >= 0) {
        close(fd);
    }

    struct io_uring_sqe *sqe = &ring.sqes[0];
    memset(sqe, 0, sizeof(*sqe));
    sqe->opcode = IORING_OP_SYMLINKAT;
    sqe->fd = AT_FDCWD;
    sqe->addr = (uint64_t)test_src;
    sqe->addr2 = (uint64_t)test_tgt;

    *ring.sq_tail = 1;
    ring.sq_array[0] = 0;

    int ret =
        (int)syscall(__NR_io_uring_enter, ring.ring_fd, 1, 1, IORING_ENTER_GETEVENTS, NULL, 0);
    bool supported = false;
    if (ret >= 0 && *ring.cq_head != *ring.cq_tail) {
        struct io_uring_cqe *cqe = &ring.cqes[*ring.cq_head & *ring.cq_ring_mask];
        if (cqe->res == 0) {
            supported = true;
        }
    }

    unlink(test_src);
    unlink(test_tgt);
    free_io_uring(&ring);
    return supported;
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

    int target_dfd = open(ctx->target_dir, O_RDONLY | O_DIRECTORY);
    if (target_dfd < 0) {
        target_dfd = AT_FDCWD;
    }

    size_t target_dir_len = strlen(ctx->target_dir);
    size_t pkg_dir_len = strlen(ctx->pkg_dir);

    typedef struct {
        char target_path[STOW_PATH_LARGE];
        char pkg_file_path[STOW_PATH_LARGE];
        const char *rel_path;
    } PendingLink;

    static _Thread_local PendingLink batch[256];

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

            struct stat st;
            if (lstat(link->target_path, &st) == 0) {
                if (S_ISLNK(st.st_mode)) {
                    char real_pkg[STOW_PATH_LARGE];
                    if (is_symlink(link->pkg_file_path)) {
                        if (realpath(link->pkg_file_path, real_pkg) == NULL) {
                            snprintf(real_pkg, sizeof(real_pkg), "%s", link->pkg_file_path);
                        }
                    } else {
                        snprintf(real_pkg, sizeof(real_pkg), "%s", link->pkg_file_path);
                    }

                    if (is_symlink_pointing_to(link->target_path, link->pkg_file_path, real_pkg)) {
                        continue;
                    }
                    unlink(link->target_path);
                } else {
                    char backup_path[STOW_PATH_HUGE];
                    build_unique_backup_path(link->target_path, backup_path, sizeof(backup_path));
                    log_warn("Conflict! Backing up file: %s -> %s", link->target_path, backup_path);
                    if (rename(link->target_path, backup_path) != 0) {
                        log_error("Failed to backup file: %s", link->target_path);
                        ctx->errors++;
                        continue;
                    }
                }
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
            *ring->sq_tail = tail + 1;
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
            ctx->errors += (size_t)queued;
            break;
        }

        // Process completions
        uint32_t head = *ring->cq_head;
        uint32_t cq_tail = *ring->cq_tail;
        while (head != cq_tail) {
            struct io_uring_cqe *cqe = &ring->cqes[head & *ring->cq_ring_mask];
            if (cqe->res == 0) {
                ctx->created_count++;
            } else {
                size_t b_idx = (size_t)cqe->user_data;
                log_error("io_uring symlink failed (target=%s -> pkg=%s) cqe_res=%d: %s",
                          batch[b_idx].target_path,
                          batch[b_idx].pkg_file_path,
                          cqe->res,
                          strerror(-cqe->res));
                ctx->errors++;
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
