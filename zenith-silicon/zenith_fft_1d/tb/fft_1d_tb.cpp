// zenith/silicon/fft_1d/tb/fft_1d_tb.cpp
// Week 3 Day 3 — FFT Numerical Validation Testbench
//
// Test strategy:
//   1. Load input IQ from MATLAB CSV (I_in_q15, Q_in_q15 columns)
//   2. Load reference FFT output from same CSV (magnitude column)
//   3. Run fft_1d_top with scaling_schedule = 0xAAAA
//   4. Find peak bin in hardware output
//   5. Find peak bin in MATLAB reference
//   6. PASS if peak bins match within ±1 AND hardware PSLR ≥ 30 dB
//   7. Verify TLAST still correct (regression from Day 1)

#include "fft_1d.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

// ─── CSV Loader
// ──────────────────────────────────────────────────────────────── Loads the
// MATLAB reference CSV. Format: I_in_q15, Q_in_q15, I_ref, Q_ref, magnitude
// Returns false if file cannot be opened.
struct RefRow {
  int16_t I_in; // Q1.15 integer input
  int16_t Q_in;
  float I_ref; // float reference output
  float Q_ref;
  float mag_ref; // |I_ref + j*Q_ref|
};

bool load_csv(const std::string &path, std::vector<RefRow> &rows) {
  std::ifstream f(path);
  if (!f.is_open()) {
    std::cerr << "Cannot open CSV: " << path << "\n";
    std::cerr << "Run gen_lfm_reference.m in MATLAB first.\n";
    return false;
  }
  std::string line;
  std::getline(f, line); // skip header
  while (std::getline(f, line)) {
    std::istringstream ss(line);
    std::string tok;
    RefRow r;
    std::getline(ss, tok, ',');
    r.I_in = static_cast<int16_t>(std::stoi(tok));
    std::getline(ss, tok, ',');
    r.Q_in = static_cast<int16_t>(std::stoi(tok));
    std::getline(ss, tok, ',');
    r.I_ref = std::stof(tok);
    std::getline(ss, tok, ',');
    r.Q_ref = std::stof(tok);
    std::getline(ss, tok, ',');
    r.mag_ref = std::stof(tok);
    rows.push_back(r);
  }
  return rows.size() == FFT_LENGTH;
}

