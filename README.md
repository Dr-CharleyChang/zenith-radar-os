# Zenith-Radar OS

> A software-defined radar operating system for the Zynq-7020 heterogeneous platform.  
> Built in public. Every failure logged.

## What is this?

A 24-month working code-base for a complete radar signal processing pipeline on FPGA —  from zero-copy DMA transport to Ethernet track output.

**Stack:** Vitis HLS 2025.2 · C++20 · AXI4-Stream · Zenoh · ARM Cortex-A9 · Zynq-7020

## The experiment

I've spent 20 years developing radar signal processing algorithms. I know what happens to a photon between the antenna and the track file — not from textbooks, but from decades of radar research that ran from dawn past midnight.

Now I want to find out how much of that knowledge survives when my old-school MATLAB codes meet with flashing AI era. So I started this project called Zenith-Radar, an AI-assisted FPGA development project. With zero experience in hardware design and FPGA coding, I'm using Claude as my co-architect and build this radar operating system on Zynq-7020 board.  
Everything is public. Including the failures.

**The question I'm trying to answer:**  
*What's left of the engineer when the code writes itself?*
*Is it possible for a 47 years old engineer to expand  knowledge-base and make career steering at AI era?*

## Architecture

```
PS (ARM Cortex-A9)          PL (FPGA Fabric)
─────────────────────       ──────────────────────────
Zenith-Core                 Zenith-Silicon
├── DMA Engine              ├── DDS (waveform gen)
│   └── Zero-Copy std::span ├── 1D-FFT (range)
├── RRM Scheduler           ├── Corner Turn
├── Kalman Tracker          ├── 2D-FFT (Doppler)
└── Zenoh Publisher         └── CFAR (detection)
        │
        └──► Ethernet → Track data out
```

**Design principles:**
- Zero heap allocation in the radar loop (`std::array`, static pools)
- Zero memcpy between PS and PL (`std::span` over shared CMA memory)
- `II=1` pipelines on all HLS operators (AXI4-Stream throughout)
- `[[nodiscard]]` + `noexcept` + `constexpr` — deterministic by design

## Roadmap

| Milestone | Focus                                                 | Status         |
| --------- | ----------------------------------------------------- | -------------- |
| **M1**    | Zero-copy DMA transport + DDS waveform                | 🔨 In Progress |
| **M2**    | 1D-FFT range processing + MATLAB validation           | ⏳ Pending      |
| **M3**    | 2D-FFT + CFAR detection                               | ⏳ Pending      |
| **M4**    | RRM scheduler + Kalman tracker + Zenoh output         | ⏳ Pending      |
| **M5**    | Isaac Sim closed-loop validation                      | ⏳ Pending      |
| **M6**    | YAML-to-HLS compiler ("one config file → full radar") | ⏳ Pending      |
## Prior Art: Project Chimera

The CFAR detector in `zenith-silicon/cfar/` was validated in a predecessor project:

| Metric              | Result                                                       |
| ------------------- | ------------------------------------------------------------ |
| Initiation Interval | **II = 1** (full pipeline)                                   |
| Timing              | **6.81 ns** @ 100 MHz (slack +3.19 ns)                       |
| BRAM                | **0%** (window in flip-flops via `ARRAY_PARTITION complete`) |
| DSP48               | **1** (threshold multiply only)                              |
## Hardware Platform

**ALINX AX7020** — Zynq-7020 (xc7z020clg400-1)  
- ARM Cortex-A9 @ 650 MHz (PS)  
- 85K logic cells, 220 DSP48E1, 140 BRAM36 (PL)  
- 512MB DDR3, Gigabit Ethernet  

Total cost: ~$150. Intentionally not a $3000 eval board.

## Follow Along

The build journal is public. Every milestone triggers a post.

- **X/Twitter:** [@charley_builds](https://x.com/charley_builds)
- **Substack:** [Zenith Log](https://zenithlog.substack.com)
  - Post #0: [The Last Craft](https://open.substack.com/pub/zenithlog/p/the-last-craft)

Every synthesis report, every CDC bug, every moment where the AI was wrong  
and the physics was right — it all goes in.

---

*Started: 2026-03-06 · Charley Chang*  
*"20 years of physics. 0 completed FPGA projects. 47 years old. 
All three facts are relevant."*
