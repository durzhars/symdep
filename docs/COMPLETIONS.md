# Shell Autocompletion Guide

`symdep` provides high-performance autocompletion scripts for **Bash**, **Zsh**, and **Fish** shells.

Autocompletion features:
- **Dynamic package discovery**: Discovers packages across active and federated `SOURCE_DIRS` repositories.
- **Positional argument deduplication**: Filters out already-specified packages when chaining multiple package arguments (`symdep link hyprland <TAB>` will not suggest `hyprland` again).
- **Manifest-aware dependency completion**: Dynamically inspects `.symdeps` manifests to suggest existing dependencies for `symdep deps remove <pkg>` and `symdep deps edit <pkg>`.
- **Registry-aware tool discovery**: Suggests recognized tools and shell plugins from `symdep.registry` when running `symdep deps add <pkg>`.
- **Ignore-rule awareness**: Suggests existing patterns when running `symdep ignore remove <pkg>` or `symdep ignore remove -g`.
- **Subcommand & Namespace Routing**: Supports space-separated, colon-separated (`pkg:create`, `deps:add`, `ignore:show`, `config:set`), and top-level alias commands.
- **Flag & Option Completion**: Contextual completions for package managers (`pacman`, `apt`, `dnf`, `apk`, `brew`, `yay`, etc.), elevation tools (`sudo`, `doas`, `tsu`), and dependency classifications (`--required`, `--optional`, `--conflict`).

---

## Automatic System Installation

When installing `symdep` with `make install`, completion scripts are automatically copied to standard vendor directories:

```bash
sudo make install
```

Installed locations:
- **Bash**: `/usr/local/share/bash-completion/completions/symdep`
- **Zsh**: `/usr/local/share/zsh/site-functions/_symdep`
- **Fish**: `/usr/local/share/fish/vendor_completions.d/symdep.fish`

If you use a custom prefix (e.g. `PREFIX=$HOME/.local`), they will be placed under `$HOME/.local/share/...`.

---

## Manual / User-Level Setup

If you build `symdep` locally without system-wide installation, you can source or link the completion scripts directly into your user shell configuration.

### Bash

Add the following to your `~/.bashrc`:

```bash
# Sourcing symdep completion directly
source /path/to/symdep/resources/completions/bash/symdep
```

Or copy the script to your user completion directory:

```bash
mkdir -p ~/.local/share/bash-completion/completions
cp resources/completions/bash/symdep ~/.local/share/bash-completion/completions/symdep
```

### Zsh

Add the completion directory to your `fpath` before calling `compinit` in `~/.zshrc`:

```zsh
# Add symdep zsh completion directory to fpath
fpath=(/path/to/symdep/resources/completions/zsh $fpath)

autoload -U compinit
compinit
```

Or copy `_symdep` to a custom functions directory in your existing `$fpath`:

```zsh
mkdir -p ~/.zsh/completion
cp resources/completions/zsh/_symdep ~/.zsh/completion/
# Ensure ~/.zsh/completion is added to fpath in ~/.zshrc
```

### Fish

Copy the fish completion script to your fish completions directory:

```fish
mkdir -p ~/.config/fish/completions
cp resources/completions/fish/symdep.fish ~/.config/fish/completions/
```

Completions will be autoloaded on the next shell session or command execution.

---

## Dynamic Package Completion Behavior

The completion engine determines the source directory using the following lookup order:

1. Flag argument `-d` / `--source-dir` passed earlier in the current command line.
2. Environment variables `SYMDEP_SOURCE_DIR`, `SOURCE_DIR`, or `DOTFILES_DIR`.
3. The `SOURCE_DIRS` primary entry inside `~/.config/symdep/config` (or `$XDG_CONFIG_HOME/symdep/config`).
4. Current working directory (`.`).

Directories beginning with `.` (e.g. `.git`) or build directories are automatically filtered out from package suggestions.
