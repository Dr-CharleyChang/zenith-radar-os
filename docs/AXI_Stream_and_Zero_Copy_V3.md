---
tags: [AXI4-Stream, ZeroCopy, DMA, CPP20, ARM, OS, Zenoh]
for_ai: true
version: V3.0
purpose: >
  This document defines the PS-side (ARM Cortex-A9) OS architecture and the
  PS/PL communication protocol for Project Zenith-Radar OS. AI must follow
  these rules when generating any C++20 ARM-side code, DMA driver code,
  or network publishing code. Violations cause non-deterministic timing jitter
  or silent data corruption.
---

# Zenith-Radar OS: AXI4-Stream Protocol & Zero-Copy OS Architecture

**AI Instruction:** This document governs ALL PS-side C++20 code generation.
The zero-heap rule and AXI4-Stream signal semantics are hard constraints,
not style guidelines. Flag any user request that would violate them.

---

## 0. Foundational Concepts Glossary

Definitions that are prerequisites for understanding the rest of this document.

**MM2S vs S2MM — DMA Direction Convention**
Names are from the DMA engine's perspective, describing how data moves relative
to the memory bus:

| Abbreviation | Full Name | Data Direction | Zenith Usage |
|---|---|---|---|
| MM2S | Memory-Map to Stream | PS (DDR) → PL | ARM sends waveform config / TX buffer to PL HLS kernels |
| S2MM | Stream to Memory-Map | PL → PS (DDR) | PL writes processed IQ / point cloud back to DDR for ARM to read |

DDR is controlled by the PS-side DDR controller. PL accesses DDR indirectly
via the AXI HP or ACP interconnect ports — it borrows the PS's memory bus.

**"Arm the DMA"**
Means: configure and enable the DMA receive channel *before* triggering PL
processing, so that TREADY=1 is asserted when the PL starts sending data.
The correct sequence is always:
```
1. ARM writes S2MM destination address + byte count to DMA registers
2. ARM sets DMA S2MM run bit → TREADY goes HIGH
3. ARM triggers PL to start processing (e.g., write to HLS control register)
4. PL asserts TVALID; DMA accepts data (both TVALID and TREADY are HIGH)
5. ARM polls DMA SR.Idle or waits for interrupt
```
If step 1-2 are skipped, the PL asserts TVALID into TREADY=0, the AXI-Stream
stalls, and the entire pipeline deadlocks.

**STL (Standard Template Library)**
The core of the C++ standard library. Includes all containers (`std::vector`,
`std::map`, `std::list`), algorithms (`std::sort`), smart pointers
(`std::shared_ptr`), and strings (`std::string`). All STL containers that
can grow dynamically use heap allocation internally.

**CMA (Contiguous Memory Allocator)**
A Linux kernel mechanism that reserves a physically contiguous DDR region at
boot time for DMA use. Regular `malloc` returns virtually contiguous but
physically fragmented memory — DMA hardware only understands physical addresses
and requires contiguous physical pages for bulk transfers. CMA solves this by
pre-reserving a block (16 MB in Zenith) before the OS starts fragmenting RAM.
Configured via the device tree `reserved-memory` node.

**AXI HP Port (High Performance)**
A direct 64-bit path from PL to the DDR controller, bypassing the ARM CPU
caches entirely. Supports up to ~1200 MB/s throughput. Because it bypasses
the cache, the ARM's L1/L2 caches may hold stale data after the PL writes
to DDR — the ARM driver must explicitly invalidate the cache before reading.
Zynq-7020 has 4 HP ports (HP0–HP3).

**AXI ACP Port (Accelerator Coherency Port)**
Connects PL to the ARM Snoop Control Unit (SCU). The SCU automatically
maintains cache coherency: when PL writes via ACP, the SCU invalidates
the corresponding ARM cache lines so the ARM always reads fresh data.
Eliminates the need for manual cache invalidation, but has lower bandwidth
and higher latency than HP due to SCU arbitration overhead.

```
Decision tree — which port to use:
  Large block DMA (IQ data, RD Maps, >64KB)?  →  Use HP port
    → Must call cache_invalidate() before ARM reads
  Small shared config (<4KB), frequent CPU access?  →  Consider ACP
    → No cache invalidation needed, but lower throughput
  Control registers (PRF, gain, mode)?  →  Use AXI-Lite GP port
    → Not for data, only for register-mapped configuration
```

**Zenith default: HP port for all data DMA. AXI-Lite GP for control.**

---

