#pragma once
// =============================================================================
// cfar_engine_controller.hpp
// ARM-side AXI-Lite driver for the CFAR HLS IP core
//
// Register map source: Vitis HLS 2025.2 synthesis interface report (Week 1)
// Confirmed offsets:
//   0x00 → ap_ctrl   (AP_START / AP_DONE / AP_IDLE / AP_READY)
//   0x10 → threshold_alpha (CFAR α parameter, ap_fixed<16,8> format)
//
// This file runs on ARM Cortex-A9 (Linux userspace).
// It does NOT use ap_fixed.h — that is a Vitis HLS header for the PL side.
// threshold_alpha is converted from float to the ap_fixed<16,8> bit pattern
// using integer arithmetic (see set_threshold() below).
//
// Author: Charley Chang | Project Zenith-Radar OS
// =============================================================================

#include <cstdint>   // uint32_t
#include <cstddef>   // size_t

class CfarEngineController {
public:
    // =========================================================================
    // AXI-Lite Register Map (byte offsets, confirmed from HLS synthesis report)
    // =========================================================================

    static constexpr uint32_t CTRL_OFFSET      = 0x00;
    // ap_ctrl register — controls kernel execution
    // bit 0: AP_START  — write 1 to trigger one processing run
    // bit 1: AP_DONE   — reads 1 when the current run finishes (auto-cleared)
    // bit 2: AP_IDLE   — reads 1 when kernel is idle (no run in progress)
    // bit 3: AP_READY  — reads 1 when kernel can accept a new AP_START
    //
    // Typical usage:
    //   1. Poll AP_IDLE = 1 (optional, confirms previous run finished)
    //   2. Write AP_START = 1 to trigger
    //   3. Poll AP_DONE = 1 or AP_IDLE = 1 to wait for completion
    //
    // For CFAR in ap_ctrl_hs mode (our synthesis config):
    //   AP_START is self-clearing — write 1, hardware clears it automatically.
    //   Do NOT hold AP_START high while waiting (that would re-trigger).

    static constexpr uint32_t THRESHOLD_OFFSET = 0x10;
    // threshold_alpha register — the CFAR α scaling factor
    // Confirmed offset 0x10 from Vitis HLS 2025.2 interface report (Week 1).
    // Data format: ap_fixed<16,8> — 16-bit fixed-point, Q8.8 (8 integer bits,
    // 8 fractional bits). See set_threshold() for conversion arithmetic.
    //
    // ⚠️ The Chimera HAL had this at 0x00 — that was wrong.
    //    Corrected to 0x10 based on actual synthesis report.

    // ── Bit masks ─────────────────────────────────────────────────────────────
    static constexpr uint32_t AP_START = (1u << 0);
    static constexpr uint32_t AP_DONE  = (1u << 1);
    static constexpr uint32_t AP_IDLE  = (1u << 2);
    static constexpr uint32_t AP_READY = (1u << 3);

    // =========================================================================
    // Constructor
    // =========================================================================
    // Receives the mmap'd pointer to the CFAR AXI-Lite register space.
    //
    // NOTE: The CFAR AXI-Lite base address (CFAR_CTRL_BASE in
    // zenith_memory_map.hpp) is a PLACEHOLDER until the Vivado Block Design
    // address assignment is finalized in Week 2. The address is assigned by
    // Vivado's Address Editor when the CFAR IP is added to the block design.
    // Update zenith_memory_map.hpp after running assign_bd_address in Vivado.

    explicit CfarEngineController(volatile uint32_t* base) noexcept
        : base_(base) {}

