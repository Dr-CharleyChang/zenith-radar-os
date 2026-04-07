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
  - AXI
  - AXI-Stream
  - AXI-HP
  - AXI-ACP
  - AXI-Lite
  - ZeroCopy
  - TutorialTextbook
date: 2026-03-05 / 2026-03-12
author: Charley Chang
version: Day1+Day2 Full Log v2 — AXI深度补充版
---

# Project Zenith — Week 1 Day 1 & Day 2 完整战地日志

> **一句话总结：** 两天之内，项目从一个白皮书变成了一个公开运行的 GitHub 仓库、一个有真实内容的 X 账号、一个有第一篇文章的 Substack，以及一块在 PuTTY 里打印出 Linux shell 的真实硬件。

> **v2 更新说明：** 本版本在原有基础上新增了第五章「AXI 总线体系深度教材」，将后续 Q&A 中涉及的所有 AXI 知识系统整理为教科书格式：AXI 协议 vs 物理端口的区分、TVALID/TREADY 时序波形解读、AXI-ACP 与 SCU 工作原理、"Arm the DMA"的完整操作语义。同时更新了第二章「架构级知识备忘」中的 AXI-Stream 握手协议部分，使其与第五章互相对照。

---

## 快速导航

- [[#一、Day 1 完整记录]]
  - [[#1-1 工具链版本决策 ADR-001]]
  - [[#1-2 Gemini 三个补丁与架构师裁定]]
  - [[#1-3 工作站远程访问崩溃与救援]]
  - [[#1-4 JTAG 驱动问题第一次出现]]
  - [[#1-5 社交媒体建立全过程]]
  - [[#1-6 GitHub 仓库初始化与第一批提交内容]]
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
- [[#五、AXI 总线体系深度教材（Textbook）]]
  - [[#5-1 先搞清楚：协议 vs 物理端口]]
  - [[#5-2 三种物理端口：GP / HP / ACP]]
  - [[#5-3 AXI-ACP 与 SCU 工作原理]]
  - [[#5-4 为什么 Zenith 默认用 HP 而不用 ACP]]
  - [[#5-5 AXI-Stream 时序波形：TVALID/TREADY 是什么]]
  - [[#5-6 TVALID 和 TREADY 的物理含义——为什么不能自己定义 1/0]]
  - [[#5-7 Arm the DMA：完整操作语义]]
  - [[#5-8 死锁场景：不 Arm 先 Trigger 会发生什么]]
  - [[#5-9 AXI 全景图]]

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

**ADR 文件状态：** Day 1 计划中提到了 ADR-001，但文件本身直到 Week 1 结束后才正式生成（这是 Build in Public 的一个真实 gap）。文件路径：`docs/decisions/ADR-001-toolchain-version.md`，已于 Week 1 结束时补提交。

---

### 1-2 Gemini 三个补丁与架构师裁定

Gemini 对初始 Week 1 计划给出了三个技术补丁，全部经过架构师裁定：

**Patch 1：PetaLinux 离线编译（✅ 采纳，细节修正）**

PetaLinux 的 Yocto 构建系统在联网模式下会实时拉取全球开源镜像站的源码包。在国内网络环境下，这个过程大概率耗时 10 小时以上并在 99% 时报 Fetch Error。

**正确执行方案：**
```bash
# 在 petalinux-config → Yocto Settings 中设置：
SSTATE_DIR = "/opt/petalinux/sstate-cache/2025.2/sstate-cache"  # 注意：有双重路径
DL_DIR = "/opt/petalinux/downloads/2025.2"                       # 本地源码包
CONNECTIVITY_CHECK_URIS = ""  # 禁用连通性检查，但保留按需补充能力
# 注意：BB_NO_NETWORK 不要设为 1，否则 sstate 不完整时直接失败
```

**⚠️ 路径陷阱（Week 1 实测）：** sstate-cache 实际提取后存在双重路径嵌套：
```
/opt/petalinux/sstate-cache/2025.2/sstate-cache/  ← 实际路径（多一层）
```
SSTATE_DIR 必须指向最内层含有实际 `.tgz` 缓存文件的目录，否则 Bitbake 找不到缓存，仍然联网拉取。

**Patch 2：使用 ALINX 官方 BSP（✅ 采纳，升级为硬性要求）**

泛型 xc7z020 模板不包含 AX7020 专有的 DDR PHY 时序参数和千兆网卡 PHY 芯片设备树节点。硬跑泛型 Linux，串口大概率连乱码都吐不出来，直接内核恐慌（Kernel Panic）。

```bash
# 正确起手式：用 ALINX 官方 BSP 创建工程
petalinux-create -t project -s /path/to/alinx_ax7020_2025.2.bsp -n zenith-petalinux

# 备选方案（若官方无对应版本 BSP，如 Week 1 实测发现 ALINX 仅有 2023.1 BSP）：
petalinux-create -t project --template zynq -n zenith-petalinux
petalinux-config --get-hw-description=/path/to/alinx_ax7020_2023.1.xsa
```

**ALINX BSP 调查结论（Week 1 实测）：**
- ALINX GitHub 最新 AX7020 仓库：`AX7020_2023.1`（Vivado 2023.1）
- 无 2024.x 或 2025.x BSP
- **实际执行方案：** 用 2023.1 XSA + 泛型 PetaLinux 2025.2 模板，配合 sstate 离线缓存

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
  → 向日葵 Sunlogin: 断线（同上）
  → 无显示器、无键盘，人在 230km 外的办公室
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

### 1-6 GitHub 仓库初始化与第一批提交内容

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

#### 第一批 Git 提交的四件事（Day 1 计划，Day 2 完成）

这是 Day 1 制定的初始提交清单，也是后来 review Day 1 笔记时发现的 gap：

```
1. .gitignore（工业级，覆盖 Vivado/Vitis/PetaLinux/C++ 构建产物）
2. docs/decisions/ADR-001-toolchain-version.md             ← 当时遗漏，Week 1 结束补上
3. zenith-core/include/zenith/common/zenith_memory_map.hpp ← Day 2 完成（地址 Day 2 确认）
4. zenith-silicon/cfar/                                    ← Day 2 完成（Chimera 资产转移）
```

**ADR-001 文件 gap 的来龙去脉：**

Day 1 的提交记录中多处提到了 "ADR-001"，但文件本身一直没有被创建。这在 Week 1 结束时的 review 中被发现。这是 Build in Public 的真实例子：不是每件事都按计划完成，记录 gap 本身就是内容。

ADR-001 文件最终生成于 Week 1 结束，内容包含：完整决策背景、Gemini/Claude 的建议对比、最终决定、Week 1 合成结果的技术验证（CFAR @ 150 MHz II=1 通过）。

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

### 1-7 Substack 建立与第一篇文章

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

### 1-8 Day 1 结束状态

```
✅ ADR-001 决策完成（2025.2）
✅ 工作站崩溃救援完成，三层防线验证有效
✅ JTAG 问题诊断完成（PID_6014，Zadig 方案待执行）
✅ X @charley_builds 注册，Bio/Header/第一条 Thread 发出
✅ GitHub Dr-CharleyChang/zenith-radar-os 创建
✅ Substack zenithlog.substack.com 建立，Post #0 "The Last Craft" 发布
✅ .gitignore 提交
⏳ ADR-001 文件本身（遗漏，Week 1 结束时补上）
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
    .  TX Buffer  4MB   (PS→PL)  ← Zenith 分区
    .  RX Buffer  4MB   (PL→PS)
    .  BD Ring    4KB   (DMA Scatter-Gather 描述符环)
    .  Track Buf  1MB   (Zenoh 输出)
0x3FFFFFFF ──────────────── DDR 末尾（1GB 边界 - 1）
0x40000000 ──────────────── 超出 DDR 范围
```

> **BD Ring（Buffer Descriptor Ring）是什么：**
> DMA 的"自动驾驶模式"。预先把所有传输任务写入一个环形描述符数组，DMA 硬件自主遍历这个环，不需要 CPU 干预每次传输。M1 调试阶段用简单 DMA（每次手动触发），M4 实时运行阶段才启用 BD Ring。环的大小必须是 2 的幂次，方便用位运算（`& (N-1)`）实现循环索引，避免除法。

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

**AXI DMA 寄存器地图（`0x43000000` 基地址）：**

| 偏移 | 寄存器 | 功能 |
|---|---|---|
| +0x00 | MM2S_DMACR | MM2S 控制寄存器（启动/停止） |
| +0x04 | MM2S_DMASR | MM2S 状态寄存器（Idle/Error） |
| +0x18 | MM2S_SA | 源地址（DDR 物理地址） |
| +0x28 | MM2S_LENGTH | 传输字节数 |
| +0x30 | S2MM_DMACR | S2MM 控制寄存器 |
| +0x34 | S2MM_DMASR | S2MM 状态寄存器 |
| +0x48 | S2MM_DA | 目标地址（DDR 物理地址） |
| +0x58 | S2MM_LENGTH | 传输字节数 ← **写这个会触发 TREADY=1** |

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
├── cfar.cpp          ← HLS 算子实现（设计文件）
├── cfar.h            ← 算子接口声明（设计文件）
├── hls_build.tcl     ← Vitis HLS 构建脚本
├── main.cpp          ← ARM 应用程序（非 HLS 测试台，排除于综合）
├── radar_defines.h   ← 常量定义（设计文件）
└── hal/
    ├── axi_dma_controller.hpp    ← ARM DMA 驱动（排除于综合）
    └── cfar_engine_controller.hpp ← CFAR 算子控制 HAL（排除于综合）
```

**⚠️ 重要：Vitis HLS 文件分类规则**

HLS 项目中每个文件必须被明确分配为「设计文件」或「测试台文件」。`main.cpp` 和 `hal/*.hpp` 是 ARM 侧代码，包含 Xilinx SDK 头文件（`xil_cache.h`）和硬件物理地址，绝对不能加入设计文件——否则 HLS 编译器找不到 ARM-only 的头文件，立即报错。详见 Week 1 Day 3-4 日志的 Part 4。

**Week 1 重综合结果（Vitis HLS 2025.2 @ 150 MHz）：**

| 指标 | Chimera (100MHz) | Zenith 2025.2 (150MHz) | 状态 |
|---|---|---|---|
| II | 1 | **1** | ✅ Perfect |
| Slack | +3.19 ns | **+0.23 ns** | ✅ 通过（偏紧） |
| BRAM | 0 | **0** | ✅ Perfect |
| DSP | 1 | **1** | ✅ Perfect |
| LUT | ~3% | **1% (735)** | ✅ 更优 |

---

### 2-6 GitHub SSH Key 与提交

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

git push origin master
```

**⚠️ 使用 `git pull --rebase` 而非 `git pull`：**
普通 `git pull` 在本地有提交时会自动产生 Merge Commit（如 "Merge remote master"），污染历史记录。`git pull --rebase` 把本地提交 replay 到远端最新状态之上，历史保持线性。Zenith 仓库从 Week 2 起统一使用 rebase 策略。

---

## 三、两天后的状态总图

```
✅ COMPLETED
├── 社交基础设施
│   ├── X @charley_builds — 注册，Bio，Header，Thread #1 "dread"
│   ├── GitHub Dr-CharleyChang/zenith-radar-os — 创建，SSH，.gitignore，README
│   └── Substack zenithlog.substack.com — 建立，Post #0 "The Last Craft" 发布
├── 架构决策
│   └── ADR-001: Vivado/Vitis 2025.2 — 关闭（文件于 Week 1 结束时补提交）
├── 硬件验证
│   ├── AX7020 板卡上电 ✅
│   ├── UART COM3 115200 通信 ✅
│   ├── Linux 4.9.0-xilinx 双核 ARM 正常启动 ✅
│   ├── CMA 16MB @ 0x3F000000 实测确认 ✅
│   └── AXI DMA @ 0x43000000 物理路径确认 ✅
├── 代码资产
│   ├── zenith_memory_map.hpp（正式版，实测地址）
│   ├── .gitignore（工业级）
│   └── zenith-silicon/cfar/（Chimera 资产，Week 1 已完成 2025.2 重综合）
└── 工程基础设施
    ├── BIOS 来电自启 ✅
    ├── 智能插座远程重启 ✅（已演练，实战救援成功）
    ├── Parsec/Tailscale/Sunlogin 三层远程 ✅
    └── GitHub SSH ed25519 ✅

⏳ PENDING (Week 2)
├── JTAG WinUSB 驱动（Zadig，待执行）
├── Vivado Block Design 完成（AXI DMA + HP0 连接 + 地址分配 + bitstream）
├── PetaLinux 2025.2 安装 + sstate 配置
└── ARM 交叉编译链 C++20 验证（std::span）← Week 1 Day 3 已完成
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

### AXI-Stream 握手协议（简要版）

> 🔗 **详细版请见第五章 5-5 节**，包含时序波形、物理电压含义、为什么不能自己定义 1/0。

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

### Zero Copy 全链路

```
PL HLS 算子
  → 通过 AXI-HP S2MM 直写 DDR（物理地址 0x3F400000）
  → ARM mmap() 已经把这块物理地址映射到进程虚拟空间
  → ARM 调用 __builtin___clear_cache() 使 Cache 失效  ← HP 端口绕过 SCU，必须手动
  → ARM 通过 std::span<const T> 直接读取（零拷贝，零 malloc）
  → Tracker 在原地更新 g_track_pool（静态数组，.bss 段）
  → Zenoh z_publisher_put_owned() 直接指向 g_track_pool 的物理地址
  → 以太网 MAC DMA 从同一物理地址读取
  → 数据出网口

全程 memcpy 次数：0
全程 heap allocation：0
```

---

## 五、AXI 总线体系深度教材（Textbook）

> **本章定位：** 针对"AXI 这么多种类怎么区分"这一高频疑问，从第一性原理出发，完整讲解 Zynq-7020 上涉及的所有 AXI 相关概念。适合作为独立参考资料。

---

### 5-1 先搞清楚：协议 vs 物理端口

AXI 相关的词汇分两个维度，必须先把这两个维度分开理解，否则永远乱：

```
维度一：协议（数据怎么打包传输）
  AXI-Lite   ─── 慢速寄存器读写，有地址，一次一个，双方握手
  AXI4 (full)─── 高速块传输，有地址，支持突发（Burst）
  AXI-Stream ─── 流式数据，无地址，像水管一样连续流动

维度二：物理端口（数据从哪里进出 PS）
  AXI-GP     ─── ARM 伸进 PL 的手（General Purpose，通用）
  AXI-HP     ─── PL 伸进 DDR 的高速通道（High Performance，高性能）
  AXI-ACP    ─── PL 伸进 DDR 且经过 Cache 同步（Accelerator Coherency Port）
```

**类比：** 协议 = 快递规则（普通件/加急件/大件货运），端口 = 快递入口（前台/货运大门/员工通道）。快递规则和入口是两个正交的概念。

一个完整的数据通路通常同时涉及两者。例如：
- ARM 用 **AXI-Lite 协议** 通过 **GP 端口** 写 CFAR 的控制寄存器
- PL 的 DMA 用 **AXI4 协议** 通过 **HP 端口** 把处理后的数据写回 DDR
- PL 的 CFAR 算子和 DMA 之间用 **AXI-Stream 协议** 传输数据流

**完整关系总表：**

| 协议/端口 | 类型 | 有无地址 | 有无 TVALID/TREADY | Zenith 用途 |
|---|---|---|---|---|
| AXI-Lite | 协议 | ✅ 有 | ✅ 有（简化版） | ARM 写 HLS 算子控制寄存器 |
| AXI4 (full) | 协议 | ✅ 有 | ✅ 有 | DMA 突发传输大块数据 |
| AXI-Stream | 协议 | ❌ 无 | ✅ 有（核心机制） | 算子之间的数据流 |
| AXI-GP | 物理端口 | — | — | ARM 主动发指令给 PL |
| AXI-HP | 物理端口 | — | — | PL 主动搬大量数据到 DDR |
| AXI-ACP | 物理端口 | — | — | PL 搬数据 + 自动 Cache 同步 |

---

### 5-2 三种物理端口：GP / HP / ACP

Zynq-7020 的 PS 对外暴露三类 AXI 端口，功能和使用场景完全不同：
#### AXI-GP（General Purpose，通用端口）

```
ARM ──[AXI-GP Master]──► PL 的 AXI-Lite Slave 接口
                         （写 HLS 控制寄存器、读状态）
```

- **方向：** ARM 是 Master（主动方），PL 是 Slave（被动接受）
- **带宽：** 低，约 100-200 MB/s
- **协议：** AXI4 或 AXI-Lite
- **Zynq-7020 数量：** 2 个 GP Master (M_AXI_GP0, M_AXI_GP1)
- **Zenith 用途：** ARM 写 CFAR 的 `threshold_alpha` 寄存器、ARM 启动 HLS 算子（写 `ap_start` 位）
- **类比：** 前台接待——ARM 走进来，登记，写东西，然后走出去

#### AXI-HP（High Performance，高性能端口）

```
PL 的 DMA Master ──[AXI-HP Slave]──► DDR 控制器 ──► DDR 芯片
```

- **方向：** PL 是 Master（主动方），PS/DDR 是 Slave（被动接受）
- **带宽：** 高，约 600-1200 MB/s（64-bit 总线宽度）
- **Cache 一致性：** ❌ 无自动同步，ARM 必须手动调用 `__builtin___clear_cache()` 才能读到 PL 写入的最新数据
- **Zynq-7020 数量：** 4 个 HP Slave (S_AXI_HP0 ~ HP3)
- **Zenith 用途：** CFAR/FFT 算子的处理结果（IQ 数据、Range-Doppler Map、点迹）经由 DMA 通过 HP0 写入 DDR 的 RX Buffer
- **类比：** 货运大门——PL 直接把货物（数据）堆进仓库（DDR），不经过前台，速度极快，但仓库的库存系统（Cache）不会自动更新

#### AXI-ACP（Accelerator Coherency Port，加速器一致性端口）

```
PL 的 DMA Master ──[AXI-ACP Slave]──► SCU ──► L2 Cache / DDR
                                       ↓
                                    自动通知 ARM L1/L2 Cache 失效
```

- **方向：** PL 是 Master（主动方），PS 是 Slave
- **带宽：** 中等，约 400-600 MB/s（经过 SCU 仲裁，有开销）
- **Cache 一致性：** ✅ 自动，SCU（Snoop Control Unit）硬件自动维护
- **Zynq-7020 数量：** 1 个 ACP Slave
- **Zenith 用途：** 目前未使用（HP 带宽已够），企业版 DBF 多通道时可考虑
- **类比：** 特殊通道——PL 进来存货时，仓库管理系统（SCU）同步更新库存记录（Cache），ARM 下次查库存直接读到最新数据，不需要人工对账

---

### 5-3 AXI-ACP 与 SCU 工作原理

ACP 之所以能自动维护 Cache 一致性，全靠一个叫 **SCU（Snoop Control Unit，监听控制单元）** 的硬件组件。

```
Zynq-7020 内部结构（ACP 数据路径）：

ARM Core 0 ──┐
             ├── L1 D-Cache (32KB)
ARM Core 1 ──┘       │
                     L2 Cache (512KB)
                         │
                     SCU（Snoop Control Unit）
                    ╱           ╲
        AXI-ACP 入口          DDR 控制器
              ↑                     ↑
             PL                  AXI-HP 入口
                                      ↑
                                     PL
```

**SCU 的工作流程（ACP 路径）：**

1. PL 通过 AXI-ACP 发起写操作（例如把 CFAR 结果写到 DDR 地址 0x3F400000）
2. 写请求先到达 **SCU**
3. SCU 检查：ARM 的 L1/L2 Cache 里有没有缓存 0x3F400000 这个地址的数据？
4. 如果有 → SCU 自动将对应 Cache Line 标记为 **Invalid（无效）**
5. SCU 再把数据写入 DDR
6. ARM 下次读 0x3F400000 → Cache Miss → 自动从 DDR 取最新数据 ✅

**对比 HP 路径（无 SCU）：**

1. PL 通过 AXI-HP 直接写 DDR（绕过 SCU）
2. DDR 里有最新数据 ✅
3. ARM 的 Cache 里可能还是旧数据 ❌（SCU 不知道这次写操作）
4. ARM 读 0x3F400000 → Cache Hit → 读到旧数据 ❌
5. 必须显式调用 `__builtin___clear_cache()` 手动 Invalidate → 才能读到新数据

**代码对比：**

```cpp
// 使用 HP 端口时（Zenith 当前方案）：
void read_dma_results_hp(std::span<uint8_t> rx_buf) noexcept {
    // 必须在读之前手动 invalidate，否则读到 Cache 里的旧数据
    __builtin___clear_cache(rx_buf.data(), rx_buf.data() + rx_buf.size());
    // 现在可以安全读取 PL 写入的最新数据
    process(rx_buf);
}

// 使用 ACP 端口时（无需 invalidate）：
void read_dma_results_acp(std::span<uint8_t> rx_buf) noexcept {
    // SCU 已经自动 invalidate 了，直接读
    process(rx_buf);  // 更简洁，但带宽只有 HP 的一半
}
```

---

### 5-4 为什么 Zenith 默认用 HP 而不用 ACP

这是一个有意识的工程权衡，不是随意选择：

```
Zenith 雷达数据量估算（典型配置）：
  1024 range cells × 64 chirps × 4 bytes (float32) × 2 (IQ) = 512KB per CPI
  PRF = 1 kHz → 512MB/s 峰值带宽需求

AXI-HP 带宽：~1200 MB/s → 充裕（峰值需求的 2.3 倍余量）
AXI-ACP 带宽：~600 MB/s  → 勉强够（仅 1.2 倍余量，无法应对 DBF 多通道扩展）
```

**决策矩阵：**

| 因素 | HP 选择 | ACP 选择 |
|---|---|---|
| 带宽余量 | ✅ 2.3× 余量 | ⚠️ 1.2× 余量 |
| Cache 管理 | 需手动一行代码 | 全自动 |
| 代码复杂度 | +1 行 `__builtin___clear_cache` | 无额外代码 |
| 未来 DBF 扩展 | ✅ 带宽够 | ❌ 可能不够 |
| 延迟 | 低（直连 DDR） | 稍高（SCU 仲裁） |

**结论：** 手动 invalidate 是一行代码，换来 50% 的带宽提升和更充裕的扩展余量。这个交换是值得的。

---

### 5-5 AXI-Stream 时序波形：TVALID/TREADY 是什么

在后续 Q&A 中遇到了这样的时序表示：

```
TREADY: 0000000000000001  ← 很晚才变 1
TVALID: 0000111111111111  ← 早就是 1 了
```

这不是一个数字，是**时间轴上的波形**。每一位代表一个时钟周期的电平状态：

```
每一位 = 一个时钟周期
0 = 低电平（逻辑 false，该信号无效）
1 = 高电平（逻辑 true，该信号有效）

TREADY: 0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  1
        ↑                                              ↑
      第1个时钟周期                            第16个时钟周期
      TREADY=0（DMA 还没准备好）              TREADY=1（DMA 准备好了）

TVALID: 0  0  0  0  1  1  1  1  1  1  1  1  1  1  1  1
                    ↑
                  第5个时钟周期
                  TVALID=1（PL 开始发数据）
```

画成波形图：

```
时钟:   __|‾|_|‾|_|‾|_|‾|_|‾|_|‾|_|‾|_|‾|_|‾|_|‾|_|‾|_|‾|_|‾|_|‾|_
TVALID: ____________|‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
TREADY: ______________________________________________|‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
数据:                                                  ↑ 这里才真正传输
         TVALID=1 但 TREADY=0 的这段时间：数据在等待，PL 必须保持 TVALID 稳定
```

**传输条件（AXI-Stream 协议核心规则）：**

```
数据传输发生当且仅当：TVALID=1 AND TREADY=1（同一时钟上升沿）

TVALID=1, TREADY=0 → 数据等待（Backpressure，背压）
TVALID=0, TREADY=1 → DMA 空闲等待数据
TVALID=0, TREADY=0 → 双方都没准备好
TVALID=1, TREADY=1 → ✅ 数据成功传输
```

---

### 5-6 TVALID 和 TREADY 的物理含义——为什么不能自己定义 1/0

这是一个很好的问题，理解这个问题的答案，就理解了数字电路的物理本质。

**数字电路里 1 和 0 不是人为规定的，是物理电压：**

```
逻辑 1 = 高电平 ≈ 3.3V 或 1.8V（取决于工艺节点和电源轨）
逻辑 0 = 低电平 ≈ 0V（地）

TVALID = 1 的物理含义：
  发送方把 TVALID 这根导线的电压驱动到 ~1.8V
  接收方的输入缓冲器在时钟上升沿采样这根线
  采样到高电平 → 触发器翻转 → 寄存器状态 = "有效数据到来"
```

**为什么不能重新定义 1 为无效、0 为有效：**

AXI 协议是 ARM 公司制定的 AMBA 国际标准，规范里写明 TVALID 高电平有效。全球所有遵守这个标准的 IP 供应商——Xilinx 的 AXI DMA、AXI Interconnect、ILA 调试核、第三方 FFT IP——内部的逻辑门都按照"检测到 TVALID 高电平 → 处理数据"来设计。

如果你重定义"低电平=有效"：
- 你的 CFAR 输出的 TVALID（低电平）会被 DMA 的 TREADY 判断逻辑误认为无效
- DMA 永远不会接受你的数据
- 除非你同时修改 DMA 的源码，以及 Interconnect，以及所有下游 IP 的判断逻辑
- 这等同于发明一套新协议，与全球生态系统完全不兼容

**这就是协议标准的价值：** 大家约定好同一套电平含义，才能把不同公司设计的 IP 核插在一起直接工作。

---

### 5-7 Arm the DMA：完整操作语义

"Arm"在英文军事语境里的含义是**"给武器装填弹药，使其进入待发射状态"**。

```
手枪类比：
  未 armed 状态：枪里没子弹，扣扳机没反应
  armed 状态：子弹上膛，扣扳机立即射击，无延迟

DMA 类比：
  未 armed：S2MM_LENGTH 寄存器未写入，TREADY=0
             PL 发来数据，DMA 无法接收，数据堆积在 AXI-Stream FIFO 里
  armed：S2MM_LENGTH 写入后，DMA 硬件内部状态机进入 "等待数据" 状态
          TREADY 信号被驱动为 HIGH
          PL 一开始发数据，DMA 立即接收
```

**ARM the DMA 的完整代码序列：**

```cpp
// 必须严格按以下顺序执行：

// Step 1: 写目标物理地址到 S2MM_DA 寄存器
//   告诉 DMA："收到的数据放到 DDR 的哪里"
volatile uint32_t* dma = reinterpret_cast<volatile uint32_t*>(
    mmap(nullptr, 0x10000, PROT_READ|PROT_WRITE, MAP_SHARED, devmem_fd, AXI_DMA_BASE)
);
dma[0x48/4] = static_cast<uint32_t>(RX_PHYS_BASE);  // S2MM_DA = 0x3F400000

// Step 2: 写传输字节数到 S2MM_LENGTH 寄存器
//   这一步是关键触发器：写 LENGTH 后，DMA 硬件立即将 TREADY 拉高
dma[0x58/4] = static_cast<uint32_t>(transfer_size);  // ← TREADY 变为 1

// ↑ DMA 已经 "armed"
// TREADY 此刻 = 1，PL 可以开始发数据了

// Step 3: 触发 PL 开始工作（写 HLS 算子的 ap_start 位）
dma[0x00/4] = 0x1;  // 或写 CFAR 控制寄存器的 AP_START 位

// Step 4: PL 开始产生数据，TVALID=1
//   由于 TREADY 已经是 1，数据立即传输，DMA 写入 DDR

// Step 5: ARM 轮询等待完成
uint32_t status;
do {
    status = dma[0x34/4];  // 读 S2MM_DMASR（状态寄存器）
} while (!(status & (1 << 1)));  // 等待 Idle 位置 1

// Step 6: 读取结果（零拷贝）
// ⚠️ 使用 HP 端口时，必须先 invalidate cache
__builtin___clear_cache(
    reinterpret_cast<char*>(RX_PHYS_BASE),
    reinterpret_cast<char*>(RX_PHYS_BASE + transfer_size)
);
// 然后通过 std::span 直接读取，无 memcpy
auto results = std::span<const Detection>(
    reinterpret_cast<const Detection*>(RX_PHYS_BASE),
    transfer_size / sizeof(Detection)
);
```

下面进行逐行拆解，每个符号都解释清楚。
```cpp
//═══════════════════════════════════════════════════════════════════
// Step 1: 把 AXI DMA 的控制寄存器映射到 ARM 进程的虚拟地址空间
// ═══════════════════════════════════════════════════════════════════

volatile uint32_t* dma = reinterpret_cast<volatile uint32_t*>(
    mmap(nullptr, 0x10000, PROT_READ | PROT_WRITE, MAP_SHARED, devmem_fd, AXI_DMA_BASE)
);
```

**`volatile`**
告诉编译器：这块内存随时可能被硬件（DMA 控制器）修改，禁止优化掉任何读写操作。没有 `volatile`，编译器可能把连续两次相同的寄存器读优化成只读一次（缓存在寄存器里），导致状态轮询永远读到旧值。

**`uint32_t*`**
指向 32 位无符号整数的指针。AXI DMA 的每个控制寄存器宽度都是 32 bit，用 `uint32_t` 保证类型宽度精确，不受平台影响（不像 `int` 在不同平台可能是 16/32/64 bit）。

**`reinterpret_cast<volatile uint32_t*>(...)`**
强制类型转换：把 `mmap()` 返回的 `void*`（通用指针）重新解释为 `volatile uint32_t*`。`reinterpret_cast` 是 C++ 中"我知道我在做什么，编译器不要拦我"的显式转换，比 C 风格的 `(volatile uint32_t*)` 更安全，搜索时可见。

**`mmap()`** — 参数逐一拆解：
```
mmap(
    nullptr,              // 参数1: 建议映射到哪个虚拟地址
                          //   nullptr = 让内核自己选一个空闲虚拟地址
                          //   不要手动指定，否则可能与已有映射冲突

    0x10000,              // 参数2: 映射大小（字节）
                          //   0x10000 = 65536 字节 = 64KB
                          //   AXI DMA 寄存器实际只占 0x60 字节
                          //   但 mmap 的粒度必须是页的整数倍（4KB）
                          //   64KB 是保守安全值，覆盖所有寄存器偏移

    PROT_READ | PROT_WRITE, // 参数3: 访问权限
                          //   PROT_READ  = 允许 ARM 读寄存器（读状态）
                          //   PROT_WRITE = 允许 ARM 写寄存器（发命令）
                          //   | = 按位或，同时赋予两个权限

    MAP_SHARED,           // 参数4: 映射类型
                          //   MAP_SHARED  = 写操作直接穿透到物理地址
                          //   MAP_PRIVATE = 写操作只改本进程的副本（copy-on-write）
                          //   必须用 MAP_SHARED，否则写寄存器的值不会到达硬件

    devmem_fd,            // 参数5: 文件描述符
                          //   事先用 open("/dev/mem", O_RDWR | O_SYNC) 打开的句柄
                          //   /dev/mem 是 Linux 暴露物理内存的特殊字符设备
                          //   O_SYNC 确保写操作立即提交到总线，不经过缓冲

    AXI_DMA_BASE          // 参数6: 物理地址偏移（文件内偏移）
                          //   = 0x43000000，来自 zenith_memory_map.hpp
                          //   即 /proc/iomem 中确认的 AXI DMA 物理基地址
                          //   mmap 把物理地址 0x43000000 开始的 64KB
                          //   映射到 ARM 进程的某个虚拟地址
);
```

**`dma` 变量的含义** 映射完成后，dma 是一个指向虚拟地址的指针，但它背后对应的物理地址是 0x43000000。ARM 通过这个指针读写内存，MMU 自动把虚拟地址翻译成物理地址，AXI 总线把这个物理地址的访问路由到 PL 里的 DMA 控制器寄存器。

---

```cpp
dma[0x48/4] = static_cast<uint32_t>(RX_PHYS_BASE);
```

**`dma[0x48/4]`AXI DMA 寄存器** 是按字节偏移定义的（Xilinx 手册写"S2MM_DA offset = 0x48"）。但 `dma` 是 `uint32_t*`，指针运算单位是 4 字节。`dma[n]` 等价于 `*(dma + n)`，物理偏移是 `n × 4` 字节。所以要访问字节偏移 `0x48`，需要写 `dma[0x48/4] = dma[18]`。写成 `0x48/4` 而不是直接写 `18`，是为了让读代码的人能直接对照寄存器手册，不需要心算。

`S2MM_DA`（**Stream-to-Memory-Map Destination Address**）S2MM 是 PL→DDR 方向（FPGA 结果写回内存）。DA = Destination Address，目标地址。这个寄存器告诉 DMA：接收到的数据要存放到 DDR 的哪个物理地址。
`static_cast<uint32_t>(RX_PHYS_BASE)` `RX_PHYS_BASE = CMA_PHYS_BASE + RX_OFFSET = 0x3F000000 + 0x400000 = 0x3F400000`。`static_cast<uint32_t>` 是 C++ 显式类型转换：把 `uintptr_t`（64-bit）截断为 `uint32_t`（32-bit），因为寄存器只有 32 位宽，Zynq-7020 的物理地址空间也只有 32 位。

---

```cpp
dma[0x58/4] = static_cast<uint32_t>(transfer_size);
```
`dma[0x58/4]`字节偏移 `0x58` = S2MM_LENGTH 寄存器。同上，除以 4 转换为 `uint32_t*` 的索引。

`S2MM_LENGTH`（Stream-to-Memory-Map Transfer Length）写入这个寄存器的动作是整个 DMA 启动序列的触发点。Xilinx AXI DMA 的硬件状态机设计：写入 LENGTH 寄存器 → 内部 S2MM 引擎进入 "run" 状态 → AXI-Stream 的 `TREADY` 信号被硬件驱动为高电平 → DMA 开始等待 PL 发来数据。这就是"Arm the DMA"的字面含义：这一行代码让武器进入待发射状态。

`transfer_size` 要接收的字节数。对于 1024 range cells，每个 cell 是 `ap_fixed<16,8>` = 2 字节，一帧 = `1024 × 2 = 2048` 字节。这个数字必须和 CFAR 实际会发出的数据量精确匹配，否则 DMA 提前停止（数据截断）或等待超时（数据不足）。

---

```cpp
dma[0x00/4] = 0x1;
```
`dma[0x00/4]` 字节偏移 `0x00`，即 `MM2S_DMACR`（MM2S DMA Control Register）。等等，这里其实有一个细节需要注意——

这行代码的注释说"触发 PL 开始工作"，但写的是 MM2S（
Memory-to-Stream，DDR→PL 方向）的控制寄存器，不是 CFAR 算子的 `AP_START`。在完整的 Zenith 系统里，Step 3 应该是：
```cpp
// 正确 Step 3A：如果 PL 侧是由 MM2S 驱动数据进 HLS 算子
dma[0x00/4] = 0x1;  // MM2S_DMACR: bit0=RS(Run/Stop)=1，启动 MM2S 通道

// 正确 Step 3B：如果 HLS 算子由 AXI-Lite ap_ctrl_hs 独立控制
cfar_ctrl[0x00/4] = 0x1;  // CFAR 控制寄存器 offset 0x00，bit0=AP_START
```
`0x1` 32 位寄存器，写入值 `0x00000001`。对于 DMA 控制寄存器，bit0 是 `RS`（Run/Stop）位：`1` = Run（启动），`0` = Stop。对于 HLS 算子控制寄存器，bit0 是`ap_start`：写 1 触发算子执行一次（或持续运行，取决于 HLS 综合时的控制模式选择）。

---

```cpp
uint32_t status;
do {
    status = dma[0x34/4];
} while (!(status & (1 << 1)));
```
`uint32_t status` 临时变量，存放从寄存器读回的 32 位状态值。`uint32_t` 保证宽度与寄存器匹配。
`dma[0x34/4]` 字节偏移 `0x34` = `S2MM_DMASR`（S2MM DMA Status Register）。这是 S2MM 通道的只读状态寄存器，硬件实时更新它的位域。

`S2MM_DMASR` **关键位域**：

| 位     | 名称       | 含义                          |
| ----- | -------- | --------------------------- |
| bit0  | Halted   | 1 = DMA 停止（错误或未启动）          |
| bit1  | **Idle** | 1 = 传输完成，DMA 回到空闲           |
| bit4  | SGIncld  | Scatter-Gather 模式包含         |
| bit12 | IRQDelay | 延迟中断                        |
| bit13 | IOCIrq   | 完成中断（Interrupt on Complete） |

`(status & (1 << 1))`位掩码操作，提取第 1 位（Idle 位）：
- `1 << 1` = `0b00000010` = `0x00000002`（把 1 左移 1 位）
- `status & 0x2` = 只保留 status 的第 1 位，其余位清零
- 结果非零 → Idle=1 → 传输完成
- 结果为零 → Idle=0 → 还在传输中

`!(status & (1 << 1))` 逻辑非：`!0 = true`（继续循环），`!非零 = false`（退出循环）。整个条件："只要 Idle 位不是 1，就继续轮询"。

`do { } while()` 先执行一次再判断条件（区别于 `while` 的先判断后执行）。用 `do-while` 是因为传输刚启动时 Idle 位肯定还是 0，先读一次状态再判断，避免"刚启动就误判为完成"的竞争条件。

⚠️ 生产代码的改进点
这个裸轮询（busy-wait）会把 ARM Core 0 的 100% CPU 时间用在等待上。M1 阶段用于验证流程是完全合理的。M4 实时系统阶段应改为中断驱动（写 IOCIrq 使能位，DMA 完成时触发 Linux IRQ，ARM 进入睡眠等待唤醒），释放 CPU 给 Tracker 和 Zenoh 使用。

---

### 5-8 死锁场景：不 Arm 先 Trigger 会发生什么

这是最常见的 DMA 调试陷阱，理解它可以节省数小时调试时间。

**错误操作序列：**

```
Step 1（错误）: 先触发 PL 开始工作（AP_START=1）
Step 2（错误）: 然后才去写 DMA 寄存器
```

**时序图：**

```
时钟:   __|‾|_|‾|_|‾|_|‾|_|‾|_|‾|_|‾|_|‾|_|‾|_|‾|_|‾|_
TVALID: ____________|‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
TREADY: ______________________________________|‾‾‾‾‾‾‾‾‾‾
        ↑            ↑                        ↑
     PL 被触发      TVALID=1               TREADY 终于变 1
                  但 TREADY=0             （DMA 终于被 armed）
                  数据在这里等着...
```

**会发生什么：**

TVALID=1 但 TREADY=0，根据 AXI-Stream 协议，**PL 的 AXI-Stream 接口必须保持 TVALID 和数据稳定，直到 TREADY 变为 1**。这不是 Bug，这是协议规定的背压机制。

但是：
1. PL 内部的 AXI-Stream FIFO（通常深度 16-512）开始积累等待传输的数据
2. 当 TREADY 终于变为 1 时，FIFO 里积累的数据（可能是第 5、6、7... 帧的数据）全部涌出
3. TLAST 信号（帧边界标记）的位置相对于数据流已经错位
4. DMA 接收到的"帧"边界与实际 CFAR 处理帧不对齐
5. ARM 读到的数据是多个帧混合的乱序数据

**结果：** 数据在内存里，但已经损坏。ARM 代码不会崩溃（它成功读到了一些数字），但这些数字是无意义的。这种 Bug 极难调试，因为"看起来工作了"——只有当你用 MATLAB 验证 Range-Doppler Map 时才会发现 CFAR 结果完全不对。

**正确规则（永久记忆）：**
> **先 Arm DMA（写 S2MM_DA + S2MM_LENGTH），再 Trigger PL（写 AP_START）。也就是先准备好接收方，再使能发送方。永远。**

---

### 5-9 AXI 全景图

把本章所有内容整合成一张图：

```
                        Zynq-7020 Die
┌───────────────────────────────────────────────────────────────────────┐
│  PS（Processing System）              PL（Programmable Logic）         │
│  ┌─────────────────────────┐          ┌────────────────────────────┐  │
│  │  ARM Core 0             │          │  HLS 算子                  │  │
│  │  ARM Core 1             │          │  (CFAR/FFT/DDS)            │  │
│  │    │                    │          │     │ AXI-Stream            │  │
│  │  L1/L2 Cache            │          │     ▼                      │  │
│  │    │                    │          │  AXI DMA IP                │  │
│  │  SCU（Snoop Unit）◄─────┼──AXI-ACP─┤     │                      │  │
│  │    │       自动失效      │          │  S2MM（PL→DDR）│           │  │
│  │  DDR 控制器             │◄──AXI-HP─┤─────┘ 直连DDR              │  │
│  │    │       1200MB/s     │          │                            │  │
│  │  M_AXI_GP0 Master──────┼──AXI-Lite►│ CFAR/HLS 控制寄存器        │  │
│  │  （ARM 发命令给 PL）    │          │                            │  │
│  └─────────────────────────┘          └────────────────────────────┘  │
│            │                                                           │
└────────────┼───────────────────────────────────────────────────────────┘
             ▼
          DDR3 芯片（1GB）
          CMA 区域 @ 0x3F000000（16MB）
          ├── TX Buffer 0x3F000000（4MB，PS→PL 波形配置）
          ├── RX Buffer 0x3F400000（4MB，PL→PS IQ/点迹）
          ├── BD Ring   0x3F800000（4KB，DMA 自动驾驶描述符）
          └── Track Buf 0x3F801000（1MB，Zenoh 输出）

数据流向（一次完整 CPI 处理）：
  ARM 写波形参数 → TX Buffer
  DMA MM2S → TX Buffer → HLS DDS 算子 → RF 发射
  接收回波 → HLS CFAR/FFT 处理
  DMA S2MM（通过 HP0）→ RX Buffer（写 DDR，绕过 SCU）
  ARM cache_invalidate（手动对账）
  ARM 通过 std::span 零拷贝读取 RX Buffer
  Kalman Tracker 更新 Track Buffer
  Zenoh 零拷贝发布 Track Buffer → 以太网
  全程 memcpy = 0，heap allocation = 0
```

---

## 快速参考表

### AXI 类型速查

| 名称 | 维度 | 关键特征 | Zenith 对应用途 |
|---|---|---|---|
| AXI-Stream | 协议 | 无地址，TVALID/TREADY 握手 | 算子间数据流 |
| AXI-Lite | 协议 | 有地址，简单 R/W | 控制寄存器 |
| AXI4 (full) | 协议 | 有地址，Burst 支持 | DMA 大块传输 |
| AXI-GP | 物理端口 | ARM→PL 方向，低带宽 | ARM 写控制寄存器 |
| AXI-HP | 物理端口 | PL→DDR，高带宽，无 Cache 同步 | DMA 数据回写 |
| AXI-ACP | 物理端口 | PL→DDR，中带宽，经 SCU 自动 Cache 同步 | 暂不使用 |

### "Arm the DMA" 操作口诀

```
先地址（S2MM_DA），再长度（S2MM_LENGTH），后触发（AP_START）
写了长度，TREADY 立刻拉高，DMA 进入 armed 状态
顺序颠倒 = TLAST 错位 = 数据损坏 = 极难调试的隐性 Bug
```

### Cache 一致性口诀

```
HP 端口 = 快 + 需手动 invalidate
ACP 端口 = 稍慢 + 全自动
Zenith 选 HP：transfer_size * PRF < 512MB/s，HP 余量充足
invalidate 就一行：__builtin___clear_cache(start, end)
```

---

*Week 1 Day 1 + Day 2 完整战地日志 v2*
*作者：Charley Chang | Project Zenith-Radar OS*
*生成时间：2026-03-12*
*v2 更新：新增第五章 AXI 总线体系深度教材，涵盖今日 Q&A 全部知识点*
*下一次更新：Day 3-4（ARM 交叉编译 + Vivado Block Design + CFAR 重综合）见独立日志*
