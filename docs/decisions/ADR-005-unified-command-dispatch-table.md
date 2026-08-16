# ADR-005: Multi-Syntax CLI Interface & Drop-in Compatibility Architecture

## Status
Accepted

## Date
- **Decision / Commit Date**: 2026-08-04 (Commit `6e2e7d8`)
- **Recorded Date**: 2026-08-12

## Context
Command-line dotfile management tools must bridge two fundamentally different developer workflows:
1. **Drop-in Compatibility with GNU Stow**: Existing users and automation scripts rely on traditional GNU Stow flags (`-S`, `-D`, `-R`, `--dotfiles-dir`, `--target-dir`) and commands (`stow`, `unstow`, `restow`).
2. **Modern Subcommand & Namespace Syntaxes**: Modern toolchains utilize space-separated namespaces (`symdep pkg create`, `symdep deps add`) or colon-separated action syntax popular in task runners and build tools (`symdep pkg:create`, `symdep deps:add`, `symdep config:set`).
3. **Frictionless Shorthand**: Users frequently want to deploy packages without verbose keywords (e.g. running `symdep hyprland waybar` directly).

Supporting all three invocation styles within a single zero-dependency C17 binary without introducing third-party parser dependencies or fragile `getopt` spaghetti requires a unified interface contract.

## Decision
Implement a **Multi-Syntax CLI Routing Architecture** powered by a single-source-of-truth routing engine (`ROUTE_TABLE[]` in `src/cli/cmd_table.c` and `src/cli/dispatch/dispatch_core.c`):

### Route Resolution Priority & Implicit Package Fallback
When CLI arguments are passed to `symdep`, `dispatch_command` evaluates tokens in strict sequential order:

1. **Exact Group & Subcommand Route Match**:
   - Checks space-separated group and subcommand combinations (e.g. `symdep pkg create <name>`).
   - Checks colon-separated group/subcommand syntax (e.g. `symdep pkg:create <name>`, `symdep deps:add <pkg> <dep>`).
2. **Command Alias Match**:
   - Checks top-level alias matches registered in `ROUTE_TABLE` (e.g. `symdep link`, `symdep deploy`, `symdep stow`, `symdep fix`).
3. **Group Usage Guidance Fallback**:
   - If the first argument (`token1`) matches a known command group (e.g. `pkg`, `deps`, `ignore`, `config`) but the second argument is missing or invalid, `symdep` prints the list of available subcommands for that group.
4. **Implicit Package Linking Fallback**:
   - If `token1` does NOT match any registered command or group, `symdep` checks whether all positional arguments correspond to valid existing package directories within the active source repository (`source_dir`).
   - If **all** arguments are valid package directories, `symdep` implicitly routes the invocation to package deployment (`cmd_stow`), allowing clean shorthand syntaxes like `symdep hyprland waybar` (equivalent to `symdep link hyprland waybar`).
5. **Unknown Command Error**:
   - If any argument fails the package directory check and does not match a command route, `dispatch_command` outputs `Unknown command: <token1>` and exits with error code 1.

## Alternatives Considered

### Standard `getopt_long` Subcommand Parsing
- **Pros**: Standard C library usage.
- **Cons**: Difficult to support multi-word subcommand namespaces and colon-separated aliases cleanly.
- **Rejected**: Custom dispatch table provides full flexibility and cleaner error messages.

### Third-Party CLI Framework (e.g. `docopt`, `cli.c`)
- **Pros**: Pre-built parser.
- **Cons**: Introduces external dependencies, increasing binary size and build complexity.
- **Rejected**: Violates zero-dependency architecture principle (ADR-001).

## Consequences
- Clean, extensible CLI architecture where adding a new command requires only a single entry in `ROUTE_TABLE[]`.
- Intuitive user experience: explicit namespace commands (`pkg create`), shorthand aliases (`link`), and zero-keyword implicit package linking (`symdep hyprland`).
- Predictable, deterministic route resolution order preventing ambiguous command interpretation.
- Consistent error reporting across all command namespaces and aliases.
