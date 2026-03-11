---
tags: [FPGA, HLS, Zynq7020, Constraints, Pragma, CDC]
for_ai: true
version: V3.0
purpose: >
  This document is a strict hardware constraint reference for AI code generation
  in Project Zenith-Radar OS. When generating any Vitis HLS C++ code or Vivado
  Tcl scripts, ALL rules in this document are non-negotiable. Violating these
  constraints will cause synthesis failure or non-deterministic hardware behavior.
---

# Zenith-Radar OS: Zynq-7020 Hardware Constraints & Vitis HLS Directives

**AI Instruction:** Before generating any HLS kernel or Vivado constraint, verify
compliance with every section below. If a user request conflicts with these rules,
flag the conflict explicitly before proceeding.

---

## 0. Foundational Concepts Glossary

Definitions required to understand the constraint rules that follow.

**Vitis HLS (High-Level Synthesis)**
A Xilinx tool that compiles C++ code into FPGA hardware (RTL). It does not
execute C++ on a CPU — it *synthesizes* the logic gates and flip-flops that
implement the C++ computation in silicon. The output is an IP core that plugs
into Vivado Block Design. Pragmas (`#pragma HLS ...`) are directives to the
synthesis tool, not runtime instructions.

**II (Initiation Interval)**
The number of clock cycles between accepting two consecutive input samples.
`II=1` means the pipeline accepts a new sample every clock cycle — maximum
throughput. `II=2` means every other cycle is wasted — 50% throughput loss.
For a 150 MHz clock with II=1, throughput = 150 million samples/second.
II is the single most important performance metric in HLS design.

**Pipeline vs Loop Unrolling**
- `#pragma HLS PIPELINE`: the loop body executes in an overlapping (pipelined)
  fashion — iteration N+1 starts before iteration N finishes. Achieves II=1.
- `#pragma HLS UNROLL factor=N`: physically replicates the loop body N times
  in hardware so N iterations run simultaneously. Increases parallelism but
  multiplies resource consumption by N.

**BRAM (Block RAM)**
Dedicated on-chip memory blocks in the FPGA fabric. Each BRAM36 block stores
36 Kb (4.5 KB) and has exactly 2 independent read/write ports. Unlike registers
(FF), BRAMs are large but slow to infer — HLS may choose LUTRAM instead if not
explicitly directed. Zynq-7020 has 140 BRAM36 blocks = 630 KB total on-chip.

**DSP48E1 Slice**
A hardened arithmetic block in Zynq-7020 containing a pre-adder (25-bit),
multiplier (25×18 bit), and accumulator (48-bit). Native equation:
`P = (A ± D) × B + C`. One DSP48E1 can replace ~200 LUTs for a multiply-add
operation. Zynq-7020 has 220 DSP48E1 slices — a scarce resource.

**LUT (Look-Up Table)**
The fundamental logic cell of the FPGA. Each LUT implements a 6-input boolean
function. Used for everything not covered by BRAM or DSP48: control logic,
muxes, small arithmetic, the OS-CFAR sorting network. 53,200 available.

**FF (Flip-Flop)**
One-bit register. Used for pipeline stage registers, state machines, and
AXI handshake signals. 106,400 available (2 FFs per LUT slice).

**Corner Turn**
The matrix transpose operation between 1D-FFT (range processing) and 2D-FFT
(Doppler processing). 1D-FFT writes data row-by-row (one chirp per row).
2D-FFT must read column-by-column (one range bin across all chirps). Corner
Turn rearranges the memory layout so the 2D-FFT input is sequential.
It is the primary BRAM bottleneck in radar designs because the entire
CPI matrix (N_chirps × N_range × IQ bytes) must fit on-chip simultaneously.

**CDC (Clock Domain Crossing)**
When a signal generated in one clock domain is read in a different clock domain.
If not handled correctly, the receiving flip-flop may sample the signal mid-
transition, producing a metastable output — a voltage between 0 and 1 that
can propagate unpredictably through the logic, causing silent data corruption
or system hangs. All CDC boundaries must use synchronizers or async FIFOs.

**Metastability**
The condition where a flip-flop's output is neither a valid 0 nor a valid 1.
Occurs when setup/hold time is violated — typically at CDC boundaries.
Probability of metastability propagating to failure decreases exponentially
with time, which is why 2-stage synchronizers (giving the signal two clock
cycles to resolve) are the standard fix for single-bit CDC signals.

