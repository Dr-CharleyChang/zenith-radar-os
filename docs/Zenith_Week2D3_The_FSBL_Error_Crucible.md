#
---
tags:
  - Radar
  - Zenith
  - PetaLinux
  - FSBL
  - SD-Boot
  - Debugging
  - FatFs
date: 2026-03-23
author: Charley Chang
milestone: M1 — Zenith-Core Foundation
week: Week 2
day: Day 3
continues_from: "[[Zenith_Week2D2_Hardware_PetaLinux_Log]]"
status: Blocked by FSBL SD Boot FR_NOT_READY (Error 3). Exhausted standard software/hardware workarounds.
---

# Zenith-Core · Week 2, Day 3 — The FSBL "Error 3" Crucible
## Overcoming the BootROM vs. FSBL Paradox and SD Card Driver Failures

> **Day 3 summary in one sentence:**
> Attempted to boot the ALINX AX7020 board using the PetaLinux 2025.2 `BOOT.BIN` via a 32GB SD card, but encountered a persistent FSBL `Error 3 (FR_NOT_READY)`. Executed a 5-track deep-dive debugging campaign spanning FAT32 geometry enforcement (Rufus/Diskpart), hardware CD/WP pin removal, SDIO clock speed capping (Vivado/C-Macros), raw binary surgical extraction (`dd`), and SD card physical partition shrinking — all of which failed to bypass the 2025.2 `xilffs` driver incompatibility.

---

## 0. Day 2 → Day 3 Handoff: The Boot Paradox

Following the successful cross-compilation and packaging of `BOOT.BIN` (5.0MB) in Day 2, the board was powered on in SD boot mode. The UART serial terminal output revealed a critical stall:

```text
Xilinx First Stage Boot Loader
Release 2025.2  Apr  5 2011-23:00:00
Devcfg driver initialized
Silicon Version 3.1
Boot mode is SD
SD: rc= 0
SD: Unable to open file BOOT.BIN: 3
                                   SD_INIT_FAIL
FSBL Status = 0x2802E000102C

This Boot Mode Doesn't Support Fallback
In FsblHookFallback function
```

### The BootROM vs. FSBL Paradox
1. **BootROM works:** The silicon BootROM successfully read the raw FAT clusters from the 32GB SD card and loaded the FSBL into OCM. (Confirmed by the serial print `Xilinx First Stage Boot Loader`).
2. **FSBL hardware init works:** `SD: rc= 0` confirms `disk_initialize()` succeeded. The SD card responded to CMD0/CMD8/ACMD41.
3. **FSBL filesystem fails:** `Unable to open file BOOT.BIN: 3` corresponds to `FR_NOT_READY` in the FatFs (`xilffs`) library. The physical drive is "not ready" for filesystem mounting.

The remainder of Day 3 was dedicated to isolating the discrepancy between how BootROM and the 2025.2 FSBL interact with the ALINX hardware and the 32GB SD card.

---

## 1. Track 1: FAT32 Geometry & Sector Size Mismatch
**Diagnosis:** The 2025.2 `xilffs` library is hardcoded to a maximum sector size of 512 bytes (`FF_MAX_SS = 512`). However, modern 32GB cards formatted by Windows or SDFormatter default to 4096-byte logical sectors. `f_mount()` rejects the BPB (BPB_BytsPerSec = 4096).

**Execution:**
* Avoided `mkfs.vfat /dev/sdX` in WSL2 due to lack of direct physical USB passthrough.
* Attempted Windows CMD: `format E: /FS:FAT32 /A:4096 /Q`. Failed due to Windows hardcoded cluster limits for ~32GB volumes (`Specified cluster size is too small for FAT32`).
* **Solution:** Used **Rufus (Portable)** on Windows.
    * Boot selection: Non-bootable
    * Partition scheme: MBR
    * File system: FAT32
    * Cluster size: 16 Kilobytes (Default)
    * This successfully forced a strict 512-byte logical sector size on the volume.

**Result: ❌ FAILED.** `Error 3` persisted.

---

## 2. Track 2: Hardware CD/WP Pin Constraints
**Diagnosis:** The ALINX board might lack physical connections for Card Detect (CD) and Write Protect (WP) pins, or their polarity is inverted. The FSBL reads the MIO pins, assumes no card is inserted, and immediately throws `FR_NOT_READY`.

**Execution:**
1.  In Vivado: `ZYNQ7 Processing System` -> `MIO Configuration` -> `SD 0`.
2.  Unchecked (Unassigned) both `CD` and `WP` pins.
3.  Regenerated Bitstream, exported new XSA.
4.  In WSL2: `petalinux-config --get-hw-description`, followed by `petalinux-build -c fsbl -x cleansstate`.
5.  Repackaged `BOOT.BIN`.

**Result: ❌ FAILED.** `Error 3` persisted, confirming it is not a false-negative pin reading.

---

## 3. Track 3: The 50MHz High-Speed Trap & Drive Strength
**Diagnosis:** The 2025.2 `XSdPs` driver defaults to negotiating High-Speed mode (50MHz) via CMD6. If the physical trace impedance on the ALINX board cannot support 50MHz, the card drops offline post-negotiation. BootROM succeeds because it stays at ~400kHz or 25MHz.

