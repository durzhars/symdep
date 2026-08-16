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

#ifndef SYMDEP_SCANNER_PARSER_H
#define SYMDEP_SCANNER_PARSER_H

#include "utils/str.h"
#include <stdbool.h>

/**
 * @brief Check if a code/config line is a comment in shell, lua, python, or vimscript.
 *
 * @param line Input line buffer.
 * @return true if line begins with comment delimiter (#, --, //, "), false otherwise.
 */
bool is_scanner_comment_line(const char *line);

/**
 * @brief Extract prospective tool command tokens from a script or config line.
 *
 * @param line   Input text line.
 * @param tokens Output StringArray populated with extracted candidate tokens.
 */
void scanner_extract_line_tokens(const char *line, StringArray *tokens);

#endif /* SYMDEP_SCANNER_PARSER_H */
