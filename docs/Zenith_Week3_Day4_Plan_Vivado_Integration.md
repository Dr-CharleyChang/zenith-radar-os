---
tags:
  - Zenith
  - M2
  - HLS
  - FFT
  - MATLAB
  - FixedPoint
  - GoldenReference
  - RadarSignalProcessing
  - Week3
  - Day4
date: 2026-04-29
author: Charley Chang
milestone: M2
depends_on: "[[Week3_Day3_FFT test result]]"
status: In Progress
---

# Week 3 · Day 4 — Vivado 硬件系统集成与 C/RTL 共仿

> **核心目标：** 对 FFT 模块进行 C/RTL Co-Simulation（共模拟）验证其波形时序，导出 IP 核。随后打开 Vivado，拆除 M1 阶段留下的 DMA 环回线，将 FFT IP 接入真实的 AXI4-Stream 和 AXI4-Lite 总线，并生成最终的 Bitstream。

---

## 1. Phase 1: C/RTL Co-Simulation (终极真理吐露剂)

在我们将代码变成冷冰冰的硅片之前，必须进行最后一次验证：**C/RTL 共仿真**。

它与昨天的 C-Sim 不同。C-Sim 跑的是 C++ 代码；而 Co-Sim 是 Vitis HLS 在后台默默生成了 Verilog 代码，然后调用 Vivado 仿真器（XSim），把你的 C++ Testbench 产生的数据喂给 **真实的 Verilog 波形**，最后再把 Verilog 吐出来的数据拿回 C++ 里进行比对。

### 1.1 运行 Co-Sim

在终端中执行以下命令（如果你之前写了 `run_cosim.tcl` 脚本）：

Bash

```
vitis-run --mode hls --tcl run_cosim.tcl
```

_(如果只用 GUI，直接点击 `Run C/RTL Cosimulation` 即可)_

### 1.2 Co-Sim 验收标准

- 你会看到类似 `// RTL Simulation : 0 / 1024` 的进度条。
    
- 最终必须再次输出我们在 Day 3 看到的：`✅ C-Sim PASS — FFT numerical validation passed.`
    
- **Latency（延迟）检查：** 观察日志中的报告，对于 1024 点 FFT，流水线的总延迟（Latency）通常在 `1050 ~ 1100` 个时钟周期左右。这证明数据像水管一样，进去之后几微秒就能流出来。
    

---

## 2. Phase 2: 导出硬件 IP 核 (Packaging)

一旦 Co-Sim 亮起绿灯，我们就可以将它打包封装。

如果使用 Tcl 脚本（如 `run_export.tcl`），内容应包含：

Tcl

```
open_project zenith_fft_1d_prj
open_solution solution1
export_design -format ip_catalog -rtl verilog
exit
```

执行后，在你的工程目录下会生成一个 ZIP 文件（例如 `solution1/impl/ip/xilinx_com_hls_fft_1d_top_1_0.zip`）。这就是我们今天要在 Vivado 中做“心脏移植”的器官。

---

## 3. Phase 3: Vivado Block Design (核心手术)

现在，请打开你的 Vivado，加载我们在 M1 阶段创建的包含 Zynq PS 和 AXI DMA 的基础工程。

### 3.1 导入 HLS IP

1. 点击左侧导航栏的 **Settings** -> **IP** -> **Repository**。
    
2. 点击 **+** 号，添加你刚才 HLS 工程生成的 `solution1/impl/ip` 文件夹路径。
    
3. Vivado 会弹窗提示发现 1 个新的 IP (`fft_1d_top`)。
    

### 3.2 拆除 M1 的 DMA 环回 (Loopback)

在 M1 阶段，为了验证 DMA，我们把 AXI DMA 的发送端（`M_AXIS_MM2S`）直接连到了接收端（`S_AXIS_S2MM`）。

- **操作：** 在 Block Design (BD) 画布中，点击这根直接相连的粗线，按下 `Delete` 键无情删掉它。
    

### 3.3 植入 FFT IP 并连接数据流 (Data Plane)

1. 在画布中点击 `+`，搜索 `fft_1d_top` 并添加到设计中。
    
2. **数据流连线（极其关键）：**
    
    - 将 DMA 的 `M_AXIS_MM2S` 连到 FFT 的 `in_stream`。
        
    - 将 FFT 的 `out_stream` 连到 DMA 的 `S_AXIS_S2MM`。
        
3. **时钟与复位：**
    
    - 将 FFT IP 的 `ap_clk` 连到系统的 150 MHz 统一时钟树上（通常是 Zynq 的 `FCLK_CLK0`）。
        
    - 将 `ap_rst_n` 连到 `Processor System Reset` 模块的 `peripheral_aresetn`。
        

### 3.4 接入控制流 (Control Plane)

FFT IP 现在还需要接收 ARM 发来的 `scaling_schedule` 以及启动信号。