## 1. AXI4-Stream Protocol — The PS/PL Data Boundary

All baseband data (IQ samples, range-Doppler maps, point clouds, track states)
crossing the PL↔PS boundary must strictly follow AXI4-Stream semantics.

### 1.1 TVALID / TREADY Handshake

```
Master (PL)          Slave (PS/DMA)
   │                       │
   │── TVALID=1 ──────────►│   Master has valid data
   │◄─────────── TREADY=1 ─│   Slave is ready to accept
   │                       │
   │  Transfer occurs ONLY when BOTH signals are HIGH on rising clock edge
   │  If TREADY=0, the master MUST hold TVALID and data stable (stall/backpressure)
```

**Rule 1.1:** Generated HLS code must never assume TREADY is always 1.
Backpressure is mandatory. Use `hls::stream` which automatically models
TVALID/TREADY — never use raw arrays as stream substitutes in DATAFLOW regions.

**Rule 1.2:** ARM must arm the DMA S2MM channel (see §0 "Arm the DMA") before
triggering PL processing. If S2MM is not armed, TREADY=0 when the PL sends
its first sample, causing a pipeline stall that only clears when the DMA
is eventually armed — leading to corrupted packet boundaries (TLAST misaligned).

### 1.2 TLAST — Packet Boundary Semantics

TLAST defines the radar timing hierarchy. Its semantics are strictly defined
per processing stage:

| Stage | TLAST Meaning | C++ HLS Rule |
|---|---|---|
| DDS / ADC input | End of one chirp / pulse (N_range samples) | Assert after `N_range`-th sample |
| 1D-FFT output | End of one range profile | Pass through from input TLAST |
| Corner Turn output | End of one CPI row written | Assert after `N_chirps`-th write |
| 2D-FFT output | End of one Range-Doppler Map | Assert after last Doppler bin |
| CFAR output | End of one detection frame | Assert after last CUT processed |

**AI Rule:** When generating HLS pipeline stages, always explicitly manage
TLAST propagation. Do not leave it unconnected or tied to a constant.

### 1.3 TUSER — Sideband Metadata

Used to carry radar metadata without interrupting the data stream.

```cpp
// Zenith standard TUSER field definition (8-bit)
// [7:4] reserved
// [3:2] RX channel ID (0-3 for up to 4-channel DBF)
// [1]   ADC saturation flag
// [0]   chirp index LSB (toggles each chirp for sync verification)
struct ZenithTUser {
    ap_uint<8> raw;
    ap_uint<2> rx_channel()    { return raw(3,2); }
    ap_uint<1> adc_saturated() { return raw[1]; }
    ap_uint<1> chirp_lsb()     { return raw[0]; }
};
```

---

## 2. Zero-Heap Memory Doctrine — The ARM Real-Time Boundary

### 2.1 Why Dynamic Allocation Is Banned

The ARM radar loop must complete within a fixed time budget every PRI
(Pulse Repetition Interval). Heap allocation (`new`, `malloc`) has
non-deterministic latency — it may take 50 ns in the best case, or
several microseconds if the heap is fragmented. This timing jitter
propagates into the Doppler processing chain and corrupts phase coherence
across pulses, directly degrading velocity estimation accuracy.

### 2.2 The Prohibition Table

| Banned Operation | Root Cause | Compliant Alternative |
|---|---|---|
| `new` / `delete` | Heap fragmentation, non-deterministic latency | `std::array`, static pool |
| `malloc` / `free` | Same as above | Pre-allocated `.bss` buffer |
| `std::vector::push_back` | May trigger internal reallocation + memcpy | Fixed-size `std::array<T, N>` |
| `std::vector::resize` / `reserve` | Heap reallocation if called in loop | Pre-size once at boot, never again |
| `std::string` construction | Short String Optimization (SSO) fails for long strings → heap | `std::array<char, N>` |
| `std::shared_ptr` creation | Allocates a hidden "control block" on the heap for the reference counter | Raw pointer into static pool, or index into pool array |
| `std::map` / `std::list` insert | Each node is a separate heap allocation | `std::array` with manual index, or fixed-size hash map |

**Note on `std::shared_ptr`:** It is a useful ownership tool in non-realtime
contexts (initialization, teardown). The ban applies only to the *active radar
loop* — creating `shared_ptr` during system startup is acceptable.

**AI Rule:** When generating ARM C++20 code for any function that runs inside
the radar loop (DMA callbacks, RRM scheduler, Tracker update), scan for all
heap operations and replace with static alternatives. Add a comment
`// ZENITH: zero-heap verified` when the function is clean.

