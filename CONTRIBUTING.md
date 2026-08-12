# Contributing to `symdep`

Thank you for your interest in contributing to `symdep`! This document outlines the development workflow, coding standards, build targets, and pull request guidelines for the project.

---

## Architecture Overview

`symdep` is a high-performance, zero-dependency symlink manager and package dependency resolver written in **ISO C17**.

Before introducing architectural changes, please review our **Architecture Decision Records (ADRs)** located in [`docs/decisions/`](docs/decisions/):
- [ADR-001: Zero-Dependency ISO C17 Architecture](docs/decisions/ADR-001-zero-dependency-c17-architecture.md)
- [ADR-002: Dynamic Symlink Unfolding & Collision Engine](docs/decisions/ADR-002-symlink-unfolding-and-collision-engine.md)
- [ADR-003: Cross-Distro Package & Plugin Registry Engine](docs/decisions/ADR-003-cross-distro-package-and-plugin-registry.md)
- [ADR-004: AST / Shebang Code-Analysis Dependency Scanner Engine](docs/decisions/ADR-004-static-analysis-dependency-scanner.md)
- [ADR-005: Unified Command Dispatch Table & Multi-Namespace Syntax](docs/decisions/ADR-005-unified-command-dispatch-table.md)
- [ADR-006: Hierarchical Ignore Rule Engine](docs/decisions/ADR-006-hierarchical-ignore-rule-engine.md)

---

## Developer Quick Start

### Building from Source

Ensure you have standard build tools installed (`gcc` or `clang`, `make`, `bash`).

```bash
# Clone repository
git clone https://github.com/durzhars/symdep.git
cd symdep

# Compile release binary (output: bin/symdep)
make

# Run unit test suite (79+ tests)
make test

# Run integration feature test suite
make test-feature
```

---

## Advanced Clang Targets & Code Quality

The build system includes targets for code formatting, static analysis, sanitizers, and performance optimization:

```bash
# Code Formatting (enforces repository .clang-format rules)
make format          # Format all C/C++ source and header files
make format-check    # Verify formatting compliance without modifying files

# Static Analysis (uses .clang-tidy configuration)
make tidy            # Run clang-tidy static analysis

# Sanitizer Build (AddressSanitizer + UndefinedBehaviorSanitizer)
make build-sanitize  # Compiles binary with ASan/UBSan instrumentation

# ThinLTO & Size Optimization Targets
make build-clang-opt # Compiles with Clang ThinLTO, -O3, and optimization remarks
make build-pgo       # 2-Stage Profile-Guided Optimization build using feature workload
make build-size      # Binary size optimization (-Oz, ThinLTO)
```

---

## Coding Standards & Guidelines

1. **Standard Compliance**: Code must adhere strictly to **ISO C17** (`-std=c17`). POSIX.1-2008 extensions are permitted where necessary.
2. **Zero Compiler Warnings**: Code must compile cleanly with `-Wall -Wextra -Werror -pedantic`.
3. **Zero External Dependencies**: Do not introduce third-party runtime or library dependencies. Rely solely on standard C runtime and POSIX APIs.
4. **Memory Hygiene**: Use provided safe memory allocation functions (`safe_malloc`, `safe_realloc`) in `include/utils/mem.h`. Ensure all dynamic allocations are freed upon exit.
5. **Signal & Fault Handling**: File operations modifying disk states must maintain signal cleanup registration (`include/utils/signal.h`) for safe aborts.
6. **Code Formatting**: Format code using `make format` (`.clang-format` based on LLVM style).

---

## Submitting Pull Requests

1. **Fork & Branch**: Create a descriptive feature branch (e.g. `feature/add-apk-enhancement` or `fix/linker-edge-case`).
2. **Add Tests**:
   - For utility or core logic changes, add unit tests in `tests/unit/`.
   - For CLI or integration changes, add end-to-end assertions in `tests/feature/`.
3. **Run Quality Check Gate**:
   ```bash
   make clean
   make format-check
   make tidy
   make test
   make test-feature
   ```
4. **Commit Messages**: Write concise commit messages explaining *what* and *why*.
5. **Open Pull Request**: Include a summary of changes, rationale, test execution output, and relevant issue links.
