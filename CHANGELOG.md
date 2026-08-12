# Changelog

All notable changes to the `symdep` project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.0] - 2026-08-12

### Added
- **Zero-Dependency ISO C17 Architecture**: High-performance binary with zero external library or runtime dependencies.
- **Dynamic Symlink Unfolding Engine (`fix-conflicts`)**: Automatic detection and unfolding of directory symlinks in target folders to prevent folding collisions.
- **Cross-Distro Package Manager Integration**: Auto-detection and installation support across `pacman`, `apt`, `dnf`, `apk`, and `brew`.
- **Privilege Elevation Tool Abstraction**: Automatic detection and support for `sudo`, `doas`, `tsu`, or unprivileged execution.
- **Tool & Plugin Registry (`symdep.registry`)**: Map virtual package names to distribution-specific package names (e.g. `bat@ubuntu=batcat`) and shell plugin directory paths (`plugin:~/.zsh/plugins/tool`).
- **AST / Shebang Dependency Scanner (`scan`)**: Recursive static analysis of script files, shebang lines, and configuration DSLs to auto-generate `.symdeps` manifests with interactive (`-i`), dry-run (`-n`), auto-save (`-y`), and preview modes.
- **Hierarchical Ignore Rules (`.symignore`)**: Global and package-level glob pattern filtering with redundancy detection and legacy `.stowignore` support.
- **Standardized CRUD Command Namespaces**: Intuitive subcommands for `pkg` (`create`, `remove`, `list`), `deps` (`add`, `edit`, `remove`, `show`, `target`), `ignore` (`init`, `add`, `remove`, `show`, `clear`), and `config` (`show`, `set`, `add`, `remove`).
- **Multi-Repository & Per-Package Target Configuration**: Support for multiple source repositories and per-package target home directory overrides (`TARGET="/path"` in `.symdeps`).
- **Symlink Health & Integrity Audit (`check-symlinks`)**: Verification of package requirements, broken symlinks, and orphan stowed files.
- **Nanosecond Performance Profiler (`-p`, `--profile`)**: High-precision execution timing for debugging and benchmarking deployment workloads.
- **Clang LTO, PGO, and Sanitizer Build Profiles**: Advanced Makefile targets for ThinLTO (`build-clang-opt`), 2-stage PGO (`build-pgo`), AddressSanitizer/UBSan (`build-sanitize`), and binary size optimization (`build-size`).
- **Comprehensive Test Suite**: 79 unit tests and 6 feature integration test suites covering all CLI workflows.
