#
# Symlink & Dependency Manager (symdep)
# High-Performance Zero-Dependency Build System (ISO C17)
# Copyright (C) 2026 durzhars
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <https://www.gnu.org/licenses/>.
#

PREFIX ?= /usr/local
BIN_NAME ?= symdep
VERSION ?= 1.0.0
EXEC_PREFIX ?= $(PREFIX)
BINDIR ?= $(EXEC_PREFIX)/bin
DATAROOTDIR ?= $(PREFIX)/share
DATADIR ?= $(DATAROOTDIR)
SYSCONFDIR ?= $(PREFIX)/etc

SRC_DIR = src
INC_DIR = include
BUILD_DIR = build
BUILD_TEST_DIR = $(BUILD_DIR)/tests
BIN_DIR = bin
OPT_DIR = $(BUILD_DIR)/opt_records
TEST_DIR = tests
TEST_UNIT_DIR = $(TEST_DIR)/unit
TEST_FEATURE_DIR = $(TEST_DIR)/feature

# Toolchain & architecture configuration
CROSS_COMPILE ?=
CC ?= $(CROSS_COMPILE)gcc
AR ?= $(CROSS_COMPILE)ar
STRIP ?= $(CROSS_COMPILE)strip
TARGET_ARCH ?= $(shell $(CC) -dumpmachine 2>/dev/null | cut -d'-' -f1 | sed 's/armv.*/armhf/' || uname -m)

# Compiler flags & diagnostic profiles
CFLAGS ?= -Wall -Wextra -pedantic -Wconversion -Wsign-conversion \
          -Wno-overlength-strings -std=c17 -O2 -Iinclude -I$(BUILD_DIR) \
          -DDATADIR=$(DATADIR) -DSYSCONFDIR=$(SYSCONFDIR)
USE_LLD = $(shell $(CC) -fuse-ld=lld -Wl,--version 2>&1 | grep -E -q 'LLD|ld.lld' && echo "-fuse-ld=lld -Wl,--icf=all" || echo "")
CLANG_OPT_FLAGS = -O3 -fomit-frame-pointer -flto=thin -fsave-optimization-record=yaml \
                  -foptimization-record-file=$(OPT_DIR)/opt.yaml \
                  -Rpass=inline -Rpass-missed=loop-vectorize \
                  -Oz -ffunction-sections -fdata-sections -fomit-frame-pointer -flto=thin
CLANG_OPT_LDFLAGS = -flto=thin $(USE_LLD) -Wl,--gc-sections -Wl,-s
CLANG_SAN_FLAGS = -fsanitize=address,undefined -fno-omit-frame-pointer -g
CLANG_SIZE_FLAGS   = -Oz -ffunction-sections -fdata-sections -fomit-frame-pointer -flto=thin
CLANG_SIZE_LDFLAGS = -flto=thin $(USE_LLD) -Wl,--gc-sections -Wl,-s

DEPFLAGS = -MMD -MP
LDFLAGS ?= -pthread

SRCS = $(SRC_DIR)/main.c \
       $(SRC_DIR)/cli/cli.c \
       $(SRC_DIR)/cli/dispatch/dispatch_core.c \
       $(SRC_DIR)/cli/dispatch/cmd_stow_ops.c \
       $(SRC_DIR)/cli/dispatch/cmd_pkg_ops.c \
       $(SRC_DIR)/cli/dispatch/cmd_deps_ops.c \
       $(SRC_DIR)/cli/dispatch/cmd_ignore_ops.c \
       $(SRC_DIR)/cli/dispatch/cmd_config_ops.c \
       $(SRC_DIR)/cli/cmd_table.c \
       $(SRC_DIR)/cli/help.c \
       $(SRC_DIR)/core/checker.c \
       $(SRC_DIR)/core/pkg_manager.c \
       $(SRC_DIR)/core/config/config_file.c \
       $(SRC_DIR)/core/config/config_ops.c \
       $(SRC_DIR)/core/config/config_active.c \
       $(SRC_DIR)/core/ignore/ignore_file.c \
       $(SRC_DIR)/core/ignore/ignore_patterns.c \
       $(SRC_DIR)/core/ignore/ignore_show.c \
       $(SRC_DIR)/core/manifest.c \
       $(SRC_DIR)/core/registry.c \
       $(SRC_DIR)/core/scanner.c \
       $(SRC_DIR)/core/scanner/scanner_parser.c \
       $(SRC_DIR)/core/linker/linker_context.c \
       $(SRC_DIR)/core/linker/linker_walk.c \
       $(SRC_DIR)/core/linker/linker_conflicts.c \
       $(SRC_DIR)/core/linker/linker_status.c \
       $(SRC_DIR)/core/linker/linker_ops.c \
       $(SRC_DIR)/core/file_collector.c \
       $(SRC_DIR)/utils/env.c \
       $(SRC_DIR)/utils/fs/fs_check.c \
       $(SRC_DIR)/utils/fs/fs_symlink.c \
       $(SRC_DIR)/utils/fs/fs_walk.c \
       $(SRC_DIR)/utils/fs/fs_resource.c \
       $(SRC_DIR)/utils/logger.c \
       $(SRC_DIR)/utils/mem.c \
       $(SRC_DIR)/utils/path.c \
       $(SRC_DIR)/utils/signal.c \
       $(SRC_DIR)/utils/str.c \
       $(SRC_DIR)/utils/thread_pool.c \
       $(SRC_DIR)/utils/io_uring_backend.c \
       $(SRC_DIR)/utils/timer.c


