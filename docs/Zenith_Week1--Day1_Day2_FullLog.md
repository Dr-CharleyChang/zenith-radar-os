---
tags:
  - Zenith
  - BuildInPublic
  - FPGA
  - Zynq7020
  - ARM
  - DMA
  - CMA
  - JTAG
  - Linux
  - SocialMedia
  - Substack
  - GitHub
date: 2026-03-05 / 2026-03-10
author: Charley Chang
version: Day1+Day2 Full Log
---

# Project Zenith — Week 1 Day 1 & Day 2 完整战地日志

> **一句话总结：** 两天之内，项目从一个白皮书变成了一个公开运行的 GitHub 仓库、一个有真实内容的 X 账号、一个有第一篇文章的 Substack，以及一块在 PuTTY 里打印出 Linux shell 的真实硬件。

---

## 快速导航

- [[#一、Day 1 完整记录]]
  - [[#1-1 工具链版本决策 ADR-001]]
  - [[#1-2 Gemini 三个补丁与架构师裁定]]
  - [[#1-3 工作站远程访问崩溃与救援]]
  - [[#1-4 JTAG 驱动问题第一次出现]]
  - [[#1-5 社交媒体建立全过程]]
  - [[#1-6 GitHub 仓库初始化]]
  - [[#1-7 Substack 建立与第一篇文章]]
- [[#二、Day 2 完整记录]]
  - [[#2-1 板卡到办公室]]
  - [[#2-2 JTAG 驱动错误包识别]]
  - [[#2-3 PuTTY UART 连接与全部命令输出解析]]
  - [[#2-4 zenith_memory_map hpp 创建与原理]]
  - [[#2-5 Chimera CFAR 资产转移]]
  - [[#2-6 GitHub SSH Key 与提交]]
- [[#三、两天后的状态总图]]
- [[#四、架构级知识备忘]]

---

## 一、Day 1 完整记录

**日期：** 2026-03-05（项目第一周第一天）  
**核心成就：** 建立所有公开存在的外部基础设施

---

### 1-1 工具链版本决策 ADR-001

**背景：** 工作站已安装 Vivado **2025.2**，但白皮书规格写的是 2022.2。这是第一个需要立即关闭的架构决策。

**争议点：**

| 选项        | 优点                            | 风险                                     |
| --------- | ----------------------------- | -------------------------------------- |
| 坚守 2022.2 | 与白皮书一致，ALINX BSP 验证过，社区踩坑记录最全 | 需要额外安装，占用磁盘                            |
| 使用 2025.2 | 已安装，最新特性                      | ALINX AX7020 BSP 未必支持，PetaLinux 版本必须一致 |

**Gemini 的意见：** 强烈建议回退到 2022.2，PetaLinux 版本必须与 Vivado 精确匹配。

**Claude 的补充：** 今天的 P0 任务是去 ALINX 官网**实际查看** AX7020 BSP 支持的 Vivado 版本列表，再做决定。

**ADR-001 决策状态：** → **最终决定保留 Vivado/Vitis 2025.2**（ALINX 官方已支持新版本，且 ADR 后续确认关闭）

**架构教训：**  
> PetaLinux 的版本匹配是硬性约束，不是建议。用 2025.2 导出的 `.xsa` 喂给 2022.2 的 Yocto 编译树，Bitbake 大概率直接崩溃，报出几百个无法解析的依赖错误。版本链的任何一环断裂，整个 Linux 镜像构建流程就废了。

---

### 1-2 Gemini 三个补丁与架构师裁定

Gemini 对初始 Week 1 计划给出了三个技术补丁，全部经过架构师裁定：

**Patch 1：PetaLinux 离线编译（✅ 采纳，细节修正）**

PetaLinux 的 Yocto 构建系统在联网模式下会实时拉取全球开源镜像站的源码包。在国内网络环境下，这个过程大概率耗时 10 小时以上并在 99% 时报 Fetch Error。

**正确执行方案：**
```bash
# 在 petalinux-config → Yocto Settings 中设置：
SSTATE_DIR = "/opt/petalinux/sstate-cache/2022.2"  # 本地状态缓存
DL_DIR = "/opt/petalinux/downloads/2022.2"          # 本地源码包
CONNECTIVITY_CHECK_URIS = ""  # 禁用连通性检查，但保留按需补充能力
# 注意：BB_NO_NETWORK 不要设为 1，否则 sstate 不完整时直接失败
```

**Patch 2：使用 ALINX 官方 BSP（✅ 采纳，升级为硬性要求）**

泛型 xc7z020 模板不包含 AX7020 专有的 DDR PHY 时序参数和千兆网卡 PHY 芯片设备树节点。硬跑泛型 Linux，串口大概率连乱码都吐不出来，直接内核恐慌（Kernel Panic）。

```bash
# 正确起手式：用 ALINX 官方 BSP 创建工程
petalinux-create -t project -s /path/to/alinx_ax7020_2025.2.bsp -n zenith-petalinux

# 备选方案（若官方无对应版本 BSP）：
petalinux-create -t project --template zynq -n zenith-petalinux
petalinux-config --get-hw-description=/path/to/alinx_reference_design.xsa
```

**Patch 3：工业级 .gitignore（✅ 采纳，今天就建）**

Vivado 综合一次会生成几十 GB 的中间文件。没有严格的 `.gitignore`，仓库一天内膨胀到数 GB，每次 Push 是一场灾难。

**Patch 4（Claude 补充，Gemini 遗漏）：JTAG 链路先验证**

在跑任何 PetaLinux 命令之前，必须先确认 JTAG 能连上板卡。如果 JTAG 不通，`petalinux-boot --jtag` 会报一个误导性的 "No target found"，很容易被误诊为 PetaLinux 配置问题，在错误方向浪费大量时间。

**Substack 叙事重定位：**

Gemini 建议的叙事框架比"AI 写代码"更强：
> *"The code isn't the hard part; the physics is. I'm using AI to translate 20 years of radar physics intuition into deterministic C++20 silicon."*

这个框架的优势：先确立物理权威，AI 是工具而非主角，避免被硬核开发者视为"提示词工程师"。

---

### 1-3 工作站远程访问崩溃与救援

**事件时间线：**

```
尝试安装 JTAG 驱动
  → Windows 弹出"需要重启进入高级启动菜单"
  → 确认重启
  → 机器进入蓝色高级启动界面（等待用户选择）
  → Parsec: 断线（需要 Windows 正常登录才能运行）
  → Tailscale: 断线（同上）
  → 向日葵 Sunflower: 断线（同上）
  → 无显示器、无键盘，人在 500km 外的办公室
  → 完全失联
```

**救援过程：**

```
手机 → 打开智能插座 App
→ 找到 DESKTOP-CHIMERA 对应的插座
→ 关闭电源
→ 等待 10 秒
→ 重新开启
→ 机器冷启动，跳过高级启动菜单
→ 正常进入 Windows
→ 2 分钟后 Parsec/向日葵自动启动
→ 重新连上
```

**为什么这个方法有效：**

BIOS 中预先设置了 **"Restore AC Power Loss = Power On"**（断电后来电自动开机）。智能插座切断交流电，相当于强制物理断电，完全绕过 Windows 的软件层。来电后机器直接冷启动，不会进入被卡住的高级启动界面。

**三层防线设计（工程 FMEA）：**

| 层 | 机制 | 失效条件 |
|---|---|---|
| Layer 1 | BIOS 来电自启 | BIOS 设置被重置 |
| Layer 2 | 智能插座远程断电重启 | 插座网络断线 |
| Layer 3 | Parsec / Tailscale / Sunflower 三选一 | 操作系统未启动 |

**FMEA 工程原理：** Failure Mode and Effects Analysis。在雷达系统工程中，FMEA 是强制性的设计验证工具——每个子系统必须列举所有可能的失效模式，分析其影响，并设计独立的恢复路径。今天我们把这个方法论映射到了开发基础设施上。

**永久操作规程（写入内存）：**
> 远程板卡**永远保持 SD Boot 模式**。JTAG Boot 是物理在场时的调试模式。任何时候离开板卡现场，先确认 Boot 跳帽在 SD 位置，SD 卡在槽内。

---

### 1-4 JTAG 驱动问题第一次出现

**设备管理器诊断：**

```
设备管理器显示：
  端口 (COM和LPT)
    └── Silicon Labs CP210x USB to UART Bridge (COM3)  ← UART 正常
  
  通用串行总线控制器
    └── USB Serial Converter  ← JTAG 芯片，Windows 识别但驱动错误
                                 没有 Xilinx/Digilent 驱动
```

**芯片识别：**

```
VID_0403 & PID_6014 = FTDI FT232H（单通道高速 USB-串行桥）
```

**对比表：**

| 芯片 | VID | PID | 适用板卡 |
|---|---|---|---|
| FTDI FT232H | 0403 | 6014 | AX7020（本板） |
| Digilent 标准 | 0403 | 6010 | 大多数 Xilinx 官方板 |
| Cypress EZ-USB | 04B4 | 1004 | 黑金动力老款 |

**Digilent 驱动为什么不行：** `install_digilent.exe` 只识别 PID_6010，对 PID_6014 静默忽略——不报错，什么都不做。

**正确修复路径（待执行）：**
```
Zadig (zadig.akeo.ie)
→ Options → List All Devices
→ 找到 "USB Serial Converter" VID_0403 PID_6014
→ 驱动选择 WinUSB
→ Replace Driver
→ Vivado Hardware Manager → Auto Connect
```

**今天状态：** 问题识别，暂时搁置，不影响 UART 通信和板卡上电验证。

---

### 1-5 社交媒体建立全过程

#### X (Twitter) — @charley_builds

**账号注册：** 使用现有邮箱注册，不需要新邮箱。

**Bio 最终版本：**
```
Radar engineer. 20 years of physics, 0 years of social media.
Building a radar OS on FPGA with AI. Both experiments start now.
#BuildInPublic
```

**Display Name：** `Charley Chang`

**Header 图片选择：** Chimera CFAR 的 Vitis HLS 综合报告截图（显示 II=1, BRAM=0%, DSP=1）。
- 这张图的作用：让任何访问主页的技术人看到这是一个认真的工程账号，不是 AI 博主。
- 信号价值：你在白皮书里写 II=1，Header 就能证明你做到过。

**第一条推文（"dread"系列，Thread 形式）：**

```
Post 1/4:
There's a specific kind of dread that comes to 
experienced engineers around my age right now.

Not fear of being replaced. Something quieter than that.

It's the feeling of watching a craft you spent 20 years 
building get turned into a prompt.

Post 2/4:
I'm a radar engineer. I understand what happens to a photon 
between the antenna and the track file.

That knowledge lives in my hands as much as my head — 
it came from debugging hardware at midnight, from MATLAB plots 
that lied to me, from physics that didn't care about my deadline.

I don't know if that still matters the way it used to.

Post 3/4:
So I'm running an experiment.

I'm building a full software-defined radar OS on an FPGA — 
from DMA drivers to Kalman trackers — using AI as my co-architect.
In public. Every failure logged.

Not to prove AI is powerful. I already know that.

To find out what's left of the engineer when the code 
writes itself.

Post 4/4:
Thread starts here. Two years. No idea how it ends.

GitHub: github.com/Dr-CharleyChang/zenith-radar-os
#BuildInPublic #FPGA #RadarEngineering
```

**新账号字数限制问题：** 注册后 X 对新账号有临时字数限制（防垃圾账号机制）。解决方案：绑定手机号验证，限制立即解除，或拆成多条 Thread 分别发出。

**Thread 发送方式：** 点击第一条推文下面的"+"按钮，追加到同一 Thread 中，不是单独发帖。

**第一个回复：** 发出后立即有人互动。

---

#### GitHub — Dr-CharleyChang/zenith-radar-os

**仓库创建：**
- 名称：`zenith-radar-os`
- 可见性：Public
- License：**MIT**（选择理由见下）
- 初始化 README：是

**License 选择原因 — MIT vs GPL：**

| License | 约束 | Zenith 场景 |
|---|---|---|
| MIT | 几乎无约束，可商用，可闭源修改 | 社区版：鼓励使用和传播 |
| GPL | 衍生作品必须开源 | 不适合——企业客户需要闭源商用 |
| LGPL | 库可以商用链接，但库本身修改需开源 | 中间路线，复杂 |

**选 MIT 的战略逻辑：** 社区版用 MIT，让人尽可能使用。企业级算子用 IEEE 1735 加密 IP 核单独交付。两层结构，开源建标准，闭源赚利润。

**仓库 Description：**
```
A software-defined radar OS for Zynq-7020. 
Built in public with AI. By a radar engineer learning FPGA the hard way.
```

**README 核心结构：**
- 项目一句话定位
- 为什么选 Zynq-7020
- 技术约束（zero heap, zero copy, II=1）
- 构建进度表（M1-M6）
- 关注 X 和 Substack 的 CTA

**第一批提交内容：**
1. `.gitignore`（工业级，覆盖 Vivado/Vitis/PetaLinux/C++ 构建产物）
2. `docs/decisions/ADR-001-toolchain-version.md`
3. `zenith-core/include/zenith/common/zenith_memory_map.hpp`（占位版，地址待 Vivado 确认）
4. `zenith-silicon/cfar/`（Chimera CFAR 历史资产）

**SSH Key 配置（ed25519）：**

```bash
# 在工作站 WSL2 中生成
ssh-keygen -t ed25519 -C "charley@zenith-radar-os"
cat ~/.ssh/id_ed25519.pub  # 复制输出

# 粘贴到 GitHub Settings → SSH and GPG Keys → New SSH Key
# 验证：
ssh -T git@github.com
# 正确输出：Hi Dr-CharleyChang! You've successfully authenticated...
```

**为什么用 ed25519 而非 rsa：** ed25519 是椭圆曲线签名算法，密钥更短（256 bit vs RSA 的 4096 bit），安全性等价，计算更快，现代最佳实践。

**.gitignore 关键内容（工业级版本）：**

```gitignore
# Vivado 综合/实现中间文件
*.jou
*.log
*.pb
.Xil/
*/impl_1/
*/synth_1/
*.runs/
*.cache/
*.hw/
*.ip_user_files/

# 保留关键文件
!*.xpr    # Vivado 工程描述
!*.xdc    # 物理约束（管脚分配、时序）
!*.tcl    # Block Design 导出脚本（用于重建工程）

# XSA / Bitstream（体积大，从 CI 生成）
*.xsa
*.bit
*.bin

# PetaLinux/Yocto 构建产物
build/
images/linux/
components/plnx_workspace/

# C++20 ARM 编译产物
zenith-core/build/
**/*.o
**/*.elf
**/*.a
CMakeCache.txt
CMakeFiles/

# Vitis HLS 中间文件
*/solution*/
*/.autopilot/
# 保留综合报告
!*/syn/report/*.rpt
```

---

#### Substack — zenithlog.substack.com

**Publication 名称：** Zenith Log

**Substack Bio：**
```
Name: Charley Chang
Bio: Radar engineer. Building a software-defined radar OS on FPGA with AI. 
     20 years of physics. In public.
Twitter: @charley_builds
```

**Post #0 — "The Last Craft"**

**URL：** https://open.substack.com/pub/zenithlog/p/the-last-craft

**全文：**

> There's a specific kind of dread that comes to experienced engineers around my age right now.
>
> Not fear of being replaced. Something quieter than that.
>
> It's the feeling of watching a craft you spent 20 years building get turned into a prompt.
>
> I'm a radar engineer. I understand what happens to a photon between the antenna and the track file. That knowledge lives in my hands as much as my head — it came from debugging hardware at midnight, from MATLAB plots that lied to me, from physics that didn't care about my deadline.
>
> I don't know if that still matters the way it used to.
>
> So I'm running an experiment. I'm building a full software-defined radar OS on an FPGA — from DMA drivers to Kalman trackers — using AI as my co-architect. In public. Every failure logged.
>
> Not to prove AI is powerful. I already know that.
>
> To find out what's left of the engineer when the code writes itself.
>
> Subscribe if you want to watch. If you've ever stared at a synthesis report at midnight and felt both lost and completely alive — you'll feel at home here.

**为什么这个 CTA 有效：** "stared at a synthesis report at midnight and felt both lost and completely alive" 这句话让目标读者认出自己。看到这句话的硬件工程师会想"这说的就是我"，然后订阅并转发。共鸣先于说服。

**Tags：** #FPGA #Radar #BuildInPublic #AI

---

### 1-6 Day 1 结束状态

```
✅ ADR-001 决策完成（2025.2）
✅ 工作站崩溃救援完成，三层防线验证有效
✅ JTAG 问题诊断完成（PID_6014，Zadig 方案待执行）
✅ X @charley_builds 注册，Bio/Header/第一条 Thread 发出
✅ GitHub Dr-CharleyChang/zenith-radar-os 创建
✅ Substack zenithlog.substack.com 建立，Post #0 "The Last Craft" 发布
✅ .gitignore 提交
⏳ JTAG 驱动实际修复（搁置）
⏳ CFAR 资产转移（Day 2）
⏳ 板卡 UART 验证（Day 2）
⏳ zenith_memory_map.hpp 正式版（Day 2）
```

---

## 二、Day 2 完整记录

**日期：** 2026-03-10  
**核心成就：** 板卡首次上电、Linux 内核确认、CMA 地址确认、内存地图文件正式版提交

---

### 2-1 板卡到办公室

AX7020 板卡从家中带到办公室。笔记本与板卡直接连接，重工作站通过 Sunlogin 远程连接处理综合任务，OneDrive mklink 同步两端文件。

**硬件连接：**
```
AX7020 UART 口 (J7 / CP2102 芯片)
  → 红色 USB 线
  → 笔记本 USB
  → Windows: COM3 (Silicon Labs CP210x，自动安装)
  
PuTTY 配置：
  Serial line: COM3
  Speed: 115200
  Data bits: 8
  Stop bits: 1
  Parity: None
  Flow control: None
```

**为什么是 115200 波特率：**  
UART 是异步串行通信——没有时钟线，双方靠约定波特率对齐采样时序。Xilinx Zynq PS UART 的硬件默认值就是 115200。波特率 = 每秒传输的 bit 数，115200 bps 对于调试文本输出完全足够，约每秒 11520 个字符。

---

### 2-2 JTAG 驱动错误包识别

**收到的驱动包：** `USB2.0驱动.rar` → 内含 `CySuiteUSB_3_4_7_B204.exe`

**判定：错误，不要安装。**

**原因：**
- 这是 Cypress EZ-USB (VID_04B4 PID_1004) 的驱动程序
- 专门用于黑金动力（HEIJIN）系列老款板卡的高速 USB 数据传输接口
- AX7020 的 JTAG 芯片是 FTDI FT232H (VID_0403 PID_6014)，两者完全不同的 USB 设备
- 强行安装会污染系统 USB 驱动环境，可能导致 COM3 的 UART 也失效

**后续正确路径（Day 3+ 执行）：**
```
Zadig → Options → List All Devices
→ 选中 "USB Serial Converter" (VID_0403, PID_6014)
→ 目标驱动：WinUSB
→ Replace Driver
```

---

### 2-3 PuTTY UART 连接与全部命令输出解析

#### 启动序列

**一条 FAILED 信息：**
```
[FAILED] Failed to start LSB: NFS support files common to all servers
```

**这重要吗：** 完全不重要。NFS (Network File System) 是网络文件共享服务，与我们的工作无关。工厂镜像尝试启动它但没有 NFS 服务器，所以失败。关键系统全部正常启动，Linux 继续。

**登录提示：**
```
Debian GNU/Linux 8 zynq ttyPS0
zynq login:
```

`ttyPS0`：Zynq PS (Processing System) 的第一个 UART 设备。`PS` = Processing System = ARM 侧。`tty` = 电传打字机（Teletype），Unix 对串行终端的历史命名。

**登录凭据：** root / root（工厂默认，生产环境必须更改）

---

#### `uname -a` 完整输出与解析

```
Linux zynq 4.9.0-xilinx #1 SMP PREEMPT Mon Dec 16 14:37:44 CST 2019 armv7l GNU/Linux
```

| 字段 | 值 | 解释 |
|---|---|---|
| `Linux zynq` | 主机名 `zynq` | ALINX 工厂默认主机名 |
| `4.9.0-xilinx` | 内核版本 | Xilinx 定制 Linux 4.9.0 内核 |
| `#1 SMP` | 第 1 次编译，对称多处理器 | SMP = Symmetric MultiProcessing，双核支持 |
| `PREEMPT` | 抢占式内核 | 高优先级任务可打断低优先级任务 |
| `armv7l` | 指令集架构 | ARM Cortex-A9，32-bit，小端序 |

**PREEMPT 内核为什么对我们重要：**  
Zenith 的雷达循环有严格的 PRI（脉冲重复间隔）时序要求。PREEMPT 内核支持任务抢占，DMA 中断处理程序可以打断低优先级任务，保证采集时序不被延误。这是实时嵌入式系统的标准内核配置。

---

#### `cat /proc/cpuinfo | grep "model name"` 输出：

```
model name      : ARMv7 Processor rev 0 (v7l)
model name      : ARMv7 Processor rev 0 (v7l)
```

**两行 = 双核确认。** Zynq-7020 内置两个 ARM Cortex-A9 核心。

**Zenith 未来双核分工规划（M4 阶段）：**
- Core 0：RRM 调度器 + DMA 控制（实时，高优先级）
- Core 1：Tracker + Zenoh 网络发布（准实时，可被抢占）

---

#### `free -m` 完整输出与解析：

```
             total       used       free     shared    buffers     cached
Mem:          1006        128        877          7          8         68
Swap:            0          0          0
```

| 字段 | 值 | 含义 |
|---|---|---|
| total | 1006 MB | 1GB DDR，减去内核保留空间 |
| used | 128 MB | 系统 + 工厂服务占用 |
| free | 877 MB | 可用物理内存，非常充裕 |
| Swap | 全为 0 | 无交换分区 |

**为什么没有 Swap：**  
SD 卡闪存有写入次数限制（约 10,000-100,000 次/块）。频繁 Swap 会迅速磨损 SD 卡。嵌入式系统原则：如果内存够用就不 Swap。这也与 Zenith 的 Zero Heap 原则完全一致——我们的雷达循环不会触发动态内存分配，自然也不会触发 Swap。

---

#### `dmesg | grep -i cma` 输出与逐字解析：

```
[    0.000000] cma: Reserved 16 MiB at 0x3f000000
[    0.000000] Memory: 1012832K/1048576K available 
               (6144K kernel code, 210K rwdata, 1532K rodata, 
               1024K init, 238K bss, 19360K reserved, 
               16384K cma-reserved, 245760K highmem)
```

**这是整个 Day 2 最重要的输出。逐字解析：**

`cma: Reserved 16 MiB at 0x3f000000`

**CMA（Contiguous Memory Allocator）是什么：**  
Linux 内核在启动阶段，在操作系统开始管理内存之前，从 DDR 顶部预留一块物理地址连续的区域。普通 `malloc()` 分配的内存虚拟地址连续但物理地址是碎片化的——DMA 硬件只理解物理地址，它要求传输的内存在物理上连续，否则需要 Scatter-Gather 描述符（更复杂，有额外延迟）。CMA 在系统启动前预留，保证了物理连续性。

**0x3F000000 为什么在这个位置：**  
```
DDR 物理地址空间：
0x00000000 ──────────────── 内存起始
    .
    .  Linux Kernel（约 16MB）
    .  用户进程堆空间
    .
    .
0x3F000000 ──────────────── CMA 区域起始（16MB）
    .  TX Buffer  4MB   (PS→PL)
    .  RX Buffer  4MB   (PL→PS)
    .  BD Ring    4KB
    .  Track Buf  1MB
0x3FFFFFFF ──────────────── DDR 末尾（1GB 边界 - 1）
0x40000000 ──────────────── 超出 DDR 范围
```

设备树中配置在内存顶部，是防止内核堆分配器意外使用这块区域的标准做法。

`16384K cma-reserved` = 16384/1024 = 16 MB，与上一行吻合。

`245760K highmem`：ARM Cortex-A9 是 32-bit 处理器，用户空间最高访问到约 3GB，highmem 是内核需要动态映射才能访问的区域。对我们无直接影响，因为 DMA 用物理地址，不用内核虚拟地址映射。

---

#### `cat /proc/iomem` 关键段落解析：

```
00000000-3fffffff : System RAM
43000000-4300ffff : /amba_pl/dma@43000000
e0001000-e0001fff : xuartps
e000b000-e000bfff : /amba/ethernet@e000b000
e0100000-e0100fff : sdhci
```

| 地址范围 | 设备 | Zenith 相关性 |
|---|---|---|
| `0x00000000-0x3FFFFFFF` | System RAM（1GB DDR） | CMA 在此范围最顶部 |
| `0x43000000-0x4300FFFF` | AXI DMA IP | **关键：工厂 bitstream 已部署 DMA，数据通路确认** |
| `0xE0001000` | xuartps | 我们连接的 UART，COM3 的背后 |
| `0xE000B000` | ethernet | 未来 Zenoh 网络输出的接口 |
| `0xE0100000` | sdhci | SD 卡控制器 |

**重大发现：0x43000000 AXI DMA 已在 PL 中部署**  

工厂 bitstream 里已经包含了 AXI DMA IP，并且通过 AXI HP 端口连通了 PS 和 PL。这证明 **PS→DDR→PL 的物理数据通路是完整的**，无需从头建立。我们的 Zenith DMA Engine 将在相同的地址范围内工作。

**AXI DMA 寄存器地图解释（0x43000000）：**

| 偏移 | 寄存器 | 功能 |
|---|---|---|
| +0x00 | MM2S_DMACR | MM2S 控制寄存器（启动/停止） |
| +0x04 | MM2S_DMASR | MM2S 状态寄存器（Idle/Error） |
| +0x18 | MM2S_SA | 源地址（DDR 物理地址） |
| +0x28 | MM2S_LENGTH | 传输字节数 |
| +0x30 | S2MM_DMACR | S2MM 控制寄存器 |
| +0x34 | S2MM_DMASR | S2MM 状态寄存器 |
| +0x48 | S2MM_DA | 目标地址（DDR 物理地址） |
| +0x58 | S2MM_LENGTH | 传输字节数 |

**MM2S 和 S2MM 方向定义（DMA 视角）：**

| 方向缩写 | 全称 | 数据方向 | Zenith 用途 |
|---|---|---|---|
| MM2S | Memory-Map to Stream | DDR → PL | ARM 把波形配置发给 HLS 算子 |
| S2MM | Stream to Memory-Map | PL → DDR | PL 把处理结果（IQ/点迹）写回 DDR |

---

#### `df -h` 输出解析：

```
Filesystem      Size  Used Avail Use%  Mounted on
/dev/root       2.4G  1.2G  1.1G  52%  /
/dev/mmcblk0p1  361M  6.1M  355M   2%  /sd
```

| 分区 | 含义 | Zenith 用途 |
|---|---|---|
| `/dev/root` | 根文件系统（SD 卡第二分区，ext4） | Linux 系统文件 |
| `/dev/mmcblk0p1` | SD 卡第一分区（FAT32，挂载到 /sd） | BOOT.BIN + image.ub 放这里 |

`mmcblk0` = MMC Block device 0 = SD 卡。`p1` = 第一个分区。  
当我们用 PetaLinux 生成自己的镜像时，把 BOOT.BIN 和 image.ub 复制到 `/sd` 目录（即第一分区），重启即可加载新系统。355MB 空闲，足够放多个版本的 bitstream。

---

### 2-4 zenith_memory_map.hpp 创建与原理

#### 为什么需要这个文件

在 Zynq 异构系统中，同一块物理内存被三个独立系统同时引用：

```
Linux Device Tree  →  reserved-memory 节点（告诉内核不要用这块地址）
Vivado Block Design →  AXI HP 端口地址映射（告诉 PL 通过哪个窗口访问 DDR）
C++ 驱动代码       →  mmap() 物理地址参数（ARM 进程映射到虚拟地址空间）
```

如果这三处的地址不一致，结果是：
- DMA 写入地址 A，ARM 从地址 B 读 → 数据损坏，无任何报错
- AXI HP 地址越界 → 总线错误，系统挂起
- CMA 区域被内核覆盖 → 随机内存损坏，极难调试

`zenith_memory_map.hpp` 是唯一真相源（Single Source of Truth）。所有地方引用这一个文件，修改也只改这一个文件。

#### 正式版代码（基于 Day 2 实测地址）

```cpp
#pragma once
// =============================================================================
// zenith_memory_map.hpp  —  SINGLE SOURCE OF TRUTH
// =============================================================================
// Three-way consistency required:
//   1. THIS FILE (C++ constants)
//   2. Linux device tree  →  reserved-memory { reg = <0x3F000000 0x1000000>; }
//   3. Vivado Block Design →  AXI HP port address range covers CMA region
//
// Confirmed on AX7020 board:
//   dmesg: "cma: Reserved 16 MiB at 0x3f000000"  ← Day 2 PuTTY verified
//
// Author: Charley Chang | Project Zenith-Radar OS
// Date:   2026-03-10
// =============================================================================

#include <cstdint>   // uintptr_t — 精确宽度整数，保证跨平台指针宽度
#include <cstddef>   // size_t

// ── CMA 物理基地址 ────────────────────────────────────────────────────────────
// 由 dmesg 实测确认。DDR 物理布局：
//   系统 RAM: 0x00000000 - 0x3FFFFFFF (1GB)
//   CMA 区域: 0x3F000000 - 0x3FFFFFFF (16MB，位于 DDR 顶部)
constexpr uintptr_t CMA_PHYS_BASE   = 0x3F00'0000;
constexpr size_t    CMA_TOTAL_SIZE  = 0x0100'0000;  // 16 MB

// ── TX Buffer：PS→PL ──────────────────────────────────────────────────────────
// ARM 将波形参数写入此区域，DMA MM2S 通道读取后通过 AXI-Stream 发往 PL HLS 算子
// 物理地址：0x3F000000 - 0x3F3FFFFF
constexpr size_t    TX_OFFSET       = 0x0000'0000;
constexpr size_t    TX_SIZE         = 0x0040'0000;  // 4 MB

// ── RX Buffer：PL→PS ──────────────────────────────────────────────────────────
// PL HLS 算子处理后的数据（IQ / 点迹 / Range-Doppler Map）通过 AXI-HP S2MM 写入
// ARM 通过 std::span 零拷贝读取（需先 cache invalidate）
// 物理地址：0x3F400000 - 0x3F7FFFFF
constexpr size_t    RX_OFFSET       = 0x0040'0000;
constexpr size_t    RX_SIZE         = 0x0040'0000;  // 4 MB

// ── BD Ring：DMA Scatter-Gather 描述符环 ─────────────────────────────────────
// DMA 硬件自主遍历 BD 环，无需 CPU 干预每次传输
// 必须 4KB 对齐，必须是 2 的幂次大小
// 32 个 BD × 128 字节/BD = 4KB
// 物理地址：0x3F800000 - 0x3F800FFF
constexpr size_t    BD_OFFSET       = 0x0080'0000;
constexpr size_t    BD_SIZE         = 0x0000'1000;  // 4 KB

// ── Track Buffer：Zenoh 零拷贝输出 ───────────────────────────────────────────
// Kalman 滤波后的 TrackState 数组存放于此
// Zenoh 直接从此物理地址 zero-copy 发布到以太网
// 1MB = 1048576 / sizeof(TrackState=80B) ≈ 13107 条航迹（远超需求）
// 物理地址：0x3F801000 - 0x3F900FFF
constexpr size_t    TRACK_OFFSET    = 0x0080'1000;
constexpr size_t    TRACK_SIZE      = 0x0010'0000;  // 1 MB

// ── 派生绝对物理地址（计算得出，不要在其他地方硬编码）─────────────────────────
constexpr uintptr_t TX_PHYS_BASE    = CMA_PHYS_BASE + TX_OFFSET;
constexpr uintptr_t RX_PHYS_BASE    = CMA_PHYS_BASE + RX_OFFSET;
constexpr uintptr_t BD_PHYS_BASE    = CMA_PHYS_BASE + BD_OFFSET;
constexpr uintptr_t TRACK_PHYS_BASE = CMA_PHYS_BASE + TRACK_OFFSET;

// ── AXI DMA 控制寄存器基地址 ──────────────────────────────────────────────────
// 由 /proc/iomem 实测确认：
//   "43000000-4300ffff : /amba_pl/dma@43000000"
// 需要 Vivado Block Design 最终确认（工厂 bitstream 地址，可能与 Zenith BD 不同）
constexpr uintptr_t AXI_DMA_BASE    = 0x4300'0000;  // 待 Zenith Vivado BD 确认
```

#### 为什么用 `constexpr` 而不是 `#define`

| | `#define` | `constexpr` |
|---|---|---|
| 类型安全 | ❌ 无类型 | ✅ `uintptr_t`，编译器强制检查 |
| 调试可见性 | ❌ 预处理器替换，调试器看不到符号 | ✅ 调试器可以看到变量名 |
| 作用域 | ❌ 全局污染 | ✅ 遵守 C++ 命名空间规则 |
| 计算能力 | ❌ 不能做复杂计算 | ✅ 编译期计算，派生地址零运行时开销 |

**`uintptr_t` 选择原因：** 这是 C++ 标准保证"足以存放指针的无符号整数"类型。在 32-bit ARM 上是 uint32_t，在 64-bit 系统上自动变为 uint64_t。比直接用 `uint32_t` 更可移植。

#### ARM HP 端口 vs ACP 端口（DMA 路径选择）

```
                        ARM Cortex-A9
                       ┌─────────────┐
                       │  L1 Cache   │
                       │  L2 Cache   │
                       │  SCU        │ ← Snoop Control Unit
                       └──────┬──────┘
                              │
               ┌──────────────┴──────────────┐
               │ AXI HP (High Performance)   │  AXI ACP (Accelerator Coherency)
               │ 直连 DDR 控制器              │  经过 SCU
               │ 带宽：~1200 MB/s            │  带宽：~600 MB/s
               │ Cache：❌ 手动 invalidate   │  Cache：✅ 自动同步
               └─────────────────────────────┘
```

**Zenith 默认选择 HP 端口 + 手动 cache invalidate：**

```cpp
// DMA 读取 PL 写入的数据之前，必须调用：
__builtin___clear_cache(region.data(), region.data() + region.size());
// 原因：PL 通过 HP 直写 DDR，绕过了 ARM 的 L1/L2 Cache
// 如果不 invalidate，ARM 读到的是 Cache 里的旧数据，不是 PL 刚写入的新数据
```

---

### 2-5 Chimera CFAR 资产转移

Project Chimera（前身项目）的 CFAR 算子已验证：

**Chimera CFAR 规格（实测）：**
- II = 1（每时钟周期处理一个样本）
- 综合时序：6.81 ns @ 100 MHz（理论极限 10 ns，余量充足）
- BRAM 使用：0%（滑动窗口用寄存器实现，不占 BRAM36）
- DSP48 使用：1 个（α 系数乘法）
- LUT 使用：~3%（xc7z020 总量 53200，使用约 1600）
- 平台：xc7z020clg400-1（与 AX7020 完全相同的芯片）

**转移到仓库位置：**
```
zenith-silicon/cfar/
├── cfar.cpp          ← HLS 算子实现
├── cfar.h            ← 算子接口声明
├── hls_build.tcl     ← Vitis HLS 构建脚本
├── main.cpp          ← C-Sim 测试台
├── radar_defines.h   ← 常量定义
└── hal/
    ├── axi_dma_controller.hpp  ← DMA 控制器 HAL
    └── cfar_engine_controller.hpp ← CFAR 算子控制 HAL
```

**待完成：** 在 Vitis HLS 2025.2 下重新综合，确认 II=1 结论在新版本工具链下仍然成立。（2023.2 验证，2025.2 待验证）

---

### 2-6 GitHub SSH Key 与提交

**SSH Key 生成（ed25519）：**
```bash
ssh-keygen -t ed25519 -C "charley@zenith-radar-os"
# 保存到 ~/.ssh/id_ed25519
cat ~/.ssh/id_ed25519.pub
# 复制输出，粘贴到 GitHub → Settings → SSH Keys → New SSH Key
```

**验证：**
```bash
ssh -T git@github.com
# Hi Dr-CharleyChang! You've successfully authenticated...
```

**Day 2 提交清单：**
```bash
git add zenith-core/include/zenith/common/zenith_memory_map.hpp
git add zenith-silicon/cfar/
git commit -m "feat(M1): add confirmed CMA memory map and transfer Chimera CFAR assets

- zenith_memory_map.hpp: CMA base 0x3F000000 confirmed via dmesg on AX7020
- AXI DMA base 0x43000000 confirmed via /proc/iomem on factory SD
- CMA size: 16MB at DDR top, no device tree modification needed for M1
- CFAR: transfer from Project Chimera (II=1, BRAM=0%, 6.81ns@100MHz)
- Re-synthesis under Vitis HLS 2025.2 pending

Board: ALINX AX7020 (xc7z020clg400-1)
Kernel: Linux 4.9.0-xilinx
DDR: 1006MB usable, 877MB free at idle"

git push origin main
```

---

## 三、两天后的状态总图

```
✅ COMPLETED
├── 社交基础设施
│   ├── X @charley_builds — 注册，Bio，Header，Thread #1 "dread"
│   ├── GitHub Dr-CharleyChang/zenith-radar-os — 创建，SSH，.gitignore，README
│   └── Substack zenithlog.substack.com — 建立，Post #0 "The Last Craft" 发布
├── 架构决策
│   └── ADR-001: Vivado/Vitis 2025.2 — 关闭
├── 硬件验证
│   ├── AX7020 板卡上电 ✅
│   ├── UART COM3 115200 通信 ✅
│   ├── Linux 4.9.0-xilinx 双核 ARM 正常启动 ✅
│   ├── CMA 16MB @ 0x3F000000 实测确认 ✅
│   └── AXI DMA @ 0x43000000 物理路径确认 ✅
├── 代码资产
│   ├── zenith_memory_map.hpp（正式版，实测地址）
│   ├── .gitignore（工业级）
│   └── zenith-silicon/cfar/（Chimera 资产，待 2025.2 重综合）
└── 工程基础设施
    ├── BIOS 来电自启 ✅
    ├── 智能插座远程重启 ✅（已演练，实战救援成功）
    ├── Parsec/Tailscale/Sunlogin 三层远程 ✅
    └── GitHub SSH ed25519 ✅

⏳ PENDING (Week 1 剩余任务)
├── JTAG WinUSB 驱动（Zadig，待执行）
├── Vivado Block Design（PS7 + FCLK0 150MHz + AXI-HP）
├── CFAR 在 Vitis HLS 2025.2 下重综合
├── ARM 交叉编译链 C++20 验证（std::span）
├── X Thread #2（FMEA/工作站故事，5条连发）
└── X Post #3（Substack 分享）
```

---

## 四、架构级知识备忘

本节汇总两天中涉及的所有底层概念，供后续写 Substack 和给观众解释时使用。

### Linux 内存模型

```
虚拟地址空间（ARM 32-bit 进程）
0x00000000  ──  用户堆栈、代码、数据
    .
    .
0xC0000000  ──  内核空间边界（高 1GB 给内核）
    .
    .
0xFFFFFFFF  ──  地址空间末尾

物理地址空间（实际 DDR）
0x00000000  ──  DDR 起始
    .
0x3F000000  ──  CMA 起始（16MB，预留给 DMA）
0x3FFFFFFF  ──  DDR 末尾（1GB）

mmap() 的作用：
  把物理地址 0x3F000000 映射到进程的某个虚拟地址
  之后 CPU 通过虚拟地址读写，MMU 自动翻译成物理地址
  DMA 硬件直接用物理地址，不经过 MMU
```

### AXI-Stream 握手协议

```
Master (PL)    Slave (DMA/PS)
    │                │
    ├── TVALID=1 ───►│   "我有数据"
    │◄── TREADY=1 ───┤   "我准备好了"
    │                │
    │  ← 数据传输发生在双方都 HIGH 的时钟沿 →
    │                │
    │  TREADY=0 时，Master 必须保持 TVALID 和数据稳定
    │  （背压机制 Backpressure，防止数据丢失）
```

**"Arm the DMA" 操作顺序（必须严格执行）：**
```
Step 1: ARM 写 S2MM 目标地址到 DMA 寄存器
Step 2: ARM 写字节数，置 S2MM Run bit → TREADY 变为 HIGH
Step 3: ARM 触发 PL 开始处理（写 HLS 控制寄存器）
Step 4: PL 产生数据，TVALID=1，DMA 接受
Step 5: ARM 轮询 DMA SR.Idle 或等待中断

如果 Step 1-2 在 Step 3 之后才执行：
  PL 先 TVALID=1，DMA 还没准备好（TREADY=0）
  → AXI-Stream 死锁，数据包边界错位
  → 整个信号处理链数据损坏
```

### DDS（Direct Digital Synthesizer）原理简述

DDS 是用数字方法生成精确频率正弦波的硬件：

```
相位累加器（Phase Accumulator）
  每个时钟周期加一个固定步长 Δφ
  Δφ = f_out / f_clock × 2^W
  W = 相位累加器位宽（Zenith 要求 ≥ 24 bit）
  
  相位累加器 → 正弦查找表（BRAM）→ DAC → 射频前端

杂散抑制（Spur Level）≈ -6W dBc
  W=24 bit → 杂散 ≈ -144 dBc（理论值）
```

**为什么 M1 用简单脉冲而非 LFM：**  
简单脉冲 = DDS 输出单频 CW，不需要线性调频，是 DDS 最简单的配置。M1 阶段目的是验证 PS/PL 数据通路，用最少的变量。等 DMA 回环（loop-back）通了，才加 LFM 复杂度。先隔离变量，再增加复杂度，这是嵌入式调试的基本方法论。

### Zero Copy 全链路

```
PL HLS 算子
  → 通过 AXI-HP S2MM 直写 DDR（物理地址 0x3F400000）
  → ARM mmap() 已经把这块物理地址映射到进程虚拟空间
  → ARM 调用 __builtin___clear_cache() 使 Cache 失效
  → ARM 通过 std::span<const T> 直接读取（零拷贝，零 malloc）
  → Tracker 在原地更新 g_track_pool（静态数组，.bss 段）
  → Zenoh z_publisher_put_owned() 直接指向 g_track_pool 的物理地址
  → 以太网 MAC DMA 从同一物理地址读取
  → 数据出网口

全程 memcpy 次数：0
全程 heap allocation：0
```

---

*Week 1 Day 1 + Day 2 完整战地日志*  
*作者：Charley Chang | Project Zenith-Radar OS*  
*生成时间：2026-03-10*  
*下一次更新：Day 3（Vivado Block Design + CFAR 重综合）*

