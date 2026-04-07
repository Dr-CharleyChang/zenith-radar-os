// =============================================================================
// main.cpp — Zenith-Core M1 Validation Driver (Linux Userspace)
//
// Platform: ARM Cortex-A9, Linux 4.9.0-xilinx (PetaLinux), userspace
// Purpose:  Drive one complete CFAR detection cycle end-to-end:
//           map hardware → set threshold → arm DMA → trigger CFAR →
//           wait for completion → cache invalidate → read results
//
// This is the LINUX USERSPACE version. It uses mmap(/dev/mem) for hardware
// access. This is fundamentally different from the Chimera bare-metal version
// (main_baremetal_reference.cpp), which used direct physical address casting
// and Xil_DCacheInvalidateRange(). See comparison table in the Obsidian log.
//
// Build command (from WSL2 after cross-compiler is installed):
//   arm-linux-gnueabihf-g++ -std=c++20 -O2 -Wall \
//       -I../include \
//       -o zenith_m1_validate main.cpp
//
// Run on board (after scp to AX7020 via PetaLinux):
//   sudo ./zenith_m1_validate
//   (sudo required: /dev/mem access needs CAP_SYS_RAWIO)
//
// Author: Charley Chang | Project Zenith-Radar OS
// =============================================================================

// ── Standard Linux/POSIX headers ─────────────────────────────────────────────
#include <sys/mman.h>     // mmap(), munmap(), MAP_SHARED, PROT_READ, PROT_WRITE
#include <fcntl.h>        // open(), O_RDWR, O_SYNC
#include <unistd.h>       // close()
#include <cstdio>         // printf() — no heap allocation, unlike std::cout
#include <cstdint>        // uint32_t, uintptr_t
#include <cstddef>        // size_t
#include <cinttypes>      // PRIxPTR — portable printf format for uintptr_t
#include <span>           // std::span — C++20 zero-copy buffer view
#include <cerrno>         // errno, perror()

// ── Zenith project headers ────────────────────────────────────────────────────
#include "zenith_memory_map.hpp"          // Physical addresses (CMA, DMA, CFAR)
#include "hal/axi_dma_controller.hpp"     // AxiDmaController
#include "hal/cfar_engine_controller.hpp" // CfarEngineController
#include "radar_defines.h"                // FRAME_SIZE, REF_CELLS, etc.

// ── ap_fixed<16,8> conversion constant ───────────────────────────────────────
// α = 12.44 for N_total=16 reference cells, Pfa=1e-4
// Derivation: α = N × (Pfa^(-1/N) - 1) = 16 × (10^(4/16) - 1) ≈ 12.44
// NOTE: 6.89 in earlier notes was a calculation error (used N=8 instead of 16)
static constexpr float CFAR_ALPHA = 12.44f;

// ── Transfer size ─────────────────────────────────────────────────────────────
// One CFAR output frame = FRAME_SIZE cells × 2 bytes per cell (ap_fixed<16,8>)
// = 1024 × 2 = 2048 bytes
// This MUST match what the CFAR kernel will actually send (TLAST after 1024 cells)
static constexpr size_t TRANSFER_BYTES = FRAME_SIZE * sizeof(int16_t);

// =============================================================================
// Helper: map a physical address region into process virtual space
// =============================================================================
// Wraps mmap() to reduce boilerplate in main().
// Returns nullptr on failure (caller must check).
//
// phys_base: physical start address (from zenith_memory_map.hpp)
// size:      bytes to map (must be page-multiple, 4KB minimum)
//
// Why O_SYNC on devmem_fd:
//   Without O_SYNC, the kernel may buffer register writes. For hardware
//   registers, every write must reach the AXI bus immediately and in order.
//   O_SYNC forces synchronous writes — no buffering, no reordering.

static void* map_phys(int devmem_fd, uintptr_t phys_base, size_t size) noexcept {
    void* p = mmap(nullptr,               // let kernel choose virtual address
                   size,                  // bytes to map
                   PROT_READ | PROT_WRITE,// ARM can read and write
                   MAP_SHARED,            // writes go directly to physical memory
                   devmem_fd,             // /dev/mem file descriptor
                   static_cast<off_t>(phys_base));  // physical start address
    if (p == MAP_FAILED) {
        return nullptr;
    }
    return p;
}

// =============================================================================
// main()
// =============================================================================

