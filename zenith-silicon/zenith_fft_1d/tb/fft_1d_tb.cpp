// -----------------------------------------------------------------------------
// fft_1d_tb.cpp — The C-Simulation Testbench
// Annotated based on Zenith_Week3_Day1_HLS_FFT_Kernel_Design.md
//
// The testbench is not synthesized. It is a C++ program that calls fft_1d_top()
// as if it were a normal function. The principle: never run synthesis on unverified code.
// -----------------------------------------------------------------------------

#include <iostream>
#include "fft_1d.hpp"

int main() {
  // The string arguments "sim_in" and "sim_out" are debug names.
  // In C-Simulation, if an hls::stream overflows or underflows, the simulator
  // prints the stream name in the error message, making it easy to identify the bug.
  hls::stream<axis_iq_t> sim_in_stream("sim_in");
  hls::stream<axis_iq_t> sim_out_stream("sim_out");
  ap_uint<32> dummy_scaling = 0x00000000;

  // 1. Push 1024 samples into the input stream
  for (int i = 0; i < FFT_LENGTH; i++) {
    axis_iq_t pkt_in;
    
    // The test vector uses a ramp for I (expected to pass through).
    ap_int<16> test_I = i;        // Simple ramp: 0, 1, 2...
    
    // The test vector uses i + 100 for Q (expected to be zeroed by the placeholder).
    // This asymmetry ensures that if I appears in Q or vice versa, we can detect 
    // if extract/pack functions correctly separate the I and Q bit lanes.
    // The non-zero, non-ramp baseline also guards against index confusion.
    ap_int<16> test_Q = i + 100;  // Should be dropped by the hardware

    pkt_in.data = pack_iq(test_I, test_Q);
    
    // TLAST is the most safety-critical signal in the write stage.
    // It tells the AXI DMA's S2MM channel: "this is the last byte of the current DMA transfer."
    pkt_in.last = (i == FFT_LENGTH - 1) ? 1 : 0;
    
    // hls::stream.write() manages the TVALID assertion automatically.
    sim_in_stream.write(pkt_in);
  }

  // 2. Execute the HLS Top Function
  // The HLS simulator executes this C++ code at software speed and verifies 
  // functional correctness before any hardware synthesis is attempted.
  fft_1d_top(sim_in_stream, sim_out_stream, dummy_scaling);

  // 3. Verify Output
  int error_count = 0;
  for (int i = 0; i < FFT_LENGTH; i++) {
    // hls::stream.read() waits (stalls the pipeline) until TREADY is asserted.
    axis_iq_t pkt_out = sim_out_stream.read();

    ap_int<16> out_I = extract_i(pkt_out.data);
    ap_int<16> out_Q = extract_q(pkt_out.data);

    // Verification logic
    // We expect the I output to exactly match the input ramp.
    if (out_I != i) {
      std::cerr << "Mismatch I at index " << i << ": expected " << i << ", got "
                << out_I << "\n";
      error_count++;
    }
    
    // We expect the Q output to be strictly zeroed by the placeholder.
    // Any failure unambiguously means the DATAFLOW pipeline is corrupting data.
    if (out_Q != 0) {
      std::cerr << "Mismatch Q at index " << i << ": expected 0, got " << out_Q
                << "\n";
      error_count++;
    }
    
    // TLAST is verified explicitly. The testbench checks that TLAST is asserted on the last sample.
    // A kernel that produces correct data but wrong TLAST will hang the DMA on hardware.
    // (Note: A known limitation to address at M3 is checking for premature assertion).
    if (i == FFT_LENGTH - 1 && pkt_out.last != 1) {
      std::cerr << "ERROR: TLAST not asserted on final sample!\n";
      error_count++;
    }
  }

  if (error_count == 0) {
    std::cout
        << "SUCCESS: Day 1 Plumbing C-Sim Passed! TLAST and Q=0 verified.\n";
    return 0;  // Make passes
  } else {
    std::cout << "FAILED with " << error_count << " errors.\n";
    return 1;  // Make fails
  }
}
