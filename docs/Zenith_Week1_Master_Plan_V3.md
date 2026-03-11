---
Data: 2026-03-06
Author: Charley Chang
status: ACTIVE — Day 1 in progress
---
---
# Project Zenith — Week 1 Master Battle Plan (V3.0)
## 0. Situation Report: What We Actually Know Right Now

This is the ground truth as of Day 1. Every task below is calibrated to this reality.

### Hardware
| Item                                                | Status      | Notes                                     |
| --------------------------------------------------- | ----------- | ----------------------------------------- |
| ASUS Workstation Ultra 7 265KF / 64G / 2T / RTX5080 | ✅ Online    | Remote access confirmed via Tailscale     |
| ALINX AX7020 (Zynq-7020 xc7z020clg400-1)            | ⚠️ Partial  | Board powered on, JTAG not yet connecting |
| HDMI Dummy Plug on RTX5080                          | ✅ Installed | Parsec rendering confirmed                |

### Software Environment
| Item          | Version                     | Status                | Notes                                |
| ------------- | --------------------------- | --------------------- | ------------------------------------ |
| Vivado        | **2025.2**                  | ✅ Installed (Windows) | Desktop icon confirmed in screenshot |
| Vitis HLS     | **2025.2**                  | ✅ Installed (Windows) | Same installation package            |
| WSL2 / Ubuntu | 24.04 LTS                   | ✅ Running             | Chimera dev environment              |
| ROS2          | Jazzy                       | ✅ Running             | Future Zenoh bridge candidate (M4+)  |
| Remote Access | Tailscale + Parsec + ToDesk | ✅ Three-layer OOB     | Per Chimera whitepaper               |

### Prior Art: Project Chimera
| Asset | Technical Status | Zenith Destination |
|---|---|---|
| `cfar.cpp` + `cfar.h` | ✅ C-Sim validated, II=1, 6.81ns @ 100MHz | `zenith-silicon/cfar/` |
| `script.tcl` | ✅ Working HLS build script | `zenith-silicon/cfar/hls_build.tcl` |
| HLS Synthesis Report | BRAM=0%, DSP=1, LUT~3% on xc7z020 | Substack Post #2 anchor |
| Toolchain health | Vitis HLS 2023.2 used in Chimera | Version gap to investigate |

### Open Issues
| Issue | Priority | Owner |
|---|---|---|
| JTAG not connecting to AX7020 | 🔴 High — blocks M1 board work | Diagnose in Week 1 |
| ADR-001 decided: Stay on 2025.2 | ✅ Closed | See Section 1 |
| Chimera used Vitis HLS 2023.2, Zenith targets 2025.2 | 🟡 Medium | Verify CFAR re-synth in 2025.2 |
| Social media accounts | 🔴 Today — zero presence | See Section 5 |

---

## 1. Architecture Decision Record: ADR-001 (CLOSED)

**Decision: Stay on Vivado / Vitis HLS 2025.2**

This was the most important decision of Day 1. It is now closed.

**Rationale:**
- Already installed and validated by Chimera workflow
- 2-year project horizon: readers in 2027 will be on 2024.x or 2025.x, not 2022.2
- "Build in Public" value requires version currency — old tools mean non-reproducible results
- ALINX BSP gap (vendor BSPs rarely track latest Xilinx releases) is a **device tree problem**, not a version problem — solvable in ~2 days at M1, not a rollback trigger

**Mitigation for ALINX BSP gap:**
Use Xilinx's official `xc7z020clg400-1` board definition as base. Extract DDR3 timing parameters from ALINX's reference design XSA file. Apply as manual device tree overlay. This process becomes **Substack Post #1.5** — "How to adapt a vendor BSP to a newer toolchain."

**Note on Gemini's counter-recommendation:** The PetaLinux version-lock concern is technically valid but describes a solvable engineering problem. Choosing 2022.2 for stability is a reasonable position for a production system. For a 2-year public experiment whose output is a reference architecture, version currency outweighs short-term friction.

---

## 2. ADR-002: PetaLinux Strategy (DECIDE BY DAY 2)

**The question:** PetaLinux 2025.2 on Ubuntu 24.04 (WSL2) vs. dedicated Ubuntu 22.04 VM.

