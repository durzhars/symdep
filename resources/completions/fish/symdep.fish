# Fish completion for symdep (Symlink & Dependency Manager)

function __fish_symdep_needs_command
    set -l cmd (commandline -opc)
    if test (count $cmd) -eq 1
        return 0
    end
    # Check if all preceding tokens are flags
    for arg in $cmd[2..-1]
        switch $arg
            case '-*'
                continue
            case '*'
                return 1
        end
    end
    return 0
end

function __fish_symdep_using_command
    set -l cmd (commandline -opc)
    set -l target $argv[1]
    if test (count $cmd) -gt 1
        for arg in $cmd[2..-1]
            if not string match -q -- '-*' $arg
                if test "$arg" = "$target"
                    return 0
                end
                return 1
            end
        end
    end
    return 1
end

function __fish_symdep_subcommand_is
    set -l cmd (commandline -opc)
    set -l target_cmd $argv[1]
    set -l target_sub $argv[2]
    set -l found_cmd ""

    for arg in $cmd[2..-1]
        if not string match -q -- '-*' $arg
            if test -z "$found_cmd"
                set found_cmd $arg
            else if test "$found_cmd" = "$target_cmd" -a "$arg" = "$target_sub"
                return 0
            else
                return 1
            end
        end
    end
    return 1
end

function __fish_symdep_find_config_file
    if set -q SYMDEP_CONFIG_FILE; and test -f "$SYMDEP_CONFIG_FILE"
        echo "$SYMDEP_CONFIG_FILE"
        return 0
    end
    if set -q STOW_CONFIG_FILE; and test -f "$STOW_CONFIG_FILE"
        echo "$STOW_CONFIG_FILE"
        return 0
    end
    set -l xdg "$HOME/.config"
    if set -q XDG_CONFIG_HOME
        set xdg "$XDG_CONFIG_HOME"
    end
    if test -f "$xdg/symdep/config"
        echo "$xdg/symdep/config"
        return 0
    end
    if test -f "$xdg/stow-manager/config"
        echo "$xdg/stow-manager/config"
        return 0
    end
    return 1
end

function __fish_symdep_get_all_source_dirs
    set -l cmd (commandline -opc)
    set -l sdirs

    # 1. CLI flags (-d, --source-dir, --src-dir, --dotfiles-dir)
    for i in (seq 1 (count $cmd))
        switch $cmd[$i]
            case '-d' '--source-dir' '--src-dir' '--dotfiles-dir'
                set -l next_idx (math $i + 1)
                if test $next_idx -le (count $cmd)
                    set sdirs $sdirs $cmd[$next_idx]
                end
            case '--source-dir=*' '--src-dir=*' '--dotfiles-dir=*'
                set sdirs $sdirs (string split -m1 '=' $cmd[$i])[2]
        end
    end

    # 2. Environment variables
    if test (count $sdirs) -eq 0
        if set -q SYMDEP_SOURCE_DIR
            set sdirs (string split ':' $SYMDEP_SOURCE_DIR)
        else if set -q SOURCE_DIR
            set sdirs (string split ':' $SOURCE_DIR)
        else if set -q STOW_DOTFILES_DIR
            set sdirs (string split ':' $STOW_DOTFILES_DIR)
        else if set -q DOTFILES_DIR
            set sdirs (string split ':' $DOTFILES_DIR)
        end
    end

    # 3. Config file
    if test (count $sdirs) -eq 0
        set -l cfg (__fish_symdep_find_config_file)
        if test -n "$cfg"; and test -f "$cfg"
            set -l raw (grep -E '^[[:space:]]*SOURCE_DIRS=' "$cfg" 2>/dev/null | head -n1 | cut -d= -f2- | tr -d '"' | tr -d "'")
            if test -n "$raw"
                set sdirs (string split ':' $raw)
            end
        end
    end

    # 4. Upward directory traversal
    if test (count $sdirs) -eq 0
        set -l curr (pwd)
        while test "$curr" != "/" -a -n "$curr"
            if test -f "$curr/symdep.registry" -o -f "$curr/.symdepregistry" -o -f "$curr/stow.registry" -o -f "$curr/.stowregistry"
                set sdirs $curr
                break
            end
            set curr (dirname "$curr")
        end
    end

    # 5. Standard fallback dotfiles directories
    if test (count $sdirs) -eq 0
        for candidate in "$HOME/dotfiles" "$HOME/.dotfiles" "$HOME/.config/dotfiles" "$HOME/dots" "$HOME/.dots"
            if test -d "$candidate"
                set sdirs $candidate
                break
            end
        end
    end

    # 6. Fallback to current working directory
    if test (count $sdirs) -eq 0
        set sdirs "."
    end

    for d in $sdirs
        if test -d "$d"
            echo "$d"
        end
    end
end