OBJS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))
SRC_DEPS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/.deps/%.d,$(SRCS))
DEPS = $(SRC_DEPS) $(TEST_DEPS)
TARGET = $(BIN_DIR)/$(BIN_NAME)

TEST_SRCS = $(shell find $(TEST_UNIT_DIR) -name '*.c' | sort)
TEST_OBJS = $(patsubst $(TEST_UNIT_DIR)/%.c,$(BUILD_TEST_DIR)/%.o,$(TEST_SRCS)) \
            $(filter-out $(BUILD_DIR)/main.o,$(OBJS))
TEST_DEPS = $(patsubst $(TEST_UNIT_DIR)/%.c,$(BUILD_TEST_DIR)/.deps/%.d,$(TEST_SRCS))
TEST_TARGET = $(BIN_DIR)/test_runner

.PHONY: all clean static static-musl dist dist-cross dist-aarch64 dist-armhf dist-riscv64 dist-x86_64 dist-all install test test-feature test-cross-aarch64 test-cross-armhf test-cross-riscv64 test-cross-all qemu-test-aarch64 qemu-test-armhf qemu-test-riscv64 qemu-test-all toolchain-check check-toolchain doctor bench bench-clean uninstall tidy format format-check build-clang-opt build-sanitize build-pgo cross-aarch64 cross-armhf cross-riscv64 cross-x86_64 static-cross-aarch64 static-cross-armhf static-cross-riscv64 help

all: $(TARGET)

help:
	@echo "  Build Targets:"
	@echo ""
	@echo "  make                      Build release binary using default compiler ($(CC))"
	@echo "  make static               Build standalone statically linked binary (glibc)"
	@echo "  make static-musl          Build ultra-small static binary using musl-gcc (~230KB)"
	@echo "  make dist                 Create release tarball (.tar.gz) and SHA256 checksum for host"
	@echo "  make test                 Run unit test suite"
	@echo "  make test-feature         Run end-to-end integration feature tests"
	@echo "  make test-cross-aarch64   Run unit tests under QEMU ARM64 user emulation (qemu-aarch64)"
	@echo "  make test-cross-armhf     Run unit tests under QEMU ARMv7 user emulation (qemu-arm)"
	@echo "  make test-cross-riscv64   Run unit tests under QEMU RISC-V 64 user emulation (qemu-riscv64)"
	@echo "  make test-cross-all       Run all available cross-architecture tests under QEMU"
	@echo "  make toolchain-check      Inspect host/cross compilers, linkers, emulators & tools (doctor)"
	@echo "  make bench                Run sterilized benchmark suite (symdep vs GNU Stow vs Dotbot)"
	@echo "  make bench-clean          Clean benchmark report and temporary vendor artifacts"
	@echo "  make clean                Clean build and bin output directories"
	@echo ""
	@echo "  Cross-Compilation Targets:"
	@echo "  make cross-aarch64        Build for Linux ARM64 (aarch64-linux-gnu-gcc)"
	@echo "  make cross-armhf          Build for Linux ARMv7 32-bit (arm-linux-gnueabihf-gcc)"
	@echo "  make cross-riscv64        Build for Linux RISC-V 64 (riscv64-linux-gnu-gcc)"
	@echo "  make cross-x86_64         Build for Linux x86_64"
	@echo "  make static-cross-aarch64 Build static binary for ARM64"
	@echo "  make static-cross-armhf   Build static binary for ARMv7"
	@echo "  make static-cross-riscv64 Build static binary for RISC-V 64"
	@echo ""
	@echo "  Multi-Architecture Distribution Targets:"
	@echo "  make dist-cross           Package tarball for specified TARGET_ARCH & CROSS_COMPILE"
	@echo "  make dist-aarch64         Package release tarball & SHA256 for linux-aarch64"
	@echo "  make dist-armhf           Package release tarball & SHA256 for linux-armhf"
	@echo "  make dist-riscv64         Package release tarball & SHA256 for linux-riscv64"
	@echo "  make dist-x86_64          Package release tarball & SHA256 for linux-x86_64"
	@echo "  make dist-all             Build and package release tarballs for all available toolchains"
	@echo ""
	@echo "  Clang Optimization & Diagnostics Targets:"
	@echo "  make build-clang-opt      Build with Clang ThinLTO, -O3, and optimization remarks"
	@echo "  make build-pgo            Build with Clang 2-stage Profile-Guided Optimization"
	@echo "  make build-sanitize       Build with Clang AddressSanitizer & UBSanitizer"
	@echo "  make tidy                 Run clang-tidy static analysis"
	@echo "  make format               Format source files with clang-format"
	@echo "  make format-check         Check source files formatting compliance with clang-format"
	@echo ""
	@echo "  Installations:"
	@echo "  make install              Install build into $(PREFIX)/$(BIN_DIR)/$(BIN_NAME)"
	@echo "  make uninstall            Uninstall binaries and data files"