**Option A: PetaLinux 2025.2 on existing WSL2 Ubuntu 24.04**
- ✅ No new VM overhead, use existing Chimera infrastructure
- ⚠️ Ubuntu 24.04 is on the edge of official PetaLinux support
- ⚠️ Must verify `/bin/sh → bash` and Python 3.x compatibility

**Option B: Dedicated Ubuntu 22.04 VM for PetaLinux only**
- ✅ Officially supported host OS, zero compatibility risk
- ✅ Clean separation: Chimera/ROS2 env ≠ Zenith embedded env
- ✅ 64GB RAM + 2TB disk = you have the headroom
- ❌ One more environment to maintain

**Recommendation:** Option B. Your workstation has the resources. Clean separation prevents the classic "it works in one context but not the other" debugging nightmare. Chimera and Zenith should be hermetically isolated.

**Offline Build Patch (Critical for either option):**
PetaLinux's Yocto build system pulls from global mirrors. In any network environment, this causes random `Fetch Error` failures and 10+ hour builds. Download the sstate-cache offline package before building:
- sstate-cache for 2025.2: ~50-80GB
- downloads package: ~20GB  
- Set in `petalinux-config → Yocto Settings → SSTATE_DIR` and `DL_DIR`
- Set `CONNECTIVITY_CHECK_URIS = ""` (disable connectivity check, keep network available for gaps)

---

## 3. JTAG Status: What We Know and What To Check

JTAG is not connecting. This is **not blocking Week 1 software work**, but it must be resolved before any PL bitstream can be loaded.

**Most likely cause given your setup (Windows host + WSL2):**

The Vivado Hardware Manager must run on **Windows**, not inside WSL2. Your Vivado installation is on Windows (confirmed by screenshot). If you've been trying to run `hw_server` or Hardware Manager from within WSL2, that is the entire problem — the USB JTAG device is a Windows USB device and cannot be seen by WSL2 without `usbipd-win` passthrough.

**Diagnosis checklist (do this when you have board access):**

```
Step 1: Open Windows Device Manager
        Plug in the AX7020 USB-JTAG cable
        
        GOOD: "Digilent USB Device" appears under USB controllers
        BAD:  "USB Serial Port (COM3)" + "USB Serial Port (COM4)" appears
              → Generic FTDI driver grabbed it. Need cable driver reinstall.

Step 2: If BAD — reinstall cable drivers
        C:\Xilinx\Vivado\2025.2\data\xicom\cable_drivers\nt64\digilent\
        Right-click install_drivers.exe → Run as Administrator
        Unplug → replug board USB

Step 3: Physical verification
        [ ] Board powered via 12V DC barrel jack (not just USB-powered)
        [ ] USB cable is a DATA cable (not charge-only — charge-only has no data pins)
        [ ] USB cable plugs into the JTAG/UART port specifically (check silkscreen)
        [ ] Try USB 2.0 port if workstation only has USB 3.0 available
        [ ] PROG/DONE LED status: green = board alive, off = power problem

Step 4: Vivado Hardware Manager test
        Open Vivado on Windows (not WSL2)
        Flow Navigator → Open Hardware Manager → Auto Connect
        Should show: xc7z020_0 in the Hardware panel
```

**JTAG resolution is a Substack moment.** Whatever the problem turns out to be, it's relatable content. Every FPGA developer has stared at an empty Hardware Manager at least once.

---

## 4. Week 1 Daily Task Plan

### Day 1 (Today, 2026-03-06) — Audit + Commit + Publish

**Priority order — do not deviate:**

**Task 1: Chimera CFAR asset transfer (30 min)**

This is the highest-value technical action today. Your Chimera CFAR is M3 prior art sitting in the wrong directory.

```bash
# On workstation via SSH
mkdir -p ~/projects/zenith-radar-os/zenith-silicon/cfar
cp ~/projects/chimera/src/cfar.cpp \
   ~/projects/chimera/src/cfar.h \
   ~/projects/zenith-radar-os/zenith-silicon/cfar/
cp ~/projects/chimera/script.tcl \
   ~/projects/zenith-radar-os/zenith-silicon/cfar/hls_build.tcl
```

Create `zenith-silicon/cfar/CHIMERA_LINEAGE.md`:

