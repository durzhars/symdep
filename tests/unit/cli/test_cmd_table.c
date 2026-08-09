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
#include "cli/cmd_routes.h"

void test_cmd_table_lookup(void)
{
    bool found_stow = false;
    for (size_t i = 0; ROUTE_TABLE[i].group != NULL; i++) {
        if (strcmp(ROUTE_TABLE[i].group, "stow") == 0) {
            found_stow = true;
            ASSERT(ROUTE_TABLE[i].handler != NULL, "stow route handler should not be NULL");
            break;
        }
    }
    ASSERT(found_stow, "ROUTE_TABLE should contain 'stow' route entry");
}