$(TARGET): $(OBJS) | $(BIN_DIR)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET) $(LDFLAGS)

ALL_HDRS = $(shell find $(INC_DIR) $(TEST_UNIT_DIR) -name '*.h')

tidy:
	clang-tidy $(SRCS) $(TEST_SRCS) -- -Iinclude -Itests/unit -std=c17

format:
	clang-format -i $(SRCS) $(TEST_SRCS) $(ALL_HDRS)

format-check:
	clang-format --dry-run --Werror $(SRCS) $(TEST_SRCS) $(ALL_HDRS)

build-clang-opt:
	$(MAKE) clean
	mkdir -p $(OPT_DIR)
	$(MAKE) CC=clang CFLAGS="$(CFLAGS) $(CLANG_OPT_FLAGS)" LDFLAGS="$(LDFLAGS) $(CLANG_OPT_LDFLAGS)" $(TARGET)

build-sanitize:
	@echo 'int main(void){return 0;}' | clang -x c - -fsanitize=address,undefined -o /dev/null 2>/dev/null || \
		(echo "Error: Clang sanitizer runtime library (compiler-rt / libclang_rt) is missing." && exit 1)
	$(MAKE) clean
	$(MAKE) CC=clang CFLAGS="$(CFLAGS) $(CLANG_SAN_FLAGS)" LDFLAGS="$(LDFLAGS) $(CLANG_SAN_FLAGS)" $(TARGET)