**AXI-Lite vs AXI-Stream**
- **AXI-Lite**: register-mapped configuration bus. Used for slow control
  signals (set gain, set threshold, start/stop). The ARM writes a value to
  an address; the FPGA reads it as a register. Not suitable for streaming data.
- **AXI-Stream**: unidirectional data flow with TVALID/TREADY handshake.
  No addresses — data flows continuously like a pipe. Used for all baseband
  IQ data. These two interfaces typically run on different clocks, making
  every `s_axilite` → `axis` signal path a CDC boundary.

**FCLK (Fabric Clock)**
The PS generates up to 4 fabric clocks (FCLK0–FCLK3) that drive the PL.
Frequency is programmable via PS configuration registers. Zenith uses:
- FCLK0 → 150 MHz: AXI-Stream baseband data path, all HLS kernels
- FCLK1 → 100 MHz: AXI-Lite control registers (optional, can share FCLK0)

**SRL (Shift Register LUT)**
A special FPGA primitive where a LUT is configured as a shift register (up to
32 bits deep) instead of logic. Used by HLS to implement delay lines and FIFOs
efficiently. Async resets prevent SRL inference because SRLs don't support
async reset — forcing the tool to use individual FFs instead, consuming more
resources.

---

## 1. Zynq-7020 (XC7Z020) Silicon Hard Limits

These are absolute ceilings. Generated code must include resource estimates
in comments.

| Resource | Hard Limit | Primary Consumer in Zenith | Safe Budget (≤70%) |
|---|---|---|---|
| DSP48E1 Slices | 220 | FFT butterflies, FIR filters, DBF MAC | ≤ 154 |
| Block RAM 36K | 140 (= 280 × 18K) | Corner Turn buffer, CFAR line buffer, DDS LUT | ≤ 98 |
| LUT | 53,200 | OS-CFAR sorting network, AXI interconnect glue | ≤ 37,240 |
| FF | 106,400 | Pipeline registers, CDC synchronizers | ≤ 74,480 |
| Target PL Clock | 150 MHz (hard floor) | All AXI-Stream datapaths | — |

### 1.1 Corner Turn BRAM Budget Formula

Before implementing Corner Turn, verify it fits within the BRAM budget:

$$\text{BRAM}_{CT} = \left\lceil \frac{N_{range} \times N_{chirps} \times 4\ \text{bytes}}{4608\ \text{bytes}} \right\rceil$$

Where 4 bytes = 2× int16 (I + Q), 4608 bytes = one BRAM36 block usable capacity.

| N_range | N_chirps | Raw size | BRAM36 needed | Within budget? |
|---|---|---|---|---|
| 256 | 64 | 128 KB | 29 | ✅ Yes |
| 256 | 128 | 256 KB | 57 | ✅ Yes |
| 512 | 128 | 512 KB | 114 | ⚠️ Exceeds safe budget (98) |
| 1024 | 256 | 2 MB | 456 | ❌ Impossible on Zynq-7020 |

**AI Rule:** Before generating Corner Turn code, compute BRAM requirement
and warn if it exceeds 98. For large configurations, suggest splitting the
CPI across multiple passes or reducing N_chirps.

### 1.2 Resource Estimate Comment Template

Add this block at the top of every generated HLS kernel:

```cpp
// ─── RESOURCE ESTIMATE ────────────────────────────────────────────────────
// Kernel: fft_1d
// DSP48E1: ~18  (9 butterfly stages × 2 complex MACs)
// BRAM36:  ~4   (twiddle factor LUT 1024×32bit = 4KB)
// LUT:     ~300 (control logic, mux)
// II:      1    (PIPELINE applied to inner loop)
// Clock:   150 MHz target
// ─────────────────────────────────────────────────────────────────────────
```

**AI Rule:** Every generated kernel must have this comment block. If exact
numbers are unknown, provide estimates with ± range and flag as unverified.

---

## 2. BRAM True Dual-Port (TDP) Doctrine — The #1 Cause of II > 1

**Physical Reality:** One BRAM36 block has exactly **two** independent ports.
Maximum throughput = 2 accesses per clock cycle (any R/W combination).

### 2.1 The Port Collision Problem

