# 创建项目
open_project hls_cfar_proj

# 设置顶层函数
set_top cfar_detector

# 添加源文件
add_files src/cfar.cpp
add_files src/cfar.h

# 添加测试文件 (下一步我们会写)
add_files -tb src/tb_cfar.cpp

# 创建解决方案 (Solution)
open_solution "solution1" -flow_target vivado

# 设置 FPGA 芯片型号: Zynq 7020 (CLG400封装, -1速度等级)
set_part {xc7z020clg400-1}

# 创建时钟: 100MHz (10ns周期)
create_clock -period 10 -name default

# 模拟配置 (开启波形记录，方便你看波形)
config_export -format ip_catalog -rtl verilog

# 步骤: 1. C仿真 2. C综合 3. C/RTL联合仿真
csim_design
csynth_design
# cosim_design

exit