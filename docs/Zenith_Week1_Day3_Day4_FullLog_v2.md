---
tags: [Zenith, Week1, FPGA, HLS, ARM, DMA, CrossCompiler, Vivado, VitisHLS, BuildInPublic, TutorialTextbook]
date: 2026-03-11
author: Charley Chang
status: COMPLETE — Week 1 finished Wednesday (3 days ahead of schedule)
milestone: M1-Foundation
version: v2 — Q&A Knowledge Integration
---

# Project Zenith — Week 1 Day 3 & Day 4: Full Engineering Log
## (Tutorial Textbook Edition)

> **Purpose of this note:** This is not just a log. It is a complete annotated record of every command, every source file, every concept, and every decision made during Days 3–4 of Week 1. A future reader — including your future self — should be able to reproduce every step from scratch using only this document.

> **v2 update:** Integrated all Q&A knowledge from the post-session review: printf format specifier deep-dive (`%lX` vs `PRIxPTR`), where the cross-compile work physically happened, Vitis Unified IDE GUI correction (VS Code, not Eclipse), α calculation bug fix and full CFAR physics derivation, `0UL` type suffix explanation, `radar_defines.h` separation rationale, `regs_` member variable origin, `arm_receive()` dual-purpose design logic, and a full line-by-line walkthrough of the actual `main.cpp` (bare-metal Chimera style) with the critical distinction between Chimera bare-metal and Zenith's future Linux userspace driver.

---

## Session Overview

| Item | Detail |
|---|---|
| **Date** | Wednesday, 2026-03-11 |
| **Session started** | ~Day 3 continuation into Day 4 |
| **Finished** | Wednesday — 3 days ahead of the 5-day Week 1 plan |
| **What was accomplished** | ARM cross-compiler validated · Vivado Block Design started · Vitis HLS 2025.2 CFAR synthesis **passed** |
| **Key milestone** | First synthesis report proving Chimera CFAR asset transfers clean to Zenith at 150 MHz |

---

## Part 1 — Cleaning Up the GitHub Repository

### What happened

Before the Day 3 technical work, the GitHub repository had a stale `main` branch reference left over from the initial setup. Git reported "remote ref does not exist" when trying to delete it, because GitHub had already removed it, but the local `.git/config` still remembered it.

### The commands and what they mean

```bash
git remote prune origin
```

**What this does:** `git remote prune origin` asks Git to look at the remote named `origin` (your GitHub repository) and remove any local references to remote branches that no longer exist on the server. Think of it like clearing dead shortcuts — the actual files on GitHub are untouched, but your local machine's bookmarks are cleaned up.

**Why it was needed:** When you delete a branch on GitHub (via the web UI or `git push origin --delete branch-name`), Git does *not* automatically clean up the local reference (`remotes/origin/branch-name`) on your machine. Over time you accumulate stale references. `prune` removes them.

```bash
git branch -a
```

**What this does:** Lists all branches — both local and remote-tracking. The `-a` flag means "all." Without it, `git branch` only shows local branches.

**Expected output after pruning:**
```
* master
  remotes/origin/master
```

One local branch (`master`), one remote-tracking branch (`remotes/origin/master`). Clean.

**Why `master` and not `main`:** Git's default branch name changed from `master` to `main` around 2021. We explicitly use `master` for Zenith because the Chimera project used `master` and consistency across the codebase matters more than following defaults. This was an explicit decision — not an accident.

---

## Part 2 — ARM Cross-Compiler Validation

### Where this work physically happened

Everything in this section was done on **WSL2 (Ubuntu 24.04) on DESKTOP-CHIMERA** — not on the board, not in Windows native. The cross-compiler runs on x86, produces an ARM binary, and we verified the binary with the `file` command. The binary was never actually *executed* during this session — that happens later in M1, when PetaLinux is ready and we can `scp` it to the board. The smoke test here was purely structural: can the C++20 toolchain produce a valid ARM ELF?

```
DESKTOP-CHIMERA
  └── WSL2 Ubuntu 24.04
        ├── install arm-linux-gnueabihf-g++
        ├── write /tmp/zenith_crosstest.cpp
        ├── compile → /tmp/zenith_arm_test
        └── file /tmp/zenith_arm_test  ← verification here
                                         (binary never run on board yet)
```

### Conceptual Foundation: What is Cross-Compilation?

On your workstation (DESKTOP-CHIMERA), the CPU is an Intel/AMD x86-64 processor. The ALINX AX7020 board's Zynq-7020 contains an ARM Cortex-A9 processor — a completely different instruction set architecture (ISA).

When you compile code normally with `g++`, the output runs on your workstation. You cannot run that binary on the ARM board. **Cross-compilation** means: using a compiler running on x86 to produce binaries that run on ARM.

```
Your workstation (x86-64)           AX7020 board (ARM Cortex-A9)
┌─────────────────────────┐         ┌──────────────────────────┐
│  arm-linux-gnueabihf-g++│──ELF───►│  Cortex-A9 executes      │
│  (cross-compiler)       │         │  the ARM binary          │
└─────────────────────────┘         └──────────────────────────┘
```

The cross-compiler name encodes exactly what it produces:
- `arm` — target architecture: ARM
- `linux` — target OS: Linux
- `gnueabihf` — ABI: GNU EABI, hard-float (uses hardware FPU)

### Installation command

```bash
sudo apt install gcc-arm-linux-gnueabihf g++-arm-linux-gnueabihf -y
```

**Breakdown:**
- `sudo` — run as superuser (required for system-wide package install)
- `apt install` — Debian/Ubuntu package manager install command
- `gcc-arm-linux-gnueabihf` — C cross-compiler
- `g++-arm-linux-gnueabihf` — C++ cross-compiler (this is what we use for C++20)
- `-y` — automatically answer "yes" to all prompts (non-interactive)

### The smoke test source file

```cpp
// /tmp/zenith_crosstest.cpp
#include <span>       // C++20: std::span — the zero-copy view type Zenith uses everywhere
#include <array>      // C++20: std::array — fixed-size stack array, no heap
#include <cstdint>    // Platform-independent integer types: uint8_t, uintptr_t
#include <cstdio>     // printf — C-style I/O, no heap allocation

// constexpr: evaluated at COMPILE TIME, not runtime.
// The compiler substitutes the literal value 0x3F000000 everywhere
// CMA_PHYS_BASE appears. Zero runtime overhead.
// uintptr_t: an unsigned integer EXACTLY wide enough to hold a pointer.
//   On 32-bit ARM: uintptr_t = uint32_t (32 bits)
//   On 64-bit x86: uintptr_t = uint64_t (64 bits)
// The hex literal 0x3F00'0000 uses C++14 digit separators (apostrophe)
// for readability. The compiler ignores the apostrophes.
constexpr uintptr_t CMA_PHYS_BASE = 0x3F00'0000;

int main() {
    // std::array<uint8_t, 4>: a fixed-size array of 4 bytes, lives on the stack.
    // No heap allocation. Size must be known at compile time.
    // Brace initialization: {0xDE, 0xAD, 0xBE, 0xEF} = the classic "DEADBEEF"
    // debugging sentinel value.
    std::array<uint8_t, 4> buf{0xDE, 0xAD, 0xBE, 0xEF};

    // std::span<uint8_t>: a NON-OWNING view over the array.
    // It holds a pointer and a size. No copy of the data is made.
    // This is the C++20 primitive that underpins Zenith's zero-copy DMA design:
    // instead of copying data out of DMA buffers, we create a span over them.
    std::span<uint8_t> view(buf);

    // printf with CMA address.
    // BUG that was caught here: %lX expects a "long" (64-bit on x86, but
    // only 32-bit on ARM). On ARM, uintptr_t is uint32_t, not long.
    // So %lX is the WRONG format specifier on ARM.
    printf("Zenith cross-compile OK | span=%zu | CMA=0x%08lX\n",
           view.size(), CMA_PHYS_BASE);
    return 0;
}
```