When HLS unrolls a loop by factor N, the loop body's N iterations must all
execute in the same clock cycle. If they all access the same array, they need
N ports simultaneously — but BRAM only has 2. The HLS scheduler cannot
achieve II=1 and will report:

```
WARNING: [HLS 200-880] The II Violation in module 'fft_top' (loop 'VITIS_LOOP'):
  Unable to schedule 'load' operation on array 'corner_turn_buf' ...
  The resource limit of core 'RAM_2P_BRAM' is 2.
```

### 2.2 ARRAY_PARTITION — The Required Fix

```cpp
// WRONG — single BRAM block, port collision inevitable after UNROLL
int16_t corner_turn_buf[N_CHIRPS][N_RANGE];

// CORRECT — split across 4 BRAMs using cyclic partition
// cyclic factor=4: elements 0,4,8,... → BRAM_0
//                  elements 1,5,9,... → BRAM_1
//                  elements 2,6,10,... → BRAM_2
//                  elements 3,7,11,... → BRAM_3
// Now 8 ports available (4 BRAMs × 2 ports), supports unroll factor ≤ 8
int16_t corner_turn_buf[N_CHIRPS][N_RANGE];
#pragma HLS ARRAY_PARTITION variable=corner_turn_buf cyclic factor=4 dim=2
```

**Partition strategy selection guide:**

| Strategy | Access Pattern | Best For | Resource Cost |
|---|---|---|---|
| `cyclic factor=N` | Interleaved (0,1,2,3,0,1,2,...) | CFAR sliding window, sequential scan | N × BRAM |
| `block factor=N` | Grouped (0..N/4-1, N/4..N/2-1,...) | Column access, Corner Turn | N × BRAM |
| `complete` | All elements in parallel | Small LUTs ≤ 64 elements | N × registers (FF) |

**AI Rule:** Any array accessed inside a `PIPELINE II=1` region with an
`UNROLL` factor > 1 MUST have an explicit `ARRAY_PARTITION`. Never rely on
HLS auto-inference — it is conservative and will choose II > 1.

---

## 3. DSP48E1 Exploitation — Math-to-Silicon Mapping

### 3.1 Native DSP48E1 Equation

$$P = (A \pm D) \times B + C$$

| Port | Width | Role |
|---|---|---|
| A | 30-bit | Primary input (or pre-adder sum output) |
| D | 27-bit | Pre-adder second input |
| B | 18-bit | Multiplier coefficient |
| C | 48-bit | Accumulator addend |
| P | 48-bit | Pipeline output |

The pre-adder (`A ± D`) is a free hardware resource inside the DSP slice.
Using it costs zero additional LUTs and zero timing penalty.

### 3.2 C++ Inference Rules

**Rule 3.1 — Symmetric FIR: exploit the pre-adder**

```cpp
// WRONG — two separate multiplications, 2 DSP48 slices used
acc += coeff[i] * x[n - i] + coeff[i] * x[n - (N-1-i)];

// CORRECT — add symmetric taps BEFORE multiply → pre-adder inferred
// Only 1 DSP48 slice used, same arithmetic result
acc += coeff[i] * (x[n - i] + x[n - (N-1-i)]);
```

**Rule 3.2 — Synchronous reset only**

```cpp
// WRONG — async reset: prevents SRL inference and DSP register packing
// The HLS tool sees rst_n as a control signal independent of clock,
// forcing every register to use a dedicated FF with async clear.
if (!rst_n) acc = 0;
else        acc += new_sample * coeff;

// CORRECT — sync reset: inside the clocked datapath
// HLS can pack acc into the DSP48 internal P register
if (rst)  acc = 0;
else      acc += new_sample * coeff;
```

**Why this matters:** Async resets prevent the HLS tool from using DSP48
internal pipeline registers (P-register chaining). The tool falls back to
external FFs, adding ~2 cycles of latency and consuming ~15-30% more LUTs.

**Rule 3.3 — Complex multiply: 3 DSP48 minimum (not 4)**

A complex multiply $(a + jb)(c + jd) = (ac - bd) + j(ad + bc)$ naively
requires 4 real multiplies. Using the Karatsuba identity reduces to 3:

