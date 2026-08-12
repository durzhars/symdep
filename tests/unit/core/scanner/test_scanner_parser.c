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
#include "core/scanner/scanner_parser.h"

void test_scanner_parser_token_extraction(void)
{
    StringArray tokens;
    str_array_init(&tokens);

    // Test Hyprland / Sway DSL line
    scanner_extract_line_tokens("bind = $mainMod, Return, exec, alacritty", &tokens);
    ASSERT(str_array_contains(&tokens, "bind"), "Should extract positional token 'bind'");
    ASSERT(str_array_contains(&tokens, "Return"), "Should extract positional token 'Return'");
    ASSERT(str_array_contains(&tokens, "exec"), "Should extract positional token 'exec'");
    ASSERT(str_array_contains(&tokens, "alacritty"), "Should extract positional token 'alacritty'");
    ASSERT(!str_array_contains(&tokens, "$mainMod"), "Should skip variables starting with '$'");
    str_array_free(&tokens);

    // Test Lua / Neovim config line
    str_array_init(&tokens);
    scanner_extract_line_tokens("vimgrep_arguments = { \"rg\", \"--vimgrep\" }", &tokens);
    ASSERT(str_array_contains(&tokens, "vimgrep_arguments"), "Should extract 'vimgrep_arguments'");
    ASSERT(str_array_contains(&tokens, "rg"), "Should extract 'rg' bounded by quotes and commas");
    ASSERT(!str_array_contains(&tokens, "--vimgrep"), "Should skip flag options starting with '-'");
    str_array_free(&tokens);

    // Test comment line skipping
    str_array_init(&tokens);
    scanner_extract_line_tokens("# exec-once = waybar", &tokens);
    ASSERT(tokens.count == 0, "Comment line starting with '#' should yield 0 tokens");
    str_array_free(&tokens);

    str_array_init(&tokens);
    scanner_extract_line_tokens("// exec = dunst", &tokens);
    ASSERT(tokens.count == 0, "Comment line starting with '//' should yield 0 tokens");
    str_array_free(&tokens);
}