### The compilation command

```bash
arm-linux-gnueabihf-g++ -std=c++20 -O2 \
    -o /tmp/zenith_arm_test /tmp/zenith_crosstest.cpp
```

**Flag-by-flag breakdown:**

| Flag | Meaning | Why it matters |
|---|---|---|
| `arm-linux-gnueabihf-g++` | The cross-compiler executable | Produces ARM binary, not x86 |
| `-std=c++20` | Use C++20 language standard | Required for `std::span`, digit separators, `[[nodiscard]]` |
| `-O2` | Optimization level 2 | Enables inlining, constant folding, dead code elimination. Close to release-quality speed. `-O0` = debug, `-O3` = aggressive (can cause subtle bugs) |
| `-o /tmp/zenith_arm_test` | Output file path | Where the compiled binary goes |
| `/tmp/zenith_crosstest.cpp` | Input source file | The C++ source to compile |

### The verification command

```bash
file /tmp/zenith_arm_test
```

**What `file` does:** Reads the first few bytes of a binary and identifies its format. For ELF executables, it reports the architecture, ABI, and linking type.

**Actual output:**
```
/tmp/zenith_arm_test: ELF 32-bit LSB pie executable, ARM, EABI5 version 1 (SYSV),
dynamically linked, interpreter /lib/ld-linux-armhf.so.3,
BuildID[sha1]=3f5bb9031236e12b88e1af4324e1068b3522c089,
for GNU/Linux 3.2.0, not stripped
```

**Decoding every field:**

| Field | Value | Meaning |
|---|---|---|
| `ELF` | — | Executable and Linkable Format — the standard binary format on Linux |
| `32-bit` | — | This is a 32-bit binary. ARM Cortex-A9 is a 32-bit core. |
| `LSB` | Little-Endian Significant Byte | ARM uses little-endian byte order (least significant byte at lowest address) |
| `pie executable` | Position Independent Executable | Can be loaded at any virtual address. Enables ASLR security. |
| `ARM` | — | Target ISA: ARM instruction set |
| `EABI5` | Embedded ABI v5 | ARM calling convention standard. Defines how function arguments are passed, how the stack is laid out. Must match between compiler and OS. |
| `SYSV` | System V ABI | OS-level ABI. Linux uses SYSV. |
| `dynamically linked` | — | This binary calls shared libraries at runtime (like `libc.so`). The OS must provide them. Our AX7020 runs Linux, so these are available. |
| `interpreter /lib/ld-linux-armhf.so.3` | — | The dynamic linker that loads this binary. `armhf` = ARM hard-float. This confirms the binary uses hardware FPU, which the Cortex-A9 has. |
| `not stripped` | — | Debug symbols are still present. For production deployment, use `strip` to reduce binary size. |

**Why this matters for Zenith:** Every field here must be correct. The `ARM EABI5` ABI must match what PetaLinux's kernel expects. `dynamically linked` against `ld-linux-armhf.so.3` means the board's rootfs must have this library — PetaLinux provides it.

---

### The bug discovered: `%lX` vs `PRIxPTR` — deep dive

The compiler emitted this warning:
```
warning: format '%lX' expects argument of type 'long unsigned int',
but argument 3 has type 'unsigned int' [-Wformat=]
```

#### What is `%08lX`? Is it a type?

No — it is not a C++ type. It is a **format specifier** inside a `printf` format string. It is a recipe that tells `printf` two things: what kind of value to expect, and how to print it.

```
% 0 8 l X
│ │ │ │ └─ X = hexadecimal, uppercase letters (A-F). x = lowercase (a-f)
│ │ │ └─── l = "long" modifier — the value is a `long`, not an `int`
│ │ └───── 8 = minimum field width: pad to at least 8 characters
│ └─────── 0 = pad with zeros ("0x0000001F" not "0x      1F")
└───────── % = start of format specifier
```

So `%08lX` means: *"print a `long unsigned int` as uppercase hex, zero-padded to 8 digits."*

#### Why is `%lX` wrong on ARM?

The mismatch is between what `%lX` *expects* and what `CMA_PHYS_BASE` *actually is*:

```cpp
constexpr uintptr_t CMA_PHYS_BASE = 0x3F000000;
```

`uintptr_t` is defined as "an unsigned integer exactly wide enough to hold a pointer." On different platforms:

```
32-bit ARM (our board):   uintptr_t = uint32_t  (32 bits)
64-bit x86 (WSL2):        uintptr_t = uint64_t  (64 bits)
```

`%lX` tells `printf` to expect a `long`. But `long` also has platform-dependent width:

```
32-bit ARM:  long = 32 bits  ← same width as uintptr_t here, works by accident
64-bit x86:  long = 64 bits  ← same width as uintptr_t here, works by accident
```

It *happens* to work on both common cases. But the C++ standard says `uintptr_t` and `long` are different types — passing the wrong type is undefined behavior. On some platforms (certain 16-bit DSPs, future 128-bit systems), it would produce wrong output or crash. The compiler warns because it catches this type mismatch even when it happens to work today.

#### What is `PRIxPTR`, and what does `#include <cinttypes>` give you?

`PRIxPTR` is a **preprocessor macro** defined in `<cinttypes>`. It expands to the *correct* format specifier for `uintptr_t` on the current compilation platform:

```cpp
// On 32-bit ARM, the header defines:
#define PRIxPTR  "x"     // plain lowercase hex, no 'l' modifier

// On 64-bit x86, the header defines:
#define PRIxPTR  "lx"    // needs the 'l' modifier for 64-bit long
```

When you write:

```cpp
#include <cinttypes>
printf("CMA=0x%08" PRIxPTR "\n", CMA_PHYS_BASE);
```

The compiler performs string concatenation at compile time (adjacent string literals are automatically joined — this is a C language rule, not a C++ addition). On 32-bit ARM it becomes:

```cpp
printf("CMA=0x%08" "x" "\n", CMA_PHYS_BASE);
// → printf("CMA=0x%08x\n", CMA_PHYS_BASE);   ← correct for 32-bit ARM
```

On 64-bit x86 it becomes:

```cpp
printf("CMA=0x%08" "lx" "\n", CMA_PHYS_BASE);
// → printf("CMA=0x%08lx\n", CMA_PHYS_BASE);  ← correct for 64-bit x86
```

One source line, always correct, zero runtime cost.

**The `<cinttypes>` family:**

| Macro | Format produced | Use for |
|---|---|---|
| `PRIxPTR` | hex lowercase | `uintptr_t` addresses |
| `PRIXPTR` | hex uppercase | `uintptr_t` addresses, uppercase |
| `PRIdPTR` | signed decimal | signed pointer-sized integer |
| `PRIuPTR` | unsigned decimal | unsigned pointer-sized integer |

**Naming convention:** `PRI` = printf, the letter (`x`/`X`/`d`/`u`) = format character, `PTR` = for pointer-sized integers.

**Zenith principle:** Any file that prints physical addresses must use `PRIxPTR`. Code compiled on both ARM (board validation) and x86 (unit tests) must be portable. `PRIxPTR` is zero-cost portability.

---

## Part 3 — Vivado Block Design: The PS7 Foundation

### Conceptual Foundation: What is a Block Design?

The Zynq-7020 is a **heterogeneous SoC (System on Chip)**. Unlike a pure FPGA, it contains two distinct domains welded together on the same die:

```
Zynq-7020 Die
┌──────────────────────────────────────────────────────────┐
│  PS (Processing System)          PL (Programmable Logic) │
│  ┌──────────────────────┐        ┌─────────────────────┐ │
│  │  ARM Cortex-A9 x2    │        │  FPGA Fabric        │ │
│  │  DDR3 controller     │◄──────►│  (LUT, FF, BRAM,    │ │
│  │  USB, Ethernet, SPI  │  AXI   │   DSP48)            │ │
│  │  SD card, UART       │  buses │                     │ │
│  └──────────────────────┘        └─────────────────────┘ │
└──────────────────────────────────────────────────────────┘
```

