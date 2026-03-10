/**
 * cfar.h
 * 定义 AXI-Stream 接口结构体与顶层函数声明
 */

#ifndef CFAR_H
#define CFAR_H

#include "radar_defines.h"
#include <hls_stream.h>
#include <ap_axi_sdata.h> // 包含 AXI Side-Channel 定义

// // =========================================================
// // AXI-Stream 接口结构体
// // =========================================================
// // 我们不仅传输数据 (data)，还要传输总线控制信号 (last, user, keep)。
// // 对应 SystemVerilog 接口:
// // .tdata(data), .tlast(last), .tkeep(keep), .tstrb(strb)
// struct axis_t {
//     data_t       data; // 16-bit 信号强度
//     ap_uint<1>   last; // 帧结束标志 (TLAST)
//     ap_uint<1>   user; // 用户自定义 (可选，用于传 error 或 ID)
//     ap_uint<2>   keep; // 字节有效位 (TKEEP)
//     ap_uint<2>   strb; // 选通位 (TSTRB)
// };

// 【核心修复】：废弃自定义 struct，使用官方标准模板！
// hls::axis 模板会自动把 data 映射为 TDATA，把 last 映射为物理引脚 TLAST，并自动生成 TKEEP 掩码。
using axis_t = hls::axis<data_t, 0, 0, 0>;

// =========================================================
// 顶层函数声明
// =========================================================
// in_stream:  原始雷达回波 (RDM Map 或 Range Profile)
// out_stream: 检测结果 (0 或 1，或者是目标的强度)
// threshold:  CFAR 检测阈值系数 (alpha)，通过 AXI-Lite 配置
void cfar_core(
    hls::stream<axis_t>& in_stream,
    hls::stream<axis_t>& out_stream,
    param_t threshold_alpha
);

#endif