```markdown
# Chimera Lineage

Origin: Project Chimera, Week 6 (2026-01-30)
Toolchain at origin: Vitis HLS 2023.2
Target: xc7z020clg400-1 @ 100 MHz

## Validated Synthesis Results (Chimera, 2023.2)
- II: 1 (full pipeline, every clock cycle)
- Timing: 6.81 ns estimated (slack +3.19 ns, headroom to ~146 MHz)
- BRAM: 0% (window_buffer mapped entirely to FF registers via ARRAY_PARTITION complete)
- DSP: 1 (threshold multiply only)
- LUT/FF: ~3%

## Zenith Status
- [ ] Re-synthesize under Vitis HLS 2025.2 (verify numbers unchanged)
- [ ] Audit TLAST propagation for AXI4-Stream frame boundary compliance
- [ ] Audit ap_ctrl_none vs ap_ctrl_hs interface contract
- [ ] Board-level validation (Zenith M3 milestone)

## Known Interface Gaps (to fix in M3)
The Chimera CFAR was designed for standalone C-Sim validation.
For Zenith integration it needs:
1. Explicit TLAST assertion after last CFAR output per detection frame
2. TUSER sideband for RX channel ID (see AXI_Stream_and_Zero_Copy_V3.md §1.3)
3. Clock domain verification: s_axilite alpha_threshold vs axis data path (CDC trap)
```

**Task 2: GitHub repository (45 min)**

```bash
cd ~/projects/zenith-radar-os
git init
git add .
git commit -m "chore: initialize Zenith-Radar-OS

- zenith_memory_map.hpp: CMA address single source of truth
- .gitignore: Vivado/Vitis/HLS/PetaLinux industrial-grade exclusions  
- ADR-001: Vivado 2025.2 decision (closed)
- zenith-silicon/cfar: transferred from Project Chimera Week 6
  Synthesis validated: II=1, 6.81ns, BRAM=0% on xc7z020 @ 100MHz

Prior art acknowledged: Project Chimera (Vitis HLS 2023.2, 2026-01-30)
Platform: ASUS Ultra7/64G/RTX5080, WSL2/Ubuntu 24.04"
```

**Task 3: Social media accounts (45 min) — see Section 5**

**Task 4: First X post — publish today**

**Task 5: Vitis HLS 2025.2 smoke test (20 min)**

Re-synthesize the Chimera CFAR under 2025.2 to confirm toolchain health:

```bash
# Open Vitis HLS 2025.2 on Windows
# File → Open Project → navigate to chimera/hls_cfar_proj
# Solution → Run C Synthesis
# Compare: II should still be 1, timing should be similar
# If numbers differ significantly → document in ADR-003
```

---

### Day 2 — ADR-002 + Vivado Block Design Foundation

**Morning: ADR-002 decision**
- Decide PetaLinux strategy (Option A or B from Section 2)
- If Option B: create Ubuntu 22.04 VM, start PetaLinux download
- If Option A: run Ubuntu 24.04 compatibility checks

**Afternoon: First Vivado Block Design**

```tcl
# In Vivado 2025.2 Tcl Console:
create_project zenith_bd ./zenith_vivado -part xc7z020clg400-1

# Add Zynq PS7 IP
create_bd_cell -type ip -vlnv xilinx.com:ip:processing_system7 ps7_0

# Apply board preset (gets DDR3 timing right for AX7020)
# If AX7020 board files not installed: manually import from ALINX reference design
apply_bd_automation -rule xilinx.com:bd_rule:processing_system7 \
  -config {make_external "FIXED_IO, DDR" apply_board_preset "1"} \
  [get_bd_cells ps7_0]

# Configure FCLK0 for 150 MHz (Zenith baseband clock)
set_property -dict [list \
  CONFIG.PCW_FPGA0_PERIPHERAL_FREQMHZ {150} \
] [get_bd_cells ps7_0]
```

**Day 2 social content:** Photo of AX7020 board + one sentence on why you chose a $150 board over a $3000 evaluation kit.

---

### Day 3 — Cross-Compiler Validation

Chimera proved g++ works on Ubuntu 24.04. The question for Zenith is the **ARM cross-compiler**.