**Execution:**
1.  **Enhance Physical Drive Strength:** In Vivado MIO Configuration, changed `Speed` for `SD 0` pins (MIO 40-45) from `slow` to `fast`.
2.  **Cap Base Clock:** In Vivado Clock Configuration, reduced `SDIO Requested Frequency` from `100 MHz` to `50 MHz`.
3.  **Force 25MHz Software Macro:** Bypassed the High-Speed switch in the FSBL source code by passing a compiler macro.
    * Ran `petalinux-config -c fsbl`
    * Added `-DXSDPS_DEFAULT_SPEED_MODE` to `FSBL compiler flags`.
4.  Rebuilt Hardware, exported XSA, completely cleaned and rebuilt FSBL in PetaLinux.

**Result: ❌ FAILED.** The combination of MIO fast slew rate, 50MHz clock capping, and 25MHz macro forcing did not resolve `Error 3`. 

---

## 4. Track 4: The "Frankenstein" FSBL Transplant
**Diagnosis:** The 2025.2 `xilffs` driver contains a fundamental bug handling 32GB SDHC cards. We must extract the known-working FSBL from the ALINX factory `BOOT.BIN` (built circa 2019/2023) and stitch it to our new Bitstream and U-Boot.

**Execution:**
1.  Ran `bootgen -arch zynq -read BOOT.BIN` on the factory file to analyze partitions:
    * Partition 0: `zynq_fsbl.elf.0` (Offset: 0x1700 / 5888 bytes, Size: 0x18008 / 98312 bytes).
2.  Ran `bootgen -arch zynq -read BOOT.BIN -split bin`. Tool execution succeeded but generated no files (a known bug/behavior in 2025.1 Bootgen).
3.  **Surgical Extraction:** Used Linux `dd` to physically carve the FSBL out:
    ```bash
    dd if=BOOT.BIN of=factory_fsbl.bin bs=1 skip=5888 count=98312
    ```
4.  Created a custom `frankenstein.bif` to assign the entry points:
    ```text
    the_ROM_image:
    {
        [bootloader, load=0x00000000, startup=0x00000000] factory_fsbl.bin
        /home/chimera/zenith-petalinux/images/linux/system.bit
        /home/chimera/zenith-petalinux/images/linux/u-boot.elf
    }
    ```
5.  Packaged via `bootgen -arch zynq -image frankenstein.bif -w -o BOOT.BIN`.

**Result: ❌ FAILED (Silent Boot).** The board powered on with zero serial output. **Reason:** The Zynq BootROM validates partition headers, checksums, and boot vectors. A raw `dd` slice breaks the BIF header alignment expected by the BootROM, causing it to discard the payload immediately. We need the original factory `.elf` file, not a `.bin` slice.

---

## 5. Track 5: The "Stomach Stapling" (SDHC 32GB Boundary Bypass)
**Diagnosis:** 32GB is the absolute logical maximum for FAT32 / SDHC standard. The FSBL FatFs implementation may experience variable overflow or strict geometry validation failure when parsing the partition table of a max-capacity card.

**Execution:**
Instead of modifying the FSBL, we modified the physical perception of the card.
1.  In Windows `diskpart`:
    * `select disk X` -> `clean` (Wiped all partitions)
    * `create partition primary size=4096` (Created a fake 4GB partition)
    * `format fs=fat32 quick` -> `active`
2.  This forced the 32GB SD card to present itself to the FSBL as a legacy 4GB SDHC card.
3.  Copied the 2025.2 `BOOT.BIN` to the 4GB volume.

**Result: ❌ FAILED.** The `Error 3` persisted, strongly suggesting the incompatibility lies in the lowest level of SD CMD handshakes in the 2025.2 driver, rather than filesystem boundaries.

---

## 6. Next Steps & Handoff to AI Consult (Claude)

**Current Status:** Hardware (DDR, UART) is healthy. PetaLinux OS build is complete. Blocked entirely by Xilinx 2025.2 FSBL `xilffs` compatibility.

**Target Inquiries for Claude:**
1. **The 2025.2 Driver Bug:** Are there documented errata for PetaLinux 2025.2 regarding `XSdPs` or `xilffs` returning `FR_NOT_READY` on SDHC cards? Does the driver require patching `ff.c` to handle specific MBR offsets?
2. **Polled Transfer Mode:** Does the 2025.2 FSBL require the explicit definition of `SD_POLLED_TRANSFER` to function correctly without OS-level interrupts?
3. **The ELF Recovery:** Since `dd` truncation failed BootROM validation, what is the mathematically correct way to extract a valid, bootable `.elf` from a legacy `BOOT.BIN` using Xilinx tools (to execute the Transplant Strategy)?

*Document version: Week 2 Day 3 Final · 2026-03-23 · Charley Chang*
```

---

Chief Engineer，这份日志涵盖了我们今晚的所有骚操作，并且用极其专业的逻辑解释了每一次失败的底层原因。把它丢给 Claude，让他看看我们在第一线都干了些什么硬核排查，期待他能从 Xilinx 源码库里翻出最后的解药。好好休息！明天再战！