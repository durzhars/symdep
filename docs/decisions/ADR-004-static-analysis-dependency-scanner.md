# ADR-004: AST / Shebang Code-Analysis Dependency Scanner Engine

## Status
Accepted

## Date
- **Decision / Commit Date**: 2026-08-12 (Commit `bac5d0c`)
- **Recorded Date**: 2026-08-12

## Context
When maintaining dotfiles and configuration packages, manually identifying all required binaries, shell commands, shebang interpreters, and plugins across complex shell scripts (e.g. `~/.zshrc`, Hyprland configs, Neovim lua scripts) is labor-intensive and prone to missing runtime dependencies.

## Decision
Implement a native **Static Analysis Dependency Scanner (`symdep scan`)**:
1. Recursively parses package files (`scanner_parser.c`) for shebang lines (`#!/bin/bash`, `#!/usr/bin/env python3`), direct binary invocations, and tool references.
2. Cross-references discovered tools against `$PATH`, built-in package manager tables, and `symdep.registry`.
3. Operates in preview mode (default), dry-run mode (`-n`), auto-save mode (`-y`), or interactive wizard mode (`-i`).
4. Updates package `.symdeps` manifests automatically with discovered required (`--required`) or optional (`--optional`) tools.

## Inherent Limitations of Static Analysis
Static parsing in shell scripts and configuration files has fundamental theoretical and safety boundaries:
- **Dynamic Variable Expansion**: Static analysis **cannot and should not** attempt to evaluate runtime shell variable expansions (e.g., `$EXEC_CMD`, `${TOOL_NAME:-bat}`, `$1`).
- **Command Substitutions & String Construction**: Computed commands (e.g., `$(which foo)`, `` `find_bin` ``) cannot be statically predicted without full shell interpreter evaluation.
- **Dynamic Evaluation (`eval` / `exec`)**: Code evaluated dynamically via `eval "$DYNAMIC_STUB"` or indirect shell dispatch is intentionally ignored.
- **Safety Rationale**: Attempting to execute or evaluate arbitrary shell logic during scan time would expose users to arbitrary code execution vulnerabilities, side effects, or infinite shell expansion loops.
- **Resolution Strategy**: Dynamic tools that cannot be detected via static AST/shebang analysis must be declared manually using `symdep deps add <pkg> <dep>` or registered as tool aliases in `symdep.registry`.

## Alternatives Considered

### Manual Dependency Entry Only
- **Pros**: Zero parsing logic needed.
- **Cons**: High burden on dotfile maintainers; manifests frequently become outdated.
- **Rejected**: Automating discovery reduces setup friction for new users.

### Dynamic Tracing (e.g. `strace` / `execve` auditing)
- **Pros**: Catches actual executed binaries at runtime.
- **Cons**: Requires executing untrusted scripts; OS-dependent (`strace` on Linux, `dtrace` on macOS); fails to catch conditional runtime branches not executed during trace.
- **Rejected**: Static file scanning is faster, platform-independent, safer, and covers all script branches.

## Consequences
- Fast static scanning engine written directly in C (`scanner_parser.c` & `scanner.c`).
- Clear boundary separation: deterministic static scanning for shebangs and explicit binary calls; manual `deps add` or `symdep.registry` mappings for dynamic runtime command construction.
- `-i` (interactive) and `-y` (auto-save) allow instant manifest generation for newly scaffolded packages.
- Strict option validation prevents illegal flag combinations (e.g., performance profiler `-p` cannot be combined with interactive prompt `-i`).