```bash
# In WSL2:
sudo apt install gcc-arm-linux-gnueabihf g++-arm-linux-gnueabihf -y

# Cross-compile the memory map header as a smoke test
cat > /tmp/zenith_crosstest.cpp << 'EOF'
#include <span>
#include <array>
#include <cstdint>
#include <cstdio>

constexpr uintptr_t CMA_PHYS_BASE = 0x1000'0000;

int main() {
    std::array<uint8_t, 4> buf{0xDE, 0xAD, 0xBE, 0xEF};
    std::span<uint8_t> view(buf);
    printf("Zenith cross-compile OK | span=%zu | CMA=0x%08lX\n",
           view.size(), CMA_PHYS_BASE);
    return 0;
}
EOF

arm-linux-gnueabihf-g++ -std=c++20 -O2 \
    -o /tmp/zenith_arm_test /tmp/zenith_crosstest.cpp

file /tmp/zenith_arm_test
# Must show: ELF 32-bit LSB executable, ARM, EABI5
```

**Day 3 social content:** Screenshot of `file` output showing ARM ELF. Caption: "First Zenith code that will actually run on silicon."

---

### Day 4 — JTAG Resolution + Board Bring-Up

This day is reserved for JTAG diagnosis and first Linux boot on AX7020.

Work through the JTAG checklist in Section 3. Do not skip steps.

**Once JTAG is working:**
- Load a minimal "Hello World" bitstream via Vivado Hardware Manager
- Observe DONE LED go green
- Connect UART at 115200 baud
- Boot PetaLinux image (from Day 2-3 build)
- Target: `uname -a` printed on UART

**Day 4 social content:** UART terminal screenshot. Even if it shows a kernel panic — *especially* if it shows a kernel panic. That is authentic content.

---

### Day 5 — CMA Verification + Week 1 Retrospective

**CMA smoke test on running Linux:**

```bash
# On AX7020 board via UART/SSH:
dmesg | grep -i cma
# Expected: "cma: Reserved 16 MiB at 0x10000000"

dmesg | grep xilinx-dma  
# Expected: "xilinx-dma" driver loaded

cat /proc/iomem | grep -i cma
# Verify address matches zenith_memory_map.hpp CMA_PHYS_BASE
```

**Friday evening: Write Week 1 retrospective**

Save as `docs/build-log/Week1_retrospective.md`. Template:

```markdown
# Week 1 Retrospective

## What the plan said would happen
...

## What actually happened  
...

## What I learned that wasn't in any tutorial
...

## What carries into Week 2
...
```

This document becomes the body of your Substack Post #1.

---

## 5. Social Media: Complete Setup Guide

You said you are a total newbie. No assumed knowledge below.

---

### Platform 1: X (Twitter) — Do This Today

**Account creation:**
1. Open x.com in browser → "Sign up"
2. Use your real name — people follow humans, not brands
3. Verify with phone number (required)

**Profile setup (20 minutes — this matters):**

- **Photo:** Your real face. A clear, well-lit photo. Not a logo.
- **Header image:** A screenshot of the Chimera CFAR synthesis report. It shows you build real things.
- **Bio (160 char limit):**
```
Radar engineer. 20 years of physics, 0 years of social media.
Building a radar OS on FPGA with AI. Both experiments start now.
#BuildInPublic
```
- **Website field:** Your GitHub repo URL (create that first)
- **Location:** Your city (optional but humanizing)

**Your first post — copy and post this today, unmodified:**

---

> There's a specific kind of dread that comes to experienced engineers around my age right now.
>
> Not fear of being replaced. Something quieter than that.
>
> It's the feeling of watching a craft you spent 20 years building get turned into a prompt.
>
> I'm a radar engineer. I understand what happens to a photon between the antenna and the track file. That knowledge lives in my hands as much as my head — it came from debugging hardware at midnight, from MATLAB plots that lied to me, from physics that didn't care about my deadline.
>
> I don't know if that still matters the way it used to.
>
> So I'm running an experiment. I'm building a full software-defined radar OS on an FPGA — from DMA drivers to Kalman trackers — using AI as my co-architect. In public. Every failure logged.
>
> Not to prove AI is powerful. I already know that.
>
> To find out what's left of the engineer when the code writes itself.
>
> GitHub: [your link] | #BuildInPublic #FPGA #RadarEngineering

---

**How to post a thread on X:**
- Type the first paragraph → click the "+" button (bottom right of the text box) to add the next post in the thread → repeat
- OR paste the entire text — X will warn you it's too long and let you split it