```cpp
// 3-multiply Karatsuba for complex multiply
// k1 = a*c,  k2 = b*d,  k3 = (a+b)*(c+d)
// real = k1 - k2,  imag = k3 - k1 - k2
ap_int<32> k1 = a * c;
ap_int<32> k2 = b * d;
ap_int<32> k3 = (a + b) * (c + d);
real_out = k1 - k2;
imag_out = k3 - k1 - k2;
// Cost: 3 DSP48 instead of 4 — saves 25% DSP budget for FFT butterflies
```

**AI Rule:** Always use Karatsuba for complex multiplications in FFT
butterfly code. With 220 DSP48E1 total and FFT consuming most of them,
every saved slice matters.

---

## 4. Mandatory HLS Pragmas — The C++20 Hardware Contract

These pragmas are instructions to the HLS synthesis tool, not to a CPU.
They define timing, parallelism, and memory contracts that are enforced
at synthesis time. Missing pragmas produce functionally correct but
timing-violating or resource-wasteful hardware.

### 4.1 `#pragma HLS INTERFACE` — Port Protocol Definition

**This is the most fundamental pragma.** It defines how each C++ function
argument maps to a hardware interface signal.

```cpp
void dds_top(
    const DdsConfig& cfg,         // control: ARM writes config registers
    hls::stream<ap_axis<32,1,1,1>>& iq_out  // data: AXI-Stream output
) {
#pragma HLS INTERFACE s_axilite port=cfg    bundle=CTRL
// ↑ cfg becomes AXI-Lite slave registers — ARM can write f_carrier etc.
// bundle=CTRL groups all s_axilite ports into one AXI-Lite interface

#pragma HLS INTERFACE axis      port=iq_out
// ↑ iq_out becomes an AXI-Stream master port with TVALID/TREADY/TLAST

#pragma HLS INTERFACE s_axilite port=return bundle=CTRL
// ↑ return value (ap_ctrl_hs) exposes start/done/idle control signals
//   via the same AXI-Lite interface — needed for ARM to start the kernel
```

**Common interface types:**

| Pragma | Hardware Port Generated | Use For |
|---|---|---|
| `s_axilite` | AXI-Lite slave registers | Scalar config (threshold, frequency) |
| `axis` | AXI-Stream port (TVALID/TREADY/TDATA/TLAST) | Streaming IQ data |
| `m_axi` | AXI4 master (burst read/write) | Direct DDR access from PL |
| `ap_none` | Bare wire, no protocol | Internal signals, not recommended for top-level |
| `ap_ctrl_hs` | Start/done/idle handshake | Kernel control (auto-added to return) |

**AI Rule:** Every top-level HLS function argument MUST have an explicit
`INTERFACE` pragma. Never rely on default inference — defaults change between
Vitis HLS versions and produce unpredictable port protocols.

### 4.2 `#pragma HLS PIPELINE II=1`

**Scope:** Every function or loop processing a continuous AXI-Stream sample.
II=1 is the only acceptable value for radar real-time datapaths.

```cpp
void cfar_1d(hls::stream<ap_axis<32,1,1,1>>& in,
             hls::stream<ap_axis<32,1,1,1>>& out) {
#pragma HLS PIPELINE II=1  // MANDATORY
    // HLS will report an error if II=1 cannot be achieved
    // Read the synthesis log: "Achieved II = X" — X must be 1
}
```

**Diagnosing II > 1:** When HLS reports `Achieved II = 2` or higher, the
cause is almost always one of:
1. BRAM port collision → fix with `ARRAY_PARTITION`
2. Loop-carried dependency → fix with `DEPENDENCE ... false`
3. Long combinational path → fix with `PIPELINE II=1` on inner loop + retiming
4. DSP cascade latency → fix by reducing chained arithmetic depth

### 4.3 `#pragma HLS DATAFLOW`

Enables task-level parallelism between sequential functions. Each function
runs concurrently, connected by hardware FIFOs (ping-pong buffers).

```
Without DATAFLOW:  [DDS] → wait → [FFT] → wait → [CFAR]   latency = sum
With DATAFLOW:     [DDS]
                      [FFT]          ← overlapped execution
                         [CFAR]      ← pipeline throughput = slowest stage
```

**SPSC constraint (Single-Producer / Single-Consumer):**