    // =========================================================================
    // set_threshold() — write α to the AXI-Lite register
    // =========================================================================
    // Converts a float α value into the ap_fixed<16,8> bit pattern and
    // writes it to the threshold_alpha register.
    //
    // ap_fixed<16,8> format (Q8.8):
    //   Total: 16 bits. Integer part: 8 bits. Fractional part: 8 bits.
    //   Encoding: value × 2^8 = value × 256, stored as a signed 16-bit integer.
    //   Range: −128.0 to +127.996 (step = 1/256 ≈ 0.0039)
    //
    // Example for α = 12.44 (correct value for N=16, Pfa=1e-4):
    //   12.44 × 256 = 3184.64 → round to 3185 → stored as 0x0C71
    //   Actual α implemented in hardware: 3185 / 256 = 12.441 (error < 0.001)
    //
    // The AXI-Lite register is 32 bits wide, but only the lower 16 bits
    // are used. The upper 16 bits are ignored by the HLS kernel.
    //
    // Why NOT use ap_fixed<16,8> directly on the ARM side:
    //   ap_fixed.h is a Vitis HLS header. It is not available in Linux
    //   userspace — it requires the Xilinx HLS compiler environment.
    //   Manual conversion with integer arithmetic is portable and correct.

    void set_threshold(float alpha) noexcept {
        // Convert float to Q8.8 fixed-point bit pattern:
        // Multiply by 2^8 = 256, then truncate to int32_t (preserves sign),
        // then mask to 16 bits for the register write.
        //
        // static_cast<int32_t> first: preserves correct rounding for negative
        // values (though α should always be positive in radar use).
        const auto raw = static_cast<uint32_t>(
            static_cast<int32_t>(alpha * 256.0f) & 0xFFFF
        );
        base_[THRESHOLD_OFFSET / 4] = raw;
    }

    // =========================================================================
    // start() — trigger one CFAR processing run
    // =========================================================================
    // Writes AP_START=1 to the ap_ctrl register.
    // The CFAR kernel begins reading from its AXI-Stream input and
    // producing output on its AXI-Stream output port.
    //
    // [[nodiscard]]: the caller must check the return value.
    //   Returns true if AP_IDLE was high before starting (kernel was ready).
    //   Returns false if kernel was still busy — caller should wait or retry.
    //
    // Calling start() when the kernel is not idle causes undefined behavior:
    //   In ap_ctrl_hs mode, a second AP_START while running is ignored.
    //   But this depends on HLS synthesis options — safer to check first.

    [[nodiscard]] bool start() noexcept {
        if (!(base_[CTRL_OFFSET / 4] & AP_IDLE)) {
            return false;  // kernel not ready — caller must not proceed
        }
        base_[CTRL_OFFSET / 4] = AP_START;
        return true;
    }

    // =========================================================================
    // wait_done() — poll until AP_DONE or AP_IDLE
    // =========================================================================
    // Busy-wait until the CFAR kernel signals completion.
    //
    // We poll for (AP_DONE | AP_IDLE) rather than AP_DONE alone because:
    //   AP_DONE is a single-cycle pulse that can be missed if the polling
    //   loop is slow or interrupted. AP_IDLE is a level signal (stays HIGH
    //   after completion) and is therefore always safe to poll.
    //
    // For M1 validation: acceptable busy-wait, kernel runs fast (~7 μs
    // for 1024 cells at 150 MHz: 1024 cycles × 6.67 ns = 6.83 μs).
    // For M4 real-time: not needed — DMA completion (poll_complete) is the
    // correct synchronization point. The DMA cannot complete before CFAR
    // finishes sending data, so DMA Idle implies CFAR is also done.

    void wait_done() const noexcept {
        uint32_t ctrl;
        do {
            ctrl = base_[CTRL_OFFSET / 4];
        } while (!(ctrl & (AP_DONE | AP_IDLE)));
    }

    // =========================================================================
    // is_idle() — non-blocking status check
    // =========================================================================
    // Returns true if the kernel is idle (not currently processing).
    // Use before start() to confirm the previous run has finished,
    // or during setup to verify the kernel initialized correctly.

    [[nodiscard]] bool is_idle() const noexcept {
        return (base_[CTRL_OFFSET / 4] & AP_IDLE) != 0u;
    }

private:
    volatile uint32_t* base_;
    // Points to the CFAR AXI-Lite register space.
    // Mapped by main() via mmap(). Physical address = CFAR_CTRL_BASE
    // (placeholder in zenith_memory_map.hpp — update after Vivado BD Week 2).
};
