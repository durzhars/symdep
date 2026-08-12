# ADR-006: Hierarchical Ignore Rule Engine with Inheritance and Redundancy Detection

## Status
Accepted

## Date
- **Decision / Commit Date**: 2026-08-02 (Commit `1154589`)
- **Recorded Date**: 2026-08-12

## Context
Dotfile repositories contain non-symlinked metadata (e.g. `.git`, `.gitignore`, `README.md`, `LICENSE`, `.DS_Store`, compiled binaries, temporary editor files). Users also require custom ignore rules at both the repository root level (global) and within individual package directories.

Without clear rule precedence and inheritance, users face unwanted symlink generation or duplicate pattern maintenance across packages.

## Decision
Implement a **Hierarchical Ignore Engine (`.symignore`)**:
1. **Built-in Standard Defaults**: Automatically ignores standard VCS metadata (`.git`, `.gitignore`, `.DS_Store`), documentation files (`README*`, `LICENSE*`), and backup files (`*~`, `#*#`, `.symdep_backup_*`).
2. **Global Ignore File**: Evaluates patterns in `root/.symignore` (or legacy `root/.stowignore`) across all packages.
3. **Package Ignore File**: Evaluates patterns in `root/<package>/.symignore` (or legacy `root/<package>/.stowignore`) specific to that package.
4. **Redundancy Warning**: When viewing active rules (`symdep ignore show`), `symdep` highlights package-level patterns that are already matched by global patterns.

## Syntax Negation (`!pattern`) Status & Planned Support
- **Current Support**: `.symignore` uses standard glob pattern matching (wildcards `*`, `?`, `[a-z]`, trailing `/` for directories). Currently, **syntax negation (`!pattern`) is NOT supported**. Lines beginning with `!` are treated as standard literal patterns or ignored.
- **Planned Future Feature**: Support for pattern negation (`!pattern`) is planned for a future update. This will allow package-level `.symignore` files to explicitly un-ignore or override patterns set in global `.symignore` (e.g. globally ignoring `*.log` while allowing `!app.log` in a specific debug package).

## Alternatives Considered

### Relying solely on `.gitignore`
- **Pros**: Reuses existing file.
- **Cons**: Conflates git version control ignore rules with symlink deployment rules. (For example, users may want git to track a config file but ignore deploying it on specific systems).
- **Rejected**: Dedicated `.symignore` gives explicit control over symlinking.

## Consequences
- Clean target home directory layouts with no leaking metadata.
- Backward compatibility for legacy `.stowignore` files.
- Clear specification of current pattern matching capabilities and future negation (`!pattern`) roadmap.
- Command family (`symdep ignore init/add/remove/show/clear`) for easy CLI management of ignore patterns.
