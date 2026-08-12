/*
 * Symlink & Dependency Manager (symdep)
 * Safe Memory Allocation Wrappers
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
#ifndef UTILS_MEM_H
#define UTILS_MEM_H

#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Safe malloc wrapper. Logs fatal error and exits process on OOM.
 *
 * @param size Allocation size in bytes.
 * @return Non-null pointer to allocated memory.
 */
void *safe_malloc(size_t size);

/**
 * @brief Safe calloc wrapper. Zero-initializes memory and exits process on OOM.
 *
 * @param num  Number of elements.
 * @param size Size of each element.
 * @return Non-null pointer to zeroed memory.
 */
void *safe_calloc(size_t num, size_t size);

/**
 * @brief Safe realloc wrapper. Exits process on OOM failure.
 *
 * @param ptr  Existing memory pointer.
 * @param size New allocation size in bytes.
 * @return Non-null pointer to reallocated memory.
 */
void *safe_realloc(void *ptr, size_t size);

/**
 * @brief Safe strdup wrapper. Exits process on OOM failure.
 *
 * @param s String to duplicate.
 * @return Non-null pointer to duplicated null-terminated string.
 */
char *safe_strdup(const char *s);

#endif /* UTILS_MEM_H */
