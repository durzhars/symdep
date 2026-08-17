#!/usr/bin/env bash
# tests/feature/test_completions_cmd.sh
# Tests shell completion scripts (Bash, Zsh, Fish) for symdep.

set -u

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
source "$SCRIPT_DIR/test_helpers.sh"

echo -e "\n${COLOR_CYAN}=== Suite: Shell Autocompletions (Bash, Zsh, Fish) ===${COLOR_RESET}\n"

# Test 1: Bash completion script existence and syntax
test_bash_completion_syntax() {
    local comp_script="$PROJECT_ROOT/resources/completions/bash/symdep"
    assert_path_exists "$comp_script" "Bash completion script exists"
    assert_success "bash -n '$comp_script'" "Bash completion script passes syntax validation"
}

# Test 2: Bash completion top-level commands and dynamic package discovery
test_bash_completion_top_level() {
    setup_sandbox
    mkdir -p "$MOCK_DOTFILES/hyprland" "$MOCK_DOTFILES/waybar"

    assert_success "bash -c '
        source \"$PROJECT_ROOT/resources/completions/bash/symdep\"
        export SYMDEP_SOURCE_DIR=\"$MOCK_DOTFILES\"
        COMP_WORDS=(symdep \"\")
        COMP_CWORD=1
        _symdep
        echo \"\${COMPREPLY[*]}\" | grep -q \"link\" && \
        echo \"\${COMPREPLY[*]}\" | grep -q \"pkg\" && \
        echo \"\${COMPREPLY[*]}\" | grep -q \"deps\" && \
        echo \"\${COMPREPLY[*]}\" | grep -q \"ignore\" && \
        echo \"\${COMPREPLY[*]}\" | grep -q \"config\" && \
        echo \"\${COMPREPLY[*]}\" | grep -q \"hyprland\" && \
        echo \"\${COMPREPLY[*]}\" | grep -q \"waybar\"
    '" "Bash completion provides top-level commands and dynamically discovers packages"
    cleanup_sandbox
}

# Test 3: Global Repo-Awareness Outside Source Repository (via config file)
test_bash_completion_outside_repo() {
    setup_sandbox
    local custom_dotfiles="$TEST_TMPDIR/remote_dotfiles"
    mkdir -p "$custom_dotfiles/alacritty" "$custom_dotfiles/tmux" "$MOCK_CONFIG/stow-manager"
    echo "SOURCE_DIRS=$custom_dotfiles" > "$MOCK_CONFIG/stow-manager/config"

    assert_success "bash -c '
        cd /tmp
        export XDG_CONFIG_HOME=\"$MOCK_CONFIG\"
        unset SYMDEP_SOURCE_DIR SOURCE_DIR STOW_DOTFILES_DIR DOTFILES_DIR
        source \"$PROJECT_ROOT/resources/completions/bash/symdep\"
        COMP_WORDS=(symdep link \"\")
        COMP_CWORD=2
        _symdep
        echo \"\${COMPREPLY[*]}\" | grep -q \"alacritty\" && \
        echo \"\${COMPREPLY[*]}\" | grep -q \"tmux\"
    '" "Bash completion works globally outside repo via configured SOURCE_DIRS"
    cleanup_sandbox
}

# Test 4: Positional argument deduplication
test_bash_completion_deduplication() {
    setup_sandbox
    mkdir -p "$MOCK_DOTFILES/hyprland" "$MOCK_DOTFILES/waybar" "$MOCK_DOTFILES/neovim"

    assert_success "bash -c '
        source \"$PROJECT_ROOT/resources/completions/bash/symdep\"
        export SYMDEP_SOURCE_DIR=\"$MOCK_DOTFILES\"
        COMP_WORDS=(symdep link hyprland \"\")
        COMP_CWORD=3
        _symdep
        echo \"\${COMPREPLY[*]}\" | grep -q \"waybar\" && \
        echo \"\${COMPREPLY[*]}\" | grep -q \"neovim\" && \
        ! echo \"\${COMPREPLY[*]}\" | grep -q \"hyprland\"
    '" "Bash completion deduplicates already chosen packages in multi-package commands"
    cleanup_sandbox
}

# Test 5: Manifest-aware dependency completion
test_bash_completion_manifest_deps() {
    setup_sandbox
    mkdir -p "$MOCK_DOTFILES/hyprland"
    cat << 'EOF' > "$MOCK_DOTFILES/hyprland/.symdeps"
TARGET="/home/mock"
REQUIRED="hyprland waybar"
OPTIONAL="rofi dunst"
CONFLICTS="sway"
EOF

    assert_success "bash -c '
        source \"$PROJECT_ROOT/resources/completions/bash/symdep\"
        export SYMDEP_SOURCE_DIR=\"$MOCK_DOTFILES\"
        COMP_WORDS=(symdep deps remove hyprland \"\")
        COMP_CWORD=4
        _symdep
        echo \"\${COMPREPLY[*]}\" | grep -q \"waybar\" && \
        echo \"\${COMPREPLY[*]}\" | grep -q \"rofi\" && \
        echo \"\${COMPREPLY[*]}\" | grep -q \"dunst\" && \
        echo \"\${COMPREPLY[*]}\" | grep -q \"sway\"
    '" "Bash completion suggests existing dependencies from .symdeps on 'deps remove'"
    cleanup_sandbox
}

# Test 6: Ignore-file aware pattern completion
test_bash_completion_ignore_patterns() {
    setup_sandbox
    mkdir -p "$MOCK_DOTFILES/hyprland"
    cat << 'EOF' > "$MOCK_DOTFILES/hyprland/.symignore"
*.log
temp_cache/
*.bak
EOF

    assert_success "bash -c '
        source \"$PROJECT_ROOT/resources/completions/bash/symdep\"
        export SYMDEP_SOURCE_DIR=\"$MOCK_DOTFILES\"
        COMP_WORDS=(symdep ignore remove hyprland \"\")
        COMP_CWORD=4
        _symdep
        echo \"\${COMPREPLY[*]}\" | grep -q -- \"*.log\" && \
        echo \"\${COMPREPLY[*]}\" | grep -q \"temp_cache/\"
    '" "Bash completion suggests existing ignore patterns from .symignore on 'ignore remove'"
    cleanup_sandbox
}

# Test 7: Bash completion subcommands and flags
test_bash_completion_subcommands() {
    setup_sandbox

    assert_success "bash -c '
        source \"$PROJECT_ROOT/resources/completions/bash/symdep\"
        COMP_WORDS=(symdep pkg \"\")
        COMP_CWORD=2
        _symdep
        echo \"\${COMPREPLY[*]}\" | grep -q \"create\" && \
        echo \"\${COMPREPLY[*]}\" | grep -q \"remove\" && \
        echo \"\${COMPREPLY[*]}\" | grep -q \"list\"
    '" "Bash completion provides 'pkg' subcommands (create, remove, list)"

    assert_success "bash -c '
        source \"$PROJECT_ROOT/resources/completions/bash/symdep\"
        COMP_WORDS=(symdep deps \"\")
        COMP_CWORD=2
        _symdep
        echo \"\${COMPREPLY[*]}\" | grep -q \"add\" && \
        echo \"\${COMPREPLY[*]}\" | grep -q \"edit\" && \
        echo \"\${COMPREPLY[*]}\" | grep -q \"remove\" && \
        echo \"\${COMPREPLY[*]}\" | grep -q \"target\"
    '" "Bash completion provides 'deps' subcommands (add, edit, remove, target)"

    assert_success "bash -c '
        source \"$PROJECT_ROOT/resources/completions/bash/symdep\"
        COMP_WORDS=(symdep config set \"\")
        COMP_CWORD=3
        _symdep
        echo \"\${COMPREPLY[*]}\" | grep -q -- \"--manager\" && \
        echo \"\${COMPREPLY[*]}\" | grep -q -- \"--elevation\" && \
        echo \"\${COMPREPLY[*]}\" | grep -q -- \"--target\" && \
        echo \"\${COMPREPLY[*]}\" | grep -q -- \"--source\"
    '" "Bash completion provides 'config set' option flags"

    assert_success "bash -c '
        source \"$PROJECT_ROOT/resources/completions/bash/symdep\"
        COMP_WORDS=(symdep -m \"\")
        COMP_CWORD=2
        _symdep
        echo \"\${COMPREPLY[*]}\" | grep -q \"pacman\" && \
        echo \"\${COMPREPLY[*]}\" | grep -q \"apt\" && \
        echo \"\${COMPREPLY[*]}\" | grep -q \"brew\"
    '" "Bash completion provides package managers for -m flag"
    cleanup_sandbox
}

# Test 8: Zsh completion script syntax
test_zsh_completion_syntax() {
    local zsh_script="$PROJECT_ROOT/resources/completions/zsh/_symdep"
    assert_path_exists "$zsh_script" "Zsh completion script exists"
    if command -v zsh >/dev/null 2>&1; then
        assert_success "zsh -n '$zsh_script'" "Zsh completion script passes syntax validation"
    fi
}

# Test 9: Fish completion script existence and syntax
test_fish_completion_file() {
    local fish_script="$PROJECT_ROOT/resources/completions/fish/symdep.fish"
    assert_path_exists "$fish_script" "Fish completion script exists"
    assert_file_contains "$fish_script" "complete -c symdep" "Fish completion script contains completion definitions"
}

# Run tests
test_bash_completion_syntax
test_bash_completion_top_level
test_bash_completion_outside_repo
test_bash_completion_deduplication
test_bash_completion_manifest_deps
test_bash_completion_ignore_patterns
test_bash_completion_subcommands
test_zsh_completion_syntax
test_fish_completion_file

print_summary