**Who to follow first:**
- Search `#BuildInPublic` → Latest → follow people posting actual project updates
- Search `#FPGA` → same
- `@adam_taylor_` — Zynq/FPGA educator, very active community
- `@fpgaengineer` — embedded FPGA community

**Daily posting rhythm:** One post per day. Observation + technical fact. Two sentences is enough.

```
Example Day 2:
"AX7020 synthesis report: BRAM utilization 0%.
The CFAR window lives entirely in flip-flops.
25 registers. Single-cycle access. No memory bus.
This is why hardware thinking is different from software thinking.
#FPGA #BuildInPublic"

Example Day 3:
"JTAG refused to connect for 3 days.
The fix: a charge-only USB cable with no data pins.
Four dollars of cable. Three days of debugging.
This is the real curriculum.
#Zynq #BuildInPublic"
```

---

### Platform 2: GitHub — Your Credibility Anchor

**Account creation:**
1. Go to github.com → "Sign up"
2. Username: match your X handle for consistency
3. Same email as X

**Create repository:**
1. Click "+" top right → "New repository"
2. Name: `zenith-radar-os`
3. Description: `A software-defined radar OS for Zynq-7020. Built in public. By a radar engineer learning FPGA the hard way.`
4. ✅ Public  
5. ✅ Add a README  
6. License: MIT  
7. Create repository

**README.md — replace default with this:**

```markdown
# Zenith-Radar OS

> A software-defined radar operating system for the Zynq-7020 heterogeneous platform.  
> Built in public. Every failure logged.

## What is this?

A 24-month working codebase for a complete radar signal processing pipeline on FPGA —  
from DMA transport to Ethernet track output.

**Stack:** Vitis HLS 2025.2 · C++20 · AXI4-Stream · Zenoh · ARM Cortex-A9

## The experiment

I've spent 20 years building radar systems. I want to find out how much  
of that knowledge survives contact with AI-assisted development.

The honest answer: I don't know yet.

## Roadmap

| Milestone | Focus | Status |
|---|---|---|
| M1 | Zero-copy DMA transport + DDS waveform | 🔨 In Progress |
| M2 | 1D-FFT range processing + MATLAB validation | ⏳ Pending |
| M3 | 2D-FFT + CFAR detection | ⏳ Prior art from Project Chimera |
| M4 | RRM scheduler + Kalman tracker + Zenoh output | ⏳ Pending |
| M5 | Isaac Sim closed-loop validation | ⏳ Pending |
| M6 | YAML-to-HLS compiler | ⏳ Pending |

## Follow along

- X/Twitter: [@your_handle]  
- Substack: [your_substack_url]

---
*Started: 2026-03-06 · Charley Chang*  
*Prior art: Project Chimera (CFAR II=1 validated on xc7z020, 2026-01-30)*
```

---

### Platform 3: Substack — Long-Form Home

**Account creation:**
1. Go to substack.com → "Start writing"
2. Publication name options (check availability):
   - **"Zenith Log"** — clean, project-specific
   - **"Silicon & Signal"** — broader radar/FPGA identity
   - **"The Radar Notebook"** — human, accessible
3. Description: *"A radar engineer builds FPGA firmware with AI. In public. One milestone at a time."*
4. Logo: same photo as X profile

**Welcome email** (auto-sent to new subscribers — write this during setup):

```
Welcome.

I'm a radar engineer who spent 20 years understanding
what happens to a signal between the antenna and the track file.

I'm now running an experiment: building a full radar OS on FPGA
using AI as my co-architect. In public. Every failure documented.

The question I'm trying to answer isn't "can AI write hardware code."
It can. I already know that.

The question is: what's left of the engineer when it does?

Each post covers one milestone. They come every 6-8 weeks,
when there's something real to show.

GitHub: [link]
X: [link]

— Charley
```

**Post #0 — "The Last Craft"**

Publish this within the first week. It is the long-form version of your X thread. Target 600-900 words. Structure:

