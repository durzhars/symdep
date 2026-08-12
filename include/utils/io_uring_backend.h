#ifndef SYMDEPT_UTILS_IO_URING_BACKEND_H
#define SYMDEPT_UTILS_IO_URING_BACKEND_H

#include <stdbool.h>
#include <stddef.h>

#include "core/linker/internal.h"

/**
 * @brief Probe if kernel supports io_uring with required syscalls.
 *
 * @return true if io_uring is supported at runtime, false otherwise.
 */
bool io_uring_is_supported(void);

/**
 * @brief Execute zero-copy batch symlink operations using Linux io_uring ring buffer.
 *
 * @param files Pointer to collected package file list.
 * @param ctx Pointer to package context.
 * @return 0 on success, -1 on failure/unsupported.
 */
int io_uring_link_batch(const PkgFileList *files, PackageContext *ctx);

#endif /* SYMDEPT_UTILS_IO_URING_BACKEND_H */