int main() {
    printf("=== Zenith M1 CFAR Validation ===\n");
    printf("FRAME_SIZE=%d, TRANSFER_BYTES=%zu, alpha=%.3f\n",
           FRAME_SIZE, TRANSFER_BYTES, CFAR_ALPHA);

    // =========================================================================
    // Step 0: Open /dev/mem
    // =========================================================================
    // /dev/mem exposes the entire physical address space as a file.
    // Reading/writing byte offset N = reading/writing physical address N.
    //
    // O_RDWR:  need both read (poll status registers) and write (send commands)
    // O_SYNC:  bypass kernel write buffering — every write reaches hardware now
    //
    // Requires root (sudo) or CAP_SYS_RAWIO capability.
    // On PetaLinux factory image: root login works directly.
    //
    // NOTE on CONFIG_STRICT_DEVMEM:
    //   Some kernels have CONFIG_STRICT_DEVMEM=y which blocks access to
    //   non-RAM physical regions via /dev/mem. Linux 4.9.0-xilinx (AX7020
    //   factory kernel) does NOT have this restriction — confirmed Day 2.

    const int devmem_fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (devmem_fd < 0) {
        perror("open /dev/mem failed");
        printf("Are you running as root? Try: sudo ./zenith_m1_validate\n");
        return 1;
    }
    printf("[OK] /dev/mem opened (fd=%d)\n", devmem_fd);

    // =========================================================================
    // Step 1: Map CMA region into process virtual space
    // =========================================================================
    // Map the entire 16MB CMA block in one call.
    // After this, cma_base[0] corresponds to physical address CMA_PHYS_BASE.
    //
    // CMA_PHYS_BASE = 0x3F000000 (confirmed from dmesg Day 2)
    // CMA_TOTAL_SIZE = 16MB
    //
    // Why map the whole CMA at once (not just RX_SIZE):
    //   1. One mmap() call is cheaper than multiple
    //   2. We may need TX and Track regions in future milestones
    //   3. The span we return from arm_receive() points into this mapping

    void* cma_virt = map_phys(devmem_fd, CMA_PHYS_BASE, CMA_TOTAL_SIZE);
    if (!cma_virt) {
        perror("mmap CMA region failed");
        close(devmem_fd);
        return 1;
    }
    printf("[OK] CMA mapped: phys=0x%08" PRIxPTR " virt=%p size=%zuMB\n",
           CMA_PHYS_BASE, cma_virt, CMA_TOTAL_SIZE / (1024 * 1024));

    // =========================================================================
    // Step 2: Map AXI DMA control registers
    // =========================================================================
    // AXI_DMA_BASE = 0x43000000 (confirmed from /proc/iomem Day 2)
    // 0x10000 = 64KB mapping — conservative, actual registers end at ~0x60

    void* dma_raw = map_phys(devmem_fd, AXI_DMA_BASE, 0x10000);
    if (!dma_raw) {
        perror("mmap AXI DMA registers failed");
        munmap(cma_virt, CMA_TOTAL_SIZE);
        close(devmem_fd);
        return 1;
    }
    printf("[OK] AXI DMA mapped: phys=0x%08" PRIxPTR " virt=%p\n",
           AXI_DMA_BASE, dma_raw);

    // =========================================================================
    // Step 3: Map CFAR AXI-Lite control registers
    // =========================================================================
    // ⚠️ CFAR_CTRL_BASE is a PLACEHOLDER in zenith_memory_map.hpp.
    // The real address is assigned by Vivado's Address Editor when the CFAR
    // IP is added to the Block Design (Week 2 task).
    // Update zenith_memory_map.hpp after running:
    //   assign_bd_address  (in Vivado Tcl) or using Address Editor GUI
    //
    // Typical Vivado assignment for a single HLS IP on GP0:
    //   Base address: 0x43C00000 or 0x43C10000 (Vivado auto-assigns)
    //   Size: 64KB (0x10000)

    void* cfar_raw = map_phys(devmem_fd, CFAR_CTRL_BASE, 0x10000);
    if (!cfar_raw) {
        perror("mmap CFAR registers failed — is CFAR_CTRL_BASE correct?");
        printf("  Current CFAR_CTRL_BASE = 0x%08" PRIxPTR "\n", CFAR_CTRL_BASE);
        printf("  Update zenith_memory_map.hpp after Vivado BD address assignment\n");
        munmap(dma_raw, 0x10000);
        munmap(cma_virt, CMA_TOTAL_SIZE);
        close(devmem_fd);
        return 1;
    }
    printf("[OK] CFAR ctrl mapped: phys=0x%08" PRIxPTR " virt=%p\n",
           CFAR_CTRL_BASE, cfar_raw);

    // =========================================================================
    // Step 4: Construct HAL objects
    // =========================================================================
    // HAL objects are stack-allocated (no heap, zero-heap principle).
    // They receive the mmap'd pointers and store them as regs_ / base_.
    // No hardware writes happen here — initialization is explicit below.

    AxiDmaController     dma (static_cast<volatile uint32_t*>(dma_raw));
    CfarEngineController cfar(static_cast<volatile uint32_t*>(cfar_raw));

    // =========================================================================
    // Step 5: Verify CFAR kernel is idle before starting
    // =========================================================================
    // On a freshly loaded bitstream, AP_IDLE should be 1.
    // If it is 0, the kernel may have been left in a running state by a
    // previous program run, or the bitstream did not load correctly.

    if (!cfar.is_idle()) {
        printf("[WARN] CFAR kernel is not idle at startup.\n");
        printf("       Check that bitstream is loaded and CFAR is not stuck.\n");
        printf("       AP_CTRL register = 0x%08X\n",
               *static_cast<volatile uint32_t*>(cfar_raw));
        // Don't abort — proceed anyway. The wait_done() call below will catch issues.
    } else {
        printf("[OK] CFAR kernel idle and ready\n");
    }

    // =========================================================================
    // Step 6: Write threshold α
    // =========================================================================
    // Must be done before start(). The register persists until overwritten —
    // set it once per program run (or every run for safety).
    //
    // set_threshold() converts float α to ap_fixed<16,8> bit pattern:
    //   12.44 × 256 = 3184.64 → 3185 → written to register as 0x00000C71
    //
    // Path: ARM → M_AXI_GP0 → AXI Interconnect → CFAR AXI-Lite slave → reg[0x10]

    cfar.set_threshold(CFAR_ALPHA);
    printf("[OK] Threshold written: alpha=%.3f → raw=0x%04X\n",
           CFAR_ALPHA,
           static_cast<uint32_t>(static_cast<int32_t>(CFAR_ALPHA * 256.0f) & 0xFFFF));

    // =========================================================================
    // Step 7: Enable DMA S2MM engine
    // =========================================================================
    // Sets S2MM_DMACR.RS = 1: engine starts running.
    // TREADY stays LOW after this — it only goes HIGH after arm_receive().
    //
    // Why enable() is separate from arm_receive():
    //   RS=1 can be set once at initialization and left set for multiple
    //   transfers. arm_receive() re-arms TREADY for each new transfer by
    //   writing S2MM_LENGTH. Keeping these separate makes the sequence explicit.

    dma.enable();
    printf("[OK] DMA S2MM engine enabled\n");

    // =========================================================================
    // Step 8: Arm the DMA
    // =========================================================================
    // Writes S2MM_DA = RX_PHYS_BASE → DMA destination address
    // Writes S2MM_LENGTH = 2048    → TREADY goes HIGH here
    //
    // After this line: DMA is waiting. The moment CFAR sends its first sample
    // (TVALID=1), data starts flowing into DDR at RX_PHYS_BASE.
    //
    // result_span is a zero-copy view over RX_PHYS_BASE.
    // We will read from it in Step 11, AFTER poll_complete() confirms the
    // transfer is done and AFTER cache invalidation.

    auto result_span = dma.arm_receive(RX_PHYS_BASE, TRANSFER_BYTES);
    printf("[OK] DMA armed: dst=0x%08" PRIxPTR " len=%zu bytes, TREADY=HIGH\n",
           RX_PHYS_BASE, TRANSFER_BYTES);

    // =========================================================================
    // Step 9: Trigger CFAR processing
    // =========================================================================
    // Writes AP_START=1 to CFAR AXI-Lite offset 0x00.
    // CFAR begins reading from its AXI-Stream input and producing output.
    // TVALID goes HIGH as CFAR starts sending results.
    // Since TREADY is already HIGH (Step 8), data transfers immediately.
    //
    // [[nodiscard]] on start() means the compiler warns if we ignore the bool.
    // Check it: false means CFAR was busy (should not happen after is_idle()).

    if (!cfar.start()) {
        printf("[ERROR] cfar.start() returned false — kernel not idle\n");
        printf("        Aborting. Check AP_CTRL: 0x%08X\n",
               *static_cast<volatile uint32_t*>(cfar_raw));
        munmap(cfar_raw, 0x10000);
        munmap(dma_raw, 0x10000);
        munmap(cma_virt, CMA_TOTAL_SIZE);
        close(devmem_fd);
        return 1;
    }
    printf("[OK] CFAR started — TVALID will go HIGH, data flowing\n");

    // =========================================================================
    // Step 10: Wait for DMA transfer completion
    // =========================================================================
    // Busy-wait until S2MM_DMASR.Idle = 1.
    //
    // DMA Idle = 1 guarantees:
    //   ✓ All TRANSFER_BYTES have been written to DDR at RX_PHYS_BASE
    //   ✓ TLAST was received (CFAR sent a complete frame)
    //   ✓ CFAR has finished processing (it sent all data before DMA can idle)
    //
    // Therefore: DMA Idle = 1 implies CFAR is also done.
    // We do NOT need a separate cfar.wait_done() call here —
    // the DMA completion is the correct synchronization point.
    // (cfar.wait_done() is available for debugging, not needed in normal flow.)

    dma.poll_complete();
    printf("[OK] DMA transfer complete — %zu bytes in DDR at 0x%08" PRIxPTR "\n",
           TRANSFER_BYTES, RX_PHYS_BASE);

    // =========================================================================
    // Step 11: Cache invalidate — MANDATORY before reading DMA results
    // =========================================================================
    // The DMA wrote to DDR via AXI-HP, which BYPASSES the ARM SCU.
    // The ARM's L1/L2 cache may still hold stale data from before the transfer.
    // Without this invalidation, reading result_span returns cached old data.
    //
    // __builtin___clear_cache(start, end):
    //   GCC/Clang built-in that issues the ARM "DMB ISH" (Data Memory Barrier,
    //   Inner Shareable domain) instruction and cache line invalidation.
    //   The three underscores before "clear_cache" are correct — historical GCC naming.
    //   start and end are char* (the built-in requires this type).
    //
    // Why the range exactly matches TRANSFER_BYTES:
    //   Invalidating more than necessary wastes time (each cache line ~10 ns).
    //   Invalidating less would leave some results stale — silent data corruption.
    //
    // If we used ACP port instead of HP: this call is not needed (SCU auto-syncs).
    // Zenith uses HP for bandwidth reasons — so this line is mandatory.

    auto* cache_start = reinterpret_cast<char*>(RX_PHYS_BASE);
    auto* cache_end   = cache_start + TRANSFER_BYTES;
    __builtin___clear_cache(cache_start, cache_end);
    printf("[OK] Cache invalidated: [0x%08" PRIxPTR ", 0x%08" PRIxPTR ")\n",
           RX_PHYS_BASE, RX_PHYS_BASE + TRANSFER_BYTES);

    // =========================================================================
    // Step 12: Read and print CFAR detections — zero copy
    // =========================================================================
    // result_span.data() points to RX_PHYS_BASE.
    // Reinterpreting as int16_t* because CFAR outputs ap_fixed<16,8>
    // which has the same bit layout as a signed 16-bit integer.
    //
    // CFAR output semantics:
    //   cell == 0       → no target in this range bin
    //   cell != 0       → target detected; value ≈ cell power above threshold
    //                     (proportional to SNR, not calibrated range/velocity yet)
    //
    // No memcpy — we read directly from the DDR address the DMA wrote to.
    // This is the end of the zero-copy chain:
    //   PL → AXI-HP → DDR → cache_invalidate → std::span → direct read

    const auto* detections = reinterpret_cast<const int16_t*>(result_span.data());
    int detection_count = 0;

    printf("\n--- CFAR Detection Results (%d range cells) ---\n", FRAME_SIZE);
    for (int i = 0; i < FRAME_SIZE; ++i) {
        if (detections[i] != 0) {
            printf("  Range cell %4d: value = %d  (SNR proxy ≈ %.2f dB)\n",
                   i,
                   static_cast<int>(detections[i]),
                   20.0f * __builtin_log10f(
                       static_cast<float>(detections[i]) / 128.0f + 1.0f));
            ++detection_count;
        }
    }

    if (detection_count == 0) {
        printf("  No detections. Expected for M1 loopback test with no target signal.\n");
        printf("  If CFAR is running on synthetic data, verify input was injected.\n");
    } else {
        printf("  Total detections: %d\n", detection_count);
    }

    // =========================================================================
    // Cleanup — unmap all regions, close file descriptor
    // =========================================================================
    // munmap() releases the virtual address mappings.
    // Physical hardware (DDR, DMA registers, CFAR registers) is unaffected.
    // close() releases the /dev/mem file descriptor.
    //
    // In a real radar loop that runs forever, these calls are never reached —
    // the process owns the mappings for its lifetime and the OS reclaims them
    // at process exit. They are included here for a clean validation run.

    munmap(cfar_raw,  0x10000);
    munmap(dma_raw,   0x10000);
    munmap(cma_virt,  CMA_TOTAL_SIZE);
    close(devmem_fd);

    printf("\n[OK] Cleanup done. Zenith M1 validation complete.\n");
    return 0;
}
