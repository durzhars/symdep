#!/usr/bin/env bash
#
# Symlink & Dependency Manager (symdep)
# Hierarchical Ignore Rules & File Filtering Feature Test Suite
# Copyright (C) 2026 durzhars
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <https://www.gnu.org/licenses/>.
#

set -u

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/test_helpers.sh"

echo -e "${COLOR_CYAN}${COLOR_BOLD}=== Feature Tests: Ignore Rules & File Filtering ===${COLOR_RESET}\n"

setup_sandbox

# Helper to locate active ignore file
get_ignore_file() {
    local target_dir="$1"
    if [ -f "$target_dir/.symignore" ]; then
        echo "$target_dir/.symignore"
    elif [ -f "$target_dir/.stowignore" ]; then
        echo "$target_dir/.stowignore"
    else
        echo "$target_dir/.symignore"
    fi
}

# [Test 1] Ignore Init (repo root & per-package)
echo -e "${COLOR_BOLD}[Test 1] Ignore Init ('ignore init')${COLOR_RESET}"
mkdir -p "$MOCK_DOTFILES/ignpkg"

assert_success "$STOW_BIN ignore init" "Global ignore file initialization"
GLOBAL_IGNORE=$(get_ignore_file "$MOCK_DOTFILES")
assert_path_exists "$GLOBAL_IGNORE" "Global ignore file created"

assert_success "$STOW_BIN ignore init ignpkg" "Package ignore file initialization"
PKG_IGNORE=$(get_ignore_file "$MOCK_DOTFILES/ignpkg")
assert_path_exists "$PKG_IGNORE" "Package ignore file created"

# [Test 2] Ignore Add (package & global -g flag)
echo -e "\n${COLOR_BOLD}[Test 2] Ignore Add ('ignore add')${COLOR_RESET}"
assert_success "$STOW_BIN ignore add ignpkg '*.log' 'temp_cache/'" "Add patterns to package ignore file"
PKG_IGNORE=$(get_ignore_file "$MOCK_DOTFILES/ignpkg")
assert_file_contains "$PKG_IGNORE" "*.log" "Package ignore file contains *.log"
assert_file_contains "$PKG_IGNORE" "temp_cache/" "Package ignore file contains temp_cache/"

assert_success "$STOW_BIN ignore add -g '*.bak'" "Add pattern to global ignore file using -g"
GLOBAL_IGNORE=$(get_ignore_file "$MOCK_DOTFILES")
assert_file_contains "$GLOBAL_IGNORE" "*.bak" "Global ignore file contains *.bak"

# Duplicate addition prevention
assert_success "$STOW_BIN ignore add ignpkg '*.log'" "Re-add duplicate pattern *.log"
dup_count=$(grep -c '^\*\.log$' "$PKG_IGNORE" || true)
if [ "$dup_count" -eq 1 ]; then
    echo -e "  ${COLOR_GREEN}✓${COLOR_RESET} Duplicate pattern *.log was not added twice"
    TESTS_PASSED=$((TESTS_PASSED + 1))
else
    echo -e "  ${COLOR_RED}✗${COLOR_RESET} Duplicate pattern *.log was added $dup_count times"
    TESTS_FAILED=$((TESTS_FAILED + 1))
fi
TESTS_RUN=$((TESTS_RUN + 1))

# [Test 3] Ignore Show ('ignore show')
echo -e "\n${COLOR_BOLD}[Test 3] Ignore Show ('ignore show')${COLOR_RESET}"
assert_success "$STOW_BIN ignore show ignpkg" "ignore show ignpkg succeeded"
assert_file_contains "$LAST_CMD_OUTPUT" "*.log" "ignore show displays *.log pattern"

assert_success "$STOW_BIN ignore show" "ignore show global succeeded"
assert_file_contains "$LAST_CMD_OUTPUT" "*.bak" "ignore show global displays *.bak pattern"

# [Test 4] Ignore Remove ('ignore remove')
echo -e "\n${COLOR_BOLD}[Test 4] Ignore Remove ('ignore remove')${COLOR_RESET}"
assert_success "$STOW_BIN ignore remove ignpkg '*.log'" "Remove *.log pattern from ignpkg ignore file"
assert_file_not_contains "$PKG_IGNORE" "*.log" "Package ignore file no longer contains *.log"
assert_file_contains "$PKG_IGNORE" "temp_cache/" "Package ignore file still contains temp_cache/"

# [Test 5] Ignore Clear ('ignore clear')
echo -e "\n${COLOR_BOLD}[Test 5] Ignore Clear ('ignore clear')${COLOR_RESET}"
assert_success "$STOW_BIN ignore clear ignpkg" "Clear package ignore file"
assert_path_not_exists "$PKG_IGNORE" "Package ignore file deleted"

assert_success "$STOW_BIN ignore clear" "Clear global ignore file"
assert_path_not_exists "$GLOBAL_IGNORE" "Global ignore file deleted"

# [Test 6] End-to-End Stow Integration with Ignore Rules
echo -e "\n${COLOR_BOLD}[Test 6] End-to-End Stow Integration with Ignore Rules${COLOR_RESET}"
mkdir -p "$MOCK_DOTFILES/apppkg"
echo "config_val=1" > "$MOCK_DOTFILES/apppkg/app.conf"
echo "secret_key=xyz" > "$MOCK_DOTFILES/apppkg/secret.key"

assert_success "$STOW_BIN ignore add apppkg 'secret.key'" "Ignore secret.key pattern for apppkg"
assert_success "$STOW_BIN stow apppkg" "Stow apppkg package"
assert_symlink_exists "$MOCK_HOME/app.conf" "$MOCK_DOTFILES/apppkg/app.conf" "app.conf is stowed as symlink"
assert_path_not_exists "$MOCK_HOME/secret.key" "secret.key is ignored and not stowed"

print_summary