- 点击画布顶部的 **Run Connection Automation**。
    
- 勾选 `fft_1d_top` 的 `s_axi_CTRL` 接口。
    
- Vivado 会自动为你实例化一个 **AXI SmartConnect** 或 **AXI Interconnect**，并将它连接到 Zynq PS 的 M_AXI_GP0 端口。
    

### 3.5 确认地址分配 (Address Editor)

打开 Block Design 的 **Address Editor** 选项卡。这是决定明天我们在 Linux 里 mmap 哪个内存地址的生死簿。

- 找到 `fft_1d_top` 的 `s_axi_CTRL`。
    
- 确保它被分配了一个合法的基地址。根据我们的架构规划，将其手动修改为 **`0x43C00000`**，大小设为 `64K`。
    
- 确认 AXI DMA 的基地址依然是 `0x43000000`。
    

---

## 4. Phase 4: 综合、实现与生成比特流

这是 Vivado 的体力活：

1. 点击 **Validate Design** (F6) 检查连线是否合法。确保没有任何 Critical Warnings。
    
2. 在 Sources 窗口右键你的 BD 文件，选择 **Create HDL Wrapper**。
    
3. 点击左侧的 **Generate Bitstream**。
    
4. 去喝杯咖啡，给电脑 10~20 分钟的时间，让它在几千万个晶体管中为你寻找最完美的布线路径。
    

当弹出 `Bitstream Generation successfully completed` 时，导出硬件描述文件（**File -> Export -> Export Hardware**，包含 Bitstream，生成 `.xsa` 文件）。

---

## 📚 首席架构师知识库 (Knowledge Base)

### 💡 理论 1: AXI4-Stream 的握手哲学 (TVALID / TREADY)

在把 DMA 和 FFT 连起来的那根线上，没有任何地址，只有纯粹的数据。它们是怎么配合的？

- **TVALID (源端发起):** 当 DMA 准备好了一个 32-bit 的数据，拉高 TVALID 告诉 FFT：“兄弟，有货了”。
    
- **TREADY (目的端发起):** 当 FFT 的输入 FIFO 还没满，它会拉高 TREADY 告诉 DMA：“可以，扔过来吧”。
    
- **握手成功:** 只有当此时钟的上升沿，TVALID 和 TREADY **同时为高 (1)**，这 32-bit 数据才算真正交接完成。
    
- **反压 (Backpressure):** 如果 FFT 算得比 DMA 发得慢（其实不会，因为我们做到了 II=1），FFT 会拉低 TREADY，DMA 就会乖乖把数据端着不放，直到 TREADY 再次拉高。**这就是为什么 AXI-Stream 是处理极速雷达数据的绝对王者，它天然具备防丢包的弹性水库属性。**
    

### 💡 理论 2: 为什么要用 DMA？(Memory-Mapped vs Stream)

ARM 处理器（PS 端）使用的是 **Memory-Mapped (内存映射)** 总线。CPU 想拿数据，必须给出物理地址（如 `0x1000_0000`），然后读回来。

但 FFT 引擎是一个 **Stream (流式)** 设备，它没有地址，它只是一根不断吞吐数字的管子。

- **DMA (Direct Memory Access) 的本质就是翻译官：** 它一头连接着 DDR 内存（懂得如何根据地址去搬运数据），另一头连接着 FFT（懂得如何通过 TVALID/TREADY 发射无地址的数据）。
    
- 没有 DMA，ARM 处理器就只能自己当搬运工，一个字节一个字节地往 FFT 里面塞数据。这不仅极其缓慢，而且会占用 100% 的 CPU 算力。有了 DMA，ARM 只需要下一道圣旨：“把 0x1000_0000 处的 1024 个复数打进 FFT，算完了通知我”，然后就可以去睡觉（挂起线程）了。
    

### 💡 理论 3: TLAST 的斩断魔法

在 AXI-Stream 中，有一根细细的信号线叫 `TLAST`。

当 DMA 发送第 1024 个点的时候，它会把 `TLAST` 置为 1。FFT IP 核极其依赖这个信号。

Xilinx 的 FFT IP 内部有一个状态机，它在等待凑齐 1024 个点。如果在第 1024 个点没有看到 `TLAST=1`，或者在第 1000 个点提前看到了 `TLAST=1`，FFT 引擎就会直接锁死报错。这也是为什么我们在 Day 1 和 Day 3 的 C++ 代码中，必须极其小心地手动管理 `pkt.last = (i == FFT_LENGTH - 1) ? 1 : 0;`。

---

**Day 4 里程碑验收：**

当 Vivado 成功吐出 `.bit` 文件时，Day 4 圆满结束。你的雷达物理引擎已经被锁印在了硅片上。准备好迎接明天的 Day 5——我们将编写 C++ Linux 驱动，在真实的 ARM 芯片上唤醒它！