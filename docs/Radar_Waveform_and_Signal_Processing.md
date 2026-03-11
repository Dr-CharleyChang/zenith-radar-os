---
tags: [Radar, Waveform, FMCW, SFCW, PulsedLFM, SimplePulse, CFAR, DSP, Physics]
for_ai: true
purpose: >
  This document is the waveform-agnostic physics and algorithm reference for
  Project Zenith-Radar OS. The architecture separates waveform-specific
  front-end processing from a common back-end pipeline (detection, tracking,
  output). When generating signal processing code, AI must identify which
  waveform type is active and apply the correct front-end equations, while
  keeping the back-end interfaces identical across all waveform modes.
---

# Zenith-Radar OS: Waveform-Agnostic Radar Physics & Signal Processing Reference

**AI Instruction:** Do NOT assume FMCW. Always ask the user which waveform
type is being used before generating any DDS, mixer, or range-processing code.
The back-end pipeline (CFAR → Tracker → Output) is identical for all waveforms.
Only the front-end (waveform generation + range/Doppler extraction) changes.

---

## 1. Architectural Philosophy: Waveform Abstraction Layer

The Zenith signal processing chain is divided into two layers:

```
┌─────────────────────────────────────────────────────────────────┐
│                    WAVEFORM FRONT-END LAYER                     │
│  (waveform-specific: generation + matched filtering + range/    │
│   Doppler extraction → outputs a unified Range-Doppler Map)     │
│                                                                 │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌───────────┐        │
│  │  FMCW    │  │  SFCW    │  │  Pulsed  │  │  Future   │        │
│  │ Dechirp  │  │  IDFT    │  │ LFM/MF   │  │ OFDM/etc  │        │
│  └────┬─────┘  └────┬─────┘  └────┬─────┘  └─────┬─────┘        │
└───────┼─────────────┼─────────────┼──────────────┼──────────────┘
        │             │             │              │
        └─────────────┴─────────────┴──────────────┘
                              │
                    [Range-Doppler Map]
                    Unified interface:
                    float32 rdmap[N_range][N_doppler]
                              │
┌─────────────────────────────┼───────────────────────────────────┐
│               COMMON BACK-END LAYER                             │
│                             │                                   │
│          ┌──────────────────▼───────────────────┐               │
│          │   CFAR Detection (CA / OS / GO / SO) │               │
│          └──────────────────┬───────────────────┘               │
│                             │  [Point Cloud]                    │
│          ┌──────────────────▼───────────────────┐               │
│          │   Track Management (RRM + Tracker)   │               │
│          └──────────────────┬───────────────────┘               │
│                             │  [Track States]                   │
│          ┌──────────────────▼────────────────────┐              │
│          │   Network Output (Zenoh / UDP)        │              │
│          └───────────────────────────────────────┘              │
└─────────────────────────────────────────────────────────────────┘
```

**Design Rule:** The Range-Doppler Map is the universal contract between
front-end and back-end. Every waveform front-end must produce a
`float32 rdmap[N_range][N_doppler]` (or its fixed-point equivalent).
The back-end never needs to know which waveform generated it.

**C++ Interface Contract:**
```cpp
// zenith/waveform/waveform_base.hpp

class IWaveformProcessor {
public:
    virtual ~IWaveformProcessor() = default;

    // Generate one CPI worth of TX waveform into the DMA TX buffer
    virtual void generate(std::span<int16_t> tx_buf) noexcept = 0;

    // Process one CPI of raw RX IQ data into a Range-Doppler Map
    // Input:  raw IQ samples from DMA RX buffer
    // Output: range-Doppler map (fixed-point Q15 or float, caller's choice)
    [[nodiscard]]
    virtual bool process(std::span<const int16_t> rx_buf,
                         std::span<float>         rdmap_out) noexcept = 0;

    // Return current waveform parameter summary for logging / Substack posts
    virtual WaveformInfo info() const noexcept = 0;
};

// Concrete implementations (one per waveform type):
class FmcwProcessor    : public IWaveformProcessor { ... };
class SfcwProcessor    : public IWaveformProcessor { ... };
class PulsedLfmProcessor : public IWaveformProcessor { ... };
class SimplePulseProcessor : public IWaveformProcessor { ... };
```

