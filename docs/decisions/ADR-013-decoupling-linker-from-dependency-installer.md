# ADR-013: Decoupling Linker Operations from Active Dependency Installation

## Status
Accepted

## Date
- **Decision Date**: 2026-08-17
- **Recorded Date**: 2026-08-17

## Context
`symdep` was originally conceived as a unified symlink manager and package dependency resolver. Historically, stowing a package via `symdep link <pkg>` would inspect required/optional tools and, if dependencies were missing, prompt the user on `stdin` to select and install them immediately using the active system package manager.

While convenient for interactive first-time dotfile bootstrapping, interleaving active package manager execution inside the core linker operation created several operational friction points:

1. **Non-Deterministic Network & Package Manager Dependencies**:
   - Linker operations are deterministic, sub-millisecond local VFS mutations (`symlink`, `unlink`, atomic unfolding).
   - Package managers (`pacman`, `apt`, `brew`, `dnf`) are non-deterministic: they require active network connectivity, lock system databases, require root/privilege escalation (`sudo`/`doas`), and can fail due to upstream repository 404s or network timeouts.
   - An interactive prompt or package manager failure would block or abort the local symlink deployment.
2. **Friction in CI/CD & Automated Non-Interactive Environments**:
   - In Docker containers, automated tests, headless servers, and shell scripts, running `symdep link` should execute purely as an offline, fast symlink generator without blocking on interactive stdin prompts or requesting sudo passwords unexpectedly.
3. **Module Coupling**:
   - The low-level VFS linker core (`linker_ops.c`) depended on high-level package manager subprocesses, muddying separation of concerns.

## Decision
Decouple **Linker Operations** from **Active Dependency Installation** at both the core engine and CLI workflow levels:

1. **Non-Blocking Passive Dependency Audit during `symdep link`**:
   - `symdep link <pkg>` performs filesystem symlinking immediately.
   - It performs a non-blocking passive audit of `.symdeps` manifest dependencies: satisfied tools are confirmed, and missing tools are logged as informational warnings (`[WARN] Missing required tool: <name>`) with actionable hints (`Run 'symdep deps install <pkg>' or 'symdep link -y <pkg>'`).
   - `symdep link` **never prompts or blocks on `stdin`**.
2. **Dedicated Standalone Dependency Installer (`symdep deps install` / `symdep install`)**:
   - Introduce `symdep deps install [pkg...]` (and top-level alias `symdep install [pkg...]`) dedicated purely to dependency resolution and package manager installation without touching symlinks.
   - Supports `-y` / `--install` for non-interactive automated installs and `-n` / `--dry-run` for installation command preview.
3. **Opt-in Unified All-in-One Experience (`symdep link -y`)**:
   - For users who desire one-command bootstrap, `symdep link -y <pkg>` (or `symdep link --install <pkg>`) automatically executes package manager installation first, then deploys symlinks on success.
4. **Pure Non-Blocking Package Manager & Privilege Elevation Probing**:
   - Package manager resolution and elevation tool probing (`sudo`, `doas`, `tsu`) auto-detect available tools on `$PATH` without interactive terminal prompt interruptions.

## Command Matrix & Mental Model

| Command | Dependency Resolution | Symlink Creation | Primary Use Case |
| :--- | :--- | :--- | :--- |
| `symdep link [pkg...]` | **Passive Audit** (logs missing tools, non-blocking) | **Active** (deploys symlink tree) | Everyday config deployment, scripts, CI/CD |
| `symdep link -y [pkg...]` | **Active Install** (installs missing packages) | **Active** (deploys symlink tree) | First-time machine bootstrapping |
| `symdep deps install [pkg...]` | **Active Install** (installs missing packages) | **None** (does not touch symlinks) | Pure system dependency management |
| `symdep diff [pkg...]` | **Dry-Run Audit** (previews dependencies) | **Dry-Run** (previews symlink diff) | Pre-flight inspection |

## Alternatives Considered

### Retaining Interactive Prompts in Default `symdep link`
- **Pros**: Fewer total subcommands.
- **Cons**: Causes unexpected terminal pauses, breaks non-interactive scripts, and risks failed symlink operations due to external network issues.
- **Rejected**: Separating passive auditing from active execution provides cleaner UX and predictable automation.

### Splitting into Two Separate Binaries (`symlink` and `depmgr`)
- **Pros**: Complete physical decoupling.
- **Cons**: Destroys `symdep`'s unified identity and increases maintenance overhead.
- **Rejected**: Keeping both engines within `symdep` while providing clean CLI routing delivers the best developer experience.

## Consequences
- **Predictable, Offline-First Symlinking**: `symdep link` executes with zero network dependencies and sub-millisecond latency.
- **Clean CI/CD Integration**: Headless scripts and automated dotfile sync pipelines run reliably without stdin locks.
- **Flexible Workflows**: Developers can install dependencies independently of deploying symlinks.
- **Zero Breaking Changes to Unified Bootstrapping**: Existing `symdep link -y` workflows continue to provide all-in-one installation.
