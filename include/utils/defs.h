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

#ifndef SYMDEP_UTILS_DEFS_H
#define SYMDEP_UTILS_DEFS_H

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif

#include <limits.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

/**
 * @name Deterministic Path Buffer Limits
 * Standard fixed-size buffer capacities guaranteeing cross-platform safety.
 * @{
 */
#define STOW_PATH_MAX 4096   /**< Standard POSIX maximum path buffer capacity */
#define STOW_PATH_LARGE 8192 /**< Double-capacity path buffer for nested joins */
#define STOW_PATH_HUGE 16384 /**< Large buffer for path formatting and multi-repo listings */
/** @} */

/** Stringification macros */
#ifndef STR
#define XSTR(s) #s
#define STR(s) XSTR(s)
#endif

/**
 * @name ANSI Terminal Color Codes
 * @{
 */
#define COLOR_RED "\033[0;31m"
#define COLOR_GREEN "\033[0;32m"
#define COLOR_YELLOW "\033[1;33m"
#define COLOR_BLUE "\033[0;34m"
#define COLOR_CYAN "\033[0;36m"
#define COLOR_WHITE "\033[1;37m"
#define COLOR_BOLD "\033[1m"
#define COLOR_RESET "\033[0m"
/** @} */

#endif /* SYMDEP_UTILS_DEFS_H */
