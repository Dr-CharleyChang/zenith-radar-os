---
status: CLOSED
Data: 2026-03-06
Author: Charley Chang
Decider: Charley Chang
---
---
# ADR-001: Vivado / Vitis HLS Toolchain Version

---

## Context

Project Zenith targets the ALINX AX7020 (Zynq-7020 xc7z020clg400-1). Before any code is written, the toolchain version must be locked. This decision affects every generated IP core, every PetaLinux build, every synthesis report, and every reader who tries to reproduce the work.

Two options were on the table:

**Option A — Vivado / Vitis HLS 2025.2** (already installed on WORKSTATION-CHIMERA)  
**Option B — Vivado / Vitis HLS 2022.2** (recommended by Gemini as the "stable" choice)

A counter-recommendation from Gemini argued for 2022.2 on the grounds that vendor BSPs (ALINX) are more likely to have explicit support for older toolchain versions, reducing PetaLinux integration risk.

---

## Decision

**Stay on Vivado / Vitis HLS 2025.2.**

---

## Rationale

**1. Version currency is a project requirement, not a preference.**  
Zenith is a 2-year Build in Public experiment. Readers attempting to reproduce the work in 2027 will be on 2024.x or 2025.x, not 2022.2. Publishing synthesis reports and PetaLinux configs against a 3-year-old toolchain produces content that is technically correct but practically non-reproducible for the target audience. The educational value of the project depends on currency.

**2. The BSP gap is a device tree problem, not a version problem.**  
Gemini's concern — that ALINX may not have a 2025.2 BSP — is technically valid. However, the practical consequence of a missing BSP is a one-time device tree exercise: extract DDR3 timing parameters from ALINX's reference design XSA, apply as a manual overlay. This is a 1-2 day task that becomes documented content (Substack Post #1.5). It is not a reason to roll back the entire toolchain.

**3. The existing installation is already validated.**  
Vivado 2025.2 and Vitis HLS 2025.2 are installed and confirmed working on WORKSTATION-CHIMERA via the Project Chimera workflow. Switching to 2022.2 introduces a new installation, new license validation, and new environment configuration with zero benefit.

**4. Chimera prior art transfers clean.**  
The Chimera CFAR kernel (II=1, BRAM=0, DSP=1 at 100MHz / Vitis HLS 2023.2) was re-synthesized under 2025.2 at 150MHz on Day 4 of Week 1. Result: II=1 confirmed, slack +0.23ns, no architectural regressions. The toolchain upgrade introduced no breakage.

---

## Consequences

**Accepted risks:**
- ALINX AX7020 BSP may not exist for 2025.2. Mitigation: use 2023.1 XSA + generic PetaLinux template (confirmed fallback path, see ADR-002).
- Minor HLS pragma syntax changes between versions may require one-time updates to Chimera assets. Accepted: already validated in Week 1.

**Downstream decisions locked by this ADR:**
- All HLS synthesis targets: Vitis HLS 2025.2, clock period 6.67ns (150MHz)
- All PetaLinux builds: PetaLinux 2025.2
- All Vivado Block Designs: Vivado 2025.2
- sstate-cache downloaded: `/opt/petalinux/sstate-cache/2025.2/sstate-cache/`

**On Gemini's recommendation:**  
Choosing 2022.2 for stability is a reasonable position for a production embedded system with a fixed deployment target. For a 2-year public experiment whose primary output is a reproducible reference architecture, version currency outweighs short-term integration friction. Both positions are defensible; context determines the right answer.

---

## Verification

Week 1, Day 4 — Vitis HLS 2025.2 synthesis of `cfar_core`:

| Metric | Chimera (2023.2, 100MHz) | Zenith (2025.2, 150MHz) | Result |
|---|---|---|---|
| II | 1 | 1 | ✅ Unchanged |
| Timing slack | +3.19 ns | +0.23 ns | ✅ Passes |
| BRAM | 0 | 0 | ✅ Unchanged |
| DSP | 1 | 1 | ✅ Unchanged |
| LUT | ~3% | 1% (735) | ✅ Better |

Toolchain upgrade introduced zero architectural regressions.

---

*ADR format: lightweight, single-file, decision-centric.*  
*Reference: [Documenting Architecture Decisions — Michael Nygard, 2011](https://cognitect.com/blog/2011/11/15/documenting-architecture-decisions)*