---

## 2. Universal Symbol Table

Symbols shared across all waveform types:

| Symbol | Meaning | Unit |
|---|---|---|
| $c$ | Speed of light | m/s ($3 \times 10^8$) |
| $\lambda$ | Wavelength at carrier $f_0$ | m |
| $f_0$ | Carrier frequency | Hz |
| $f_s$ | ADC/DAC sampling rate | Hz |
| $R$ | Target range | m |
| $\Delta R$ | Range resolution | m |
| $v$ | Target radial velocity | m/s |
| $\Delta v$ | Velocity resolution | m/s |
| $v_{max}$ | Maximum unambiguous velocity | m/s |
| $R_{max}$ | Maximum unambiguous range | m |
| $\tau$ | Round-trip time delay | s ($= 2R/c$) |
| $N_r$ | Number of range cells (range FFT size) | — |
| $N_d$ | Number of Doppler cells (Doppler FFT size) | — |
| $T_{CPI}$ | Coherent Processing Interval duration | s |
| $\text{SNR}_{in}$ | Input SNR (single pulse) | dB |
| $G_{coh}$ | Coherent integration gain | dB |

---

## 3. Waveform Front-Ends

### 3.1 FMCW — Frequency Modulated Continuous Wave

**Operating Principle:** Transmit continuously. Each chirp sweeps linearly from
$f_0$ to $f_0 + B$ over duration $T_c$. Mix echo with TX replica to produce
a low-frequency beat signal whose frequency encodes range.

**Additional Symbols:**

| Symbol | Meaning | Unit |
|---|---|---|
| $B$ | Sweep bandwidth | Hz |
| $T_c$ | Chirp duration | s |
| $S$ | Chirp slope $= B/T_c$ | Hz/s |
| $f_b$ | Beat frequency after mixing | Hz |
| $N$ | Chirps per CPI | — |

**Key Equations:**

$$S = \frac{B}{T_c}, \qquad f_b = \frac{2R\cdot S}{c}, \qquad R = \frac{c \cdot f_b}{2S}$$

$$\Delta R = \frac{c}{2B}, \qquad R_{max} = \frac{c \cdot f_s}{4S}$$

$$\Delta v = \frac{\lambda}{2NT_c}, \qquad v_{max} = \frac{\lambda}{4T_c}$$

**Processing Chain:**
```
TX chirp ──► Mix with RX ──► LPF ──► ADC ──► 1D-FFT (range)
                                              ──► Corner Turn
                                              ──► 2D-FFT (Doppler)
                                              ──► [RD Map]
```

**Coherent gain:** $G_{coh} = 10\log_{10}(N)$ dB

**HLS Notes:**
- Beat signal is low-frequency → ADC rate can be much lower than carrier ($f_s \ll f_0$)
- Corner Turn between 1D and 2D FFT is the primary BRAM bottleneck (see Constraints doc)
- `TLAST` asserts at end of each chirp ($N_r$ samples)

**Constraint — Integer samples per chirp:**
$$N_r = f_s \cdot T_c \in \mathbb{Z}^+$$
ADC clock and chirp timer must derive from the same PLL.

---

### 3.2 SFCW — Stepped Frequency Continuous Wave

**Operating Principle:** Instead of a continuous sweep, transmit $N$ discrete
tones stepped by $\Delta f$ Hz. Record complex IQ at each step. Synthesize a
wide-bandwidth range profile via IDFT across the frequency steps.

**Additional Symbols:**

| Symbol | Meaning | Unit |
|---|---|---|
| $\Delta f$ | Frequency step size | Hz |
| $N_{steps}$ | Number of frequency steps | — |
| $B_{eff}$ | Effective bandwidth $= N_{steps} \cdot \Delta f$ | Hz |
| $T_{step}$ | Dwell time per frequency step | s |
| $T_{CPI}$ | Total CPI $= N_{steps} \cdot T_{step}$ | s |

**Key Equations:**

$$\Delta R = \frac{c}{2 B_{eff}} = \frac{c}{2 N_{steps} \Delta f}$$