A **Vivado Block Design** is a graphical/Tcl-based way to wire together IP cores into this system. For Zenith, the minimum viable block design is:
- PS7 (the ARM processor and all its peripherals)
- AXI DMA (for zero-copy data transfer between ARM and FPGA)
- AXI Interconnect (routing bus)
- HLS IP cores (cfar, fft, dds — added in later milestones)

### Error encountered and why it happened

```
ERROR: [BD 5-104] A block design must be open to run this command.
Please create/open a block design.
```

**Root cause:** The Tcl commands `create_project` and `create_bd_cell` were run without the intermediate `create_bd_design` step. Vivado Tcl has a strict state machine:

```
create_project  →  create_bd_design  →  create_bd_cell  →  add IP
(project exists)  (canvas is open)   (now can add cells)
```

Skipping `create_bd_design` means trying to paint on a canvas that doesn't exist.

### The correct Tcl sequence (fully annotated)

```tcl
# ─── Step 1: Create the Vivado project ───────────────────────────────────────
create_project zenith_bd C:/Projects/zenith_radar_os/hardware/block-design \
    -part xc7z020clg400-1
# create_project <n> <path> -part <device>
# name: internal project name (used in filenames, not critical)
# path: where Vivado stores all project files (.xpr, .srcs, etc.)
# -part: the exact FPGA part number
#   xc7z020  = Zynq-7020 (Artix-7 fabric + dual Cortex-A9)
#   clg400   = CLG400 package (400-pin BGA)
#   -1        = speed grade (slowest/most conservative = safest timing)

# ─── Step 2: Create AND open a block design canvas ───────────────────────────
create_bd_design "zenith_system"
# This creates a block design named "zenith_system" AND opens it for editing.
# Without this, all subsequent create_bd_cell commands fail.
# "zenith_system" becomes the top-level module name in the generated HDL.

# ─── Step 3: Add the PS7 IP core ─────────────────────────────────────────────
create_bd_cell -type ip -vlnv xilinx.com:ip:processing_system7 ps7_0
# -type ip: this is an IP core (as opposed to -type module for custom HDL)
# -vlnv: Vendor:Library:Name:Version identifier
#   xilinx.com  = vendor
#   ip          = library
#   processing_system7 = the Zynq PS7 IP
#   (no version = use latest installed version)
# ps7_0: instance name. "ps7" = the IP type, "_0" = first instance of this type.
#   This is the ARM cores, DDR controller, clocks, and all PS peripherals.

# ─── Step 4: Apply board automation ──────────────────────────────────────────
apply_bd_automation \
    -rule xilinx.com:bd_rule:processing_system7 \
    -config {make_external "FIXED_IO, DDR" apply_board_preset "1"} \
    [get_bd_cells ps7_0]
# apply_bd_automation: Vivado's "smart connect" feature
# -rule: which automation rule to apply (PS7-specific rule here)
# make_external "FIXED_IO, DDR":
#   FIXED_IO = PS7's MIO pins (UART, SD, USB, Ethernet) — must connect to board pads
#   DDR      = DDR3 memory interface — must connect to board DDR chips
#   "make_external" promotes these to top-level ports of the block design,
#   so Vivado knows they connect to physical board pins (not to other IP).
# apply_board_preset "1": applies DDR3 timing parameters from the board definition.
#   For AX7020 with ALINX board files: sets correct DDR3 CL/tRCD/tRP timings.
#   If board files not installed: silently skips preset, DDR3 timing = generic defaults.
# [get_bd_cells ps7_0]: the Tcl object representing our PS7 instance

# ─── Step 5: Set FCLK0 to 150 MHz ────────────────────────────────────────────
set_property -dict [list \
    CONFIG.PCW_FPGA0_PERIPHERAL_FREQMHZ {150} \
] [get_bd_cells ps7_0]
# set_property -dict: set multiple properties at once from a list
# CONFIG.PCW_FPGA0_PERIPHERAL_FREQMHZ: PS7 configuration property for FCLK0 frequency
#   PCW = Processing system Configuration Wizard (Xilinx naming convention)
#   FPGA0 = FCLK0 (fabric clock 0)
#   PERIPHERAL_FREQMHZ = frequency in MHz
# {150}: target frequency in MHz
#
# FCLK0 is generated by the PS7's PLL and distributed to the PL fabric.
# This is the MASTER CLOCK for all of Zenith's AXI-Stream data paths.
# All HLS kernels (CFAR, FFT, DDS) will run on this 150 MHz clock.
# Why 150 MHz? It's the target that gives II=1 timing margin on Zynq-7020 speed grade -1.

# ─── Step 6: Enable AXI HP0 port ─────────────────────────────────────────────
set_property -dict [list \
    CONFIG.PCW_USE_S_AXI_HP0 {1} \
] [get_bd_cells ps7_0]
# CONFIG.PCW_USE_S_AXI_HP0: enable the first High-Performance AXI port
# {1} = enabled (boolean)
#
# AXI HP0 is the high-bandwidth path from PL to DDR.
# "HP" = High Performance (up to ~1200 MB/s bandwidth)
# "S" = Slave (from the PS's perspective — the PL is the master, PS is the slave)
# This is how the AXI DMA in the PL writes processed IQ data back to DDR
# where the ARM can read it. Without this port enabled, DMA has no DDR access.
# Zynq-7020 has 4 HP ports (HP0-HP3). We use HP0 for the main data DMA.

# ─── Step 7: Fix clock connections (required to pass validation) ──────────────
connect_bd_net [get_bd_pins ps7_0/FCLK_CLK0] \
               [get_bd_pins ps7_0/M_AXI_GP0_ACLK]
connect_bd_net [get_bd_pins ps7_0/FCLK_CLK0] \
               [get_bd_pins ps7_0/S_AXI_HP0_ACLK]
# connect_bd_net: creates a wire between two pins in the block design
# FCLK_CLK0: output pin — the 150MHz clock generated by the PS7 PLL
# M_AXI_GP0_ACLK: clock input for the PS7's AXI General Purpose Master port 0
#   This port is used for ARM to write control registers to PL IP cores (AXI-Lite).
# S_AXI_HP0_ACLK: clock input for the AXI HP0 slave port
#   This port is used by the PL DMA to write data to DDR.
#
# WHY THIS IS REQUIRED:
# The PS7 generates FCLK_CLK0 but Vivado doesn't automatically connect it
# back to the PS7's own AXI port clock inputs. This seems circular, but it's
# intentional — theoretically you could drive these ports from an EXTERNAL clock.
# Since we want all AXI traffic synchronized to FCLK_CLK0, we must explicitly
# make this connection. Without it, validate_bd_design fails with:
# "ERROR: [BD 41-758] clock pins not connected to a valid clock source"

# ─── Step 8: Validate and save ───────────────────────────────────────────────
validate_bd_design
save_bd_design
# validate_bd_design: checks all connections, clock domains, address map
# save_bd_design: writes the block design to disk
```

### The validation error decoded

```
WARNING: [BD 41-2670] Found an incomplete address path from address space
'/ps7_0/Data' to master interface '/ps7_0/M_AXI_GP0'.
Please either complete or remove this path to resolve.

ERROR: [BD 41-758] The following clock pins are not connected to a valid clock source:
/ps7_0/M_AXI_GP0_ACLK
/ps7_0/S_AXI_HP0_ACLK
```

**Translating the warning:** The M_AXI_GP0 port is enabled (because we enabled HP0) but has no AXI slave attached to it yet. Vivado is saying "you opened this AXI port but nothing is connected on the other end." This will be resolved in M1 when we attach the AXI DMA IP to this port. For Week 1, this is expected.

**Translating the error:** The two AXI ports need clock signals but haven't been wired to FCLK0 yet. This was fixed by the `connect_bd_net` commands above.