### 2.3 Pre-allocation Pattern

All pools must be declared at file scope or as `static` class members so they
reside in the `.bss` segment — allocated by the OS loader at program start,
with fixed physical addresses for the lifetime of the process.

**Why file scope / static is required:**
- Physical address is fixed → DMA descriptors can reference them safely
- OS will not page them out (unlike heap memory which can be swapped)
- Zero runtime allocation cost — memory exists before `main()` runs

```cpp
// zenith/core/static_pools.hpp

constexpr size_t MAX_TRACKS     = 32;
constexpr size_t MAX_DETECTIONS = 256;
constexpr size_t DMA_BD_RING    = 32;  // Buffer Descriptor ring, must be power of 2

// BD Ring: a circular queue of DMA transfer descriptors.
// The DMA hardware walks this ring autonomously in Scatter-Gather mode,
// processing one BD per transfer without CPU intervention.
// Fixed size = fixed physical address = safe for DMA hardware access.

// Static pools — live in .bss, zero runtime cost
static std::array<TrackState,    MAX_TRACKS>     g_track_pool;
static std::array<Detection,     MAX_DETECTIONS> g_detection_pool;
static std::array<DmaDescriptor, DMA_BD_RING>    g_bd_ring;

// Pool allocator: return reference to pre-existing slot, never construct new
TrackState& alloc_track(size_t id) noexcept {
    // Caller is responsible for bounds checking before calling this
    return g_track_pool[id];
}
```

---

## 3. Zero-Copy DMA via C++20 `std::span`

### 3.1 Physical Memory Architecture

```
DDR Physical Memory Layout (CMA Reserved Region)
─────────────────────────────────────────────────────────────────
  ⚠️  IMPORTANT: These addresses are EXAMPLES ONLY.
  They MUST match your Vivado Block Design AXI address map
  AND your Linux device tree reserved-memory node.
  Define them once in zenith_memory_map.hpp and include everywhere.
─────────────────────────────────────────────────────────────────

┌─────────────────────────────────────────────────────────────┐
│ 0x1000_0000  TxBuffer  4MB   (PS→PL: config / waveform)     │
│ 0x1040_0000  RxBuffer  4MB   (PL→PS: IQ / point cloud)      │
│ 0x1080_0000  BD Ring   4KB   (Scatter-Gather descriptors)   │
│ 0x1080_1000  TrackBuf  1MB   (Zenoh zero-copy output)       │
└─────────────────────────────────────────────────────────────┘
        │  AXI-HP (MM2S)                ▲  AXI-HP (S2MM)
        ▼                               │
   [PL: DDS / HLS kernels] ────────────►│

Three-way consistency required:
  1. Linux device tree  →  reserved-memory reg = <phys_base total_size>
  2. Vivado Block Design →  HP port address range covers this region
  3. C++ zenith_memory_map.hpp →  PHYS_BASE, TX_OFFSET, RX_OFFSET constants
```

### 3.2 Single Source of Truth: Memory Map Header

```cpp
// zenith/common/zenith_memory_map.hpp
// ⚠️ Edit this file when changing Vivado address map. Nowhere else.

constexpr uintptr_t CMA_PHYS_BASE  = 0x1000'0000;
constexpr size_t    CMA_TOTAL_SIZE  = 0x0100'0000;  // 16 MB

constexpr size_t    TX_OFFSET       = 0x0000'0000;  // 4 MB: PS→PL waveform
constexpr size_t    TX_SIZE         = 0x0040'0000;

constexpr size_t    RX_OFFSET       = 0x0040'0000;  // 4 MB: PL→PS IQ data
constexpr size_t    RX_SIZE         = 0x0040'0000;

constexpr size_t    BD_OFFSET       = 0x0080'0000;  // 4 KB: DMA descriptors
constexpr size_t    BD_SIZE         = 0x0000'1000;

constexpr size_t    TRACK_OFFSET    = 0x0080'1000;  // 1 MB: Zenoh output
constexpr size_t    TRACK_SIZE      = 0x0010'0000;
```

### 3.3 C++20 std::span Zero-Copy Pattern

