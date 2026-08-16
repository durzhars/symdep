#!/usr/bin/env bash
# tests/feature/test_observability_cmd.sh
# Feature test suite for Observability, Instrumentation & Telemetry in symdep.

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/test_helpers.sh"

echo -e "${COLOR_CYAN}${COLOR_BOLD}=== Feature Tests: Observability, Instrumentation & Telemetry ===${COLOR_RESET}\n"

# 1. Performance Profiling Telemetry (--profile / -p)
echo -e "${COLOR_BOLD}[Test 1] Performance Profiling Telemetry (--profile / -p)${COLOR_RESET}"
setup_sandbox
mkdir -p "$STOW_DOTFILES_DIR/perf_pkg/.config/perf"
echo "profile content" > "$STOW_DOTFILES_DIR/perf_pkg/.config/perf/perf.conf"

assert_success "$STOW_BIN --profile stow perf_pkg" "symdep --profile stow perf_pkg succeeded"
assert_file_contains "$LAST_CMD_OUTPUT" "[PERF]" "Output contains [PERF] telemetry tag"
assert_file_contains "$LAST_CMD_OUTPUT" "link_package completed in" "Output contains link_package performance metrics"
assert_file_contains "$LAST_CMD_OUTPUT" "check_package_dependencies completed in" "Output contains check_package_dependencies metrics"
assert_file_contains "$LAST_CMD_OUTPUT" "handle_mutual_exclusions completed in" "Output contains handle_mutual_exclusions metrics"
assert_file_contains "$LAST_CMD_OUTPUT" "unfold_directory_symlinks completed in" "Output contains unfold_directory_symlinks metrics"

# 2. Performance Profiling on Unstow & Restow
echo -e "\n${COLOR_BOLD}[Test 2] Performance Profiling on Restow & Unstow${COLOR_RESET}"
assert_success "$STOW_BIN -p restow perf_pkg" "symdep -p restow perf_pkg succeeded"
assert_file_contains "$LAST_CMD_OUTPUT" "[PERF]" "Restow emits [PERF] telemetry"
assert_file_contains "$LAST_CMD_OUTPUT" "UNLINK:" "Output contains per-unlink operation telemetry"

assert_success "$STOW_BIN -p unstow perf_pkg" "symdep -p unstow perf_pkg succeeded"
assert_file_contains "$LAST_CMD_OUTPUT" "[PERF]" "Unstow emits [PERF] telemetry"

# 3. Structured Logging Tags & Business Event Telemetry
echo -e "\n${COLOR_BOLD}[Test 3] Structured Logging Tags & Statuses${COLOR_RESET}"
setup_sandbox
mkdir -p "$STOW_DOTFILES_DIR/stat_pkg/.config"
echo "sample config" > "$STOW_DOTFILES_DIR/stat_pkg/.config/app.conf"

assert_success "$STOW_BIN stow stat_pkg" "Stow stat_pkg succeeded"
assert_file_contains "$LAST_CMD_OUTPUT" "[INFO]" "Output contains [INFO] structured tag"
assert_file_contains "$LAST_CMD_OUTPUT" "[SUCCESS]" "Output contains [SUCCESS] structured tag"
assert_file_contains "$LAST_CMD_OUTPUT" "All required dependencies and optional plugins are installed!" "Output contains dependency check success event confirmation"

# 4. Dry-Run Telemetry & Action Previews
echo -e "\n${COLOR_BOLD}[Test 4] Dry-Run Telemetry & Action Previews${COLOR_RESET}"
setup_sandbox
mkdir -p "$STOW_DOTFILES_DIR/dry_pkg/.config"
echo "dry run test" > "$STOW_DOTFILES_DIR/dry_pkg/.config/test.conf"

assert_success "$STOW_BIN -n stow dry_pkg" "symdep -n stow dry_pkg succeeded"
assert_file_contains "$LAST_CMD_OUTPUT" "[DRY-RUN]" "Output contains [DRY-RUN] preview tag"
assert_file_contains "$LAST_CMD_OUTPUT" "Would create symlink:" "Output previews planned symlink actions"
assert_file_contains "$LAST_CMD_OUTPUT" "Summary for 'dry_pkg': 1 new symlink(s)" "Output provides planned mutation summary"
assert_path_not_exists "$HOME/.config/test.conf" "Dry-run did not modify filesystem"

# 5. Error Observability & Invariant Violations
echo -e "\n${COLOR_BOLD}[Test 5] Error Telemetry on Missing Package${COLOR_RESET}"
setup_sandbox
assert_failure "$STOW_BIN stow nonexistent_pkg_404" "Stowing nonexistent package cleanly fails"
assert_file_contains "$LAST_CMD_OUTPUT" "[ERROR]" "Failure outputs [ERROR] structured tag"
assert_file_contains "$LAST_CMD_OUTPUT" "Package directory does not exist" "Error describes missing package directory"

print_summary
