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

#include "core/scanner/scanner_parser.h"
#include "utils/mem.h"
#include "utils/str.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool is_scanner_comment_line(const char *line)
{
    if (!line) {
        return false;
    }
    const char *p = line;
    while (*p == ' ' || *p == '\t') {
        p++;
    }
    return (*p == '#' || *p == ';' || (*p == '/' && *(p + 1) == '/') ||
            (*p == '-' && *(p + 1) == '-'));
}

static bool is_valid_identifier_token(const char *tok)
{
    if (!tok || *tok == '\0') {
        return false;
    }
    if (!isalpha((unsigned char)tok[0])) {
        return false;
    }
    for (const char *p = tok; *p != '\0'; p++) {
        if (!isalnum((unsigned char)*p) && *p != '-' && *p != '_') {
            return false;
        }
    }
    return true;
}

void scanner_extract_line_tokens(const char *line, StringArray *tokens)
{
    if (!line || is_scanner_comment_line(line)) {
        return;
    }

    char *copy = safe_strdup(line);
    if (!copy) {
        return;
    }

    char *saveptr1 = NULL;
    char *clause = strtok_r(copy, ";|&\r\n", &saveptr1);

    while (clause) {
        char *saveptr2 = NULL;
        char *token = strtok_r(clause, " \t=:,\"'()[]{}", &saveptr2);

        while (token) {
            char *trimmed = trim_whitespace(token);

            if (is_valid_identifier_token(trimmed) && strlen(trimmed) >= 2) {
                if (!str_array_contains(tokens, trimmed)) {
                    str_array_append(tokens, trimmed);
                }
            }
            token = strtok_r(NULL, " \t=:,\"'()[]{}", &saveptr2);
        }

        clause = strtok_r(NULL, ";|&\r\n", &saveptr1);
    }

    free(copy);
}