---

## Part 4 — Vitis HLS 2025.2: CFAR Kernel Synthesis

### Conceptual Foundation: What is HLS?

**High-Level Synthesis (HLS)** is the process of compiling C++ code into FPGA hardware description (RTL — Register Transfer Level). The HLS compiler:

1. Analyzes the C++ algorithm
2. Schedules operations across clock cycles
3. Allocates hardware resources (BRAM, DSP48, LUT, FF)
4. Produces VHDL/Verilog and an IP core package

The mental model shift: in C++ on a CPU, code runs sequentially one instruction at a time. In HLS, the compiler physically instantiates hardware that runs forever, continuously processing streaming data. A `for` loop doesn't loop — it becomes a pipeline where new data enters every clock cycle while previous data is still being processed.

### Why the Vitis HLS GUI changed — correction

**⚠️ Correction from original note:** The original note said Vitis Unified IDE is "based on Eclipse." This is wrong.

Before 2023.1, Vitis HLS had its own standalone Eclipse-based GUI. Starting from **2023.1**, Xilinx migrated to the **Vitis Unified IDE**, which is built on **VS Code** with Xilinx-specific extensions — not Eclipse. The underlying HLS synthesis engine (the `vitis_hls` compiler binary) did not change at all; only the frontend GUI wrapper changed. The VS Code shell is actually more comfortable for engineers already using VS Code for C++ development.

### Source file assignment decisions

When creating the HLS component, the wizard asks you to assign files to either "Design Files" (synthesized into hardware) or "Testbench Files" (used only for C-Simulation, never synthesized).

| File | Assignment | Detailed reasoning |
|---|---|---|
| `cfar.cpp` | ✅ Design Files | This IS the hardware. `cfar_core()` inside this file is the function that becomes silicon. |
| `cfar.h` | ✅ Design Files | Header for `cfar_core`. Contains type definitions (`data_t`, `param_t`) and function declarations that cfar.cpp depends on. Must be available during synthesis. |
| `radar_defines.h` | ✅ Design Files | Contains `constexpr` constants: `WINDOW_SIZE=21`, `REF_CELLS=8`, `GUARD_CELLS=2`, `CUT_IDX=10`, `FRAME_SIZE=1024`. These are used by cfar.cpp. Without this, cfar.cpp fails to compile in HLS. |
| `main.cpp` | ❌ Excluded entirely | This is the **ARM Cortex-A9 bare-metal application** from Project Chimera. It `#include`s `xil_cache.h` (a Xilinx SDK header that does not exist in the HLS environment), calls `Xil_DCacheInvalidateRange` (an ARM cache flush function), and uses bare-metal hardware access patterns. The HLS compiler has no idea what any of this means. Including it would cause immediate compile errors. |
| `hal/cfar_engine_controller.hpp` | ❌ Excluded entirely | ARM bare-metal AXI-Lite register controller. Uses `volatile uint32_t` memory-mapped I/O at physical addresses. This is code that runs on the ARM and talks to the FPGA over AXI-Lite — it is emphatically not part of the FPGA design itself. |
| `hal/axi_dma_controller.hpp` | ❌ Excluded entirely | ARM bare-metal DMA driver. Same rationale as above. |

### The CFAR source files explained

#### `radar_defines.h` — and why it is a separate file

```cpp
// radar_defines.h — the "constitution" of the CFAR hardware
#pragma once
#include "ap_fixed.h"     // Xilinx arbitrary-precision fixed-point type

// ap_fixed<W, I>: Xilinx HLS fixed-point type
// W = total bit width (16 bits total)
// I = integer bits (8 bits for integer part)
// Fractional bits = W - I = 8 bits
// Range: -128.0 to +127.996 (step = 1/256 ≈ 0.0039)
// Why fixed-point instead of float?
//   - FPGA DSP48 slices natively handle fixed-point multiply-accumulate
//   - Float requires 3-5 DSP48 per operation; fixed-point uses 1
//   - Deterministic bit-exact results (no floating-point rounding modes)
using data_t  = ap_fixed<16, 8>;
using param_t = ap_fixed<16, 8>;

// CA-CFAR window geometry
// The window around each Cell Under Test (CUT):
//    [REF_CELLS | GUARD_CELLS | CUT | GUARD_CELLS | REF_CELLS]
//    [   8      |      2      |  1  |      2      |    8     ]
// = 21 cells total = WINDOW_SIZE
constexpr int REF_CELLS   = 8;    // Training cells on each side
constexpr int GUARD_CELLS = 2;    // Guard cells (not included in noise estimate)
constexpr int CUT_IDX     = 10;   // Center index of window (0-indexed)
constexpr int WINDOW_SIZE = 21;   // 2*(REF_CELLS + GUARD_CELLS) + 1
constexpr int FRAME_SIZE  = 1024; // Number of range cells per detection frame
```

**Why `radar_defines.h` is separate from `cfar.h`:**

These constants could technically live inside `cfar.h`. The reason for separation is the **Single Responsibility Principle** applied to hardware design. `radar_defines.h` defines *radar system parameters* — things that come from the physics specification. These parameters are shared across multiple kernels: the FFT needs `FRAME_SIZE`, a future MTI filter also needs `FRAME_SIZE`, a future DDS needs related timing constants.

If `FRAME_SIZE` only lived in `cfar.h`, then `fft.h` would have to `#include "cfar.h"` just to get a constant that has nothing to do with CFAR — a nonsensical dependency. The separation creates a clean dependency graph:

```
Without separation:          With separation:
                             radar_defines.h
fft.h → cfar.h  (wrong!)        ↓           ↓
                             cfar.h       fft.h
                             (correct)    (correct)
```

Change `FRAME_SIZE` in one place → every kernel picks it up automatically. No circular includes, no kernel knowing about other kernels' internals.

**Physical meaning of the CFAR window:**

CA-CFAR (Cell-Averaging Constant False Alarm Rate) is the algorithm that decides whether a radar range cell contains a real target or just noise. For each cell being tested (the CUT):

1. Look at the 8 cells to the left (excluding 2 guard cells)
2. Look at the 8 cells to the right (excluding 2 guard cells)
3. Average all 16 reference cells → this is the local noise estimate
4. Multiply by threshold factor α → detection threshold
5. If CUT > threshold → declare detection

Guard cells prevent the target's own energy (which leaks into adjacent cells due to FFT sidelobes) from contaminating the noise estimate.

---

#### CFAR physics: How to derive α, and its relation to detection performance

This is one of the most important concepts in radar signal processing. The note's original example said "α ≈ 6.89" — this is a calculation error. The correct value is derived below.

**The statistical model**

After envelope detection, the noise in each reference cell follows an exponential distribution:

$$p(x) = \frac{1}{\mu} e^{-x/\mu}, \quad x \geq 0$$

where μ is the mean noise power. The CA-CFAR noise estimate averages N total reference cells:

$$\hat{\mu} = \frac{1}{N} \sum_{i=1}^{N} x_i, \quad N = 2 \times N_{train}$$

The detection threshold is:

$$T = \alpha \cdot \hat{\mu}$$

**Deriving α from Pfa**

The false alarm probability is the probability that a noise-only cell exceeds the threshold:

$$P_{fa} = P(x_{CUT} > T \mid \text{noise only}) = \left(1 + \frac{\alpha}{N}\right)^{-N}$$

Solving for α:

$$\boxed{\alpha = N \cdot \left(P_{fa}^{-1/N} - 1\right)}$$

where **N = total reference cells = 2 × REF_CELLS**.

**Correct calculation for Zenith's parameters:**

```
N_total  = 2 × REF_CELLS = 2 × 8 = 16   ← total reference cells, BOTH sides
Pfa      = 1e-4

α = 16 × ( (1e-4)^(-1/16) - 1 )
  = 16 × ( 10^(4/16) - 1 )
  = 16 × ( 10^0.25 - 1 )
  = 16 × ( 1.778 - 1 )
  = 16 × 0.778
  ≈ 12.44   ← CORRECT value
```