```cpp
// WRONG — buf read by two consumers, DATAFLOW will fail or produce warning
void top(hls::stream<int>& in) {
#pragma HLS DATAFLOW
    int buf[N];
    stage_a(in, buf);
    stage_b(buf, out1);  // consumer 1
    stage_c(buf, out2);  // consumer 2 — ILLEGAL
}

// CORRECT — chain via intermediate FIFOs
void top(hls::stream<int>& in) {
#pragma HLS DATAFLOW
    hls::stream<int> fifo_ab, fifo_bc;
    stage_a(in, fifo_ab);
    stage_b(fifo_ab, fifo_bc);
    stage_c(fifo_bc, out);
}
```

**AI Rule:** Draw the dataflow graph before generating code. Each node has
exactly one producer and one consumer. Global variables accessed by multiple
stages silently break DATAFLOW — HLS may not always error, but throughput
will degrade.

### 4.4 `#pragma HLS DEPENDENCE`

HLS is conservative about loop-carried dependencies. When a loop reads data
it wrote in a previous iteration (e.g., a sliding window sum), HLS assumes
a dependency and forces II > 1 to be safe. This pragma explicitly tells
HLS the dependency is false (or specifies the true distance).

```cpp
// CFAR sliding window — running sum updated every iteration
int32_t window_sum = 0;
cfar_loop: for (int i = 0; i < N_RANGE; i++) {
#pragma HLS PIPELINE II=1
#pragma HLS DEPENDENCE variable=window_sum inter RAW false
// ↑ "inter" = across loop iterations, "RAW" = Read After Write, "false" = no dependency
// Without this pragma: HLS forces II ≥ latency_of_addition (~3 cycles)
    window_sum += new_cell - old_cell;
    threshold[i] = (window_sum * alpha) >> 15;
}
```

**When to use:** Any accumulator or state variable updated inside a
`PIPELINE II=1` loop where the update depends on the previous iteration's
value. This includes: running sums, peak trackers, MTI delay lines.

### 4.5 `#pragma HLS BIND_STORAGE`

Forces the HLS tool to use a specific memory implementation for an array.

```cpp
// DDS sine LUT — must be BRAM, not LUTRAM (LUTRAM has timing issues at 150MHz)
int16_t sin_lut[1024];
#pragma HLS BIND_STORAGE variable=sin_lut type=ROM_1P impl=BRAM

// Corner Turn buffer — dual-port RAM for simultaneous read/write
int16_t corner_turn_buf[N_CHIRPS][N_RANGE];
#pragma HLS BIND_STORAGE variable=corner_turn_buf type=RAM_2P impl=BRAM
```

**Storage type options:**

| Type | Ports | Use For |
|---|---|---|
| `ROM_1P` | 1 read | Lookup tables (sin/cos, alpha table) |
| `ROM_2P` | 2 reads | LUTs accessed from two pipeline stages |
| `RAM_1P` | 1 R/W | Simple buffers |
| `RAM_2P` | 1R + 1W or 2R | Corner Turn (simultaneous read/write) |
| `RAM_S2P` | 1R + 1W separate | Streaming FIFOs |

**AI Rule:** Never leave storage type to HLS inference for arrays > 256
elements. Always bind explicitly. Default inference often chooses LUTRAM
for speed, which fails timing at 150 MHz for large arrays.

### 4.6 `#pragma HLS INLINE`

Dissolves a function boundary, merging its logic into the parent scope for
global optimization. Used for small helper functions inside II=1 pipelines
where the function-call overhead would break the pipeline.

```cpp
// Small helper — inline so HLS can pipeline across the function boundary
inline int16_t saturate(int32_t x) {
#pragma HLS INLINE
    return (x > 32767) ? 32767 : (x < -32768) ? -32768 : (int16_t)x;
}
```

**Use sparingly:** Inlining large functions increases synthesis time
exponentially and makes resource attribution in the synthesis report
unreadable. Only inline functions < 10 lines called inside II=1 loops.

---

## 5. Clock Domain Crossing (CDC) Rules

### 5.1 Zenith Clock Domain Topology

| Domain | Clock Source | Frequency | Interfaces |
|---|---|---|---|
| PS (ARM Cortex-A9) | CPU PLL | 650 MHz (core), 150 MHz (FCLK0 output) | AXI-GP master, DMA control |
| PL Baseband | FCLK0 | 150 MHz | AXI-Stream data path, all HLS kernels |
| AXI-Lite Control | FCLK0 or FCLK1 | 100–150 MHz | s_axilite register configuration |