1. The dread (expand the X post opening — 2 paragraphs)
2. One specific technical story from your past (not a CV bullet — a scene)
3. What Zenith is (3 sentences, no jargon)
4. The experiment (the question you're answering)
5. The rules (everything public, no cherry-picking successes)
6. CTA: GitHub + "subscribe if you want to watch"

---

### The Content Flywheel

```
Daily engineering work
        │
        ▼
GitHub commit  ←───────────────────────── proof of work
        │
        ├──► X post: short observation (daily, 2 min to write)
        │         └──► replies, follows, community builds
        │
        └──► Substack post: full milestone story (~every 6 weeks)
                        └──► subscribers → future customers (M6+)

Every platform points to GitHub.
GitHub gives everything else credibility.
```

---

### Week 1 Content Calendar

| Day | Engineering | X Post Content | Other |
|---|---|---|---|
| **Day 1 (Today)** | Chimera transfer + GitHub setup | **The first post** (copy above) | GitHub repo live |
| **Day 2** | ADR-002 + Vivado BD start | AX7020 board photo + "why a $150 board" | Substack account created |
| **Day 3** | ARM cross-compile | ARM ELF screenshot: "first code that runs on silicon" | — |
| **Day 4** | JTAG diagnosis + board bring-up | UART screenshot (whatever it shows) | — |
| **Day 5** | CMA test | Chimera CFAR synthesis report: your technical credibility | Substack Post #0 published |
| **Weekend** | Write Week 1 retrospective | "Plan vs. reality" thread | Save as `docs/build-log/Week1.md` |

---

## 6. Repository Structure (Final)

```
zenith-radar-os/
├── README.md
├── .gitignore                          ← Vivado/HLS/PetaLinux (industrial grade)
│
├── docs/
│   ├── architecture/
│   │   └── Project_Zenith_V5.1.md     ← Single source of truth (white paper)
│   ├── decisions/
│   │   ├── ADR-001-toolchain-version.md   ← CLOSED: 2025.2
│   │   └── ADR-002-petalinux-strategy.md  ← DECIDE Day 2
│   └── build-log/
│       └── Week1_retrospective.md      ← Write Friday
│
├── hardware/
│   ├── constraints/                    ← .xdc files
│   └── block-design/                  ← Vivado TCL export
│
├── zenith-core/
│   ├── include/zenith/common/
│   │   └── zenith_memory_map.hpp      ← First committed file ✅
│   └── src/
│
├── zenith-silicon/
│   └── cfar/
│       ├── cfar.cpp                   ← Transferred from Chimera ✅
│       ├── cfar.h                     ← Transferred from Chimera ✅
│       ├── hls_build.tcl              ← Transferred from Chimera ✅
│       └── CHIMERA_LINEAGE.md         ← Synthesis record + upgrade notes ✅
│
└── zenith-validator/
    └── matlab/                        ← Golden reference scripts (M2+)
```

---

## 7. Obsidian Knowledge Base Structure

```
Zenith/
├── 00_白皮书/
│   └── Project_Zenith_V5.1.md
├── 01_架构决策记录 (ADR)/
│   ├── ADR-001_工具链版本_CLOSED.md    ← 2025.2, closed today
│   └── ADR-002_PetaLinux策略.md        ← Write Day 2
├── 02_踩坑日志/
│   └── 2026-03-06_Week1_Day1.md        ← Start now
├── 03_硬件参考/
│   ├── AX7020_JTAG_诊断记录.md         ← Fill as you debug
│   └── Chimera_CFAR_综合报告.md        ← Transfer now
├── 04_Substack草稿/
│   └── Post_00_The_Last_Craft.md       ← Draft today
└── 05_资产追踪/
    └── Chimera_to_Zenith_Map.md        ← Fill today
```

---

## 8. Key Numbers to Know (From Chimera)

These are your baseline. Every future synthesis report is compared against these.

| Metric | Chimera Value | Zenith Target | Notes |
|---|---|---|---|
| II | 1 | 1 | Non-negotiable |
| Timing (estimated) | 6.81 ns | ≤ 6.67 ns (150 MHz) | Chimera ran at 100 MHz; Zenith targets 150 MHz |
| BRAM | 0% | 0% for CFAR | Window in FFs, not BRAM |
| DSP48 | 1 | ≤ 154 total across all operators | White paper hard limit |
| LUT | ~3% | — | Baseline established |
| Window size | 25 (8+4+1+4+8) | Parametric via `constexpr` | Already correct architecture |

---

*Document version: V3.0 | 2026-03-06 | Charley Chang*  
*This is a living document. Update every Friday.*  
*JTAG status: unresolved. Does not block Day 1-3 tasks.*
