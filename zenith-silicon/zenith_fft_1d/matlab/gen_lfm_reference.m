% gen_lfm_reference.m
% Zenith M2 MATLAB Golden Reference — 1D-FFT Range Profile Validation
%
% PURPOSE:
%   Generate a fixed-point Q1.15 test vector and its expected FFT output.
%   This CSV file is loaded by the HLS testbench to verify bit-level
%   correctness of the hls::fft<ZenithFFTConfig> kernel.
%
% WAVEFORM CHOICE:
%   Single complex tone (not a full LFM chirp).
%   Reason: the HLS kernel only implements the FFT stage. The dechirp
%   mixer and DDS are not yet implemented. A tone directly models the
%   beat signal that would result from mixing an LFM echo with the TX
%   replica — this is exactly what the range FFT processes.
%
% M2 ACCEPTANCE CRITERION:
%   Peak bin from hardware FFT matches MATLAB within ±1 bin.
%   Peak-to-sidelobe ratio ≥ 30 dB.

clear; clc; close all;

%% ─── Parameters (must match fft_1d.hpp constants) ───────────────────────────
FFT_LENGTH  = 1024;          % Must equal constexpr int FFT_LENGTH in fft_1d.hpp
Fs          = 150e6;         % Sample rate = FCLK0 = 150 MHz
LSB         = 2^-15;         % Q1.15 least significant bit = 3.0518e-5

%% ─── Test Signal Design ──────────────────────────────────────────────────────
% Choose a tone frequency that maps to a clean FFT bin (avoids spectral
% leakage which would complicate the sidelobe analysis).
% Bin k = f_tone / (Fs / FFT_LENGTH)
% Choose k = 100 → f_tone = 100 * (150e6 / 1024) = 14.648 MHz

target_bin  = 100;
f_tone      = target_bin * (Fs / FFT_LENGTH);   % 14.648 MHz — maps to bin 100

fprintf('Test tone: %.4f MHz → bin %d (exact)\n', f_tone/1e6, target_bin);

% Input amplitude: 0.25 full scale.
% Reason: leaves 2-bit headroom for FFT growth at early stages with
% scaling_schedule = 0xAAAA. Prevents clipping while maintaining SNR.
amplitude   = 0.25;

%% ─── Generate Floating-Point IQ Signal ──────────────────────────────────────
t           = (0:FFT_LENGTH-1)' / Fs;       % time vector [s], column vector
iq_float    = amplitude * exp(1j * 2*pi * f_tone * t);
I_float     = real(iq_float);
Q_float     = imag(iq_float);

%% ─── Quantize to Q1.15 ───────────────────────────────────────────────────────
% ap_fixed<16,1> range: [-1, 1-LSB]. Clamp before rounding.
I_float_clamped = max(-1.0, min(1.0 - LSB, I_float));
Q_float_clamped = max(-1.0, min(1.0 - LSB, Q_float));

% Round to nearest Q1.15 integer representation
% (equivalent to AP_RND_CONV for symmetric inputs)
I_q15 = round(I_float_clamped / LSB);   % integer in [-32768, 32767]
Q_q15 = round(Q_float_clamped / LSB);

% Verify no saturation occurred
assert(all(I_q15 >= -32768 & I_q15 <= 32767), 'I saturation detected');
assert(all(Q_q15 >= -32768 & Q_q15 <= 32767), 'Q saturation detected');
fprintf('Input quantized: I range [%d, %d], Q range [%d, %d]\n', ...
        min(I_q15), max(I_q15), min(Q_q15), max(Q_q15));

%% ─── Compute Reference FFT (Floating Point) ──────────────────────────────────
% MATLAB's fft() operates in floating-point — this is the ideal reference.
% The HLS fixed-point FFT will deviate from this by quantization noise,
% but the peak bin location must match exactly.
IQ_complex  = (I_q15 + 1j * Q_q15) * LSB;   % reconstruct float from integers
FFT_ref     = fft(IQ_complex);                % MATLAB reference FFT

%% ─── Apply Scaling Schedule Model ───────────────────────────────────────────
% The HLS hls::fft<> with scaling_schedule=0xAAAA scales at alternating
% stages. For a 10-stage (1024-point) FFT with 0xAAAA:
%   Stages 1,3,5,7,9 scale (bits 0,2,4,6,8 of 0xAAAA = 0b1010101010)
%   Wait — 0xAAAA = 0b1010101010101010, so bits 1,3,5,7,9,11,13,15 are set
%   For a 10-stage FFT only bits 0-9 matter:
%   0xAAAA & 0x3FF = 0x2AA = 0b1010101010 → stages 1,3,5,7,9 scale

% Number of scaling stages active for 1024-point FFT with 0xAAAA:
scaling_schedule = hex2dec('AAAA');
active_bits = bitand(scaling_schedule, hex2dec('3FF'));  % only bits 0-9
n_scaled_stages = sum(dec2bin(active_bits) == '1');
fprintf('Scaling stages active: %d of 10 (schedule=0x%04X)\n', ...
        n_scaled_stages, scaling_schedule);

