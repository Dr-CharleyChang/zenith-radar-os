---
tags:
  - Radar
  - Zenith
  - M1-Complete
  - Debugging
  - PetaLinux
  - Zynq-7000
  - AXI-DMA
date: 2026-04-07
author: Charley Chang
milestone: M1 — Zenith-Core Foundation
week: Week 2
day: Day 3
continues_from: "[[Zenith_Week2D2_Hardware_PetaLinux_Log]]"
status: M1 Milestone COMPLETED. Hardware validated. OS booted. DMA loopback verified.
---
# Zenith-Core · Week 2, Day 3 — The M1 Victory
## Conquering the Hardware-Software Divide: SD Boot, Presets, and AXI DMA Closure

> **Day 3 summary in one sentence:**
> Resolved the fatal PetaLinux 2025.2 `Error 3 (FR_NOT_READY)` SD boot failure by abandoning software hacks and fixing root physical mismatches (Bank 1 at 1.8V, Speed Grade -2) using ALINX factory TCL presets; overcame PetaLinux login security changes, cross-compiled a static C++ payload, corrected a fatal 1GB AXI address out-of-bounds error, and successfully achieved 100MHz DMA zero-copy loopback.

---

## 1. The FSBL "Error 3" Crucible and the Hardware Truth

The session began blocked by a persistent BootROM vs. FSBL paradox: the BootROM could read the 32GB SD card to load the FSBL, but the PetaLinux 2025.2 FSBL failed to mount the FAT32 filesystem (`Unable to open file BOOT.BIN: 3`).

### 1.1 The Failed Software Tracks
Extensive efforts to bypass the 2025.2 `xilffs` driver incompatibility at the software level failed:
* **Track 1:** FAT32 geometry enforcement (16KB clusters, 512B logical sectors via Rufus).
* **Track 2:** Removing CD/WP hardware pin dependencies in Vivado.
* **Track 3:** Capping SDIO clock to 50MHz and forcing `-DXSDPS_DEFAULT_SPEED_MODE` to bypass CMD6 high-speed negotiation.
* **Track 4:** "Frankenstein" binary stitching (failed due to BootROM zero-address validation).
* **Track 5:** Shrinking the SD card partition to 4GB to bypass SDHC capacity boundaries.
* **Track 6:** Device Tree overrides (`no-1-8-v`, `non-removable`) — failed because Zynq-7000 bare-metal FSBL does not parse the device tree at early boot.

### 1.2 The Root Cause: Physical Hardware Mismatch
The breakthrough occurred when reverse-engineering the ALINX factory `ps_config.tcl` scripts. The custom Vivado Block Design used Xilinx default parameters, which clashed fatally with the ALINX board's physical layout:
1.  **I/O Voltage Mismatch:** The custom design assumed Bank 1 was LVCMOS 3.3V. The factory board physically wired Bank 1 to **LVCMOS 1.8V**. The 2025.2 FSBL attempted 1.8V UHS-I negotiation, but the misconfigured Zynq MIO pins crashed the SD card interface.
2.  **Silicon Speed Grade:** The Vivado project targeted `xc7z020clg400-1`. The physical chip was a `-2` speed grade.
3.  **CD Pin:** The card detect pin was physically hardwired to MIO 47.

