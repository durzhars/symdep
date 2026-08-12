# Symlink & Dependency Manager (`symdep`)

**Usage**: `symdep [options] <command> [arguments]`

A high-performance, zero-dependency symlink manager and package dependency resolver written in C.

## Global Options

- **`-d, --source-dir`** `<path>`                    Set source repository directory for current command (aliases: `--src-dir`, `--dotfiles-dir`) (e.g. `-d ~/src`)
- **`-t, --target-dir`** `<path>`                      Set target home directory for current command (e.g. `-t ~/`)
- **`-m, --manager`** `<name>`                         Override active package manager for current command (aliases: `--pkg-mgr`, `--package-manager`) (e.g. `-m yay`)
- **`-i, --interactive`**                              Interactive wizard to confirm discovered scanner dependencies
- **`-y, --install`**                                  Auto-confirm installation of missing required dependencies & optional plugins
- **`-n, --dry-run`**                                  Dry-run mode (preview disk changes, symlink creations & backups without modifying disk)
- **`-s, --save`**                                     Save command-line directory overrides (`-d`/`-t`) to config
- **`-p, --profile`**                                  Enable execution performance profiler logging
- **`-h, --help`**                                     Display this comprehensive help manual

## Configuration Commands (`config:*`)

- **`config show`**                                    Display active configuration, source repositories & package manager settings
- **`config set -m, --manager`** `<name>`             Set default package manager (alias: `--pkg-manager`) (e.g. `symdep config set --manager yay`)
- **`config set -e, --elevation`** `<tool>`            Set default privilege elevation tool (e.g. `symdep config set --elevation tsu`)
- **`config set -t, --target`** `<path>`               Set target home directory (e.g. `symdep config set --target ~/`)
- **`config set -d, --source`** `<path>`               Set primary source repository directory (e.g. `symdep config set --source ~/dotfiles`)
- **`config add`** `<path>`                            Add an additional source repository directory (multi-repo mode)
- **`config remove`** `<path>`                         Remove a source repository directory from config

## Package Management Commands (`pkg:*`)

- **`pkg create`** `<name>`                            Scaffold a new package directory & initialize `.symdeps` manifest (aliases: `pkg:create`, `package:create`, `make:pkg`)
- **`pkg remove`** `<name...>`                         Safely unlink and remove one or multiple package directories (aliases: `pkg:remove`, `package:remove`, `pkg:rm`, `remove`)
- **`pkg list`**                                       List all packages with status: `[LINKED]`, `[PARTIAL]`, or `[UNLINKED]` (aliases: `pkg:list`, `package:list`, `pkg:show`, `list`)

## Dependency & Manifest Commands (`deps:*`)

- **`deps add`** `<pkg> <dep> [--type]`                Add dependency/conflict to `.symdeps` (`--required`, `--optional`, `--conflict`)
- **`deps edit`** `<pkg> <dep> <type>`                 Edit existing dependency classification (`--required`, `--optional`, `--conflict`)
- **`deps remove`** `<pkg> <dep>`                        Remove a dependency or conflict entry from package `.symdeps` (alias: `deps rm`)
- **`deps show`** `<pkg>`                              Display raw `.symdeps` manifest contents for a package
- **`deps target`** `<pkg> <path>`                     Set per-package target directory in `.symdeps` manifest
- **`scan`** `[pkg...]`                                Recursively scan package scripts/configs to auto-detect required tools & plugins

## File Filtering Commands (`ignore:*`)

- **`ignore init`** `[pkg...]`                         Scaffold global or package-level `.symignore` template
- **`ignore add`** `[pkg] <pat...>`                    Append glob pattern(s) to package or global (`-g`) `.symignore`
- **`ignore remove`** `[pkg] <pat...>`                 Remove glob pattern(s) from `.symignore` (`-g` for global)
- **`ignore clear`** `[pkg...]`                        Purge `.symignore` file(s) for package(s) or repo root
- **`ignore show`** `[pkg...]`                         Display active `.symignore` patterns (alias: `ignore list`)

## Symlink & Deployment Commands

- **`link`** `<pkg...>`                                Link / deploy one or multiple packages (aliases: `stow`, `deploy`)
- **`unlink`** `<pkg...>`                              Unlink / remove symlinks for one or multiple packages (alias: `unstow`)
- **`relink`** `<pkg...>`                              Relink (unlink & link) one or multiple packages (alias: `restow`)
- **`all`**                                            Link all packages present in source repository
- **`diff`** `[pkg...]`                                Preview symlink creations, conflict backups, and missing dependencies (dry-run)
- **`fix`**                                            Unfold directory symlinks in target to resolve collisions (alias: `fix-conflicts`)
- **`check`** `[pkg...]`                               Verify required/optional tools, plugins, and symlink integrity for packages
- **`check-symlinks`**                                 Scan repository & target home for broken symlinks and unmanaged orphan symlinks
- **`help`**                                           Display this help manual

## Workflow Examples

- **Scaffold & Configure Package**:
  `symdep pkg create hyprland`
  `symdep deps add hyprland waybar --required`
  `symdep deps add hyprland rofi --optional`

- **Link Package with Auto-Install**:
  `symdep -y link hyprland`

- **Preview Link Changes (Dry-Run)**:
  `symdep -n link terminal`

- **Unlink & Delete Package**:
  `symdep pkg remove hyprland`
