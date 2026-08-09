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
#include "utils/str.h"

void test_trim_whitespace(void)
{
    ASSERT(trim_whitespace(NULL) == NULL, "trim_whitespace(NULL) should return NULL");

    char s1[] = "  hello world  ";
    ASSERT_STR_EQ(trim_whitespace(s1), "hello world");

    char s2[] = "\"quoted string\"";
    ASSERT_STR_EQ(trim_whitespace(s2), "quoted string");

    char s2_single[] = "'single quoted'";
    ASSERT_STR_EQ(trim_whitespace(s2_single), "single quoted");

    char s3[] = "\t\n  ";
    ASSERT_STR_EQ(trim_whitespace(s3), "");
}

void test_string_array(void)
{
    StringArray arr;
    str_array_init(&arr);

    ASSERT(arr.count == 0, "Array count should initially be 0");
    str_array_append(&arr, "item1");
    str_array_append(&arr, "item2");

    ASSERT(arr.count == 2, "Array count should be 2");
    ASSERT(str_array_contains(&arr, "item1"), "Array should contain 'item1'");
    ASSERT(str_array_contains(&arr, "item2"), "Array should contain 'item2'");
    ASSERT(!str_array_contains(&arr, "item3"), "Array should not contain 'item3'");

    str_array_free(&arr);
    ASSERT(arr.count == 0, "Array count should be 0 after free");
}

void test_str_set(void)
{
    StrSet set;
    str_set_init(&set);

    ASSERT(!str_set_contains(&set, "apple"), "Empty set should not contain apple");

    ASSERT(str_set_add(&set, "apple") == true, "First add of apple should succeed");
    ASSERT(str_set_contains(&set, "apple") == true, "Set should contain apple");
    ASSERT(str_set_add(&set, "apple") == false, "Duplicate add of apple should return false");

    ASSERT(str_set_add(&set, "banana") == true, "First add of banana should succeed");
    ASSERT(str_set_add(&set, "cherry") == true, "First add of cherry should succeed");

    ASSERT(str_set_contains(&set, "apple"), "Set should contain apple");
    ASSERT(str_set_contains(&set, "banana"), "Set should contain banana");
    ASSERT(str_set_contains(&set, "cherry"), "Set should contain cherry");
    ASSERT(!str_set_contains(&set, "date"), "Set should not contain date");

    str_set_free(&set);
    ASSERT(set.keys == NULL && set.count == 0, "Set should be cleaned up");
}