function __fish_symdep_packages
    set -l sdirs (__fish_symdep_get_all_source_dirs)
    set -l cmd (commandline -opc)

    for sdir in $sdirs
        if test -d "$sdir"
            for item in $sdir/*
                if test -d "$item"
                    set -l b (basename "$item")
                    switch $b
                        case '.*' 'build' 'bin' 'include' 'src' 'tests' 'Testing' 'profiles' 'resources' 'docs' '.agents'
                            continue
                        case '*'
                            # Filter out packages already present on command line
                            if not contains -- $b $cmd
                                echo "$b"
                            end
                    end
                end
            end
        end
    end
end

function __fish_symdep_pkg_deps
    set -l cmd (commandline -opc)
    set -l target_pkg ""
    for arg in $cmd[3..-1]
        if not string match -q -- '-*' $arg
            set target_pkg $arg
            break
        end
    end

    if test -n "$target_pkg"
        set -l sdirs (__fish_symdep_get_all_source_dirs)
        for sdir in $sdirs
            set -l mfile "$sdir/$target_pkg/.symdeps"
            if not test -f "$mfile"
                set mfile "$sdir/$target_pkg/.stowdeps"
            end
            if test -f "$mfile"
                set -l raw (grep -E '^[[:space:]]*(REQUIRED|OPTIONAL|CONFLICTS)=' "$mfile" 2>/dev/null | cut -d= -f2- | tr -d '"' | tr -d "'" | tr " " "\n")
                for d in $raw
                    if test -n "$d"
                        echo "$d"
                    end
                end
            end
        end
    end
end

function __fish_symdep_pkg_ignores
    set -l cmd (commandline -opc)
    set -l target_pkg ""
    set -l is_global 0
    for arg in $cmd[3..-1]
        switch $arg
            case '-g' '--global'
                set is_global 1
            case '-*'
                continue
            case '*'
                if test -z "$target_pkg"
                    set target_pkg $arg
                end
        end
    end

    set -l sdirs (__fish_symdep_get_all_source_dirs)
    for sdir in $sdirs
        set -l ifile
        if test $is_global -eq 1 -o -z "$target_pkg"
            set ifile "$sdir/.symignore"
            if not test -f "$ifile"
                set ifile "$sdir/.stowignore"
            end
        else
            set ifile "$sdir/$target_pkg/.symignore"
            if not test -f "$ifile"
                set ifile "$sdir/$target_pkg/.stowignore"
            end
        end

        if test -f "$ifile"
            set -l raw (grep -v "^#" "$ifile" 2>/dev/null | grep -v "^[[:space:]]*\$")
            for line in $raw
                if test -n "$line"
                    echo "$line"
                end
            end
        end
    end
end

function __fish_symdep_registry_tools
    set -l sdirs (__fish_symdep_get_all_source_dirs)
    for sdir in $sdirs
        for rname in "symdep.registry" ".symdepregistry" "stow.registry" ".stowregistry"
            set -l rfile "$sdir/$rname"
            if test -f "$rfile"
                set -l raw (grep -v "^#" "$rfile" 2>/dev/null | grep -v "^[[:space:]]*\$" | cut -d= -f1 | cut -d@ -f1 | tr -d " ")
                for tool in $raw
                    if test -n "$tool"
                        echo "$tool"
                    end
                end
                return 0
            end
        end
    end
    for rfile in "/usr/local/share/symdep/symdep.registry" "/usr/share/symdep/symdep.registry" "resources/symdep.registry"
        if test -f "$rfile"
            set -l raw (grep -v "^#" "$rfile" 2>/dev/null | grep -v "^[[:space:]]*\$" | cut -d= -f1 | cut -d@ -f1 | tr -d " ")
            for tool in $raw
                if test -n "$tool"
                    echo "$tool"
                end
            end
            return 0
        end
    end
end

function __fish_symdep_config_sources
    set -l cfg (__fish_symdep_find_config_file)
    if test -n "$cfg"; and test -f "$cfg"
        set -l raw (grep -E '^[[:space:]]*SOURCE_DIRS=' "$cfg" 2>/dev/null | head -n1 | cut -d= -f2- | tr -d '"' | tr -d "'")
        for d in (string split ':' $raw)
            if test -n "$d"
                echo "$d"
            end
        end
    end
end

# Disable default file completions unless requested
complete -c symdep -f

# Global Options
complete -c symdep -s d -l source-dir -l src-dir -l dotfiles-dir -d "Set source repository directory" -r -a "(__fish_complete_directories)"
complete -c symdep -s t -l target-dir -d "Set target home directory" -r -a "(__fish_complete_directories)"
complete -c symdep -s m -l manager -l pkg-mgr -l package-manager -d "Override active package manager" -x -a "pacman yay paru apt dnf apk brew nix-env zypper emerge xbps-install pkg"
complete -c symdep -s i -l interactive -d "Launch interactive dependency confirmation wizard"
complete -c symdep -s y -l install -d "Auto-confirm missing dependencies and plugins"
complete -c symdep -s n -l dry-run -d "Preview operations without modifying filesystem"
complete -c symdep -s s -l save -d "Save CLI directory overrides to configuration"
complete -c symdep -s p -l profile -l perf -l performance -l profiler -d "Enable nanosecond execution profiler"
complete -c symdep -s h -l help -d "Display help manual"

# Top-level Subcommands
complete -c symdep -n "__fish_symdep_needs_command" -a "link" -d "Deploy symlinks for package(s)"
complete -c symdep -n "__fish_symdep_needs_command" -a "stow" -d "Deploy symlinks (GNU Stow alias)"
complete -c symdep -n "__fish_symdep_needs_command" -a "deploy" -d "Deploy symlinks for package(s)"
complete -c symdep -n "__fish_symdep_needs_command" -a "unlink" -d "Safely remove package symlinks"
complete -c symdep -n "__fish_symdep_needs_command" -a "unstow" -d "Remove symlinks (GNU Stow alias)"
complete -c symdep -n "__fish_symdep_needs_command" -a "relink" -d "Relink (unlink then link) package(s)"
complete -c symdep -n "__fish_symdep_needs_command" -a "restow" -d "Relink packages (GNU Stow alias)"
complete -c symdep -n "__fish_symdep_needs_command" -a "install" -d "Install missing package dependencies"
complete -c symdep -n "__fish_symdep_needs_command" -a "all" -d "Link all packages in source repository"
complete -c symdep -n "__fish_symdep_needs_command" -a "diff" -d "Preview pending symlinks, backups, and dependencies"
complete -c symdep -n "__fish_symdep_needs_command" -a "scan" -d "Auto-scan package scripts/configs for dependencies"
complete -c symdep -n "__fish_symdep_needs_command" -a "check" -d "Verify dependencies and symlink health"
complete -c symdep -n "__fish_symdep_needs_command" -a "check-symlinks" -d "Scan repository for broken and orphan symlinks"
complete -c symdep -n "__fish_symdep_needs_command" -a "fix" -d "Unfold directory symlinks to resolve collisions"
complete -c symdep -n "__fish_symdep_needs_command" -a "fix-conflicts" -d "Unfold directory symlinks to resolve collisions"
complete -c symdep -n "__fish_symdep_needs_command" -a "pkg" -d "Package management (create, remove, list)"
complete -c symdep -n "__fish_symdep_needs_command" -a "deps" -d "Dependency and conflict manifest management"
complete -c symdep -n "__fish_symdep_needs_command" -a "ignore" -d "File filtering and .symignore rules"
complete -c symdep -n "__fish_symdep_needs_command" -a "config" -d "Global configuration and multi-repo settings"
complete -c symdep -n "__fish_symdep_needs_command" -a "help" -d "Display help manual"
complete -c symdep -n "__fish_symdep_needs_command" -a "remove" -d "Safely unlink and remove package directory"

# Default to package names when no subcommand is specified (shorthand: symdep <pkg>)
complete -c symdep -n "__fish_symdep_needs_command" -a "(__fish_symdep_packages)" -d "Package"

# Package-taking Commands
for cmd in link stow deploy unlink unstow relink restow install diff scan check remove
    complete -c symdep -n "__fish_symdep_using_command $cmd" -a "(__fish_symdep_packages)" -d "Package"
end

# pkg subcommands
complete -c symdep -n "__fish_symdep_using_command pkg" -a "create" -d "Scaffold new package directory & manifest"
complete -c symdep -n "__fish_symdep_using_command pkg" -a "remove" -d "Safely unlink and delete package directory"
complete -c symdep -n "__fish_symdep_using_command pkg" -a "list" -d "List all packages with deployment status"
complete -c symdep -n "__fish_symdep_subcommand_is pkg remove" -a "(__fish_symdep_packages)" -d "Package to remove"

# deps subcommands
complete -c symdep -n "__fish_symdep_using_command deps" -a "install" -d "Install missing package dependencies"
complete -c symdep -n "__fish_symdep_using_command deps" -a "add" -d "Add dependency or conflict to manifest"
complete -c symdep -n "__fish_symdep_using_command deps" -a "edit" -d "Modify dependency classification"
complete -c symdep -n "__fish_symdep_using_command deps" -a "remove" -d "Remove dependency or conflict from manifest"
complete -c symdep -n "__fish_symdep_using_command deps" -a "show" -d "Display raw .symdeps manifest contents"
complete -c symdep -n "__fish_symdep_using_command deps" -a "target" -d "Set per-package target directory override"

complete -c symdep -n "__fish_symdep_subcommand_is deps install" -a "(__fish_symdep_packages)" -d "Package"

complete -c symdep -n "__fish_symdep_subcommand_is deps add" -a "(__fish_symdep_packages)" -d "Package"
complete -c symdep -n "__fish_symdep_subcommand_is deps add" -a "(__fish_symdep_registry_tools)" -d "Known Registry Tool"
complete -c symdep -n "__fish_symdep_subcommand_is deps add" -l required -d "Classify as required tool"
complete -c symdep -n "__fish_symdep_subcommand_is deps add" -l optional -d "Classify as optional tool/plugin"
complete -c symdep -n "__fish_symdep_subcommand_is deps add" -l conflict -d "Classify as mutually exclusive conflicting package"

complete -c symdep -n "__fish_symdep_subcommand_is deps edit" -a "(__fish_symdep_packages)" -d "Package"
complete -c symdep -n "__fish_symdep_subcommand_is deps edit" -a "(__fish_symdep_pkg_deps)" -d "Existing Dependency"
complete -c symdep -n "__fish_symdep_subcommand_is deps edit" -l required -d "Classify as required tool"
complete -c symdep -n "__fish_symdep_subcommand_is deps edit" -l optional -d "Classify as optional tool/plugin"
complete -c symdep -n "__fish_symdep_subcommand_is deps edit" -l conflict -d "Classify as mutually exclusive conflicting package"

complete -c symdep -n "__fish_symdep_subcommand_is deps remove" -a "(__fish_symdep_packages)" -d "Package"
complete -c symdep -n "__fish_symdep_subcommand_is deps remove" -a "(__fish_symdep_pkg_deps)" -d "Existing Dependency"
complete -c symdep -n "__fish_symdep_subcommand_is deps show" -a "(__fish_symdep_packages)" -d "Package"
complete -c symdep -n "__fish_symdep_subcommand_is deps target" -a "(__fish_symdep_packages)" -d "Package"

# ignore subcommands
complete -c symdep -n "__fish_symdep_using_command ignore" -a "init" -d "Scaffold global or package-level .symignore"
complete -c symdep -n "__fish_symdep_using_command ignore" -a "add" -d "Append glob pattern to .symignore"
complete -c symdep -n "__fish_symdep_using_command ignore" -a "remove" -d "Remove glob pattern from .symignore"
complete -c symdep -n "__fish_symdep_using_command ignore" -a "show" -d "Display active merged ignore patterns"
complete -c symdep -n "__fish_symdep_using_command ignore" -a "clear" -d "Purge .symignore patterns"

for sub in init add remove show clear
    complete -c symdep -n "__fish_symdep_subcommand_is ignore $sub" -s g -l global -d "Apply to repository root global .symignore"
    complete -c symdep -n "__fish_symdep_subcommand_is ignore $sub" -a "(__fish_symdep_packages)" -d "Package"
end

complete -c symdep -n "__fish_symdep_subcommand_is ignore remove" -a "(__fish_symdep_pkg_ignores)" -d "Configured Ignore Pattern"

# config subcommands
complete -c symdep -n "__fish_symdep_using_command config" -a "show" -d "Display active configuration settings"
complete -c symdep -n "__fish_symdep_using_command config" -a "set" -d "Update package manager, elevation, or directory"
complete -c symdep -n "__fish_symdep_using_command config" -a "add" -d "Register additional source dotfiles repository"
complete -c symdep -n "__fish_symdep_using_command config" -a "remove" -d "Unregister source dotfiles repository"

complete -c symdep -n "__fish_symdep_subcommand_is config set" -s m -l manager -l pkg-mgr -l pkg-manager -d "Set preferred package manager" -x -a "pacman yay paru apt dnf apk brew nix-env zypper emerge xbps-install pkg"
complete -c symdep -n "__fish_symdep_subcommand_is config set" -s e -l elevation -l elevation-tool -d "Set privilege elevation tool" -x -a "sudo doas tsu none"
complete -c symdep -n "__fish_symdep_subcommand_is config set" -s t -l target -l target-dir -d "Set default target directory" -r -a "(__fish_complete_directories)"
complete -c symdep -n "__fish_symdep_subcommand_is config set" -s d -l source -l source-dir -l src-dir -l dotfiles-dir -d "Set primary source repository" -r -a "(__fish_complete_directories)"
complete -c symdep -n "__fish_symdep_subcommand_is config add" -d "Source directory to register" -r -a "(__fish_complete_directories)"
complete -c symdep -n "__fish_symdep_subcommand_is config remove" -d "Source directory to unregister" -x -a "(__fish_symdep_config_sources)"

# help topics
complete -c symdep -n "__fish_symdep_using_command help" -a "link unlink relink all diff scan check check-symlinks fix pkg deps ignore config" -d "Help topic"