$$R_{max} = \frac{c}{2 \Delta f}$$

Range profile from IDFT across frequency steps:
$$r(n) = \sum_{k=0}^{N_{steps}-1} S_{rx}(k) \cdot e^{j 2\pi k n / N_{steps}}$$

Velocity measurement requires multiple CPIs (inter-CPI phase comparison).

**Processing Chain:**
```
N_steps CW tones (sequential) ──► IQ capture per step
──► IDFT across steps ──► Range Profile (1D)
──► Inter-CPI phase difference ──► Doppler estimation
──► [RD Map]
```

**HLS Notes:**
- No Corner Turn needed (IDFT is 1D, not 2D)
- Doppler resolution is poor for a single CPI; velocity estimated from
  inter-CPI phase → requires persistent state across CPIs in RRM
- `TLAST` asserts after all $N_{steps}$ IQ pairs collected
- DDS must support fast frequency hopping: settling time per step ≤ $T_{step}/10$

**Trade-off vs FMCW:**

| Property | FMCW | SFCW |
|---|---|---|
| Hardware complexity | Medium | Lower (no Corner Turn) |
| Range resolution | $c/2B$ | $c/2B_{eff}$ (same formula) |
| Velocity measurement | Per CPI (2D-FFT) | Inter-CPI (slower update) |
| Interference sensitivity | High (broadband) | Lower (narrow per step) |
| Suitable for | Fast targets, automotive | Static/slow targets, imaging |

---

### 3.3 Pulsed LFM — Linear Frequency Modulated Pulse

**Operating Principle:** Transmit short bursts of LFM chirp energy (not
continuous). During TX, receiver is blanked. Echo processed via matched
filter (pulse compression) to achieve fine range resolution despite short pulse.

**Additional Symbols:**

| Symbol | Meaning | Unit |
|---|---|---|
| $\tau_p$ | Pulse width (transmitted) | s |
| $B_p$ | Pulse bandwidth (LFM sweep) | Hz |
| $T_{PRI}$ | Pulse Repetition Interval | s |
| $\text{PRF}$ | Pulse Repetition Frequency $= 1/T_{PRI}$ | Hz |
| $D$ | Time-Bandwidth product $= B_p \cdot \tau_p$ | — (typically 50-1000) |
| $G_{PC}$ | Pulse compression gain $= 10\log_{10}(D)$ | dB |

**Key Equations:**

$$\Delta R_{compressed} = \frac{c}{2B_p} \qquad \text{(after pulse compression)}$$

$$\Delta R_{uncompressed} = \frac{c \cdot \tau_p}{2} \qquad \text{(before compression, much coarser)}$$

$$R_{max} = \frac{c \cdot T_{PRI}}{2} \qquad \text{(limited by PRI, not pulse width)}$$

$$v_{max} = \frac{\lambda \cdot \text{PRF}}{4}, \qquad \Delta v = \frac{\lambda}{2 N_{pulses} T_{PRI}}$$

**Processing Chain:**
```
LFM pulse TX ──► Blank RX during TX ──► Receive echo
──► Matched Filter (MF) ──► Pulse Compression (range)
──► N_pulses × compressed range profiles
──► 2D-FFT across slow-time (Doppler)
──► [RD Map]
```

**Matched Filter:** Cross-correlation of received signal with TX replica.
In frequency domain: $Y(f) = X(f) \cdot H^*(f)$, where $H(f)$ is the
conjugate spectrum of the TX chirp. Implemented as FFT → multiply → IFFT.

**HLS Notes:**
- Matched filter requires storing the TX replica spectrum (complex float32,
  $N_r$ points) → 1 BRAM per replica copy
- TX blanking must be hardware-enforced (AXI-Lite GPIO to RF switch)
- `TLAST` asserts at end of each PRI (not pulse width — the window spans the full PRI)
- Duty cycle $= \tau_p / T_{PRI}$, typically 1-10%

**Trade-off vs FMCW:**

| Property | FMCW | Pulsed LFM |
|---|---|---|
| Average TX power | High (continuous) | Lower (pulsed) |
| Range-velocity coupling | Exists (range-Doppler coupling) | Decoupled |
| Close-range blindspot | None | Yes ($\approx c\tau_p/2$) |
| Hardware TX/RX isolation | Easier (frequency offset) | Harder (must blank) |
| Long-range performance | Limited by leakage | Better (high peak power) |