```cpp
// zenith/transport/dma_engine.cpp
#include "zenith_memory_map.hpp"
#include <sys/mman.h>
#include <span>

class DmaEngine {
public:
    // Called ONCE at boot — maps CMA physical region into process virtual space
    void init() noexcept {
        fd_ = open("/dev/mem", O_RDWR | O_SYNC);
        void* virt = mmap(nullptr, CMA_TOTAL_SIZE,
                          PROT_READ | PROT_WRITE, MAP_SHARED,
                          fd_, CMA_PHYS_BASE);
        auto* base = static_cast<uint8_t*>(virt);
        rx_view_   = std::span<uint8_t>(base + RX_OFFSET,    RX_SIZE);
        track_view_= std::span<uint8_t>(base + TRACK_OFFSET, TRACK_SIZE);
    }

    // Zero-copy view of RX buffer — no memcpy, just a pointer+size wrapper
    // [[nodiscard]]: compiler warns if caller discards the return value
    [[nodiscard]]
    std::span<const PointCloud> get_detections() const noexcept {
        // MANDATORY for HP port: invalidate ARM L2 cache before reading DDR
        // The PL wrote fresh data to DDR via HP, bypassing the cache.
        // Without this, ARM reads stale cached values, not the PL's output.
        invalidate_cache(rx_view_);
        return std::span<const PointCloud>(
            reinterpret_cast<const PointCloud*>(rx_view_.data()),
            rx_view_.size() / sizeof(PointCloud));
    }

private:
    int fd_{-1};
    std::span<uint8_t> rx_view_;
    std::span<uint8_t> track_view_;

    void invalidate_cache(std::span<uint8_t> region) const noexcept {
        // ARM Cortex-A9 cache invalidation
        // TODO: remove this call if switching to ACP port (hardware coherent)
        __builtin___clear_cache(region.data(),
                                region.data() + region.size());
    }
};
```

### 3.4 Cache Coherency: HP vs ACP Summary

| | HP Port | ACP Port |
|---|---|---|
| Cache coherency | ❌ Manual — call `cache_invalidate()` before ARM read | ✅ Automatic — SCU handles it |
| Max bandwidth | ~1200 MB/s | ~600 MB/s |
| Latency | Low | Slightly higher (SCU arbitration) |
| Best for | Large DMA blocks (IQ, RD Maps) | Small frequently-shared buffers |
| Zenith default | ✅ Yes | Only if coherency overhead is unacceptable |

---

## 4. Modern C++ Contracts

### 4.1 `[[nodiscard]]` — Silent Failure Prevention

When a function returns a status code or a data view, the caller *must* use
the return value. Without `[[nodiscard]]`, a caller can accidentally write:
```cpp
engine.transmit(offset, len);  // return value ignored — DMA error silently lost
```
With `[[nodiscard]]`, the compiler emits a warning for this pattern, forcing
the caller to handle it:
```cpp
[[nodiscard]] DmaError transmit(uint32_t offset, uint32_t len) noexcept;

// Correct usage — must capture and check:
if (auto err = engine.transmit(offset, len); err != DmaError::Ok) { ... }
```

**AI Rule:** Every function returning `DmaError`, `bool` status, or
`std::span` in Zenith code MUST be marked `[[nodiscard]]`.

### 4.2 `noexcept` — Deterministic Execution Time

**Why exceptions break real-time code:**
When a C++ function can throw, the compiler inserts hidden "stack unwinding"
bookkeeping code at function entry/exit. This code doesn't execute unless
an exception is thrown, but its *presence* prevents key compiler optimizations
(inlining, register allocation, instruction scheduling), making the function's
execution time slightly longer and — more importantly — variable.

In the radar loop, every function must complete within a fixed time budget
every PRI. Even a few hundred nanoseconds of jitter in `tracker_predict()`
accumulates across pulses and corrupts the phase coherence needed for
accurate Doppler velocity estimation.

`noexcept` tells the compiler: "this function will never throw." The compiler
responds by removing all unwinding bookkeeping, enabling full optimization,
and producing machine code with a deterministic instruction count.

```cpp
// These functions are called every PRI — must be noexcept
void tracker_predict(TrackState& t, float dt) noexcept;
void cfar_update(CfarWindow& w, int16_t new_sample) noexcept;
DmaError dma_poll_complete(uint32_t timeout_us) noexcept;

// Pattern for wrapping a throwing library call before the hot path:
DmaError DmaEngine::safe_init() noexcept {
    try {
        some_library_that_throws();  // exception handled HERE, outside hot path
        return DmaError::Ok;
    } catch (...) {
        return DmaError::HwFault;    // convert to error code, never propagate
    }
}
// Once safe_init() returns, the hot path only deals with DmaError codes — no exceptions
```

