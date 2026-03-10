/**
 * radar_defines.h
 * 定义全系统的物理参数与数据类型
 * 首席架构师批注：这里的每一个数字都直接影响 FPGA 的资源占用 (LUT/DSP)。
 */

#ifndef RADAR_DEFINES_H
#define RADAR_DEFINES_H

#include <ap_fixed.h>
#include <ap_int.h>

// =========================================================
// 1. 数据类型定义 (Type Definitions)
// =========================================================

// 为什么用 ap_fixed<16, 8> ?
// W=16: 总位宽16位，对应 Zynq DSP48E1 的输入位宽，效率最高。
// I=8:  整数位8位 (范围 -128 到 127)。
// Q=8:  小数位8位 (精度 1/256 ≈ 0.0039)。
// 物理意义：雷达回波的幅度通常归一化处理，这个范围足够覆盖。
using data_t = ap_fixed<16, 8>; 

// 阈值系数类型，通常需要更高精度
using param_t = ap_fixed<16, 8>;

// =========================================================
// 2. 算法参数 (Algorithm Parameters)
// =========================================================

// CFAR 窗口结构: [ Ref Left | Guard Left | CUT | Guard Right | Ref Right ]
// constexpr在编译时会直接用数字代替，从而提高效率
constexpr int REF_CELLS   = 8;  // 单侧参考单元数 (用于估算噪声底噪)
constexpr int GUARD_CELLS = 2;  // 单侧保护单元数 (防止目标能量泄漏到参考单元)

// 总窗口大小 = (8 + 2) * 2 + 1 = 21
constexpr int WINDOW_SIZE = (REF_CELLS + GUARD_CELLS) * 2 + 1;

// 待检测单元 (Cell Under Test) 在窗口中的索引
constexpr int CUT_IDX     = WINDOW_SIZE / 2;

// 硬件常量：数据包长度 (例如 1024 个采样点为一个 Range Profile)
constexpr int FRAME_SIZE  = 1024;

#endif