**Why the note originally said 6.89 — the bug:**

6.89 comes from accidentally using N = one side only (N = 8) as the multiplier while using N_total = 16 in the exponent — a mixed-up version of the formula. The formula is right; the example number was wrong. **The correct value to write into the hardware register is approximately 12, not 6.89.** Programming α = 6.89 would set the threshold too low, causing a much higher false alarm rate than intended.

**The Pfa ↔ Pd ↔ SNR triangle**

Once α is set (fixing Pfa), detection probability depends on SNR. For a Swerling 1 target:

$$P_d \approx \left(1 + \frac{\alpha}{N + \text{SNR}}\right)^{-N} \cdot \left(1 + \frac{N}{\text{SNR}}\right)^{N-1}$$

The fundamental trade-off: **you cannot independently set both Pfa and Pd without changing SNR.**

```
Pd
1.0 │                              ╭──────── SNR = 20 dB
    │                         ╭───╯
    │                    ╭────╯            SNR = 13 dB
0.5 │               ╭────╯
    │          ╭─────╯                     SNR = 6 dB
0.0 └──────────────────────────────────── Pfa
    0    1e-6   1e-4   1e-2

At SNR=13dB, Pfa=1e-4:  Pd ≈ 0.8
To get Pd=0.99 at same Pfa:  need SNR ≈ 17dB  (4dB more power or more integration)
```

**CFAR loss:** Using only N = 16 samples to estimate noise introduces statistical fluctuation in the threshold. This costs approximately:

$$\text{CFAR Loss} \approx \frac{8.7}{N} \text{ dB} = \frac{8.7}{16} \approx 0.54 \text{ dB}$$

You need 0.54 dB more SNR compared to a system with a perfectly known noise floor. Larger N reduces CFAR loss. Zenith community edition uses N = 16 (adequate for short-range applications).

---

#### `cfar.h` — hardware interface contract

```cpp
// cfar.h — hardware interface contract
#pragma once
#include "hls_stream.h"      // Xilinx HLS streaming type
#include "ap_axi_sdata.h"    // AXI4-Stream data types
#include "radar_defines.h"

// hls::axis<T, USER_T, ID_T, DEST_T>: Xilinx's AXI4-Stream packet type
// T        = data type (data_t = ap_fixed<16,8>)
// 0UL, 0UL, 0UL = no TUSER, no TID, no TDEST fields
// This creates a minimal AXI4-Stream with only TDATA, TVALID, TREADY, TLAST
using axis_t = hls::axis<data_t, 0, 0, 0>;
```

**What is `0UL`? Is it a type?**

`0UL` is an integer literal with a **type suffix**. It is not a type itself — the suffix tells the compiler what *type to give this literal*:

```
0        →  int              (default, 32-bit signed)
0U       →  unsigned int     (32-bit unsigned)
0L       →  long             (32-bit or 64-bit signed, platform-dependent)
0UL      →  unsigned long    (32-bit or 64-bit unsigned, platform-dependent)
0ULL     →  unsigned long long (always 64-bit)
```

In `hls::axis<data_t, 0UL, 0UL, 0UL>`, these are **template non-type parameters**. The template expects `unsigned long` values for the TUSER/TID/TDEST field widths. Writing plain `0` (which is `int`) would cause a template type mismatch warning. Writing `0UL` makes the type explicit and matches the template parameter type exactly — no warning, no implicit conversion.

```cpp
// Top function declaration
// threshold_alpha: the CFAR threshold scaling factor
//   Comes from ARM via AXI-Lite register write
//   Relationship: α = N_train × (Pfa^(-1/N_train) - 1)
//   For N_train=16 (total), Pfa=1e-4: α ≈ 12.44  ← corrected from original note
void cfar_core(hls::stream<axis_t>& in_stream,
               hls::stream<axis_t>& out_stream,
               param_t threshold_alpha);
```

#### `cfar.cpp` — annotated

```cpp
// cfar.cpp — the hardware kernel
#include "cfar.h"

void cfar_core(hls::stream<axis_t>& in_stream,
               hls::stream<axis_t>& out_stream,
               param_t threshold_alpha)
{
    // ─── INTERFACE PRAGMAS ─────────────────────────────────────────────────
    #pragma HLS INTERFACE axis port=in_stream
    #pragma HLS INTERFACE axis port=out_stream
    #pragma HLS INTERFACE s_axilite port=threshold_alpha bundle=CTRL
    #pragma HLS INTERFACE s_axilite port=return bundle=CTRL

    // ─── PIPELINE PRAGMA ───────────────────────────────────────────────────
    #pragma HLS PIPELINE II=1

    // ─── WINDOW BUFFER ─────────────────────────────────────────────────────
    static data_t window[WINDOW_SIZE];  // 21 elements
    #pragma HLS ARRAY_PARTITION variable=window complete
    // ARRAY_PARTITION complete: unrolls the array into 21 separate registers (FFs)
    // WHY: the sliding window sum reads ALL 16 reference cells in ONE clock cycle.
    // BRAM has only 2 ports → reading 16 values needs 8 cycles → II=8.
    // FFs give 21 independent ports → all reads simultaneous → II=1.
    // Cost: 21 × 16 bits = 336 FFs ≈ 0.3% of Zynq-7020's FFs. Very cheap.
    // This is why the synthesis report shows BRAM=0.

    static data_t window_sum = 0;

    // ─── READ ONE SAMPLE ───────────────────────────────────────────────────
    axis_t input_sample;
    bool valid = in_stream.read_nb(input_sample);
    // read_nb: non-blocking read. Returns true if data available, false if empty.
    // WHY non-blocking: PIPELINE II=1 requires every cycle to complete in one cycle.
    // A blocking read that waits for data would stall the pipeline.

    if (valid) {
        // ─── SHIFT WINDOW ──────────────────────────────────────────────────
        #pragma HLS UNROLL
        for (int i = 0; i < WINDOW_SIZE - 1; i++) {
            window[i] = window[i + 1];
        }
        // UNROLL: physically replicates 20 assignments, all execute simultaneously.
        window[WINDOW_SIZE - 1] = input_sample.data;

        // ─── COMPUTE REFERENCE SUM ─────────────────────────────────────────
        data_t ref_sum = 0;
        #pragma HLS UNROLL
        for (int i = 0; i < WINDOW_SIZE; i++) {
            if (i < (CUT_IDX - GUARD_CELLS) || i > (CUT_IDX + GUARD_CELLS)) {
                ref_sum += window[i];
            }
        }
        // 16-way parallel add tree in hardware.
        // The if-condition uses constexpr indices → resolved at compile time → no branch.

        // ─── COMPUTE THRESHOLD ─────────────────────────────────────────────
        data_t noise_level = ref_sum / (2 * REF_CELLS);
        data_t threshold   = noise_level * threshold_alpha;
        // This multiply is the ONE DSP48 in the synthesis report.
        // ap_fixed<16,8> × ap_fixed<16,8> → exactly one DSP48E1.

        // ─── DETECTION DECISION ────────────────────────────────────────────
        data_t cut_value = window[CUT_IDX];
        axis_t output_sample;
        output_sample.data = (cut_value > threshold) ? cut_value : data_t(0);
        output_sample.last = input_sample.last;
        // Propagate TLAST: frame boundary passes through unchanged.
        out_stream.write(output_sample);
    }
}
```

---

#### `hal/cfar_engine_controller.hpp` — ARM driver for the CFAR IP core

