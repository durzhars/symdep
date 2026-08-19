#!/usr/bin/env bash
#
# Symlink & Dependency Manager (symdep)
# Configuration Subsystem & Precedence Feature Test Suite
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

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/test_helpers.sh"

echo -e "${COLOR_CYAN}${COLOR_BOLD}=== Feature Tests: Config Operations ===${COLOR_RESET}\n"

# 1. Config Show
echo -e "${COLOR_BOLD}[Test 1] Config Show${COLOR_RESET}"
setup_sandbox
assert_success "$STOW_BIN config show" "symdep config show succeeded"
assert_file_contains "$LAST_CMD_OUTPUT" "Configuration" "config show displays configuration header"
assert_file_contains "$LAST_CMD_OUTPUT" "Target Directory:" "config show outputs target home path section"

# 2. Config Set [target|source]
echo -e "\n${COLOR_BOLD}[Test 2] Config Set (target & source)${COLOR_RESET}"
setup_sandbox
CUSTOM_TARGET="$TEST_TMPDIR/custom_target"
CUSTOM_DOTFILES="$TEST_TMPDIR/custom_dotfiles"
mkdir -p "$CUSTOM_TARGET" "$CUSTOM_DOTFILES"

assert_success "$STOW_BIN config set --target $CUSTOM_TARGET" "symdep config set target succeeded"
assert_success "$STOW_BIN config set --source $CUSTOM_DOTFILES" "symdep config set source succeeded"

CONFIG_FILE="$XDG_CONFIG_HOME/symdep/config"
if [ ! -f "$CONFIG_FILE" ]; then
    CONFIG_FILE="$XDG_CONFIG_HOME/stow-manager/config"
fi
assert_file_contains "$CONFIG_FILE" "TARGET_DIR=$CUSTOM_TARGET" "config file updated with TARGET_DIR"
assert_file_contains "$CONFIG_FILE" "$CUSTOM_DOTFILES" "config file updated with SOURCE_DIRS"

assert_success "$STOW_BIN config show" "symdep config show succeeded with updated settings"
assert_file_contains "$LAST_CMD_OUTPUT" "$CUSTOM_TARGET" "config show outputs configured target path"
assert_file_contains "$LAST_CMD_OUTPUT" "$CUSTOM_DOTFILES" "config show outputs configured source repository path"

# 3. Config Add & Remove (Multi-Repository Search Paths)
echo -e "\n${COLOR_BOLD}[Test 3] Config Add & Remove (Multi-Repository Paths)${COLOR_RESET}"
setup_sandbox
REPO_PRIMARY="$TEST_TMPDIR/primary_repo"
REPO_SECONDARY="$TEST_TMPDIR/secondary_repo"
mkdir -p "$REPO_PRIMARY" "$REPO_SECONDARY"

assert_success "$STOW_BIN config set source $REPO_PRIMARY" "Set primary repository path succeeded"
assert_success "$STOW_BIN config add $REPO_SECONDARY" "symdep config add secondary repository succeeded"

CONFIG_FILE="$XDG_CONFIG_HOME/symdep/config"
if [ ! -f "$CONFIG_FILE" ]; then
    CONFIG_FILE="$XDG_CONFIG_HOME/stow-manager/config"
fi
assert_file_contains "$CONFIG_FILE" "$REPO_PRIMARY:$REPO_SECONDARY" "Config file contains colon-separated multi-repository paths"

assert_success "$STOW_BIN config show" "symdep config show displays multi-repository configuration"
assert_file_contains "$LAST_CMD_OUTPUT" "$REPO_PRIMARY" "config show contains primary repository path"
assert_file_contains "$LAST_CMD_OUTPUT" "$REPO_SECONDARY" "config show contains secondary repository path"

assert_success "$STOW_BIN config remove $REPO_SECONDARY" "symdep config remove succeeded"
assert_file_not_contains "$CONFIG_FILE" "$REPO_SECONDARY" "Config file no longer contains removed repository path"

# 4. Save CLI overrides to config (-s / --save)
echo -e "\n${COLOR_BOLD}[Test 4] Save CLI overrides to config (-s / --save)${COLOR_RESET}"
setup_sandbox
SAVE_TARGET="$TEST_TMPDIR/saved_target"
SAVE_DOTFILES="$TEST_TMPDIR/saved_dotfiles"
mkdir -p "$SAVE_TARGET" "$SAVE_DOTFILES"

assert_success "$STOW_BIN -t $SAVE_TARGET -d $SAVE_DOTFILES -s help" "symdep -s --save flag succeeded"
CONFIG_FILE="$XDG_CONFIG_HOME/symdep/config"
if [ ! -f "$CONFIG_FILE" ]; then
    CONFIG_FILE="$XDG_CONFIG_HOME/stow-manager/config"
fi
assert_file_contains "$CONFIG_FILE" "TARGET_DIR=$SAVE_TARGET" "config file persisted TARGET_DIR via -s / --save"
assert_file_contains "$CONFIG_FILE" "$SAVE_DOTFILES" "config file persisted SOURCE_DIRS via -s / --save"

print_summary
