#pragma once
// =============================================================================
// zenith_memory_map.hpp
// Zenith-Radar OS — Physical Memory Map
// =============================================================================
// ⚠️  SINGLE SOURCE OF TRUTH. Edit here only.
//     All DMA drivers, device tree overlays, and Vivado address maps
//     must agree with these values.
//
// CONFIRMED: ALINX AX7020 factory bring-up 2026-03-10
// Source:    dmesg + /proc/iomem, Linux 4.9.0-xilinx
// =============================================================================

#include <cstdint>
#include <cstddef>

// -----------------------------------------------------------------------------
// DDR System RAM: 0x00000000 - 0x3FFFFFFF (1GB)
// CMA Reserved:   0x3F000000 - 0x3FFFFFFF (16MB, top of DDR)
// AXI DMA (PL):  0x43000000 (factory reference, Zenith will reuse)
// -----------------------------------------------------------------------------

constexpr uintptr_t CMA_PHYS_BASE   = 0x3F00'0000;  // 16MB CMA base
constexpr size_t    CMA_TOTAL_SIZE  = 0x0100'0000;  // 16MB total

// PS → PL: waveform config / TX waveform buffer
constexpr size_t    TX_OFFSET       = 0x0000'0000;
constexpr size_t    TX_SIZE         = 0x0040'0000;  // 4MB

// PL → PS: IQ data / point cloud
constexpr size_t    RX_OFFSET       = 0x0040'0000;
constexpr size_t    RX_SIZE         = 0x0040'0000;  // 4MB

// DMA Scatter-Gather Buffer Descriptor ring
constexpr size_t    BD_OFFSET       = 0x0080'0000;
constexpr size_t    BD_SIZE         = 0x0000'1000;  // 4KB

// Zenoh zero-copy track output buffer
constexpr size_t    TRACK_OFFSET    = 0x0080'1000;
constexpr size_t    TRACK_SIZE      = 0x0010'0000;  // 1MB

// Derived absolute physical addresses (use these in mmap calls)
constexpr uintptr_t TX_PHYS_BASE    = CMA_PHYS_BASE + TX_OFFSET;
constexpr uintptr_t RX_PHYS_BASE    = CMA_PHYS_BASE + RX_OFFSET;
constexpr uintptr_t BD_PHYS_BASE    = CMA_PHYS_BASE + BD_OFFSET;
constexpr uintptr_t TRACK_PHYS_BASE = CMA_PHYS_BASE + TRACK_OFFSET;

// AXI DMA base address in PL (Vivado Block Design — to be confirmed in M1)
constexpr uintptr_t AXI_DMA_BASE    = 0x4300'0000;  // factory reference