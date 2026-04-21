// -----------------------------------------------------------------------------
// fft_1d.hpp — The Hardware Contract
// Annotated based on Zenith_Week3_Day1_HLS_FFT_Kernel_Design.md
// -----------------------------------------------------------------------------

// Standard include guard. Prevents the header from being parsed twice during
// synthesis, which would cause duplicate type definitions that confuse the
// HLS type-checker.
#pragma once

// Xilinx Arbitrary Precision (AP) libraries — not standard C++.
// These ship with Vitis HLS and are understood by the synthesis engine.

// ap_axiu<W,U,I,D> models all 8 AXI-Stream signals in one C++ type
#include "ap_axi_sdata.h"
// ap_fixed<W,I,...> for fixed-point numbers; radar ADC data is fixed-point, 
// native type prevents accidental float inference
#include "ap_fixed.h"
// ap_int<N>, ap_uint<N> for N-bit signed/unsigned integers; 
// hardware infers exactly N flip-flops
#include "ap_int.h"
// hls::stream<T> is a synthesizable FIFO queue; the only correct way 
// to model AXI-Stream dataflow in HLS
#include "hls_stream.h"

// constexpr tells both the C++ compiler and the HLS synthesis engine that
// these values are compile-time constants. The synthesis engine uses them to
// statically size arrays, loop bounds, and TLAST generation counters.

// 1024 = 2^10 is required for a radix-2 Cooley-Tukey FFT. 
// Non-power-of-2 sizes require more complex DIF/DIT architectures with much higher DSP48 consumption.
constexpr int FFT_LENGTH = 1024;

// The M1 AXI DMA is configured for a 32-bit stream width (AXI4-Stream Data Width = 32).
// This is locked in the Block Design. We pack 16-bit I and 16-bit Q into this single 32-bit word.
constexpr int IQ_STREAM_WIDTH = 32;

// Q1.15 fixed-point format (W=16, I=1): 1 sign bit, 15 fractional bits.
// AP_TRN (truncation) drops excess fractional bits without rounding; cheaper in hardware 
// (zero extra logic) and introduces a deterministic downward bias of at most -0.5 LSB.
// AP_WRAP (wrap-around) is cheaper than saturation (zero extra logic vs. a comparator + clamp mux).
// Overflow should never occur at the ADC input stage if properly scaled.
using iq_sample_t = ap_fixed<16, 1, AP_TRN, AP_WRAP>;

// AXI-Stream payload modeling the complete AXI4-Stream packet.
// DataWidth=32, TUSERWidth=1, TIDWidth=1, TDESTWidth=1
typedef ap_axiu<IQ_STREAM_WIDTH, 1, 1, 1> axis_iq_t;

// Inline bit-slicing functions
// The .range(hi, lo) method is an AP library operator that extracts a contiguous bit slice.
// It compiles to a wire connection in hardware — literally zero logic gates.

// Why ap_uint<32> input but ap_int<16> output?
// TDATA is an unsigned bit container, but the I-sample extracted from it IS a signed Q1.15 value.
inline ap_int<16> extract_i(ap_uint<32> tdata) {
// #pragma HLS INLINE forces the HLS tool to dissolve this function's boundary 
// and merge its logic into whatever calls it, preventing pipeline optimizations from being blocked.
#pragma HLS INLINE
  // Routes wires directly from TDATA bits 15:0 to the output port.
  return tdata.range(15, 0);
}

inline ap_int<16> extract_q(ap_uint<32> tdata) {
#pragma HLS INLINE
  // Extracts bits 31:16 for the Q component.
  return tdata.range(31, 16);
}

inline ap_uint<32> pack_iq(ap_int<16> i_val, ap_int<16> q_val) {
#pragma HLS INLINE
  ap_uint<32> word;
  word.range(15, 0) = i_val;
  word.range(31, 16) = q_val;
  return word;
}

// Top-level function declaration.
// hls::stream is passed by reference, not by value. In hardware, a stream is a wire connection 
// between modules. Passing by value would cause the HLS tool to error or create spurious FIFO copies.
void fft_1d_top(hls::stream<axis_iq_t>& in_stream,
                hls::stream<axis_iq_t>& out_stream,
                ap_uint<32> scaling_schedule);
