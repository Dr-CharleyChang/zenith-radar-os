/**
 * cfar.cpp
 * CA-CFAR (Cell Averaging) 核心硬件实现
 * * 核心逻辑：
 * 1. 数据流进入 Line Buffer (移位寄存器)。
 * 2. 并行读取窗口内的所有参考单元。
 * 3. 计算噪声平均值 (Noise Floor)。
 * 4. CUT (Cell Under Test) > 噪声 * Alpha ? 目标 : 0。
 */

#include "cfar.h"

void cfar_core(hls::stream<axis_t> &in_stream, hls::stream<axis_t> &out_stream,
               param_t threshold_alpha) {
// ------------------------------------------------------
// 1. 接口指令 (Pragmas)
// ------------------------------------------------------
// 定义 AXI-Stream 接口，硬件上生成 tdata, tvalid, tready, tlast 等信号
#pragma HLS INTERFACE axis port = in_stream
#pragma HLS INTERFACE axis port = out_stream

// 定义 AXI-Lite 接口，生成寄存器映射，让 ARM CPU 可以动态配置 threshold
#pragma HLS INTERFACE s_axilite port = threshold_alpha bundle = CTRL
#pragma HLS INTERFACE s_axilite port = return bundle = CTRL

// 核心指令：流水线模式。II=1 意味着每个时钟周期处理一个数据点。
#pragma HLS PIPELINE II = 1

  // ------------------------------------------------------
  // 2. 内部状态寄存器 (Internal Storage)
  // ------------------------------------------------------
  // 静态数组保持状态，模拟硬件移位寄存器链
  static axis_t window[WINDOW_SIZE];

// 【关键】Array Partition:
// 将数组完全打散成 WINDOW_SIZE 个独立的寄存器。
// 如果不加这句，它就是一块 BRAM，一周期只能读2次，无法做单周期 CFAR。
#pragma HLS ARRAY_PARTITION variable = window complete

  // ------------------------------------------------------
  // 3. 逻辑处理
  // ------------------------------------------------------
  axis_t input_sample;
  axis_t output_sample;

  // 非阻塞读取：检查输入 FIFO 是否有数据
  // 硬件对应： if (in_stream_tvalid && in_stream_tready)
  if (in_stream.read_nb(input_sample)) {

  // --- Step A: 移位逻辑 (Shift Register) ---
  // 模拟数据在延迟线上的流动。
  // 由于 Unroll，这在硬件上是并行的连线，没有时间开销。
  Shift_Loop:
    for (int i = WINDOW_SIZE - 1; i > 0; i--) {
#pragma HLS UNROLL
      window[i] = window[i - 1];
    }
    window[0] = input_sample; // 新数据进入窗口最右端

    // --- Step B: CFAR 计算核心 ---
    // 只有当窗口填满后计算才有意义 (这里为了简化略去了 fill_count 逻辑)

    ap_fixed<24, 14> sum_noise = 0;

  // 遍历窗口，累加参考单元，跳过保护单元和 CUT
  // 硬件对应：加法树 (Adder Tree)，在一个周期内完成所有加法
  Calc_Loop:
    for (int i = 0; i < WINDOW_SIZE; i++) {
#pragma HLS UNROLL

      // 判断是否是保护单元或 CUT
      // 这里的 Abs 逻辑在编译时会变成固定的电路连线选择
      int dist = (i > CUT_IDX) ? (i - CUT_IDX) : (CUT_IDX - i);

      if (dist > GUARD_CELLS) {
        sum_noise += window[i].data;
      }
    }

    // --- Step C: 平均值与判决 ---
    // Noise Floor = Sum / (2 * REF_CELLS)
    // 这里的除法如果是2的幂次，会被优化为移位。如果不是，会消耗 DSP 除法器。
    // 建议 REF_CELLS 设为 8, 16 等。
    data_t noise_level = (data_t)(sum_noise / (2 * REF_CELLS));
    data_t threshold_val = noise_level * threshold_alpha;

    // CUT (Cell Under Test) 位于窗口中心
    data_t cut_val = window[CUT_IDX].data;

    // --- Step D: 输出构造 ---
    output_sample = window[CUT_IDX];

    // 【首席架构师修正】：
    // 绝对不要让控制信号进入延迟线！将 TLAST 和 TKEEP
    // 信号与输入端(window[0])强绑定。 这样可以确保输出帧的长度与输入帧绝对一致
    // (都是 1024)，完美配合 DMA。 代价是：雷达帧最后的 CUT_IDX
    // 个点将因为来不及滑入中心而被截断，这在雷达工程中是完全可以接受的边界效应。
    output_sample.last = window[0].last;
    output_sample.keep = window[0].keep;
    output_sample.strb = window[0].strb;

    if (cut_val > threshold_val) {
      output_sample.data = cut_val; // 检测到目标
    } else {
      output_sample.data = 0; // 噪声抑制
    }

    out_stream.write(output_sample);
  }
}