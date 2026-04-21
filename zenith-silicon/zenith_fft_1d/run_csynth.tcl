open_project zenith_fft_1d_prj
set_top fft_1d_top

add_files src/fft_1d.cpp
add_files src/fft_1d.hpp
add_files -tb tb/fft_1d_tb.cpp

open_solution solution1
set_part {xc7z020clg400-2}
create_clock -period 6.67 -name default

csynth_design
exit