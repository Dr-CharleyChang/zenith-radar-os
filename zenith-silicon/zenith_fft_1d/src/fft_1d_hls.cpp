#include "fft_1d.hpp"
#include <ap_fixed.h>
#include <complex>

void stage_process_fft_real(ap_int<16> in_bufI[FFT_LENGTH],
                            ap_int<16> in_bufQ[FFT_LENGTH],
                            ap_int<16> out_bufI[FFT_LENGTH],
                            ap_int<16> out_bufQ[FFT_LENGTH],
                            ap_uint<16> scaling_schedule) {
  // 1. 声明最纯粹的硬件 FIFO 流
  hls::stream<std::complex<ap_fixed<16, 1>>> fft_in_stream("fft_in_stream");
  hls::stream<std::complex<ap_fixed<16, 1>>> fft_out_stream("fft_out_stream");
  hls::stream<hls::ip_fft::status_t<ZenithFFTConfig>> fft_status_stream(
      "fft_status_stream");
  hls::stream<hls::ip_fft::config_t<ZenithFFTConfig>> fft_config_stream(
      "fft_config_stream");

#pragma HLS STREAM variable = fft_in_stream depth = 1024
#pragma HLS STREAM variable = fft_out_stream depth = 1024
#pragma HLS STREAM variable = fft_config_stream depth = 2
#pragma HLS STREAM variable = fft_status_stream depth = 2

  // 2. 发送配置快照入流
  hls::ip_fft::config_t<ZenithFFTConfig> fft_config;
  fft_config.setDir(1);
  fft_config.setSch(scaling_schedule);
  fft_config_stream.write(fft_config);

// 3. 极速吸入流水线
feed_loop:
  for (int i = 0; i < FFT_LENGTH; i++) {
#pragma HLS PIPELINE II = 1
    ap_fixed<16, 1> real_val;
    ap_fixed<16, 1> imag_val;
    real_val.range() = in_bufI[i];
    imag_val.range() = in_bufQ[i];
    std::complex<ap_fixed<16, 1>> s(real_val, imag_val);
    fft_in_stream.write(s);
  }

  // 4. 执行核心硬件 FFT
  hls::fft<ZenithFFTConfig>(fft_in_stream, fft_out_stream, fft_status_stream,
                            fft_config_stream);

  // 5. 魔法子弹：非阻塞清除状态（防止死锁）
  hls::ip_fft::status_t<ZenithFFTConfig> dummy_status;
  fft_status_stream.read_nb(dummy_status);

// 6. 极速吐出流水线
drain_loop:
  for (int i = 0; i < FFT_LENGTH; i++) {
#pragma HLS PIPELINE II = 1
    // 因为修好了顶层 Bug，这里我们绝对有信心能读到 1024 个点
    std::complex<ap_fixed<16, 1>> s = fft_out_stream.read();
    out_bufI[i] = s.real().range();
    out_bufQ[i] = s.imag().range();
  }
}