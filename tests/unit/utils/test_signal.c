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
#include "utils/signal.h"

void test_is_executable_in_path(void)
{
    ASSERT(is_executable_in_path("sh"), "sh should be in PATH");
    ASSERT(!is_executable_in_path("nonexistent_binary_xyz_12345"),
           "nonexistent binary should not be in PATH");
}

void test_temp_path_registration(void)
{
    register_temp_path("/tmp/dummy_temp_path_1");
    register_temp_path("/tmp/dummy_temp_path_2");

    unregister_temp_path("/tmp/dummy_temp_path_1");
    cleanup_temp_paths();
}
