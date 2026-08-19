/*
 * Symlink & Dependency Manager (symdep)
 * Linux io_uring Asynchronous Batch Symlink Backend Header
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

#ifndef SYMDEP_UTILS_IO_URING_BACKEND_H
#define SYMDEP_UTILS_IO_URING_BACKEND_H

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

#endif /* SYMDEP_UTILS_IO_URING_BACKEND_H */