**AI Rule:** Any function called inside the RRM loop, Tracker update, or DMA
callback MUST be `noexcept`. If it calls something that can throw, wrap it
with try-catch at the boundary and convert to an error code before entering
the hot path.

---

## 5. Type Safety for Zero-Copy: Trivial Types and `static_assert`

### 5.1 Why TrackState Must Be Trivially Copyable

**Trivially copyable** means: safe to copy with `memcpy`. The requirements are:
- No user-defined copy constructor or destructor
- No virtual functions
- All members are themselves trivially copyable

Types like `int`, `float`, and plain C structs are trivially copyable.
`std::string`, `std::vector`, `std::shared_ptr` are NOT — they contain
internal pointers to heap memory. A `memcpy` of a `std::string` produces
two objects pointing to the same heap buffer; when both are destroyed,
the heap is freed twice (double-free crash).

Zenoh's zero-copy API treats your struct as raw bytes (equivalent to `memcpy`
semantics). If `TrackState` contains a `std::string` member, Zenoh will
transmit garbage pointer values across the network, not the string content.

### 5.2 Why Padding Must Be Eliminated

The compiler inserts silent padding bytes between struct members to satisfy
CPU alignment requirements:

```cpp
struct Padded {
    char  flag;   // 1 byte
                  // 3 bytes padding inserted silently by compiler
    float value;  // 4 bytes
};
// sizeof(Padded) == 8, not 5
// The 3 padding bytes contain undefined (garbage) values
```

When this struct is transmitted over Zenoh, the receiver gets 3 bytes of
garbage in the middle of every `TrackState`. This can cause deserialization
errors or security issues. Fix by reordering members (largest to smallest)
or using `__attribute__((packed))` (with caution — may hurt cache performance).

### 5.3 `static_assert` — Compile-Time Verification

`static_assert` evaluates a condition at compile time. If false, compilation
fails with your error message — no runtime cost, catches bugs before any code runs.

```cpp
// zenith/common/types.hpp

struct Detection {
    float    range_m;        // [m]
    float    velocity_mps;   // [m/s], positive = approaching
    float    azimuth_deg;    // [deg], 0 = broadside
    float    snr_db;         // [dB]
    uint8_t  waveform_id;
    uint8_t  flags;          // bit0: velocity ambiguous, bit1: range ambiguous
    uint16_t reserved;       // explicit padding — value set to 0 at init
};

// Verified at compile time — if either assertion fails, build breaks immediately
// instead of discovering data corruption at runtime during a field test
static_assert(sizeof(Detection) == 20,
    "Detection has unexpected padding — reorder members or add explicit padding");
static_assert(std::is_trivially_copyable_v<Detection>,
    "Detection must be trivially copyable for Zenoh zero-copy API");

struct TrackState {
    float    x, y;           // position [m]
    float    vx, vy;         // velocity [m/s]
    float    covariance[16]; // Kalman covariance matrix (4x4)
    uint32_t track_id;
    uint8_t  status;         // 0=tentative, 1=confirmed, 2=lost
    uint8_t  age;            // frames since last update
    uint16_t reserved;
};
static_assert(sizeof(TrackState) == 80);
static_assert(std::is_trivially_copyable_v<TrackState>);
```

---

## 6. Zenoh Zero-Copy Network Publishing

The zero-copy pipeline extends from PL silicon all the way to the Ethernet MAC.
No serialization, no intermediate copies — the Ethernet DMA reads directly
from the static pool buffer.

```cpp
// zenith/core/network_publisher.cpp
#include <zenoh.h>
#include <span>

class ZenithPublisher {
public:
    void publish_tracks(std::span<const TrackState> tracks) noexcept {
        // static_assert above guarantees TrackState is safe for this call
        z_publisher_put_owned(
            publisher_,
            z_bytes_from_static_buf(
                reinterpret_cast<const uint8_t*>(tracks.data()),
                tracks.size_bytes()),
            nullptr);
    }
private:
    z_owned_publisher_t publisher_;
};
```

**Full zero-copy chain:**
```
PL (HLS kernel)
  → writes IQ/detections to DDR via AXI-HP (S2MM)
  → ARM reads via std::span (cache invalidated, no memcpy)
  → Tracker updates g_track_pool in-place (static pool, no allocation)
  → Zenoh reads g_track_pool via std::span (no memcpy)
  → Ethernet MAC DMA reads from same physical buffer
  → Data exits on the wire
Total memcpy operations: 0
```
