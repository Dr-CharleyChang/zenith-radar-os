#pragma once
// =============================================================================
// axi_dma_controller.hpp
// ARM-side driver for the Xilinx AXI DMA IP core (Simple DMA mode, S2MM only)
//
// Register map source: Xilinx PG021 "AXI DMA Product Guide"
// Hardware base address: 0x43000000 (confirmed from /proc/iomem, Day 2)
//
// S2MM = Stream to Memory-Map: PL → DDR direction
// This is the direction that matters for M1: CFAR results flow from FPGA to DDR.
//
// Three-step usage sequence (must follow this order):
//   Step 1: dma.enable()          → set S2MM RS=1, engine starts, TREADY still LOW
//   Step 2: dma.arm_receive()     → write DA + LENGTH, TREADY goes HIGH
//   Step 3: trigger PL to run     → CFAR writes TVALID=1, data flows
//   Step 4: dma.poll_complete()   → busy-wait until S2MM Idle=1
//   Step 5: cache invalidate      → __builtin___clear_cache() before reading
//   Step 6: read results via span → zero copy, no memcpy
//
// Author: Charley Chang | Project Zenith-Radar OS
// Platform: ARM Cortex-A9, Linux userspace, mmap'd /dev/mem
// =============================================================================

#include <cstdint>   // uint32_t, uintptr_t
#include <cstddef>   // size_t
#include <span>      // std::span — zero-copy buffer view

class AxiDmaController {
public:
    // =========================================================================
    // AXI DMA Register Map — S2MM channel (byte offsets, Xilinx PG021 Table 2-9)
    // =========================================================================
    // All offsets are BYTE offsets. When indexing regs_[] (which is uint32_t*),
    // divide by 4. Example: byte offset 0x48 → regs_[0x48/4] = regs_[18].
    //
    // MM2S registers (Memory-Map to Stream, DDR→PL) are at 0x00–0x28.
    // We don't use MM2S for M1 validation, so only S2MM is included here.

    static constexpr uint32_t S2MM_DMACR_OFFSET  = 0x30;
    // S2MM DMA Control Register
    // bit 0: RS (Run/Stop)  — write 1 to start the S2MM engine
    // bit 2: Reset          — write 1 to reset S2MM (clears error states)
    // bit 12: IOC_IrqEn     — enable Interrupt on Complete (used in M4)
    // Writing RS=1 is required before any transfer can occur.
    // TREADY stays LOW after RS=1 — it only goes HIGH after LENGTH is written.

    static constexpr uint32_t S2MM_DMASR_OFFSET  = 0x34;
    // S2MM DMA Status Register (READ ONLY — writes are ignored)
    // bit 0: Halted  — engine is stopped (either never started or error)
    // bit 1: Idle    — 1 = transfer complete, engine waiting for next transfer
    // bit 4: SGIncld — Scatter-Gather mode included (0 = Simple mode, our case)
    // bit 13: IOC_Irq — Interrupt on Complete fired (cleared by writing 1)
    // We poll bit 1 (Idle) to detect transfer completion.

    static constexpr uint32_t S2MM_DA_OFFSET     = 0x48;
    // S2MM Destination Address Register
    // Write the physical DDR address where DMA should place incoming data.
    // Must be written BEFORE S2MM_LENGTH.
    // On 32-bit Zynq-7020: full 32-bit physical address.

    static constexpr uint32_t S2MM_LENGTH_OFFSET = 0x58;
    // S2MM Transfer Length Register — THE KEY REGISTER
    // Writing this register is the "arm" trigger:
    //   Before write: TREADY = 0 (DMA not ready to receive)
    //   After write:  TREADY = 1 (DMA armed, ready to receive)
    // Unit: bytes. Must match exactly the number of bytes CFAR will send.
    // Mismatch causes: premature stop (too small) or timeout (too large).

    // ── Bit masks for the registers above ────────────────────────────────────
    static constexpr uint32_t DMACR_RS      = (1u << 0);  // Run/Stop
    static constexpr uint32_t DMACR_RESET   = (1u << 2);  // Soft reset
    static constexpr uint32_t DMASR_HALTED  = (1u << 0);  // Engine halted
    static constexpr uint32_t DMASR_IDLE    = (1u << 1);  // Transfer complete

    // =========================================================================
    // Constructor
    // =========================================================================
    // Receives the mmap'd pointer to the DMA register space.
    // The caller is responsible for mmap() — this class does not open /dev/mem.
    //
    // Typical usage in main():
    //   void* raw = mmap(nullptr, 0x10000, PROT_READ|PROT_WRITE,
    //                    MAP_SHARED, devmem_fd, AXI_DMA_BASE);
    //   AxiDmaController dma(static_cast<volatile uint32_t*>(raw));
    //
    // Why the constructor does NOT call enable() automatically:
    //   Hardware initialization should be explicit and visible in main().
    //   Hiding a hardware write inside a constructor makes the initialization
    //   sequence invisible and hard to debug. Explicit is better.

    explicit AxiDmaController(volatile uint32_t* base) noexcept
        : regs_(base) {}

