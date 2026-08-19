/*
 * Symlink & Dependency Manager (symdep)
 * Dynamic String, StringArray & StrSet Utilities Header
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
#ifndef SYMDEP_UTILS_STR_H
#define SYMDEP_UTILS_STR_H

#include <stdbool.h>
#include <stddef.h>

/**
 * @struct StringArray
 * @brief Dynamic resizable array of heap-allocated strings.
 */
typedef struct {
    char **items;    /**< Array of string pointers */
    size_t count;    /**< Number of items currently stored */
    size_t capacity; /**< Allocated storage capacity */
} StringArray;

/** Initialize an empty StringArray */
void str_array_init(StringArray *arr);

/** Append a duplicate of str to StringArray */
void str_array_append(StringArray *arr, const char *str);

/** Check if StringArray contains an exact match for str */
bool str_array_contains(const StringArray *arr, const char *str);

/** Free all strings and internal memory allocated by StringArray */
void str_array_free(StringArray *arr);

/**
 * @struct StrSet
 * @brief Unique string set collection with linear probing and deduplication.
 */
typedef struct {
    char **keys;     /**< Array of unique key strings */
    size_t capacity; /**< Allocated capacity */
    size_t count;    /**< Count of distinct keys stored */
} StrSet;

/** Initialize an empty StrSet */
void str_set_init(StrSet *set);

/** Add a string to StrSet if not already present (returns true if newly inserted) */
bool str_set_add(StrSet *set, const char *str);

/** Check if StrSet contains str */
bool str_set_contains(const StrSet *set, const char *str);

/** Free all memory allocated by StrSet */
void str_set_free(StrSet *set);

/** In-place trim leading and trailing whitespace from string */
char *trim_whitespace(char *str);

/** Escape shell argument with single quotes for safe command execution */
void escape_shell_arg(const char *src, char *dest, size_t dest_size);

/** Split string by delimiter characters and append tokens to out_arr */
void str_split_delim(const char *src, const char *delim, StringArray *out_arr);

/** Callback function pointer type for variable resolution */
typedef const char *(*StrVarResolver)(const char *var_name, void *ctx);

/**
 * @brief Expand variables within a template string using a custom resolver callback.
 *
 * @param src      Source template string containing $VAR or ${VAR}.
 * @param out      Output destination buffer.
 * @param out_size Size of destination buffer.
 * @param resolver Callback function invoked per variable key.
 * @param ctx      Opaque context pointer passed to resolver.
 */
void str_expand_vars(const char *src,
                     char *out,
                     size_t out_size,
                     StrVarResolver resolver,
                     void *ctx);

#endif /* SYMDEP_UTILS_STR_H */