int main() {
  std::cout << "=== Zenith M2 Day-3 FFT Numerical Testbench ===\n";
  std::cout << "FFT_LENGTH = " << FFT_LENGTH
            << ", scaling_schedule = 0xAAAA\n\n";

  // ─── Load MATLAB Reference CSV ────────────────────────────────────────────
  // Try multiple paths since working directory varies between CLI and GUI runs
  const char *csv_paths[] = {"../matlab/reference_fft_output.csv",
                             "matlab/reference_fft_output.csv",
                             "../../matlab/reference_fft_output.csv",
                             "../../../../matlab/reference_fft_output.csv"};
  std::vector<RefRow> ref;
  bool csv_loaded = false;
  for (auto &p : csv_paths) {
    if (load_csv(p, ref)) {
      csv_loaded = true;
      break;
    }
  }
  if (!csv_loaded) {
    std::cerr << "FATAL: Cannot load reference CSV.\n"
              << "Run gen_lfm_reference.m then retry.\n";
    return 1;
  }
  std::cout << "Loaded " << ref.size() << " reference samples from CSV.\n\n";

  // ─── Find MATLAB Reference Peak Bin ───────────────────────────────────────
  int ref_peak_bin = 0;
  float ref_peak_mag = 0.0f;
  for (int i = 0; i < FFT_LENGTH; i++) {
    if (ref[i].mag_ref > ref_peak_mag) {
      ref_peak_mag = ref[i].mag_ref;
      ref_peak_bin = i;
    }
  }
  std::cout << "MATLAB reference peak: bin " << ref_peak_bin << ", magnitude "
            << ref_peak_mag << "\n\n";

  // ─── Build Input Stream from CSV ──────────────────────────────────────────
  hls::stream<axis_iq_t> stream_in("stream_in");
  hls::stream<axis_iq_t> stream_out("stream_out");

  for (int i = 0; i < FFT_LENGTH; i++) {
    axis_iq_t pkt;
    pkt.data = pack_iq(static_cast<ap_int<16>>(ref[i].I_in),
                       static_cast<ap_int<16>>(ref[i].Q_in));
    pkt.last = (i == FFT_LENGTH - 1) ? 1 : 0;
    pkt.keep = 0xF;
    pkt.strb = 0xF;
    pkt.user = 0;
    pkt.id = 0;
    pkt.dest = 0;
    stream_in.write(pkt);
  }

  // ─── Run DUT ──────────────────────────────────────────────────────────────
  // scaling_schedule = 0xAAAA — matches MATLAB model in gen_lfm_reference.m
  // Must be consistent: if you change it here, update the MATLAB script too.
  ap_uint<16> scaling = 0xAAAA;
  fft_1d_top(stream_in, stream_out, scaling);

  // ─── Collect Hardware Output ──────────────────────────────────────────────
  std::vector<float> hw_mag(FFT_LENGTH);
  int hw_peak_bin = 0;
  float hw_peak_mag = 0.0f;
  int tlast_count = 0;
  int error_count = 0;

  for (int i = 0; i < FFT_LENGTH; i++) {
    if (stream_out.empty()) {
      std::cerr << "FAIL: stream underflow at i=" << i << "\n";
      return 1;
    }
    axis_iq_t pkt = stream_out.read();

    // Reconstruct magnitude from Q1.15 integer output
    // Scale back to float by multiplying by LSB = 2^-15
    float I_hw = static_cast<int16_t>(extract_i(pkt.data).to_int()) / 32768.0f;
    float Q_hw = static_cast<int16_t>(extract_q(pkt.data).to_int()) / 32768.0f;
    hw_mag[i] = std::sqrt(I_hw * I_hw + Q_hw * Q_hw);

    if (hw_mag[i] > hw_peak_mag) {
      hw_peak_mag = hw_mag[i];
      hw_peak_bin = i;
    }

    // TLAST regression check (Day 1 contract must still hold)
    if (pkt.last == 1) {
      tlast_count++;
      if (i != FFT_LENGTH - 1) {
        std::cerr << "FAIL: TLAST early at i=" << i << "\n";
        error_count++;
      }
    }
  }

  if (!stream_out.empty()) {
    std::cerr << "FAIL: stream has leftover packets\n";
    error_count++;
  }
  if (tlast_count != 1) {
    std::cerr << "FAIL: TLAST count=" << tlast_count << " (expected 1)\n";
    error_count++;
  }

  // ─── Peak Bin Comparison ──────────────────────────────────────────────────
  std::cout << "Hardware FFT peak:    bin " << hw_peak_bin << ", magnitude "
            << hw_peak_mag << "\n";
  std::cout << "MATLAB reference peak: bin " << ref_peak_bin << ", magnitude "
            << ref_peak_mag << "\n\n";

  int bin_error = std::abs(hw_peak_bin - ref_peak_bin);
  if (bin_error > 1) {
    std::cerr << "FAIL: Peak bin mismatch = " << bin_error
              << " bins (threshold: ≤ 1)\n";
    error_count++;
  } else {
    std::cout << "Peak bin match: " << bin_error << " bin error (OK)\n";
  }

  // ─── PSLR Check ───────────────────────────────────────────────────────────
  // Peak-to-Sidelobe Ratio: peak vs highest non-peak bin
  float sidelobe_max = 0.0f;
  for (int i = 0; i < FFT_LENGTH; i++) {
    if (std::abs(i - hw_peak_bin) > 2) // exclude ±2 around peak
      sidelobe_max = std::max(sidelobe_max, hw_mag[i]);
  }
  float PSLR_dB = 20.0f * std::log10(hw_peak_mag / (sidelobe_max + 1e-12f));
  std::cout << "Hardware PSLR: " << PSLR_dB << " dB";
  if (PSLR_dB < 30.0f) {
    std::cerr << " — FAIL (threshold: >= 30 dB)\n";
    error_count++;
  } else {
    std::cout << " (OK, threshold: >= 30 dB)\n";
  }

  // ─── Result ───────────────────────────────────────────────────────────────
  std::cout << "\n";
  if (error_count == 0) {
    std::cout << "✅ C-Sim PASS — FFT numerical validation passed.\n";
    std::cout << "   Peak bin: " << hw_peak_bin << " (ref: " << ref_peak_bin
              << ")\n";
    std::cout << "   PSLR: " << PSLR_dB << " dB\n";
    std::cout << "   TLAST: correct\n";
    std::cout << "   Ready for Day 3 synthesis.\n";
    return 0;
  } else {
    std::cout << "❌ C-Sim FAILED with " << error_count << " errors.\n";
    return 1;
  }
}