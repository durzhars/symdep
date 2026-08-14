# Changelog

All notable changes to the `symdep` project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.0] - 2026-08-15

### Added
- **Zero-Dependency ISO C17 Architecture**: High-performance binary with zero external library or runtime dependencies.
- **Linux `io_uring` Asynchronous Kernel Backend**: SQE ring-buffer submission for ultra-fast batch symlinking on modern Linux kernels.
- **Ultra-Small Musl Static Build (`static-musl`)**: Single target `make static-musl` producing a ~230KB standalone binary with direct `/etc/passwd` fallback for zero-glibc/NSS dependencies.
- **Distribution Package Generator (`make dist`)**: Automated target to generate release `.tar.gz` distribution tarballs and SHA256 checksums.
- **Dynamic Symlink Unfolding Engine (`fix-conflicts`)**: Automatic detection and unfolding of directory symlinks in target folders to prevent folding collisions.
- **Cross-Distro Package Manager Integration**: Auto-detection and installation support across `pacman`, `apt`, `dnf`, `apk`, and `brew`.
- **Privilege Elevation Tool Abstraction**: Automatic detection and support for `sudo`, `doas`, `tsu`, or unprivileged execution.
- **Tool & Plugin Registry (`symdep.registry`)**: Map virtual package names to distribution-specific package names (e.g. `bat@ubuntu=batcat`) and shell plugin directory paths (`plugin:~/.zsh/plugins/tool`).
- **AST / Shebang Dependency Scanner (`scan`)**: Recursive static analysis of script files, shebang lines, and configuration DSLs to auto-generate `.symdeps` manifests with interactive (`-i`), dry-run (`-n`), auto-save (`-y`), and preview modes.
- **Hierarchical Ignore Rules (`.symignore`)**: Global and package-level glob pattern filtering with redundancy detection and legacy `.stowignore` support.
- **Standardized CRUD Command Namespaces**: Intuitive subcommands for `pkg` (`create`, `remove`, `list`), `deps` (`add`, `edit`, `remove`, `show`, `target`), `ignore` (`init`, `add`, `remove`, `show`, `clear`), and `config` (`show`, `set`, `add`, `remove`).
- **Multi-Repository & Per-Package Target Configuration**: Support for multiple source repositories and per-package target home directory overrides (`TARGET="/path"` in `.symdeps`).
- **Symlink Health & Integrity Audit (`check-symlinks`)**: Verification of package requirements, broken symlinks, and orphan stowed files.
- **Sterilized Benchmark Suite**: RAMDisk auto-cleanup benchmarking suite evaluating performance up to `--mega` scale (100,000 symlinks) against GNU Stow and Dotbot.
- **Nanosecond Performance Profiler (`-p`, `--profile`)**: High-precision execution timing for debugging and benchmarking deployment workloads.
- **Clang LTO, PGO, and Sanitizer Build Profiles**: Advanced Makefile targets for ThinLTO (`build-clang-opt`), 2-stage PGO (`build-pgo`), AddressSanitizer/UBSan (`build-sanitize`), and binary size optimization (`build-size`).
- **Comprehensive Test Suite**: 81 unit & integration tests covering all CLI workflows, `io_uring` probes, and fallback mechanisms.

### Changed & Performance
- **TOCTOU Security Hardening**: Atomic symlink replacement and path validation to eliminate time-of-check to time-of-action race conditions.
- **Linker & Path Caching**: Cached executable discovery and package manager lookups to eliminate redundant disk `stat` syscalls.

### Security
- **No-NSS Static Fallback**: Direct `/etc/passwd` parsing under static builds to avoid dynamic NSS library loading vulnerabilities.
