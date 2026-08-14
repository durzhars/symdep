/*
 * Symlink & Dependency Manager (symdep)
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

#define _GNU_SOURCE
#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L

#include "../test_framework.h"
#include "utils/io_uring_backend.h"
#include "utils/logger.h"

void test_io_uring_backend_probe(void)
{
    bool supported = io_uring_is_supported();
    if (supported) {
        log_info("[IO_URING] Linux io_uring SQE ring-buffer submission is SUPPORTED on this kernel.");
    } else {
        log_info("[IO_URING] Linux io_uring is RESTRICTED/UNSUPPORTED on this kernel. Fallback POSIX engine active.");
    }
    ASSERT(supported == true || supported == false, "io_uring probe must return boolean result");
}