### 1.3 The Resolution: Genetic Injection
The fixes were entirely physical, applied within Vivado:
1.  Upgraded the project device to `xc7z020clg400-2`.
2.  Imported the `alinx_ax7020_preset.tcl` to enforce 1.8V Bank 1, correct MIO assignments, and ALINX DDR timing calibrations.
3.  **Architectural Guardrail:** Manually restored `FCLK_CLK0` to **100MHz** (overriding the preset's 50MHz) to preserve AXI DMA bandwidth required for M2/M3 radar data throughput.
4.  Re-exported the XSA, cleaned all PetaLinux hacks, and repackaged `BOOT.BIN`.

**Result:** The board successfully booted PetaLinux OS.

---

## 2. OS Bring-Up and Cross-Compilation Hurdles

### 2.1 PetaLinux 2025.2 Security Changes
The standard `root/root` login failed. PetaLinux versions >= 2022.1 enforce a secure-by-default policy:
* Default user is `petalinux`.
* First boot forces a password change.
* Root access requires executing `sudo su` from the `petalinux` user shell.

### 2.2 C++ Runtime Dependency (libstdc++)
The dynamic C++ validation binary (`zenith_m1_validate`) failed to execute with `error while loading shared libraries: libstdc++.so.6`. To avoid rebuilding the PetaLinux rootfs to include the C++ standard library, the validation payload was quickly adapted via static compilation:
```bash
arm-linux-gnueabihf-g++ -std=c++20 -O2 -Wall -static main.cpp -o zenith_m1_validate
````

---

## 3. The Final DMA Loopback Verification

The initial execution of the DMA payload locked up, polling indefinitely on `Transferring...`.

### 3.1 The AXI DECERR Bug

A review of the Day 1 memory map revealed a fatal out-of-bounds address assignment. The ALINX AX7020 has 512 MB of DDR3 (`0x0000_0000` to `0x1FFF_FFFF`). The Day 2 validation code assigned:

- `TX_PHYS_BASE = 0x3F000000` (1GB range)
- `RX_PHYS_BASE = 0x3F400000` (1GB range)

The DMA engine, attempting to write to a non-existent physical memory bank via AXI HP0, triggered an AXI `DECERR` (Decode Error) and permanently halted its internal state machine.

### 3.2 The M1 Success Payload

The validation payload was corrected and enhanced with hardware resilience:

1. Addresses moved to a safe 480MB boundary (`0x1E000000`).
2. Added DMA S2MM/MM2S software reset pulses (`0x4` to `DMACR`) to clear historical halts.
3. Added `__builtin___clear_cache()` to force the ARM Cortex-A9 to read the fresh DMA data from DDR instead of stale L2 cache lines.
4. Added a polling timeout with status register (`0x04`, `0x34`) dumping.

**Final Output:**

Plaintext

```
zenith-petalinux:/home/petalinux# ./zenith_m1_validate
Transferring...
ZENITH M1 SUCCESS: Loopback Verified!
```

## 4. Key Decisions Log (M1 Final ADR)

|**Decision**|**Chosen**|**Rejected**|**Rationale**|
|---|---|---|---|
|Hardware Baseline|ALINX `ps_config.tcl` presets|Custom Xilinx defaults|Resolved Bank 1 1.8V discrepancy and SD CMD11 crashes.|
|Speed Grade|`xc7z020clg400-2`|`xc7z020clg400-1`|Matches physical silicon; resolves hidden timing violations.|
|Validation Execution|Static Compilation (`-static`)|OS Rootfs inclusion|Fastest path to validation without triggering a 1-hour PetaLinux rootfs rebuild.|
|DMA TX/RX Addr|`0x1E000000`|`0x3F000000`|AX7020 is limited to 512MB (`0x1FFFFFFF`). Previous address caused AXI DECERR.|
|PL Clock|100MHz|50MHz (ALINX default)|50MHz halves AXI bandwidth (200MB/s). 100MHz (400MB/s) is necessary for radar data.|

**M1 IS COMPLETE.** Proceeding to M2: HLS Radar Kernels.

````

---

### 第二部分：GitHub Commit 规划

既然 M1 已经完美收官，你的 Git 仓库需要一次极具专业性的提交。

**1. 暂存所有文件:**
```bash
git add hardware/ software/ petalinux/ docs/
````

**2. 提交命令 (Copy & Paste):**

Bash

```
git commit -m "feat(M1): achieve full milestone closure with successful DMA zero-copy loopback" -m "This commit finalizes the Zenith M1 Milestone. 

Hardware Fixes:
- Upgraded target device to xc7z020clg400-2 to match physical silicon.
- Imported ALINX factory TCL presets, resolving the Bank 1 1.8V / 3.3V discrepancy that caused the FSBL FR_NOT_READY (Error 3) SD boot failure.
- Restored FCLK_CLK0 to 100MHz to guarantee 400MB/s AXI DMA bandwidth.

Software & Validation:
- Successfully booted PetaLinux 2025.2.
- Fixed fatal AXI DECERR in DMA payload by migrating TX/RX physical addresses to 0x1E000000 (within 512MB limits).
- Implemented static C++ compilation for the validation binary to bypass missing libstdc++ in minimal rootfs.
- Added L2 cache invalidation and DMA hardware reset logic.
- Validation output: 'ZENITH M1 SUCCESS: Loopback Verified!'"
```

---

### 第三部分：Substack Post #1 博客规划

你这几天的经历，简直是嵌入式技术圈最受欢迎的“破案类”硬核长文素材。以下是为你规划的 Substack 博文结构：

**标题 (Title):**

_The 1.8V Betrayal: How a 32GB SD Card Taught Me the Hardest Lesson in FPGA Hardware-Software Co-Design_

（1.8V 的背叛：一张 32GB SD 卡如何教给我 FPGA 软硬协同设计最惨痛的一课）

**副标题 (Subtitle):**

_Building a Custom Radar OS (Part 1) — From an impossible boot loop to a 100MHz zero-copy DMA loopback._

**正文大纲 (Outline):**

1. **The Hook (引子):**
    
    - 贴上那张 `SD: Unable to open file BOOT.BIN: 3` 的截图。
        
    - 讲述背景：我在用 AI 从零手搓一个雷达操作系统，硬件综合完美，时序收敛完美（甚至为了时序降频到了 100MHz），但就在最后一步“从 SD 卡启动”时，板子死活不认。
        
2. **The False Leads (迷雾追踪):**
    
    - 介绍我们在软件层面的疯狂挣扎：FAT32 扇区对齐、设备树屏蔽 `no-1-8-v`、甚至尝试在 Windows `diskpart` 里给 SD 卡做“缩胃手术”划出 4GB 分区。
        
    - _技术干货：_ 解释 BootROM 能读，但 FSBL 读不了的底层 Paradox（阶段性驱动差异）。
        
3. **The Smoking Gun (致命真相):**
    
    - 揭秘：这不是软件 Bug，是极其隐蔽的**物理硬件信息差**！
        
    - 贴上 Vivado MIO 截图对比：原厂板子的 Bank 1 是 **1.8V**，而我的自定义工程默认为 **3.3V**！2025.2 的先进驱动跑去拉高电平，直接把 SD 卡干休眠了。
        
    - _经验总结：_ 不要迷信纯软件思维，硬件配置（Speed Grade -2, 预设 TCL）是一切的根基。
        
4. **The Final Sprint (最后一公里):**
    
    - 如何用静态编译绕过缺失的 C++ 动态库。
        
    - 如何抓出导致 DMA 死锁的 AXI `DECERR`（512MB RAM 被写了 1GB 的地址）。
        
    - 贴上最终的 C++ 零拷贝核心代码（展示 `__builtin___clear_cache` 的魅力）。
        
5. **The Result (胜利结语):**
    
    - 贴上终端打印出 `ZENITH M1 SUCCESS` 的截图。
        
    - 预告 Part 2：M2 阶段将引入 HLS 射频处理内核，真正的数学与硅片的碰撞即将开始。
        

---

Chief Engineer，从一堆报错日志到如今完整的系统底座，你的执行力令人敬佩。把代码推上去，把博客发出来，好好休息一下，我们随时准备进军 **M2：高层次综合 (HLS) 硬件加速内核设计**！