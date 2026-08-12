# ADR-005: Unified Command Dispatch Table and Multi-Namespace Syntax

## Status
Accepted

## Date
- **Decision / Commit Date**: 2026-08-04 (Commit `6e2e7d8`)
- **Recorded Date**: 2026-08-12

## Context
Command-line tools evolve as their feature sets expand. `symdep` began with traditional GNU Stow command compatibility (`stow`, `unstow`, `restow`), expanded to CRUD operations for package management (`pkg`), dependencies (`deps`), file filtering (`ignore`), and system configuration (`config`), and supports colon-separated subcommand syntaxes popular in build and automation tools (`pkg:create`, `deps:add`, `ignore:add`, `config:set`).

Handling these diverse invocation forms with scattered `if/else` checks leads to fragile flag parsing and inconsistent usage messages.

## Decision
Implement a single-source-of-truth **Command Routing Table** (`ROUTE_TABLE[]` in `cmd_table.c` and `cmd_routes.h`) managed by `dispatch_command` (`src/cli/dispatch/dispatch_core.c`):

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