build-pgo:
	@echo 'int main(void){return 0;}' | clang -x c - -fprofile-instr-generate -o /dev/null 2>/dev/null || \
		(echo "Error: Clang profiling runtime library (libclang_rt.profile.a) is missing." && exit 1)
	@command -v llvm-profdata >/dev/null 2>&1 || \
		(echo "Error: llvm-profdata tool is missing." && exit 1)
	@echo "=== Stage 1: Building instrumented binary ==="
	$(MAKE) clean
	mkdir -p $(OPT_DIR)
	$(MAKE) CC=clang CFLAGS="$(CFLAGS) $(CLANG_OPT_FLAGS) -fprofile-instr-generate" LDFLAGS="$(LDFLAGS) $(CLANG_OPT_LDFLAGS) -fprofile-instr-generate" $(TARGET)
	@echo "=== Stage 2: Collecting execution workload profile ==="
	@rm -rf $(BUILD_DIR)/profiles symdep_app.profdata
	@mkdir -p $(BUILD_DIR)/profiles
	-@LLVM_PROFILE_FILE="$(CURDIR)/$(BUILD_DIR)/profiles/symdep_%p_%m.profraw" bash $(TEST_FEATURE_DIR)/run_feature_tests.sh > /dev/null 2>&1
	@llvm-profdata merge -output=symdep_app.profdata $(BUILD_DIR)/profiles/*.profraw
	@test -s symdep_app.profdata || (echo "Error: Failed to generate symdep_app.profdata profile feedback file." && exit 1)
	@echo "=== Stage 3: Compiling PGO production binary with profile feedback ==="
	$(MAKE) clean
	mkdir -p $(OPT_DIR)
	$(MAKE) CC=clang CFLAGS="$(CFLAGS) $(CLANG_OPT_FLAGS) -fprofile-instr-use=symdep_app.profdata" LDFLAGS="$(LDFLAGS) $(CLANG_OPT_LDFLAGS) -fprofile-instr-use=symdep_app.profdata" $(TARGET)
	@rm -rf $(BUILD_DIR)/profiles symdep_app.profdata
	@echo "=== PGO build complete ==="

build-size:
	$(MAKE) clean
	$(MAKE) CC=clang CFLAGS="$(filter-out -O2 -O3,$(CFLAGS)) $(CLANG_SIZE_FLAGS)" LDFLAGS="$(LDFLAGS) $(CLANG_SIZE_LDFLAGS)" $(TARGET)

static:
	$(MAKE) clean
	$(MAKE) CFLAGS="$(CFLAGS) -static -DNO_NSS_FALLBACK" LDFLAGS="$(LDFLAGS) -static" $(TARGET)

static-musl:
	@command -v musl-gcc >/dev/null 2>&1 || \
		(echo "Error: musl-gcc is not installed. Install it via 'sudo apt install musl-tools' or your package manager." && exit 1)
	$(MAKE) clean
	$(MAKE) CC=musl-gcc CFLAGS="$(filter-out -O2,$(CFLAGS)) -O3 -ffunction-sections -fdata-sections -static -DNO_NSS_FALLBACK" LDFLAGS="$(LDFLAGS) -static -Wl,--gc-sections" $(TARGET)
	@strip $(TARGET) 2>/dev/null || true

RELEASE_DIR = release

define package_dist
	@mkdir -p $(RELEASE_DIR)
	@rm -rf $(BUILD_DIR)/pkg_$(1)
	@mkdir -p $(BUILD_DIR)/pkg_$(1)/symdep-v$(VERSION)-linux-$(1)
	@cp $(TARGET) $(BUILD_DIR)/pkg_$(1)/symdep-v$(VERSION)-linux-$(1)/
	@cp README.md LICENSE CHANGELOG.md $(BUILD_DIR)/pkg_$(1)/symdep-v$(VERSION)-linux-$(1)/
	@cp -r resources $(BUILD_DIR)/pkg_$(1)/symdep-v$(VERSION)-linux-$(1)/
	@tar -C $(BUILD_DIR)/pkg_$(1) -czvf $(RELEASE_DIR)/symdep-v$(VERSION)-linux-$(1).tar.gz symdep-v$(VERSION)-linux-$(1)
	@cd $(RELEASE_DIR) && sha256sum symdep-v$(VERSION)-linux-$(1).tar.gz > symdep-v$(VERSION)-linux-$(1).tar.gz.sha256
	@rm -rf $(BUILD_DIR)/pkg_$(1)
	@echo "=== Created distribution tarball: $(RELEASE_DIR)/symdep-v$(VERSION)-linux-$(1).tar.gz ==="
	@echo "=== SHA256 Checksum: $$(cat $(RELEASE_DIR)/symdep-v$(VERSION)-linux-$(1).tar.gz.sha256) ==="
endef

# Cross-compilation targets (dynamic)
cross-aarch64:
	@command -v aarch64-linux-gnu-gcc >/dev/null 2>&1 || \
		(echo "Error: aarch64-linux-gnu-gcc not found. Install gcc-aarch64-linux-gnu or cross toolchain." && exit 1)
	$(MAKE) clean
	$(MAKE) CC=aarch64-linux-gnu-gcc STRIP=aarch64-linux-gnu-strip TARGET_ARCH=aarch64 $(TARGET)
	@aarch64-linux-gnu-strip $(TARGET) 2>/dev/null || true

cross-armhf:
	@command -v arm-linux-gnueabihf-gcc >/dev/null 2>&1 || \
		(echo "Error: arm-linux-gnueabihf-gcc not found. Install gcc-arm-linux-gnueabihf or cross toolchain." && exit 1)
	$(MAKE) clean
	$(MAKE) CC=arm-linux-gnueabihf-gcc STRIP=arm-linux-gnueabihf-strip TARGET_ARCH=armhf $(TARGET)
	@arm-linux-gnueabihf-strip $(TARGET) 2>/dev/null || true

cross-riscv64:
	@command -v riscv64-linux-gnu-gcc >/dev/null 2>&1 || \
		(echo "Error: riscv64-linux-gnu-gcc not found. Install gcc-riscv64-linux-gnu or cross toolchain." && exit 1)
	$(MAKE) clean
	$(MAKE) CC=riscv64-linux-gnu-gcc STRIP=riscv64-linux-gnu-strip TARGET_ARCH=riscv64 $(TARGET)
	@riscv64-linux-gnu-strip $(TARGET) 2>/dev/null || true

cross-x86_64:
	$(MAKE) clean
	$(MAKE) TARGET_ARCH=x86_64 $(TARGET)

# Static cross-compilation targets
static-cross-aarch64:
	@command -v aarch64-linux-gnu-gcc >/dev/null 2>&1 || \
		(echo "Error: aarch64-linux-gnu-gcc not found." && exit 1)
	$(MAKE) clean
	$(MAKE) CC=aarch64-linux-gnu-gcc STRIP=aarch64-linux-gnu-strip TARGET_ARCH=aarch64 \
		CFLAGS="$(filter-out -O2,$(CFLAGS)) -O3 -ffunction-sections -fdata-sections -static -DNO_NSS_FALLBACK" \
		LDFLAGS="$(LDFLAGS) -static -Wl,--gc-sections" $(TARGET)
	@aarch64-linux-gnu-strip $(TARGET) 2>/dev/null || true

static-cross-armhf:
	@command -v arm-linux-gnueabihf-gcc >/dev/null 2>&1 || \
		(echo "Error: arm-linux-gnueabihf-gcc not found." && exit 1)
	$(MAKE) clean
	$(MAKE) CC=arm-linux-gnueabihf-gcc STRIP=arm-linux-gnueabihf-strip TARGET_ARCH=armhf \
		CFLAGS="$(filter-out -O2,$(CFLAGS)) -O3 -ffunction-sections -fdata-sections -static -DNO_NSS_FALLBACK" \
		LDFLAGS="$(LDFLAGS) -static -Wl,--gc-sections" $(TARGET)
	@arm-linux-gnueabihf-strip $(TARGET) 2>/dev/null || true

static-cross-riscv64:
	@command -v riscv64-linux-gnu-gcc >/dev/null 2>&1 || \
		(echo "Error: riscv64-linux-gnu-gcc not found." && exit 1)
	$(MAKE) clean
	$(MAKE) CC=riscv64-linux-gnu-gcc STRIP=riscv64-linux-gnu-strip TARGET_ARCH=riscv64 \
		CFLAGS="$(filter-out -O2,$(CFLAGS)) -O3 -ffunction-sections -fdata-sections -static -DNO_NSS_FALLBACK" \
		LDFLAGS="$(LDFLAGS) -static -Wl,--gc-sections" $(TARGET)
	@riscv64-linux-gnu-strip $(TARGET) 2>/dev/null || true

# Multi-architecture distribution targets
dist: static-musl
	$(call package_dist,x86_64)

dist-cross:
	@test -n "$(TARGET_ARCH)" || (echo "Error: TARGET_ARCH must be specified (e.g., make dist-cross TARGET_ARCH=aarch64 CROSS_COMPILE=aarch64-linux-gnu-)" && exit 1)
	$(MAKE) clean
	$(MAKE) CC=$(CC) STRIP=$(STRIP) TARGET_ARCH=$(TARGET_ARCH) \
		CFLAGS="$(filter-out -O2,$(CFLAGS)) -O3 -ffunction-sections -fdata-sections -static -DNO_NSS_FALLBACK" \
		LDFLAGS="$(LDFLAGS) -static -Wl,--gc-sections" $(TARGET)
	@$(STRIP) $(TARGET) 2>/dev/null || true
	$(call package_dist,$(TARGET_ARCH))

dist-aarch64: static-cross-aarch64
	$(call package_dist,aarch64)

dist-armhf: static-cross-armhf
	$(call package_dist,armhf)

dist-riscv64: static-cross-riscv64
	$(call package_dist,riscv64)

dist-x86_64: dist

dist-all:
	@echo "=== Building multi-architecture distribution packages ==="
	@$(MAKE) dist-x86_64
	@if command -v aarch64-linux-gnu-gcc >/dev/null 2>&1; then \
		$(MAKE) dist-aarch64; \
	else \
		echo "Skipping aarch64 (aarch64-linux-gnu-gcc not found)"; \
	fi
	@if command -v arm-linux-gnueabihf-gcc >/dev/null 2>&1; then \
		$(MAKE) dist-armhf; \
	else \
		echo "Skipping armhf (arm-linux-gnueabihf-gcc not found)"; \
	fi
	@if command -v riscv64-linux-gnu-gcc >/dev/null 2>&1; then \
		$(MAKE) dist-riscv64; \
	else \
		echo "Skipping riscv64 (riscv64-linux-gnu-gcc not found)"; \
	fi
	@echo "=== All available distributions built in $(RELEASE_DIR)/ ==="
	@ls -lh $(RELEASE_DIR)

test: $(TEST_TARGET)
	./$(TEST_TARGET)

test-feature: $(TARGET)
	bash $(TEST_FEATURE_DIR)/run_feature_tests.sh

# QEMU User-Mode Test Emulation Configuration
QEMU_AARCH64 ?= qemu-aarch64
QEMU_ARM ?= qemu-arm
QEMU_RISCV64 ?= qemu-riscv64

AARCH64_SYSROOT ?= $(shell test -d /usr/aarch64-linux-gnu && echo "-L /usr/aarch64-linux-gnu")
ARMHF_SYSROOT ?= $(shell test -d /usr/arm-linux-gnueabihf && echo "-L /usr/arm-linux-gnueabihf")
RISCV64_SYSROOT ?= $(shell test -d /usr/riscv64-linux-gnu && echo "-L /usr/riscv64-linux-gnu")

test-cross-aarch64 qemu-test-aarch64:
	@command -v aarch64-linux-gnu-gcc >/dev/null 2>&1 || \
		(echo "Error: aarch64-linux-gnu-gcc not found. Install gcc-aarch64-linux-gnu or cross toolchain." && exit 1)
	@command -v $(QEMU_AARCH64) >/dev/null 2>&1 || \
		(echo "Error: $(QEMU_AARCH64) not found. Install qemu-user." && exit 1)
	$(MAKE) clean
	$(MAKE) CC=aarch64-linux-gnu-gcc TARGET_ARCH=aarch64 $(TEST_TARGET)
	@echo "=== Running AArch64 Unit Tests via $(QEMU_AARCH64) ==="
	$(QEMU_AARCH64) $(AARCH64_SYSROOT) ./$(TEST_TARGET)

test-cross-armhf qemu-test-armhf:
	@command -v arm-linux-gnueabihf-gcc >/dev/null 2>&1 || \
		(echo "Error: arm-linux-gnueabihf-gcc not found. Install gcc-arm-linux-gnueabihf or cross toolchain." && exit 1)
	@command -v $(QEMU_ARM) >/dev/null 2>&1 || \
		(echo "Error: $(QEMU_ARM) not found. Install qemu-user." && exit 1)
	$(MAKE) clean
	$(MAKE) CC=arm-linux-gnueabihf-gcc TARGET_ARCH=armhf $(TEST_TARGET)
	@echo "=== Running ARMhf Unit Tests via $(QEMU_ARM) ==="
	$(QEMU_ARM) $(ARMHF_SYSROOT) ./$(TEST_TARGET)

test-cross-riscv64 qemu-test-riscv64:
	@command -v riscv64-linux-gnu-gcc >/dev/null 2>&1 || \
		(echo "Error: riscv64-linux-gnu-gcc not found. Install gcc-riscv64-linux-gnu or cross toolchain." && exit 1)
	@command -v $(QEMU_RISCV64) >/dev/null 2>&1 || \
		(echo "Error: $(QEMU_RISCV64) not found. Install qemu-user." && exit 1)
	$(MAKE) clean
	$(MAKE) CC=riscv64-linux-gnu-gcc TARGET_ARCH=riscv64 $(TEST_TARGET)
	@echo "=== Running RISC-V 64 Unit Tests via $(QEMU_RISCV64) ==="
	$(QEMU_RISCV64) $(RISCV64_SYSROOT) ./$(TEST_TARGET)

test-cross-all qemu-test-all:
	@echo "=== Running available cross-architecture QEMU tests ==="
	@if command -v aarch64-linux-gnu-gcc >/dev/null 2>&1 && command -v $(QEMU_AARCH64) >/dev/null 2>&1; then \
		$(MAKE) test-cross-aarch64; \
	else \
		echo "Skipping aarch64 cross-test (toolchain or $(QEMU_AARCH64) missing)"; \
	fi
	@if command -v arm-linux-gnueabihf-gcc >/dev/null 2>&1 && command -v $(QEMU_ARM) >/dev/null 2>&1; then \
		$(MAKE) test-cross-armhf; \
	else \
		echo "Skipping armhf cross-test (toolchain or $(QEMU_ARM) missing)"; \
	fi
	@if command -v riscv64-linux-gnu-gcc >/dev/null 2>&1 && command -v $(QEMU_RISCV64) >/dev/null 2>&1; then \
		$(MAKE) test-cross-riscv64; \
	else \
		echo "Skipping riscv64 cross-test (toolchain or $(QEMU_RISCV64) missing)"; \
	fi

$(TEST_TARGET): $(TEST_OBJS) | $(BIN_DIR)
	$(CC) $(CFLAGS) $(TEST_OBJS) -o $(TEST_TARGET) $(LDFLAGS)

HELP_TXT_GEN = $(BUILD_DIR)/help_text_plain.h

$(HELP_TXT_GEN): resources/help.txt | $(BUILD_DIR)
	@echo "Generating embedded plain text help string from resources/help.txt..."
	@echo "static const char *EMBEDDED_HELP_TXT =" > $@
	@sed -e 's/\\/\\\\/g' -e 's/"/\\"/g' -e 's/^/"/' -e 's/$$/\\n"/' $< >> $@
	@echo ";" >> $@

$(BUILD_DIR)/main.o: $(HELP_TXT_GEN)
$(BUILD_DIR)/cli/help.o: $(HELP_TXT_GEN)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	@mkdir -p $(dir $@) $(dir $(BUILD_DIR)/.deps/$*)
	$(CC) $(CFLAGS) $(DEPFLAGS) -MF $(BUILD_DIR)/.deps/$*.d -c $< -o $@

$(BUILD_TEST_DIR)/%.o: $(TEST_UNIT_DIR)/%.c | $(BUILD_DIR)
	@mkdir -p $(dir $@) $(dir $(BUILD_TEST_DIR)/.deps/$*)
	$(CC) $(CFLAGS) $(DEPFLAGS) -MF $(BUILD_TEST_DIR)/.deps/$*.d -I$(TEST_UNIT_DIR) -c $< -o $@

-include $(DEPS)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(OPT_DIR): | $(BUILD_DIR)
	mkdir -p $(OPT_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR) *.opt.yaml

toolchain-check check-toolchain doctor:
	@echo "=== Symlink & Dependency Manager (symdep) Toolchain Doctor ==="
	@echo ""
	@bash -c '\
	GREEN="\033[1;32m"; YELLOW="\033[1;33m"; RED="\033[1;31m"; CYAN="\033[1;36m"; BOLD="\033[1m"; RESET="\033[0m"; \
	check_tool() { \
		local name="$$1"; local cmd="$$2"; local req="$$3"; \
		if command -v "$$cmd" >/dev/null 2>&1; then \
			local ver=$$("$$cmd" --version 2>/dev/null | head -n 1 | sed "s/^Free Software Foundation.*//g"); \
			[ -z "$$ver" ] && ver=$$("$$cmd" -v 2>&1 | head -n 1); \
			printf "  %-26s : $${GREEN}[OK]$${RESET} %s ($${CYAN}%s$${RESET})\n" "$$name" "$$cmd" "$$ver"; \
			return 0; \
		else \
			if [ "$$req" = "yes" ]; then \
				printf "  %-26s : $${RED}[MISSING (REQUIRED)]$${RESET} %s not found\n" "$$name" "$$cmd"; \
			else \
				printf "  %-26s : $${YELLOW}[OPTIONAL (ABSENT)]$${RESET} %s not installed\n" "$$name" "$$cmd"; \
			fi; \
			return 1; \
		fi; \
	}; \
	check_dir() { \
		local name="$$1"; local path="$$2"; \
		if [ -d "$$path" ]; then \
			printf "  %-26s : $${GREEN}[FOUND]$${RESET} %s\n" "$$name" "$$path"; \
		else \
			printf "  %-26s : $${YELLOW}[NOT FOUND]$${RESET} %s\n" "$$name" "$$path"; \
		fi; \
	}; \
	check_feature() { \
		local name="$$1"; local desc="$$2"; local ok="$$3"; \
		if [ "$$ok" = "1" ]; then \
			printf "  %-26s : $${GREEN}[SUPPORTED]$${RESET} %s\n" "$$name" "$$desc"; \
		else \
			printf "  %-26s : $${YELLOW}[UNSUPPORTED]$${RESET} %s\n" "$$name" "$$desc"; \
		fi; \
	}; \
	echo -e "$${BOLD}1. Host Environment & System Info$${RESET}"; \
	echo "  Architecture               : $$(uname -m)"; \
	echo "  Kernel Release             : $$(uname -r)"; \
	echo "  Default CC ($(CC))        : $$(which $(CC) 2>/dev/null || echo "not found")"; \
	echo ""; \
	echo -e "$${BOLD}2. Host Compilers & Build Tools$${RESET}"; \
	check_tool "Host C Compiler (CC)" "$(CC)" "yes"; \
	check_tool "Host Stripper (STRIP)" "$(STRIP)" "yes"; \
	check_tool "Clang Compiler" "clang" "no"; \
	check_tool "LLD Linker" "ld.lld" "no"; \
	check_tool "musl-gcc (Static)" "musl-gcc" "no"; \
	echo ""; \
	echo -e "$${BOLD}3. Static Analysis & Diagnostics Tools$${RESET}"; \
	check_tool "Clang-Format" "clang-format" "no"; \
	check_tool "Clang-Tidy" "clang-tidy" "no"; \
	check_tool "LLVM Profile Data (PGO)" "llvm-profdata" "no"; \
	echo ""; \
	echo -e "$${BOLD}4. Cross-Compilation Toolchains$${RESET}"; \
	check_tool "ARM64 GCC (AArch64)" "aarch64-linux-gnu-gcc" "no"; \
	check_tool "ARMhf GCC (ARMv7)" "arm-linux-gnueabihf-gcc" "no"; \
	check_tool "RISC-V 64 GCC" "riscv64-linux-gnu-gcc" "no"; \
	check_dir "AArch64 Sysroot" "/usr/aarch64-linux-gnu"; \
	check_dir "ARMhf Sysroot" "/usr/arm-linux-gnueabihf"; \
	check_dir "RISC-V 64 Sysroot" "/usr/riscv64-linux-gnu"; \
	echo ""; \
	echo -e "$${BOLD}5. QEMU Emulation Engines$${RESET}"; \
	check_tool "QEMU AArch64 User" "qemu-aarch64" "no"; \
	check_tool "QEMU ARMhf User" "qemu-arm" "no"; \
	check_tool "QEMU RISC-V 64 User" "qemu-riscv64" "no"; \
	check_tool "QEMU AArch64 System" "qemu-system-aarch64" "no"; \
	echo ""; \
	echo -e "$${BOLD}6. Toolchain Feature Capabilities$${RESET}"; \
	echo "int main(void){return 0;}" | $(CC) -x c - -std=c17 -o /dev/null 2>/dev/null && ok_c17=1 || ok_c17=0; \
	check_feature "ISO C17 Standard" "Compliant with -std=c17" "$$ok_c17"; \
	echo "int main(void){return 0;}" | $(CC) -x c - -static -o /dev/null 2>/dev/null && ok_static=1 || ok_static=0; \
	check_feature "Glibc Static Linking" "-static builds supported" "$$ok_static"; \
	if command -v clang >/dev/null 2>&1; then \
		echo "int main(void){return 0;}" | clang -x c - -fsanitize=address,undefined -o /dev/null 2>/dev/null && ok_san=1 || ok_san=0; \
		check_feature "Clang Sanitizers" "ASan & UBSan runtime available" "$$ok_san"; \
	fi; \
	echo ""; \
	echo -e "$${BOLD}=== Toolchain Doctor Check Complete ===$${RESET}" \
	'