---

### 3.4 Simple Pulse (Unmodulated)

**Operating Principle:** Transmit a short rectangular pulse of CW at $f_0$.
No frequency modulation. Range resolution set by pulse width alone.

**Key Equations:**

$$\Delta R = \frac{c \cdot \tau_p}{2}$$

$$R_{max} = \frac{c \cdot T_{PRI}}{2}$$

No pulse compression — range resolution is coarse without large bandwidth.
Doppler processing same as Pulsed LFM (slow-time FFT across PRIs).

**HLS Notes:**
- DDS: single-frequency output, no sweep required — simplest DDS configuration
- No matched filter needed (or use a simple rectangular MF = integrate over $\tau_p$)
- Primarily useful for initial hardware bringup and educational demos
- For Zenith M1/M2 validation: use Simple Pulse first to verify the
  PS/PL data path before adding LFM complexity

**AI Rule:** When the user is at M1 or early M2 stage, default to
Simple Pulse for hardware bringup. Do not generate FMCW dechirp or
Corner Turn code until the basic DMA loop-back is validated.

---

### 3.5 Extension Points — Future Waveforms

The `IWaveformProcessor` interface accommodates future waveform types
without modifying the back-end. Planned extensions:

| Waveform | Key Advantage | Processing Difference vs FMCW |
|---|---|---|
| OFDM Radar | Range-Doppler fully decoupled, waveform agility | 2D-IFFT instead of separate 1D FFTs |
| Phase-Coded (Barker, Golay) | Low sidelobe after compression | Matched filter = convolution with code sequence |
| Noise / Random FM | Low probability of intercept | Cross-correlation with stored TX reference |
| Sparse Frequency (Cognitive) | Avoids interference bands | Non-uniform IDFT (NUFFT) |

**AI Rule:** If a user requests one of these future waveforms, implement
it as a new class inheriting from `IWaveformProcessor`. Never modify the
CFAR or Tracker back-end to accommodate a new waveform.

---

## 4. Common Back-End: Range-Doppler Processing

This section applies identically to all waveform types once the RD Map is produced.

### 4.1 MTI Clutter Suppression (Pre-CFAR, Optional)

Applied in slow-time before Doppler FFT when strong stationary clutter exists.

**2-pulse canceler:**
$$H(z) = 1 - z^{-1}, \qquad \text{zeros at DC (0 m/s)}$$

**3-pulse canceler (deeper null):**
$$H(z) = 1 - 2z^{-1} + z^{-2}$$

**Blind velocity** (same for all waveforms using a PRI $T_{PRI}$):
$$v_{blind} = n \cdot \frac{\lambda}{2 T_{PRI}}, \quad n = 1, 2, \ldots$$

**HLS cost:** 1-2 FIFOs (depth = $N_r$) + 1-2 subtractors. Negligible resources.

### 4.2 Coherent Integration Gain

For any waveform integrating $N$ pulses or chirps coherently:
$$G_{coh} = 10\log_{10}(N) \text{ dB}$$

This gain is why Doppler FFT (MTD) dominates over non-coherent integration:
non-coherent gain is only $5\log_{10}(N)$ dB.

### 4.3 CFAR Detection

#### CA-CFAR (Community Edition)

For each Cell Under Test (CUT):

| Symbol | Meaning | Unit | Typical Value |
|---|---|---|---|
| $N_{train}$ | Training cells per side | — | 16–32 |
| $N_{guard}$ | Guard cells per side | — | 2–4 |
| $P_{fa}$ | Desired false alarm probability | — | $10^{-4}$ to $10^{-6}$ |
| $\alpha$ | Threshold scaling factor | — | computed below |

$$T = \alpha \cdot \hat{\sigma}^2, \qquad \hat{\sigma}^2 = \frac{1}{N_{train}} \sum_{i \in \text{training}} x_i$$

$$\alpha = N_{train} \cdot \left( P_{fa}^{-1/N_{train}} - 1 \right)$$

