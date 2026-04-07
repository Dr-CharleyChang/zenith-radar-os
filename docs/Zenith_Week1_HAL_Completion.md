---
tags: [Zenith, Week1, HAL, DMA, CFAR, CPP20, ARM, Linux, ZeroCopy, TutorialTextbook]
date: 2026-03-14
author: Charley Chang
status: COMPLETE
milestone: M1-Foundation HAL completion
version: v1
---

# Project Zenith — Week 1 HAL Completion: Missing Functions Added

> **Purpose of this note:** Three files were incomplete at the end of Week 1: `axi_dma_controller.hpp` had no `poll_complete()`, `cfar_engine_controller.hpp` had no `set_threshold()` or `wait_done()`, and `main.cpp` was a bare-metal Chimera reference not a Linux userspace driver. This session completes all three. Every function is annotated at the variable and register level.

---

## What Was Missing and Why

At the end of Week 1, the HAL layer was a **skeleton** — enough to document the architecture but not enough to actually run hardware. The gap was identified during note review:

```
cfar.start()        ✅ existed  (writes AP_START bit)
cfar.set_threshold() ❌ missing  (need to write α before start)
cfar.wait_done()     ❌ missing  (need to confirm kernel finished)

dma.arm_receive()    ✅ existed  (writes S2MM_DA + S2MM_LENGTH)
dma.enable()         ❌ missing  (S2MM RS=1 must happen first)
dma.poll_complete()  ❌ missing  (the entire "wait for transfer" step)

main.cpp             ❌ wrong    (was Chimera bare-metal, not Linux userspace)
```

The note also referred to `dma.poll_complete()` and `cfar.start()` as if they both existed. Only `start()` was real. This session adds everything missing and writes the correct Linux `main.cpp`.

---

## Design Decisions Before Writing Code

Three decisions were made explicitly before writing a single line.

**Decision 1: `enable()` is separate from `arm_receive()`**

`enable()` sets the S2MM RS (Run/Stop) bit once. `arm_receive()` writes DA + LENGTH for each transfer. In theory these could be one function. They are kept separate because RS=1 is an engine-level initialization (done once) while DA+LENGTH is a per-transfer setup (done every frame). Merging them would either re-write RS=1 every frame (unnecessary bus traffic) or require a flag to track whether the engine is already running (hidden state). Explicit is cleaner.

**Decision 2: `poll_complete()` is on the DMA, not on CFAR**

There are two things that could signal "done": DMA Idle (transfer complete) and CFAR AP_DONE (kernel computation complete). We poll DMA Idle, not CFAR AP_DONE. The reason: DMA cannot go Idle until it has received all the bytes, and CFAR cannot have sent all the bytes until it finished computing them. DMA Idle logically implies CFAR done. Polling DMA also avoids the one-clock-pulse problem with AP_DONE (it can be missed if the poll loop is slow).

**Decision 3: `set_threshold()` takes `float`, not `ap_fixed`**

`ap_fixed.h` is a Vitis HLS header. It does not exist in Linux userspace or in the ARM cross-compiler's include path. The conversion from float to the ap_fixed<16,8> bit pattern is simple integer arithmetic: `value × 256`, cast to int16_t. This is both portable and correct. No Xilinx headers needed on the ARM side.

---

## File 1: `hal/axi_dma_controller.hpp` — Complete

### Full register map (S2MM channel only)

| Byte Offset | Register | Key Bits | Description |
|---|---|---|---|
| `0x30` | `S2MM_DMACR` | bit0: RS | Write 1 to start engine. TREADY stays LOW. |
| `0x34` | `S2MM_DMASR` | bit0: Halted, bit1: Idle | Status. Poll bit1 for completion. |
| `0x48` | `S2MM_DA` | [31:0] | Destination physical address in DDR. |
| `0x58` | `S2MM_LENGTH` | [25:0] | Transfer length in bytes. **Writing this triggers TREADY=1.** |

### Function summary

**`enable()`**

```cpp
void enable() noexcept {
    regs_[S2MM_DMACR_OFFSET / 4] |= DMACR_RS;
}
```

Sets S2MM_DMACR bit0 = RS = 1. The `|=` preserves any other control bits (interrupt enable, etc.) that may have been configured. After this: engine is running, TREADY is still LOW. TREADY only goes HIGH after `arm_receive()` writes LENGTH.

**`arm_receive()`** — unchanged from Week 1, kept for reference