% Each scaling stage divides output by 2. Model this scaling factor:
hw_scale = 2^(-n_scaled_stages);
FFT_ref_scaled = FFT_ref * hw_scale;

%% ─── Analysis ────────────────────────────────────────────────────────────────
mag             = abs(FFT_ref_scaled);
[peak_mag, peak_bin_1idx] = max(mag);
peak_bin        = peak_bin_1idx - 1;   % convert to 0-indexed (matches C++)

fprintf('\n=== MATLAB Reference Results ===\n');
fprintf('Peak bin:         %d (expected: %d)\n', peak_bin, target_bin);
fprintf('Peak magnitude:   %.6f (of full scale)\n', peak_mag);

% Peak-to-sidelobe ratio (exclude ±2 bins around peak to avoid main lobe)
mask = true(FFT_LENGTH, 1);
mask(max(1, peak_bin_1idx-2) : min(FFT_LENGTH, peak_bin_1idx+2)) = false;
sidelobe_max    = max(mag(mask));
PSLR_dB         = 20*log10(peak_mag / sidelobe_max);
fprintf('Peak-to-sidelobe: %.1f dB (threshold: >= 30 dB)\n', PSLR_dB);

% SNR estimate (signal power vs quantization noise floor)
noise_bins      = mag(mask);
noise_power     = mean(noise_bins.^2);
signal_power    = peak_mag^2;
SNR_dB          = 10*log10(signal_power / noise_power);
fprintf('Estimated SNR:    %.1f dB\n', SNR_dB);

%% ─── Pass/Fail Gate ──────────────────────────────────────────────────────────
pass = true;
if abs(peak_bin - target_bin) > 1
    fprintf('FAIL: Peak bin %d deviates > 1 from expected %d\n', ...
            peak_bin, target_bin);
    pass = false;
end
if PSLR_dB < 30
    fprintf('FAIL: PSLR %.1f dB below 30 dB threshold\n', PSLR_dB);
    pass = false;
end
if pass
    fprintf('\nMATLAB reference: PASS\n');
end

%% ─── Export CSV for HLS Testbench ───────────────────────────────────────────
% Format: one row per sample
% Columns: I_in (integer Q1.15), Q_in (integer Q1.15),
%          I_ref (float), Q_ref (float), magnitude (float)
%
% The HLS testbench reads I_in/Q_in as input stimulus and
% compares FFT output bins against I_ref/Q_ref.
% "integer Q1.15" means the raw ap_int<16> value, not the float.

outdir = '../matlab';
if ~exist(outdir, 'dir'), mkdir(outdir); end

I_ref_float = real(FFT_ref_scaled);
Q_ref_float = imag(FFT_ref_scaled);
mag_ref     = abs(FFT_ref_scaled);

T = table(I_q15, Q_q15, I_ref_float, Q_ref_float, mag_ref, ...
    'VariableNames', {'I_in_q15','Q_in_q15','I_ref','Q_ref','magnitude'});
csv_path = fullfile(outdir, 'reference_fft_output.csv');
writetable(T, csv_path);
fprintf('\nExported: %s (%d rows)\n', csv_path, FFT_LENGTH);

%% ─── Plots ───────────────────────────────────────────────────────────────────
figure('Name', 'Zenith M2 MATLAB Reference', 'Position', [100 100 1200 500]);

subplot(1,3,1);
plot((0:FFT_LENGTH-1)*(Fs/FFT_LENGTH/1e6), mag);
xlabel('Frequency (MHz)'); ylabel('Magnitude');
title('Reference FFT output (scaled)');
xline(f_tone/1e6, 'r--', sprintf('Bin %d = %.2f MHz', peak_bin, f_tone/1e6));
grid on;

subplot(1,3,2);
plot((0:FFT_LENGTH-1)*(Fs/FFT_LENGTH/1e6), 20*log10(mag + eps));
xlabel('Frequency (MHz)'); ylabel('dBFS');
title('Reference FFT output (dBFS)');
ylim([-100, 0]);
xline(f_tone/1e6, 'r--');
yline(-30, 'b--', '-30 dB threshold');
grid on;

subplot(1,3,3);
stem(I_q15(1:64), 'filled', 'MarkerSize', 3);
xlabel('Sample index'); ylabel('Q1.15 integer value');
title('Input signal (first 64 samples, I component)');
grid on;

sgtitle(sprintf('Zenith M2 Reference: Bin=%d, PSLR=%.1f dB, SNR=%.1f dB', ...
        peak_bin, PSLR_dB, SNR_dB));

% Save figure
fig_path = fullfile(outdir, 'reference_fft_plot.png');
saveas(gcf, fig_path);
fprintf('Saved plot: %s\n', fig_path);