**All AXI-Lite → AXI-Stream signal paths are CDC boundaries.**
Even if both run at 150 MHz, they are driven by logically independent clock
edges until properly synchronized in the Vivado IP Integrator.

### 5.2 CDC Rule 5.1 — Single-Bit Signals: 2-Stage Synchronizer

For single-bit control signals (enable, reset, mode select) crossing clock
domains, the minimum safe solution is a 2-stage synchronizer:

```cpp
// In HLS, model the synchronizer as a 2-element shift register
// This gives the metastable flip-flop 2 clock cycles to resolve
ap_uint<1> sync_reg[2];
#pragma HLS ARRAY_PARTITION variable=sync_reg complete
#pragma HLS PIPELINE II=1

sync_reg[1] = sync_reg[0];   // stage 2 — stable output
sync_reg[0] = async_input;   // stage 1 — may be metastable

ap_uint<1> safe_signal = sync_reg[1];  // use this, never sync_reg[0]
```

**Why 2 stages:** Each FF stage reduces the probability of metastability
propagation by ~$e^{-T_{clk}/\tau}$ where $\tau \approx 30$ ps for 28nm.
Two stages at 150 MHz gives a Mean Time Between Failures (MTBF) > 10^9 years.

### 5.3 CDC Rule 5.2 — Multi-Bit Data: Async FIFO

For multi-bit data (configuration structs, threshold values) crossing clock
domains, a 2-stage synchronizer is insufficient — bits can change at different
times, producing a glitched intermediate value. Use an async FIFO:

```cpp
// DO NOT synchronize multi-bit data with a 2-stage synchronizer
// Example of the bug: threshold = 0x00FF changes to 0x0100
// During transition, you may read 0x01FF or 0x0000 — neither is correct

// CORRECT: use Xilinx async FIFO IP or HLS async FIFO pragma
// In HLS top-level DATAFLOW region, declare the FIFO explicitly:
hls::stream<DdsConfig> config_fifo;
#pragma HLS STREAM variable=config_fifo depth=4
// Vitis HLS will infer a synchronizing FIFO when the producer/consumer
// clocks differ in the IP Integrator
```

### 5.4 CDC Rule 5.3 — The s_axilite → axis Trap

This is the most common CDC mistake in Zenith kernels. The AXI-Lite interface
(ARM-written configuration registers) and the AXI-Stream data path run
asynchronously. Using an `s_axilite` register value directly inside an
`axis`-clocked pipeline creates a CDC without any synchronization:

```cpp
// WRONG — threshold is written by ARM via AXI-Lite (100 MHz domain)
//         but read inside axis pipeline (150 MHz domain) — metastable!
void cfar_kernel(hls::stream<ap_axis<32,1,1,1>>& in,
                 int threshold) {
#pragma HLS INTERFACE s_axilite port=threshold
#pragma HLS PIPELINE II=1
    auto x = in.read();
    if (x.data > threshold) { ... }  // threshold may be metastable here
}

// CORRECT option A — use a shadow register with explicit handshake
// ARM writes new_threshold + writes update_flag → kernel reads both
// atomically on the same AXI-Lite write burst

// CORRECT option B — pass threshold as an AXI-Stream config packet
// before the data burst, processed in the same clock domain as the data
```

**AI Rule:** Any `s_axilite` scalar used inside a `PIPELINE II=1` loop
body MUST be flagged as a CDC risk. Always ask the user which synchronization
strategy they intend before generating the kernel.

### 5.5 CDC Rule 5.4 — TLAST Clock Domain

`TLAST` marks the end of a packet (chirp, CPI, detection frame). It must be
generated by logic running in the same clock domain as TDATA and TVALID.
Never assert TLAST from an ARM GPIO write — the ARM-side clock and the
AXI-Stream clock are asynchronous, producing a TLAST with undefined
phase relative to the data samples.

```cpp
// CORRECT — TLAST generated by a counter in the PL datapath clock domain
ap_uint<16> sample_count = 0;
auto x = in_stream.read();
out_sample.data = processed;
out_sample.last = (sample_count == N_RANGE - 1);  // generated in PL clock domain
out_sample.valid = 1;
out_stream.write(out_sample);
sample_count = (sample_count == N_RANGE - 1) ? 0 : sample_count + 1;
```