    // =========================================================================
    // Step 1: enable()
    // =========================================================================
    // Set the S2MM RS (Run/Stop) bit to start the DMA engine.
    // Must be called ONCE before the first arm_receive().
    // After this call: engine is running, but TREADY is still LOW.
    // TREADY only goes HIGH after arm_receive() writes S2MM_LENGTH.
    //
    // Using |= to set only the RS bit, preserving other control bits
    // (interrupt enables, etc.) that may have been set by prior configuration.

    void enable() noexcept {
        regs_[S2MM_DMACR_OFFSET / 4] |= DMACR_RS;
    }

    // =========================================================================
    // Step 2: arm_receive() — the "arm the DMA" function
    // =========================================================================
    // Configures the DMA destination and arms TREADY.
    // Returns a zero-copy std::span pointing to the same DDR region.
    //
    // [[nodiscard]]: compiler warns if caller discards the return value.
    //   The span is the only way to read the results after transfer.
    //   Discarding it means results are written to DDR but never read.
    //
    // Two independent actions in one call (this design is intentional):
    //   Action 1 — talk to the hardware chip:
    //     regs_[S2MM_DA]     = phys_dst  → "DMA: write data to this address"
    //     regs_[S2MM_LENGTH] = len       → "DMA: expect this many bytes" + TREADY=1
    //   Action 2 — describe the destination to the C++ caller:
    //     return std::span over the same phys_dst address
    //     caller uses this span to read results after poll_complete()
    //
    // Why these two actions belong in one function:
    //   They both reference the same phys_dst. Splitting them would force
    //   the caller to pass phys_dst twice (once to arm, once to get the span),
    //   which creates an opportunity for the two values to diverge.

    [[nodiscard]]
    std::span<const uint8_t> arm_receive(uintptr_t phys_dst,
                                         size_t    len) noexcept {
        // Write destination address (must come BEFORE length)
        regs_[S2MM_DA_OFFSET / 4] = static_cast<uint32_t>(phys_dst);

        // Write transfer length — this is the hardware trigger: TREADY goes HIGH
        regs_[S2MM_LENGTH_OFFSET / 4] = static_cast<uint32_t>(len);

        // Return a zero-copy view. No data has moved yet — this is just
        // a pointer+size describing WHERE the DMA will write its results.
        // The caller reads from this span AFTER poll_complete().
        return std::span<const uint8_t>(
            reinterpret_cast<const uint8_t*>(phys_dst), len);
    }

    // =========================================================================
    // Step 4: poll_complete()
    // =========================================================================
    // Busy-wait until S2MM_DMASR.Idle = 1 (transfer finished).
    //
    // Why busy-wait for M1:
    //   Simplest possible implementation. No interrupt handler, no kernel
    //   driver, no threading. ARM Core 0 spins here until done.
    //   For a 2048-byte transfer at 1200 MB/s: done in ~1.7 microseconds.
    //   Even at 1 MB/s (conservative): done in ~2 milliseconds.
    //   Acceptable for validation runs that happen once per debug session.
    //
    // Why NOT for M4 real-time operation:
    //   The radar loop must process data AND run the Tracker AND publish Zenoh
    //   within one PRI (~1 ms at 1 kHz PRF). Spinning here wastes the entire
    //   CPU budget. M4 replaces this with:
    //   - Enable IOC_IrqEn bit in DMACR
    //   - Register a Linux interrupt handler for the DMA IRQ line
    //   - ARM Core 0 sleeps (sched_yield or futex), woken by IRQ
    //
    // noexcept: this function is called inside the radar loop. Any exception
    //   path would insert unwinding bookkeeping that breaks deterministic timing.

    void poll_complete() const noexcept {
        uint32_t sr;
        do {
            sr = regs_[S2MM_DMASR_OFFSET / 4];
            // Read S2MM_DMASR every iteration.
            // volatile guarantees the compiler re-reads from hardware each time.
            // Without volatile, the compiler might hoist the read out of the loop
            // (read once, cache in a register) → infinite loop even after DMA finishes.
        } while (!(sr & DMASR_IDLE));
        // Loop condition: keep looping while Idle bit is 0 (not done yet)
        // Exit condition: Idle bit is 1 (transfer complete)
    }

    // =========================================================================
    // Diagnostic: status()
    // =========================================================================
    // Returns the raw S2MM_DMASR register value.
    // Useful for debugging: check Halted bit, error bits, interrupt flags.
    // Typical use: if (dma.status() & AxiDmaController::DMASR_HALTED) { error }

    [[nodiscard]]
    uint32_t status() const noexcept {
        return regs_[S2MM_DMASR_OFFSET / 4];
    }

private:
    volatile uint32_t* regs_;
    // Points to the base of the AXI DMA register space (0x43000000).
    // Mapped into the process virtual address space via mmap() in main().
    // volatile: mandatory for memory-mapped hardware registers.
    //   Without it, the compiler may eliminate reads (status polling)
    //   or reorder writes (DA before LENGTH) — both silently corrupt behavior.
    // Trailing underscore: C++ convention for private member variables,
    //   distinguishes from local variables at a glance.
};