```cpp
[[nodiscard]]
std::span<const uint8_t> arm_receive(uintptr_t phys_dst, size_t len) noexcept {
    regs_[S2MM_DA_OFFSET / 4]     = static_cast<uint32_t>(phys_dst);
    regs_[S2MM_LENGTH_OFFSET / 4] = static_cast<uint32_t>(len);  // TREADY=1 here
    return std::span<const uint8_t>(
        reinterpret_cast<const uint8_t*>(phys_dst), len);
}
```

Two independent actions in one call: tells the DMA chip where to write (via `regs_`) and gives the caller a window to read from (via the return span). Both reference the same `phys_dst`. See Day 3-4 notes for the full design explanation.

**`poll_complete()`**

```cpp
void poll_complete() const noexcept {
    uint32_t sr;
    do {
        sr = regs_[S2MM_DMASR_OFFSET / 4];
    } while (!(sr & DMASR_IDLE));
}
```

Reads S2MM_DMASR (0x34) in a busy loop until bit1 (Idle) = 1. The `volatile` on `regs_`  is what makes this work correctly: it forces the compiler to re-read the register every iteration. Without `volatile`, the compiler could hoist the read outside the loop (since `sr` appears to never change from the C++ optimizer's perspective), producing an infinite loop even after the DMA finishes.

**When to replace this with interrupts (M4):**
Enable `S2MM_DMACR.IOC_IrqEn` (bit 12), register a Linux IRQ handler for the DMA interrupt line, and put the CPU to sleep with `sched_yield()` or a `futex` wait. The IRQ wakes it when done. This frees Core 0 to run the Tracker and Zenoh during the transfer instead of spinning.

**`status()`**

```cpp
[[nodiscard]]
uint32_t status() const noexcept {
    return regs_[S2MM_DMASR_OFFSET / 4];
}
```

Returns the raw status register for diagnostics. Useful during bringup to check the Halted bit (engine not started or error condition).

---

## File 2: `hal/cfar_engine_controller.hpp` — Complete

### Full register map (confirmed from Vitis HLS 2025.2 synthesis report)

| Byte Offset | Register | Bit Fields | Description |
|---|---|---|---|
| `0x00` | `ap_ctrl` | bit0: AP_START, bit1: AP_DONE, bit2: AP_IDLE, bit3: AP_READY | Kernel execution control |
| `0x10` | `threshold_alpha` | [15:0] ap_fixed<16,8> | CFAR threshold factor α |

**⚠️ Original Chimera HAL had THRESHOLD_OFFSET = 0x00 — wrong. Corrected to 0x10.**

### ap_fixed<16,8> bit format — the conversion explained

`ap_fixed<16,8>` is a Q8.8 fixed-point number:
- 16 bits total
- 8 bits for the integer part
- 8 bits for the fractional part
- Encoding: the stored integer = actual value × 2⁸ = actual value × 256

```
Example: α = 12.44
  12.44 × 256 = 3184.64
  Round to nearest integer: 3185
  Hex: 0x0C71
  Actual α in hardware: 3185 / 256 = 12.4414...  (error < 0.01%)

In the register (32-bit wide, only lower 16 used):
  Written value: 0x00000C71
```

The ARM-side conversion:

```cpp
void set_threshold(float alpha) noexcept {
    const auto raw = static_cast<uint32_t>(
        static_cast<int32_t>(alpha * 256.0f) & 0xFFFF
    );
    base_[THRESHOLD_OFFSET / 4] = raw;
}
```

`static_cast<int32_t>` first: correctly handles negative values (shouldn't occur for α, but correct is correct). `& 0xFFFF`: masks to 16 bits, preventing any overflow from propagating into the upper 16 bits of the 32-bit register write.

### Function summary

**`set_threshold(float alpha)`** — write α before `start()`

```cpp
void set_threshold(float alpha) noexcept {
    const auto raw = static_cast<uint32_t>(
        static_cast<int32_t>(alpha * 256.0f) & 0xFFFF
    );
    base_[THRESHOLD_OFFSET / 4] = raw;
}
```

**`start()`** — trigger one processing run, with idle check

```cpp
[[nodiscard]] bool start() noexcept {
    if (!(base_[CTRL_OFFSET / 4] & AP_IDLE)) {
        return false;  // kernel busy — caller must not proceed
    }
    base_[CTRL_OFFSET / 4] = AP_START;
    return true;
}
```

Added an idle check that was missing in the original. `[[nodiscard]]` forces the caller to handle the `false` case (kernel was busy). The Week 1 version just wrote AP_START unconditionally and always returned true — that would silently fail if triggered when the kernel was mid-run.

**`wait_done()`** — poll until AP_DONE or AP_IDLE

```cpp
void wait_done() const noexcept {
    uint32_t ctrl;
    do {
        ctrl = base_[CTRL_OFFSET / 4];
    } while (!(ctrl & (AP_DONE | AP_IDLE)));
}
```

Polls for either AP_DONE (one-cycle pulse when kernel finishes) or AP_IDLE (level signal, stays high after AP_DONE clears). Using both avoids the race condition where AP_DONE is cleared by hardware before the polling loop samples it.

**Note on when to use `wait_done()` vs `dma.poll_complete()`:**

In `main.cpp`, only `dma.poll_complete()` is called. This is correct: DMA Idle implies CFAR done (CFAR sent all data before DMA can idle). `cfar.wait_done()` is available for debugging (e.g., to measure how long CFAR takes independently of DMA).

**`is_idle()`** — non-blocking check

```cpp
[[nodiscard]] bool is_idle() const noexcept {
    return (base_[CTRL_OFFSET / 4] & AP_IDLE) != 0u;
}
```

Used in `main()` before `start()` to verify the kernel initialized correctly. Also useful for monitoring kernel state during debugging without stalling the CPU.

---

## File 3: `main.cpp` — Linux Userspace, Complete

### Bare-metal Chimera vs Linux Zenith — the full comparison

This table explains every difference between the old `main.cpp` (Chimera bare-metal) and the new one:

| | Chimera `main.cpp` (old, reference only) | Zenith `main.cpp` (new, Linux userspace) |
|---|---|---|
| **OS** | None — bare-metal | Linux 4.9.0-xilinx |
| **Register access** | `reinterpret_cast<volatile uint32_t*>(0x43000000)` works directly | Must call `mmap()` via `/dev/mem` first |
| **Cache invalidation** | `Xil_DCacheInvalidateRange()` from `xil_cache.h` | `__builtin___clear_cache()` — GCC built-in |
| **Printf** | `xil_printf()` from `xil_printf.h` | `printf()` from glibc |
| **Address constants** | `xparameters.h` (auto-generated by Vitis SDK) | `zenith_memory_map.hpp` (hand-written, ours) |
| **Line endings** | `\r\n` (UART needs explicit CR) | `\n` (Linux TTY inserts CR) |
| **Program exit** | Triggers system reset or infinite loop | Clean `return 0`, OS reclaims resources |
| **Privileges** | Owns the hardware directly | Requires `sudo` for `/dev/mem` access |

### The complete hardware access sequence in `main()`

```
open("/dev/mem", O_RDWR|O_SYNC)
   ↓
mmap(CMA_PHYS_BASE, 16MB)    → cma_virt
mmap(AXI_DMA_BASE, 64KB)     → dma_raw
mmap(CFAR_CTRL_BASE, 64KB)   → cfar_raw   ← address TBD until Vivado BD done
   ↓
AxiDmaController dma(dma_raw)
CfarEngineController cfar(cfar_raw)
   ↓
cfar.is_idle()               → verify kernel ready
cfar.set_threshold(12.44f)   → write α=12.44 to reg[0x10]
   ↓
dma.enable()                 → S2MM_DMACR RS=1, engine running, TREADY=LOW
   ↓
result_span = dma.arm_receive(RX_PHYS_BASE, 2048)
                             → S2MM_DA = 0x3F400000
                             → S2MM_LENGTH = 2048
                             → TREADY = HIGH  ← this is the "arm" moment
   ↓
cfar.start()                 → AP_START=1, CFAR begins, TVALID=HIGH
                             → since TREADY=HIGH, data flows immediately
   ↓
dma.poll_complete()          → busy-wait until S2MM_DMASR.Idle=1
                             → 2048 bytes written to DDR at 0x3F400000
   ↓
__builtin___clear_cache(...)  → invalidate ARM cache over RX region (HP port bypass)
   ↓
read result_span             → zero copy, direct from DDR
   ↓
munmap × 3, close()
```

### CFAR_CTRL_BASE — the placeholder

The CFAR AXI-Lite base address is not yet in `zenith_memory_map.hpp` because the Vivado Block Design address assignment is incomplete (Week 2 task). The placeholder needs to be updated after running the following in Vivado Tcl:

```tcl
# After adding CFAR IP to the block design:
assign_bd_address
# Then check the assigned address:
get_bd_addr_segs
# Update zenith_memory_map.hpp with the result
```

Typical Vivado auto-assignment for a single HLS IP on M_AXI_GP0: `0x43C00000`.

### α = 12.44 — the corrected value

Week 1 notes said α ≈ 6.89. This was wrong (used N=8 instead of N_total=16 in the formula). The correct derivation:

```
N_total = 2 × REF_CELLS = 16
Pfa     = 1e-4

α = N × (Pfa^(-1/N) - 1)
  = 16 × (10^(4/16) - 1)
  = 16 × (10^0.25 - 1)
  = 16 × 0.778
  ≈ 12.44
```

Programming 6.89 into the hardware would set the threshold too low — approximately 4× more false alarms than intended.

---

## Git Commit for This Session

```bash
cd C:\Projects\zenith_radar_os

git add zenith-core/src/hal/axi_dma_controller.hpp
git add zenith-core/src/hal/cfar_engine_controller.hpp
git add zenith-core/src/main.cpp

git commit -m "feat(core): complete HAL layer and Linux userspace main.cpp

axi_dma_controller.hpp:
- Add enable(): set S2MM RS=1 (required before arm_receive)
- Add poll_complete(): busy-wait on S2MM_DMASR.Idle bit1
- Add status(): raw DMASR read for diagnostics
- Add full S2MM register map with bit-level comments
- Existing arm_receive() unchanged

cfar_engine_controller.hpp:
- Add set_threshold(float): converts float α to ap_fixed<16,8> Q8.8
- Fix start(): add AP_IDLE pre-check, was unconditional before
- Add wait_done(): poll AP_DONE | AP_IDLE (debug use)
- Add is_idle(): non-blocking status check
- Fix THRESHOLD_OFFSET: was 0x00, corrected to 0x10 (synthesis report)

main.cpp: rewrite as Linux userspace driver (was Chimera bare-metal)
- Uses mmap(/dev/mem) for all hardware access
- __builtin___clear_cache() for HP port cache invalidation
- Full 6-step DMA sequence: enable→arm→trigger→poll→invalidate→read
- CFAR_CTRL_BASE placeholder: update after Vivado BD Week 2
- alpha=12.44 (corrected from erroneous 6.89 in earlier notes)

Note: Chimera bare-metal original preserved as main_baremetal_reference.cpp"

git push origin master
```

---

## Open Issues After This Session

```
⏳ P0: CFAR_CTRL_BASE — add real address to zenith_memory_map.hpp
        (after Vivado Block Design address assignment, Week 2)

⏳ P0: Vivado Block Design — add AXI DMA IP, AXI Interconnect,
        wire DMA S2MM to HP0, generate bitstream

⏳ P1: PetaLinux — install 2025.2, add device tree nodes
        (AXI DMA, CMA reserved-memory)

⏳ P2: Compile and scp to board for first actual execution
        arm-linux-gnueabihf-g++ -std=c++20 -O2 ...

⏳ P2: Replace poll_complete() with interrupt-driven at M4
        Enable S2MM_DMACR.IOC_IrqEn, register Linux IRQ handler

⏳ P2: Post-route timing — verify CFAR slack +0.23ns holds after Vivado impl
```

---

## Quick Reference: Complete Six-Step DMA Sequence

```cpp
// Step 1: Start S2MM engine (once per program)
dma.enable();

// Step 2: Arm DMA + get zero-copy result window
auto result_span = dma.arm_receive(RX_PHYS_BASE, TRANSFER_BYTES);
//                                   ↑ TREADY goes HIGH here

// Step 3: Trigger CFAR (TVALID goes HIGH, data flows)
if (!cfar.start()) { /* handle error */ }

// Step 4: Wait for transfer completion
dma.poll_complete();   // S2MM_DMASR.Idle = 1

// Step 5: Cache invalidate (mandatory for HP port)
__builtin___clear_cache(
    reinterpret_cast<char*>(RX_PHYS_BASE),
    reinterpret_cast<char*>(RX_PHYS_BASE + TRANSFER_BYTES));

// Step 6: Read results — zero copy, no memcpy
const auto* det = reinterpret_cast<const int16_t*>(result_span.data());
for (int i = 0; i < FRAME_SIZE; ++i) {
    if (det[i] != 0) { /* detection at range cell i */ }
}
```

---

*Obsidian log: Week1-HAL-Completion-v1*
*Date: 2026-03-14*
*Author: Charley Chang / Chief Architect: Claude*
*Next: Week 2 — Vivado BD completion → CFAR_CTRL_BASE confirmed → first board execution*