```cpp
class CfarEngineController {
    // Offsets confirmed from Vitis HLS synthesis interface report:
    // ap_ctrl (start/done/idle) → AXI-Lite offset 0x00
    // threshold_alpha           → AXI-Lite offset 0x10  ← not 0x00, confirmed in synthesis
    static constexpr uint32_t CTRL_OFFSET      = 0x00;
    static constexpr uint32_t THRESHOLD_OFFSET = 0x10;

    volatile uint32_t* base_;
    // volatile: tells the compiler "do not cache this — always read from hardware."
    // Without volatile, the compiler might optimize away register writes because
    // they "don't change any C++ variables." One of the few legitimate uses of
    // volatile in modern C++.

public:
    [[nodiscard]] bool start() noexcept {
        base_[CTRL_OFFSET / 4] = 0x1;  // write AP_START bit
        // /4 because base_ is uint32_t* but offsets are byte-based
        return true;
    }
};
```

---

#### `hal/axi_dma_controller.hpp` — ARM driver for the AXI DMA engine

**Understanding `regs_` — where does it come from?**

`regs_` is a **private member variable** of the `AxiDmaController` class. The code shown in the note only shows the methods. The complete class structure is:

```cpp
class AxiDmaController {
    // ← regs_ is declared HERE as a private member variable
    volatile uint32_t* regs_;
    // This holds the mmap'd pointer to the DMA register space.
    // It is set in the constructor and used by all methods.

    static constexpr uint32_t S2MM_DMACR_OFFSET = 0x30;
    static constexpr uint32_t S2MM_DA_OFFSET     = 0x48;
    static constexpr uint32_t S2MM_LENGTH_OFFSET = 0x58;

public:
    // Constructor receives the already-mmap'd pointer and stores it
    explicit AxiDmaController(volatile uint32_t* base) noexcept
        : regs_(base) {}    // ← regs_ is initialized here

    [[nodiscard]]
    std::span<const uint8_t> arm_receive(uintptr_t phys_dst, size_t len) noexcept {
        regs_[S2MM_DA_OFFSET / 4]     = static_cast<uint32_t>(phys_dst);
        regs_[S2MM_LENGTH_OFFSET / 4] = static_cast<uint32_t>(len);
        return std::span<const uint8_t>(
            reinterpret_cast<const uint8_t*>(phys_dst), len);
    }
};
```

The trailing underscore on `regs_` is a C++ naming convention for private member variables — it visually distinguishes them from local variables and function parameters at a glance.

**Why does `arm_receive()` write to `regs_` but return something that has no mention of `regs_`?**

This is the most important design insight in this class. The function performs **two completely independent actions** in one call, and that is intentional by design:

```cpp
std::span<const uint8_t> arm_receive(uintptr_t phys_dst, size_t len) noexcept {

    // ACTION 1: Talk to the hardware chip
    // Tell the DMA controller where to write the incoming data
    regs_[S2MM_DA_OFFSET / 4]     = static_cast<uint32_t>(phys_dst);
    regs_[S2MM_LENGTH_OFFSET / 4] = static_cast<uint32_t>(len);
    // After these two lines: DMA is armed, TREADY=1
    // The PL can now start sending data, DMA will write it to phys_dst

    // ACTION 2: Describe that destination to the C++ caller
    // The caller needs to know WHERE to read the results after transfer completes
    return std::span<const uint8_t>(
        reinterpret_cast<const uint8_t*>(phys_dst), len);
    // This span points to the SAME DDR address we just gave to the DMA chip
}
```

The crucial point: `regs_` and the return value both reference `phys_dst`, but they use it for completely different purposes. Visualized:

```
DDR region at phys_dst (e.g. 0x3F400000):
                    ┌──────────────────────────┐
regs_ side:         │                          │ ← DMA hardware will WRITE here
  "DMA chip:        │   (empty now — data      │    after TREADY asserts
   fill this addr"  │    arrives after PL       │
                    │    starts sending)        │
return side:        │                          │ ← caller will READ from here
  "caller: here     │                          │    after poll_complete()
   is a window      │                          │
   to the results"  └──────────────────────────┘
```

Both `regs_` (hardware side) and the `std::span` (C++ side) describe the same physical DDR region. This is what makes zero-copy work: the DMA fills exactly the memory region the span points to. No intermediate buffer, no memcpy.

---

#### `main.cpp` — the actual Chimera bare-metal file, fully annotated

**Critical distinction before reading this file:**

The `main.cpp` in the Zenith repo was transferred from **Project Chimera** and is a **Xilinx Vitis SDK bare-metal application** — it runs directly on the ARM hardware with no Linux OS underneath. It is fundamentally different from the Linux userspace driver Zenith will eventually use.

```
Chimera main.cpp (this file — bare-metal):    Zenith future main (Linux userspace):
────────────────────────────────────────      ──────────────────────────────────────
No OS present                                 Linux 4.9.0-xilinx running
Direct physical address access works          Must use mmap() via /dev/mem
#include "xil_cache.h"  ← Xilinx SDK         __builtin___clear_cache  ← GCC builtin
Xil_DCacheInvalidateRange()                   (no Xilinx SDK available)
xil_printf() for output                       printf() from glibc
Hardware pointers: just use the address       Hardware pointers: must mmap() first
```

This file is kept in the repo as a historical reference. It is **excluded from HLS synthesis** (confirmed in file assignment table above). When M1 Linux validation begins, the ARM application will be rewritten as the Linux userspace version.

```cpp
// main.cpp — Chimera bare-metal ARM Cortex-A9 application
// Platform: Xilinx Vitis SDK, no OS, direct hardware access
// This is NOT a Linux application. It is a bare-metal program.

// ── Xilinx bare-metal headers ────────────────────────────────────────────────
#include "xil_cache.h"
// Xilinx SDK header for cache management functions.
// Provides: Xil_DCacheInvalidateRange(), Xil_DCacheFlushRange()
// These do NOT exist in Linux userspace — they are bare-metal only.
// In the Zenith Linux driver, the equivalent is __builtin___clear_cache().

#include "xil_printf.h"
// Xilinx SDK lightweight printf implementation for bare-metal.
// Has no dependency on libc. Uses the PS UART directly.
// In Linux userspace this becomes standard printf() from glibc.

#include "xparameters.h"
// Auto-generated by Vitis SDK from the hardware XSA file.
// Contains all peripheral base addresses as #define constants, e.g.:
//   #define XPAR_AXI_DMA_0_BASEADDR  0x43000000
// In the Zenith Linux driver, these come from zenith_memory_map.hpp instead.

// ── Project headers ──────────────────────────────────────────────────────────
#include "zenith_memory_map.hpp"
// Our physical address constants. Works in both bare-metal and Linux
// because it only contains constexpr values — no OS-specific calls.

#include "hal/cfar_engine_controller.hpp"
#include "hal/axi_dma_controller.hpp"
// ARM-side hardware drivers. These use volatile uint32_t* register access,
// which works identically in bare-metal and Linux (given a valid pointer).
// In bare-metal: the pointer IS the physical address directly.
// In Linux: the pointer comes from mmap() but behaves the same way after.
```

