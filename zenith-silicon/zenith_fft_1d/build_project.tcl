# Create the Vitis HLS project
open_project zenith_fft_1d_prj

# Set the top-level function
set_top fft_1d_top

# Add synthesizable source files
add_files src/fft_1d.cpp
add_files src/fft_1d.hpp

# Add the testbench file (only used for C-Sim/Co-Sim)
add_files -tb tb/fft_1d_tb.cpp

# Create the solution
open_solution "solution1"

# Target the Zynq-7020 chip
set_part {xc7z020clg400-2}

# Set the clock target to 150 MHz (6.67 ns) to force aggressive pipelining
create_clock -period 10.0 -name default

# Enforce strict AXI-Stream alignment for Zynq DMA
config_interface -m_axi_alignment_byte_size 64
config_dataflow -strict_mode warning

# Save and close
close_project