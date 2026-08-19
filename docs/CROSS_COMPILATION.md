# Cross-Compilation Guide for `symdep`

`symdep` is written in pure ISO C17 with zero external runtime library dependencies (only standard POSIX threads / libc). This architecture makes cross-compiling for embedded devices, single-board computers (Raspberry Pi, Orange Pi), server architectures (AArch64, RISC-V), and containers straightforward.

---

## 1. Supported Architectures & Triplet Mapping

| Architecture | GNU Triplet | Makefile Target | Static Target | Distribution Package |
|---|---|---|---|---|
| **ARM64 / AArch64** | `aarch64-linux-gnu` | `make cross-aarch64` | `make static-cross-aarch64` | `make dist-aarch64` |
| **ARMv7 32-bit (Hard Float)** | `arm-linux-gnueabihf` | `make cross-armhf` | `make static-cross-armhf` | `make dist-armhf` |
| **RISC-V 64-bit** | `riscv64-linux-gnu` | `make cross-riscv64` | `make static-cross-riscv64` | `make dist-riscv64` |
| **x86_64 Linux** | `x86_64-linux-gnu` | `make cross-x86_64` | `make static-musl` | `make dist-x86_64` |

---

## 2. Toolchain Prerequisites

### Debian / Ubuntu / Linux Mint
```bash
# ARM64 (AArch64)
sudo apt install gcc-aarch64-linux-gnu binutils-aarch64-linux-gnu

# ARMv7 32-bit (armhf)
sudo apt install gcc-arm-linux-gnueabihf binutils-arm-linux-gnueabihf

# RISC-V 64-bit
sudo apt install gcc-riscv64-linux-gnu binutils-riscv64-linux-gnu

# Musl GCC (for ultra-small x86_64 static binaries)
sudo apt install musl-tools
```

### Arch Linux / Manjaro
```bash
# ARM64
sudo pacman -S aarch64-linux-gnu-gcc aarch64-linux-gnu-binutils

# ARMv7
sudo pacman -S arm-linux-gnueabihf-gcc arm-linux-gnueabihf-binutils

# RISC-V 64
sudo pacman -S riscv64-linux-gnu-gcc riscv64-linux-gnu-binutils

# Musl
sudo pacman -S musl
```

### Alpine Linux
```bash
apk add gcc musl-dev binutils make
```

---

## 3. Makefile Cross-Build Commands

### Dynamic Binaries
```bash
# Build dynamically linked binary for ARM64
make cross-aarch64

# Build dynamically linked binary for ARMv7
make cross-armhf

# Build dynamically linked binary for RISC-V 64
make cross-riscv64
```
Binaries are placed into `bin/symdep`.

### Fully Standalone Static Binaries
Static binaries embed the C runtime and require zero external shared libraries, running seamlessly across any Linux distribution regardless of glibc version or musl environment:

```bash
# Build static standalone binary for ARM64
make static-cross-aarch64

# Build static standalone binary for ARMv7
make static-cross-armhf

# Build static standalone binary for RISC-V 64
make static-cross-riscv64
```

### Multi-Architecture Distribution Packaging
Package ready-to-distribute `.tar.gz` release archives along with SHA256 checksums in the `release/` directory:

```bash
# Package ARM64 release tarball
make dist-aarch64

# Package ARMv7 release tarball
make dist-armhf

# Package RISC-V release tarball
make dist-riscv64

# Build all available architecture packages at once (auto-detects installed toolchains)
make dist-all
```

Outputs in `release/`:
- `release/symdep-v1.0.0-linux-aarch64.tar.gz` + `.sha256`
- `release/symdep-v1.0.0-linux-x86_64.tar.gz` + `.sha256`
- `release/symdep-v1.0.0-linux-armhf.tar.gz` + `.sha256`
- `release/symdep-v1.0.0-linux-riscv64.tar.gz` + `.sha256`

---

## 4. Generic GNU `CROSS_COMPILE` Variable Support

If you have a custom cross-compilation toolchain or embedded SDK (e.g., Yocto, Buildroot, OpenWrt, Android NDK), invoke `make` with the `CROSS_COMPILE` prefix:

```bash
# Using standard GNU triplet prefix:
make CROSS_COMPILE=aarch64-linux-gnu-

# Specifying explicit compiler and arch:
make CC=aarch64-linux-musl-gcc STRIP=aarch64-linux-musl-strip TARGET_ARCH=aarch64

# Packaging custom cross-compilation:
make dist-cross CROSS_COMPILE=aarch64-unknown-linux-musl- TARGET_ARCH=aarch64
```

---

## 5. Emulation & Testing with QEMU

You can execute and test cross-compiled binaries on your host machine without target hardware using QEMU user-mode emulation:

### Install QEMU User Emulators
```bash
# Debian/Ubuntu
sudo apt install qemu-user-static

# Arch Linux
sudo pacman -S qemu-user-static
```

### Run Tests & Binaries Under Emulation
```bash
# Automated cross-architecture test suites via Makefile
make test-cross-aarch64    # Build and run unit tests under qemu-aarch64
make test-cross-armhf      # Build and run unit tests under qemu-arm
make test-cross-riscv64    # Build and run unit tests under qemu-riscv64
make test-cross-all        # Run all available cross-architecture tests

# Manual invocation under QEMU
qemu-aarch64 -L /usr/aarch64-linux-gnu ./bin/symdep help
qemu-aarch64 -L /usr/aarch64-linux-gnu ./bin/test_runner

# Transparent execution via binfmt_misc (if configured)
./bin/symdep help
```

---

## 6. Verification Checklist

1. **Check ELF Target Architecture**:
   ```bash
   file bin/symdep
   # Expected output: ELF 64-bit LSB executable, ARM aarch64, version 1 (SYSV)...
   ```
2. **Verify Shared Library Dependencies**:
   ```bash
   # Dynamic ARM64 binary:
   aarch64-linux-gnu-readelf -d bin/symdep | grep NEEDED

   # Static ARM64 binary:
   # (No dynamic section / statically linked)
   ```

---

## 7. Modern Seccomp & Syscall Compatibility Layer

Modern Linux kernels, container environments (Docker, Podman, systemd-nspawn), and 64-bit architectures (AArch64, RISC-V 64) enforce strict seccomp filters or completely omit legacy direct filesystem syscalls (`symlink`, `unlink`, `rmdir`, `mkdir`, `rename`, `readlink`, `stat`, `lstat`, `open`, `access`). Calling legacy syscall numbers on these systems triggers `SIGSYS` (signal 31 / "Bad system call").

`symdep` provides a dual-driver abstraction in `include/utils/fs.h`:
- **Modern Kernels (Default)**: Uses directory-relative `*at` syscalls with `AT_FDCWD` (`symlinkat`, `unlinkat`, `mkdirat`, `renameat`, `readlinkat`, `fstatat`, `openat`, `faccessat`, `fchmodat`, `fchownat`). Multi-arch `SYS_renameat2` mappings guarantee zero-window atomic directory swaps across all CPU architectures.
- **Legacy Kernels**: Set `-DSYMDEP_LEGACY_SYSCALLS` during build to route filesystem operations through classic direct POSIX syscalls for older kernels/embedded targets.