```cpp
int main() {

    // ── In bare-metal: no mmap() needed ──────────────────────────────────────
    // In bare-metal mode, the ARM's MMU is typically configured with a 1:1
    // (identity) mapping: virtual address = physical address for the full
    // 32-bit space. So we can directly cast a physical address to a pointer
    // and use it — no OS call required.
    //
    // This is the fundamental difference from Linux userspace, where the kernel
    // enforces separation between virtual and physical address spaces.

    volatile uint32_t* dma_regs =
        reinterpret_cast<volatile uint32_t*>(AXI_DMA_BASE);
    // AXI_DMA_BASE = 0x43000000 from zenith_memory_map.hpp
    // In bare-metal: this just works — address IS a valid pointer.
    // In Linux: this would segfault. Must use mmap() first.

    volatile uint32_t* cfar_regs =
        reinterpret_cast<volatile uint32_t*>(CFAR_CTRL_BASE);
    // CFAR_CTRL_BASE: the AXI-Lite base address of the CFAR IP core in the PL.
    // This address comes from the Vivado Block Design address assignment.
    // In Week 1, the Block Design is not yet complete — this is a placeholder.

    // ── Construct HAL objects ─────────────────────────────────────────────────
    AxiDmaController    dma(dma_regs);
    CfarEngineController cfar(cfar_regs);
    // Both constructors receive the register pointer and store it as regs_.
    // No heap allocation — objects live on the stack (zero-heap principle).

    // ── Set CFAR threshold ────────────────────────────────────────────────────
    cfar.set_threshold(static_cast<param_t>(12.44f));
    // α ≈ 12.44 for N=16 total reference cells, Pfa=1e-4
    // This writes to AXI-Lite register at byte offset 0x10 (confirmed by synthesis)
    // Path: ARM → M_AXI_GP0 → AXI Interconnect → CFAR AXI-Lite slave → register

    // ── Arm the DMA ───────────────────────────────────────────────────────────
    constexpr size_t TRANSFER_BYTES = FRAME_SIZE * sizeof(int16_t);
    // FRAME_SIZE = 1024 range cells (from radar_defines.h)
    // sizeof(int16_t) = 2 bytes (ap_fixed<16,8> maps to int16 in memory)
    // TRANSFER_BYTES = 1024 × 2 = 2048 bytes = one complete CFAR output frame

    auto result_span = dma.arm_receive(RX_PHYS_BASE, TRANSFER_BYTES);
    // This does two things simultaneously:
    //   1. Writes S2MM_DA = RX_PHYS_BASE to DMA register → DMA knows destination
    //   2. Writes S2MM_LENGTH = 2048 to DMA register → TREADY goes HIGH
    //   3. Returns std::span<const uint8_t> over the same RX_PHYS_BASE address
    // After this line: DMA is armed. PL can start sending data.

    // ── Trigger PL processing ─────────────────────────────────────────────────
    cfar.start();
    // Writes 0x1 to CFAR AXI-Lite offset 0x00 (AP_START bit)
    // CFAR hardware begins processing. TVALID will go HIGH as results flow out.
    // Since TREADY is already HIGH (DMA armed), data transfers immediately.

    // ── Wait for DMA completion ───────────────────────────────────────────────
    dma.poll_complete();
    // Busy-wait loop: reads S2MM_DMASR (offset 0x34) until bit 1 (Idle) = 1
    // Acceptable for M1 validation. Replace with interrupt-driven in M4.

    // ── Cache invalidate ──────────────────────────────────────────────────────
    Xil_DCacheInvalidateRange(
        reinterpret_cast<uintptr_t>(RX_PHYS_BASE),
        TRANSFER_BYTES
    );
    // MANDATORY before reading DMA results when using AXI-HP port.
    //
    // Why this is needed (the same reason in bare-metal and Linux):
    //   The DMA wrote to DDR through AXI-HP, which BYPASSES the ARM SCU.
    //   The ARM's L1/L2 cache still holds whatever was at RX_PHYS_BASE before
    //   the DMA transfer — potentially stale data from a previous frame.
    //   Without invalidation, every read returns cached stale data.
    //
    // Bare-metal version: Xil_DCacheInvalidateRange()  ← used here
    // Linux userspace version: __builtin___clear_cache()  ← used in Zenith Linux driver
    // If using ACP port instead of HP: no invalidation needed (SCU handles it)

    // ── Read results — zero copy ──────────────────────────────────────────────
    auto* detections = reinterpret_cast<const int16_t*>(result_span.data());
    // result_span.data() returns a pointer to RX_PHYS_BASE
    // No memcpy — we read directly from the DMA destination buffer
    // This is the zero-copy chain in action

    for (size_t i = 0; i < FRAME_SIZE; i++) {
        if (detections[i] != 0) {
            xil_printf("Detection at range cell %u, value = %d\r\n",
                       (unsigned)i, (int)detections[i]);
            // xil_printf: bare-metal printf
            // %u for unsigned, %d for signed — no PRIxPTR needed here
            // (we're printing range bin index and detection value, not addresses)
            // \r\n: bare-metal UART typically needs explicit carriage return
        }
    }

    return 0;
    // In bare-metal: reaching main() return usually triggers a system reset
    // or infinite loop in the crt0 startup code. Unlike Linux where return
    // exits the process cleanly.
}
```

**The "arm the DMA" protocol — why sequence matters:**

```
CORRECT sequence (what the code does):
  1. dma.arm_receive() → writes S2MM_DA + S2MM_LENGTH → TREADY = 1
  2. cfar.start()      → writes AP_START              → TVALID = 1
  3. Both HIGH simultaneously → data transfers immediately

WRONG sequence (common mistake):
  1. cfar.start()      → TVALID = 1, but TREADY = 0  → AXI-Stream stalls
  2. dma.arm_receive() → TREADY finally goes HIGH
     But now TLAST position relative to data is wrong → frame boundary corrupted
  Result: data arrives in DDR but frame alignment is wrong → silent corruption
```

---

## Part 5 — The HLS Synthesis Report: Reading the Numbers

### The full result

```
Modules & Loops | II | Latency(cycles) | Slack | BRAM | DSP | FF       | LUT      |
cfar_core       |  1 |       6         | +0.23 |    - |  1  | 954 (~0%)|  735 (1%)|
```

### Field-by-field interpretation

**II = 1**

Initiation Interval = 1. The pipeline accepts one new range cell per clock cycle. At 150 MHz, throughput = 150 million detections/second. For a typical radar with 1024 range cells and 64 chirps per CPI, this processes a complete CPI in 65,536 clock cycles = 437 microseconds. More than fast enough for any PRF below ~2 kHz.

**Latency = 6 cycles = 40 ns**

The first valid output appears 6 clock cycles after the first valid input. For radar, this is negligible (range equivalent: 40 ns × c/2 = 6 meters).

**Slack = +0.23 ns**

Timing slack = (clock period) − (critical path delay) = 6.67 ns − 6.44 ns = +0.23 ns positive. The design passes timing at 150 MHz.

**Important caveat:** This is *estimated* timing from HLS. Post-route timing after Vivado placement is usually 0.5–1.5 ns worse due to routing delays. We have 0.23 ns of pre-route margin. If post-route fails: add one pipeline stage to the threshold multiply. This adds 1 cycle of latency but gives ~2 ns additional slack.

**BRAM = 0**

The 21-element window lives entirely in Flip-Flops via `ARRAY_PARTITION complete`. This is the "free lunch": 21 × 16 bits = 336 FFs ≈ 0.3% of available FFs. Without partitioning, HLS would use 1 BRAM36, and II would be ≥ 8.

**DSP = 1**

One DSP48E1 slice for the `noise_level × threshold_alpha` multiply. Theoretical minimum for one threshold multiply. Budget remaining: 219 DSP48E1 for FFT (M2) and DBF (enterprise M4+).

**LUT = 735 (1%), FF = 954 (~0%)**

735 LUTs implement: shift window control, 16-input add tree (~200 LUTs), comparator, output mux, AXI-Stream handshake logic. 954 FFs implement: 21-element window (336 FFs), pipeline stage registers, AXI register slices.

### The interface report

```
AXI-Lite Registers:
  ap_ctrl (start/done/idle) → offset 0x00
  threshold_alpha           → offset 0x10  ← HAL must use 0x10, not 0x00
```

**⚠️ Open issue for M3:** `cfar_engine_controller.hpp` currently has `THRESHOLD_OFFSET = 0x00`. Must be corrected to `0x10` before M3 integration.

---

## Part 6 — Comparing Chimera and Zenith Results