**Sliding window HLS pattern (II=1):**
```cpp
// Running sum update: O(1) per CUT
window_sum += rdmap[cut + N_GUARD + 1]   // add new leading cell
            - rdmap[cut - N_TRAIN - N_GUARD]; // remove old trailing cell
int32_t threshold = (window_sum * ALPHA_Q15) >> 15;
```

**Weakness:** Fails in dense target environments (masking) and at clutter edges.

#### OS-CFAR (Enterprise Edition)

Select the $k$-th order statistic of training cells as noise estimate.
Recommended: $k = \lfloor 0.75 \cdot N_{train} \rfloor$.

HLS implementation requires Bitonic Sort Network (see Constraints doc).
Comparator stages: $\frac{1}{2}\log_2(N_{train}) \cdot (\log_2(N_{train})+1)$

#### GO-CFAR / SO-CFAR

- **GO (Greatest Of):** Take the larger of leading and lagging window averages.
  Better at clutter edges; more false alarms in homogeneous clutter.
- **SO (Smallest Of):** Take the smaller of the two windows.
  Better in dense targets; worse at edges.

All four CFAR variants share the same sliding window accumulator structure.
Only the final threshold selection logic differs — a natural `if/else` in HLS.

### 4.4 Detection Output: Point Cloud Format

```cpp
// zenith/common/types.hpp — shared across all waveforms
struct Detection {
    float    range_m;        // [m]
    float    velocity_mps;   // [m/s], positive = approaching
    float    azimuth_deg;    // [deg], 0 = broadside, if DBF available
    float    snr_db;         // [dB], detection SNR above CFAR threshold
    uint8_t  waveform_id;    // which waveform produced this detection
    uint8_t  flags;          // bit0: velocity ambiguous, bit1: range ambiguous
    uint16_t reserved;
};
static_assert(sizeof(Detection) == 20);  // ensure no padding surprises
static_assert(std::is_trivially_copyable_v<Detection>);  // Zenoh zero-copy safe
```

This struct is the universal output of all CFAR implementations and the
universal input to the Tracker — regardless of which waveform is active.

---

## 5. Waveform Selection Guide

Use this table when helping the user decide which waveform to use:

| Requirement                         | Recommended Waveform        | Reason                                            |
| ----------------------------------- | --------------------------- | ------------------------------------------------- |
| Hardware bringup / validation       | Simple Pulse                | Minimal DDS config, no compression                |
| Short range (< 50m), fast update    | FMCW                        | Low complexity, good velocity resolution          |
| Long range (> 200m)                 | Pulsed LFM                  | High peak power, better range-velocity decoupling |
| High range resolution, slow targets | SFCW                        | No Corner Turn, easy hardware                     |
| Interference-dense environment      | SFCW or Phase-coded         | Narrowband per step / spread spectrum             |
| Learning / experimentation          | FMCW first, then Pulsed LFM | Most documentation available                      |

**AI Rule:** Always surface this trade-off table when a user asks
"which waveform should I use?" rather than defaulting to FMCW.

---

## 6. Parameter Consistency Checker

Before generating any signal processing code, verify these constraints:

```
1. Integer samples per pulse/chirp:
   N_r = f_s × T_pulse ∈ ℤ⁺   (all waveforms)

2. Range-Doppler map dimensions fit in BRAM:
   BRAM_needed = ⌈(N_r × N_d × 4 bytes) / 36Kb⌉ ≤ 98   (safe budget)

3. Phase accumulator width (DDS):
   W ≥ 24 bits   (all waveforms using DDS)
   Spur level ≈ -6W dBc

4. FMCW-specific: beat frequency fits ADC bandwidth:
   f_b_max = 2 × S × R_max / c ≤ f_s / 2

5. Pulsed: duty cycle reasonable:
   duty = τ_p / T_PRI ∈ [0.01, 0.10]   (1%-10%)

6. Velocity unambiguous range covers target scenario:
   v_max > v_target_max

7. DSP48 budget (if DBF enabled):
   4 × M_elements × N_beams ≤ 154
```

**AI Rule:** Run through this checklist for every new waveform configuration
the user proposes. Print which checks pass and which fail before writing code.
