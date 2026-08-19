#!/usr/bin/env bash
#
# Symlink & Dependency Manager (symdep)
# Feature Test Suite Orchestration Runner
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
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
STOW_BIN="${STOW_BIN:-$PROJECT_ROOT/bin/symdep}"

if [ -t 1 ] || [ "${FORCE_COLOR:-0}" = "1" ]; then
    COLOR_RED='\033[0;31m'
    COLOR_GREEN='\033[0;32m'
    COLOR_YELLOW='\033[1;33m'
    COLOR_CYAN='\033[0;36m'
    COLOR_BOLD='\033[1m'
    COLOR_RESET='\033[0m'
else
    COLOR_RED=''
    COLOR_GREEN=''
    COLOR_YELLOW=''
    COLOR_CYAN=''
    COLOR_BOLD=''
    COLOR_RESET=''
fi

echo -e "${COLOR_CYAN}${COLOR_BOLD}=========================================${COLOR_RESET}"
echo -e "${COLOR_CYAN}${COLOR_BOLD}    symdep Feature Test Suite Runner     ${COLOR_RESET}"
echo -e "${COLOR_CYAN}${COLOR_BOLD}=========================================${COLOR_RESET}\n"

if [ ! -x "$STOW_BIN" ]; then
    echo -e "${COLOR_YELLOW}Binary '$STOW_BIN' not found. Building now...${COLOR_RESET}"
    make -C "$PROJECT_ROOT" all
fi

TEST_SUITES=(
    "test_stow_cmd.sh"
    "test_config_cmd.sh"
    "test_deps_cmd.sh"
    "test_ignore_cmd.sh"
    "test_inspection_cmd.sh"
    "test_degraded_env_cmd.sh"
    "test_observability_cmd.sh"
    "test_completions_cmd.sh"
)

SUITES_PASSED=0
SUITES_FAILED=0

for suite in "${TEST_SUITES[@]}"; do
    suite_path="$SCRIPT_DIR/$suite"
    if [ ! -f "$suite_path" ]; then
        echo -e "${COLOR_RED}Error: Test suite '$suite' not found at '$suite_path'${COLOR_RESET}"
        SUITES_FAILED=$((SUITES_FAILED + 1))
        continue
    fi

    echo -e "${COLOR_BOLD}Running suite: $suite${COLOR_RESET}"
    echo "----------------------------------------"

    bash "$suite_path"
    exit_code=$?

    if [ $exit_code -eq 0 ]; then
        echo -e "\n${COLOR_GREEN}✓ Suite $suite PASSED${COLOR_RESET}\n"
        SUITES_PASSED=$((SUITES_PASSED + 1))
    else
        echo -e "\n${COLOR_RED}✗ Suite $suite FAILED${COLOR_RESET}\n"
        SUITES_FAILED=$((SUITES_FAILED + 1))
    fi
done

echo -e "${COLOR_CYAN}${COLOR_BOLD}=========================================${COLOR_RESET}"
echo -e "${COLOR_CYAN}${COLOR_BOLD}           Final Feature Test Summary    ${COLOR_RESET}"
echo -e "${COLOR_CYAN}${COLOR_BOLD}=========================================${COLOR_RESET}"
echo -e "Total Suites: $((SUITES_PASSED + SUITES_FAILED))"
echo -e "  ${COLOR_GREEN}Passed: $SUITES_PASSED${COLOR_RESET}"

if [ $SUITES_FAILED -gt 0 ]; then
    echo -e "  ${COLOR_RED}Failed: $SUITES_FAILED${COLOR_RESET}"
    exit 1
else
    echo -e "  Failed: 0"
    exit 0
fi
