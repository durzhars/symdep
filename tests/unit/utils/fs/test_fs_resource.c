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

#include "../../test_framework.h"
#include "utils/fs.h"

void test_fs_resource_management(void)
{
    FILE *fp = open_resource_file("help.txt");
    if (fp) {
        fclose(fp);
    }
    ASSERT(open_resource_file(NULL) == NULL, "open_resource_file(NULL) should return NULL");
    ASSERT(open_resource_file("nonexistent_resource_xyz_123") == NULL,
           "open_resource_file on nonexistent resource should return NULL");
}
