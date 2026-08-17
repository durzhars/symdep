#!/usr/bin/env bash
# tests/feature/test_deps_cmd.sh
# Feature test suite for symdep dependency management commands.

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/test_helpers.sh"

echo -e "${COLOR_CYAN}${COLOR_BOLD}=== Feature Tests: Dependency Management ===${COLOR_RESET}\n"

# 1. pkg:create <name>
echo -e "${COLOR_BOLD}[Test 1] Scaffold package (pkg:create / make:package)${COLOR_RESET}"
setup_sandbox

assert_success "$STOW_BIN pkg:create editor" "symdep pkg:create editor succeeded"
assert_path_exists "$STOW_DOTFILES_DIR/editor" "Scaffolded package directory created"
MANIFEST_FILE="$STOW_DOTFILES_DIR/editor/.symdeps"
if [ ! -f "$MANIFEST_FILE" ]; then
    MANIFEST_FILE="$STOW_DOTFILES_DIR/editor/.stowdeps"
fi
assert_path_exists "$MANIFEST_FILE" "Package manifest file created (.symdeps)"
assert_file_contains "$MANIFEST_FILE" "Package Dependency Manifest for 'editor'" "Manifest header initialized"

# 2. deps:add [--required | --optional | --conflict]
echo -e "\n${COLOR_BOLD}[Test 2] Add dependencies & conflicts (deps:add)${COLOR_RESET}"
setup_sandbox
mkdir -p "$STOW_DOTFILES_DIR/terminal"

assert_success "$STOW_BIN deps:add terminal bash --required" "deps:add terminal bash --required succeeded"
MANIFEST_FILE="$STOW_DOTFILES_DIR/terminal/.symdeps"
if [ ! -f "$MANIFEST_FILE" ]; then
    MANIFEST_FILE="$STOW_DOTFILES_DIR/terminal/.stowdeps"
fi
assert_file_contains "$MANIFEST_FILE" 'REQUIRED="bash"' "Manifest updated with REQUIRED=\"bash\""

assert_success "$STOW_BIN deps:add terminal fzf --optional" "deps:add terminal fzf --optional succeeded"
assert_file_contains "$MANIFEST_FILE" 'OPTIONAL="fzf"' "Manifest updated with OPTIONAL=\"fzf\""

assert_success "$STOW_BIN deps:add terminal zsh --conflict" "deps:add terminal zsh --conflict succeeded"
assert_file_contains "$MANIFEST_FILE" 'CONFLICTS="zsh"' "Manifest updated with CONFLICTS=\"zsh\""

# 3. deps:show <pkg>
echo -e "\n${COLOR_BOLD}[Test 3] Display package manifest (deps:show)${COLOR_RESET}"
assert_success "$STOW_BIN deps:show terminal" "symdep deps:show terminal succeeded"
assert_file_contains "$LAST_CMD_OUTPUT" "terminal" "deps:show outputs package name header"
assert_file_contains "$LAST_CMD_OUTPUT" 'REQUIRED="bash"' "deps:show outputs REQUIRED entries"
assert_file_contains "$LAST_CMD_OUTPUT" 'OPTIONAL="fzf"' "deps:show outputs OPTIONAL entries"
assert_file_contains "$LAST_CMD_OUTPUT" 'CONFLICTS="zsh"' "deps:show outputs CONFLICTS entries"

# 4. deps:edit <pkg> <dep> <new_type>
echo -e "\n${COLOR_BOLD}[Test 4] Edit dependency type (deps:edit)${COLOR_RESET}"
setup_sandbox
mkdir -p "$STOW_DOTFILES_DIR/terminal"
$STOW_BIN deps:add terminal fzf --optional >/dev/null
MANIFEST_FILE="$STOW_DOTFILES_DIR/terminal/.symdeps"
if [ ! -f "$MANIFEST_FILE" ]; then
    MANIFEST_FILE="$STOW_DOTFILES_DIR/terminal/.stowdeps"
fi
assert_file_contains "$MANIFEST_FILE" 'OPTIONAL="fzf"' "Initially OPTIONAL=\"fzf\""

assert_success "$STOW_BIN deps:edit terminal fzf --required" "deps:edit terminal fzf --required succeeded"
assert_file_contains "$MANIFEST_FILE" 'REQUIRED="fzf"' "Manifest updated to REQUIRED=\"fzf\""
assert_file_contains "$MANIFEST_FILE" 'OPTIONAL=""' "Manifest OPTIONAL is now empty"