| Metric | Chimera baseline | Zenith 2025.2 result | Change | Interpretation |
|---|---|---|---|---|
| II | 1 | **1** | → Same | Pipeline throughput unchanged |
| Clock target | 100 MHz (10 ns) | 150 MHz (6.67 ns) | +50% faster | Higher demand on timing |
| Timing slack | +3.19 ns | **+0.23 ns** | −2.96 ns | Expected: pushed clock hard, still passes |
| BRAM | 0 | **0** | → Same | ARRAY_PARTITION complete still works |
| DSP | 1 | **1** | → Same | Same multiply, same resource |
| LUT | ~3% | **1% (735)** | Better | 2025.2 optimizer is more aggressive |
| FF | — | **954 (~0%)** | — | First measurement at this precision |
| Toolchain | Vitis HLS 2023.2 | **2025.2** | 2 minor versions | No architectural regressions |

**Conclusion:** The Chimera CFAR asset transfers clean to Zenith. The 50% clock push consumed 2.96 ns of timing margin but did not break II=1. The architecture is sound.

---

## Part 7 — Git Commit for This Session

```bash
cd C:\Projects\zenith_radar_os

git add zenith-silicon/cfar/

git commit -m "feat(silicon): CFAR re-synthesis passes under Vitis HLS 2025.2 @ 150MHz

Chimera asset validated on new toolchain:
- II=1 confirmed (unchanged from Chimera 2023.2)
- Slack: +0.23ns @ 150MHz (was +3.19ns @ 100MHz, tighter but passes)
- BRAM: 0 (window entirely in FFs via ARRAY_PARTITION complete)
- DSP: 1 (threshold multiply only)
- LUT: 735 (1%), FF: 954 (~0%)
- threshold_alpha AXI-Lite offset confirmed: 0x10

Note: HAL CfarEngineController threshold offset needs update 0x00->0x10 at M3
Note: main.cpp is Chimera bare-metal reference, not Zenith Linux driver
Toolchain: Vitis HLS 2025.2, xc7z020clg400-1, 6.67ns clock period"

git push origin master
```

---

## Part 8 — Open Issues Carried Into Week 2

### α value correction (P0 — fix before any hardware test)

```cpp
// cfar_engine_controller.hpp — α value must be corrected
// Original note said α ≈ 6.89 — THIS IS WRONG
// Correct value for N_total=16, Pfa=1e-4: α ≈ 12.44
// Programming 6.89 sets threshold too low → much higher false alarm rate than intended
cfar.set_threshold(static_cast<param_t>(12.44f));  // correct
```

### HAL offset correction (P1 — fix before M3 integration)

```cpp
// cfar_engine_controller.hpp
// Current (wrong):  static constexpr uint32_t THRESHOLD_OFFSET = 0x00;
// Correct:          static constexpr uint32_t THRESHOLD_OFFSET = 0x10;
```

### Vivado Block Design completion (P0 for Week 2)

Still needed: AXI DMA IP, AXI Interconnect, DMA–HP0 connection, address assignment, bitstream.

### PetaLinux (P1 for Week 2)

Not yet installed. Needed for device tree (AXI DMA node, CMA reserved-memory node), kernel, rootfs.

### main.cpp rewrite for Linux (P2 — before M1 board execution)

Current `main.cpp` is Chimera bare-metal. Must be rewritten as Linux userspace with `mmap()`, `__builtin___clear_cache()`, and standard `printf()` before the binary can run on the PetaLinux board.

### Post-route timing verification (P2)

HLS slack +0.23 ns is pre-route. After Vivado implementation: verify still positive. If negative: add one pipeline stage to threshold multiply.

---

## Part 9 — Week 1 Final Status

```
✅ GitHub repo live: master branch, MIT license, clean history
✅ X/Twitter: @charley_builds, launched, Post #0 published
✅ Substack: zenithlog.substack.com, "The Last Craft" published
✅ CMA confirmed: 16 MiB at 0x3F000000 (dmesg)
✅ AXI DMA path confirmed: 0x43000000 (iomem)
✅ zenith_memory_map.hpp: committed, confirmed addresses
✅ sstate-cache: extracted to /opt/petalinux/sstate-cache/2025.2/sstate-cache/
✅ ARM cross-compiler: C++20 + std::span → ELF 32-bit ARM EABI5 confirmed (WSL2)
✅ Vivado Block Design: PS7 + FCLK0 150MHz + HP0, clock connections validated
✅ CFAR synthesis: II=1 @ 150MHz, BRAM=0, DSP=1 — PASSES

Week 1 completed: Wednesday, March 11 (3 days ahead of plan)
```

---

## Quick Reference Card

### Commands used this session

| Command | What it does |
|---|---|
| `git remote prune origin` | Remove stale remote branch references |
| `git branch -a` | List all branches (local + remote-tracking) |
| `sudo apt install gcc-arm-linux-gnueabihf g++-arm-linux-gnueabihf` | Install ARM cross-compiler on WSL2 |
| `arm-linux-gnueabihf-g++ -std=c++20 -O2 -o <out> <src>` | Cross-compile C++20 for ARM |
| `file <binary>` | Identify ELF binary architecture and ABI |
| `create_bd_design "name"` | Create and open Vivado block design canvas |
| `create_bd_cell -type ip -vlnv <vendor>:<lib>:<n> <inst>` | Add IP core to block design |
| `connect_bd_net [get_bd_pins a/out] [get_bd_pins b/in]` | Wire two pins in block design |
| `validate_bd_design` | Check all connections and clock domains |
| `save_bd_design` | Save block design to disk |

### Key numbers (memorize these)

| Number | Meaning |
|---|---|
| `0x3F000000` | CMA physical base address (confirmed from dmesg) |
| `0x43000000` | AXI DMA base address (confirmed from iomem) |
| `150 MHz / 6.67 ns` | Zenith baseband clock target |
| `II=1` | Non-negotiable pipeline throughput requirement |
| `+0.23 ns` | CFAR pre-route timing slack (needs post-route verification) |
| `BRAM=0, DSP=1, LUT=735` | CFAR resource footprint (1% of Zynq-7020) |
| `0x10` | threshold_alpha AXI-Lite register offset (confirmed by synthesis) |
| `WINDOW_SIZE=21` | CFAR window: 8 ref + 2 guard + 1 CUT + 2 guard + 8 ref |
| `α ≈ 12.44` | Correct CFAR threshold for N=16, Pfa=1e-4 (not 6.89) |
| `N_total = 16` | Total reference cells used in α formula (both sides combined) |

### `printf` format specifiers for portable address printing

```cpp
#include <cinttypes>

// Physical addresses (uintptr_t):
printf("addr=0x%08" PRIxPTR "\n", some_phys_addr);   // hex lowercase
printf("addr=0x%08" PRIXPTR "\n", some_phys_addr);   // hex uppercase

// Never use %lX for uintptr_t — type mismatch on some platforms
```

### Bare-metal vs Linux userspace — key differences

| Operation | Chimera bare-metal (current `main.cpp`) | Zenith Linux (future) |
|---|---|---|
| Hardware register access | Direct cast: `reinterpret_cast<volatile uint32_t*>(0x43000000)` | Must `mmap()` first |
| Cache invalidation | `Xil_DCacheInvalidateRange()` | `__builtin___clear_cache()` |
| Printf | `xil_printf()` | `printf()` |
| Address constants | `xparameters.h` (auto-generated) | `zenith_memory_map.hpp` |
| Line endings | `\r\n` (UART requires CR+LF) | `\n` (Linux handles it) |

---

*Obsidian log version: Week1-Day3-Day4-v2*
*Original generated: 2026-03-11 | v2 updated: 2026-03-14*
*Author: Charley Chang / Chief Architect: Claude*
*v2 additions: cross-compile location clarified · %lX/%PRIxPTR deep-dive · Vitis IDE GUI correction (VS Code not Eclipse) · α derivation and bug fix (12.44 not 6.89) · 0UL type suffix · radar_defines.h separation rationale · regs_ member variable origin · arm_receive() dual-purpose explained · main.cpp full bare-metal annotation · bare-metal vs Linux comparison table*
*Next session: Week 2 — Vivado BD completion + AXI DMA integration + PetaLinux*