install: $(TARGET)
	install -d $(DESTDIR)$(BINDIR)
	install -m 755 $(TARGET) $(DESTDIR)$(BINDIR)/$(BIN_NAME)
	install -d $(DESTDIR)$(DATADIR)/$(BIN_NAME)
	install -m 644 resources/help.md $(DESTDIR)$(DATADIR)/$(BIN_NAME)/help.md
	install -m 644 resources/help.txt $(DESTDIR)$(DATADIR)/$(BIN_NAME)/help.txt
	install -m 644 resources/symignore.default $(DESTDIR)$(DATADIR)/$(BIN_NAME)/symignore.default
	install -m 644 resources/symignore.template $(DESTDIR)$(DATADIR)/$(BIN_NAME)/symignore.template
	install -m 644 resources/symdeps.template $(DESTDIR)$(DATADIR)/$(BIN_NAME)/symdeps.template
	install -d $(DESTDIR)$(DATAROOTDIR)/bash-completion/completions
	install -m 644 resources/completions/bash/symdep $(DESTDIR)$(DATAROOTDIR)/bash-completion/completions/$(BIN_NAME)
	install -d $(DESTDIR)$(DATAROOTDIR)/zsh/site-functions
	install -m 644 resources/completions/zsh/_symdep $(DESTDIR)$(DATAROOTDIR)/zsh/site-functions/_$(BIN_NAME)
	install -d $(DESTDIR)$(DATAROOTDIR)/fish/vendor_completions.d
	install -m 644 resources/completions/fish/symdep.fish $(DESTDIR)$(DATAROOTDIR)/fish/vendor_completions.d/$(BIN_NAME).fish

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/$(BIN_NAME)
	rm -rf $(DESTDIR)$(DATADIR)/$(BIN_NAME)
	rm -f $(DESTDIR)$(DATAROOTDIR)/bash-completion/completions/$(BIN_NAME)
	rm -f $(DESTDIR)$(DATAROOTDIR)/zsh/site-functions/_$(BIN_NAME)
	rm -f $(DESTDIR)$(DATAROOTDIR)/fish/vendor_completions.d/$(BIN_NAME).fish

bench: $(TARGET)
	@bash tests/benchmark/run_benchmark.sh

bench-clean:
	rm -rf tests/benchmark/vendor tests/benchmark/BENCHMARK_REPORT.md tests/benchmark/*.json /tmp/symdep_benchmark_workspace