# 5. deps:remove <pkg> <dep>
echo -e "\n${COLOR_BOLD}[Test 5] Remove dependency (deps:remove)${COLOR_RESET}"
setup_sandbox
mkdir -p "$STOW_DOTFILES_DIR/terminal"
$STOW_BIN deps:add terminal bash --required >/dev/null
MANIFEST_FILE="$STOW_DOTFILES_DIR/terminal/.symdeps"
if [ ! -f "$MANIFEST_FILE" ]; then
    MANIFEST_FILE="$STOW_DOTFILES_DIR/terminal/.stowdeps"
fi
assert_file_contains "$MANIFEST_FILE" 'REQUIRED="bash"' "Initially REQUIRED=\"bash\""

assert_success "$STOW_BIN deps:remove terminal bash" "symdep deps:remove terminal bash succeeded"
assert_file_contains "$MANIFEST_FILE" 'REQUIRED=""' "Manifest REQUIRED is now cleared"
assert_file_not_contains "$MANIFEST_FILE" 'bash' "Manifest no longer contains bash"

# 6. pkg:create & pkg:remove
echo -e "\n${COLOR_BOLD}[Test 6] Package CRUD operations (pkg:create & pkg:remove)${COLOR_RESET}"
setup_sandbox
assert_success "$STOW_BIN pkg:create mynewpkg" "symdep pkg:create mynewpkg succeeded"
assert_path_exists "$STOW_DOTFILES_DIR/mynewpkg" "Created package directory exists"
assert_success "$STOW_BIN pkg:remove mynewpkg" "symdep pkg:remove mynewpkg succeeded"
assert_path_not_exists "$STOW_DOTFILES_DIR/mynewpkg" "Removed package directory no longer exists"

# 7. deps target <pkg> <path>
echo -e "\n${COLOR_BOLD}[Test 7] Per-package target directory configuration (deps target)${COLOR_RESET}"
setup_sandbox
mkdir -p "$STOW_DOTFILES_DIR/sysconfig"
assert_success "$STOW_BIN deps target sysconfig /etc/custom" "deps target sysconfig /etc/custom succeeded"
MANIFEST_FILE="$STOW_DOTFILES_DIR/sysconfig/.symdeps"
if [ ! -f "$MANIFEST_FILE" ]; then
    MANIFEST_FILE="$STOW_DOTFILES_DIR/sysconfig/.stowdeps"
fi
assert_file_contains "$MANIFEST_FILE" 'TARGET="/etc/custom"' "Manifest updated with TARGET=\"/etc/custom\""

# 8. deps install & install standalone installer
echo -e "\n${COLOR_BOLD}[Test 8] Standalone Dependency Installer (deps install & install)${COLOR_RESET}"
setup_sandbox
mkdir -p "$STOW_DOTFILES_DIR/pkg_dep_test"
echo "test_content" > "$STOW_DOTFILES_DIR/pkg_dep_test/.test_file"
cat << 'EOF' > "$STOW_DOTFILES_DIR/pkg_dep_test/.symdeps"
REQUIRED="nonexistent_tool_alpha"
OPTIONAL="nonexistent_tool_beta"
EOF

assert_success "$STOW_BIN -n deps install pkg_dep_test" "deps install -n pkg_dep_test succeeded"
assert_file_contains "$LAST_CMD_OUTPUT" "[DRY-RUN]" "deps install dry-run outputs [DRY-RUN] preview tag"
assert_file_contains "$LAST_CMD_OUTPUT" "nonexistent_tool_alpha" "deps install identifies missing required tool"

assert_success "$STOW_BIN -n install pkg_dep_test" "Top-level install -n pkg_dep_test alias succeeded"
assert_file_contains "$LAST_CMD_OUTPUT" "[DRY-RUN]" "install alias outputs [DRY-RUN] preview tag"

# 9. Decoupled link operation (non-blocking passive audit)
echo -e "\n${COLOR_BOLD}[Test 9] Decoupled Link Operation (Passive Non-blocking Dependency Audit)${COLOR_RESET}"
assert_success "$STOW_BIN link pkg_dep_test" "symdep link pkg_dep_test succeeded without blocking on stdin"
assert_path_exists "$HOME/.test_file" "Symlink deployed despite missing dependency"
assert_file_contains "$LAST_CMD_OUTPUT" "Dependencies:" "Output informs user with numerical dependency count"
assert_file_contains "$LAST_CMD_OUTPUT" "1 required missing" "Output reports exact missing required count"

print_summary


