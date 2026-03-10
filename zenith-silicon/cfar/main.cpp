#include <cstdint>
#include <span>
#include <array>

#include "xil_cache.h" // Xilinx 提供的底层缓存控制头文件

// 引入我们之前写好的硬件抽象层头文件
#include "hal/cfar_engine_controller.hpp"
#include "hal/axi_dma_controller.hpp"

// 架构师的绝对纪律：物理内存对齐 (Memory Alignment)
// Zynq-7020 的 Cache Line 是 32 字节，为了防止 Cache 颠簸和 DMA 传输错误，
// 接收缓冲区必须严格对齐到 64 字节边界。
alignas(64) static std::array<uint32_t, 1024> g_cfar_rx_buffer;

// 固化外设的物理基地址 (根据 Vivado Block Design 里的 Address Editor 决定)
constexpr uintptr_t CFAR_ENGINE_BASE_ADDR = 0x40000000;
constexpr uintptr_t AXI_DMA_BASE_ADDR     = 0x40400000;

// 实例化硬件控制器 (全编译期常量，零运行时开销)
static CfarEngineController cfar_engine(CFAR_ENGINE_BASE_ADDR);
static AxiDmaController     dma_engine(AXI_DMA_BASE_ADDR);

// 模拟底层的 Cache 无效化操作（在真实的 Xilinx 库中通常是 Xil_DCacheInvalidateRange）
// 在这里用空函数占位，但在物理层面它是生死攸关的！
inline void invalidate_dcache_range(uintptr_t addr, size_t length) noexcept {
    // 物理动作：强行作废 ARM L1/L2 数据缓存中这块地址的内容。
    // 下次 CPU 读这段内存，必须老老实实去 DDR 里取 FPGA 刚写进去的新鲜数据。
    Xil_DCacheInvalidateRange(addr, length);
}

int main() {
    // 1. 系统初始化与硬件自检
    // 唤醒雷达引擎，写入初始虚警阈值 (例如 15)
    if (!cfar_engine.set_cfar_threshold(15)) {
        // 硬件挂起，进入死循环等待看门狗复位
        while (true) {} 
    }

    // 2. 构建安全的内存视图 (C++20 std::span)
    // 把静态数组转换为 span，不发生任何拷贝！
    std::span<uint32_t> rx_span(g_cfar_rx_buffer);

    // 3. 核心主循环 (Deterministic Main Loop)
    while (true) {
        // 步骤 A: 把目的地的物理指针交给 DMA 包工头，启动接收
        if (!dma_engine.start_rx_transfer(rx_span)) {
            continue; // DMA 忙碌或故障，重试
        }

        // 步骤 B: 触发 PL 端的 CFAR 加速器开始一次帧处理
        cfar_engine.start_processing();

        // 步骤 C: 物理等待 (Polling)
        // 在未来的 ROS 2 中，这里会换成中断挂起 (Interrupt & Sleep)，交出 CPU 控制权。
        // 在裸机下，我们死等 DMA 的“传输完成”标志。
        while (!dma_engine.is_transfer_complete()) {
            // ARM 核空转，或者执行其他非关键计算
        }

        // 步骤 D: 架构师的生命线 —— Cache Coherence（缓存一致性）处理
        // 此时，FPGA 已经把数据打进了 DDR 物理内存。
        // 但 ARM CPU 以为数据没变，去读的话会读到 L1/L2 Cache 里的老旧脏数据！
        // 必须强制 CPU 丢弃 Cache，去物理 DDR 重新抓取数据。
        invalidate_dcache_range(
            reinterpret_cast<uintptr_t>(rx_span.data()), 
            rx_span.size_bytes()
        );

        // 步骤 E: 零拷贝数据处理！
        // 此时 rx_span 里已经是新鲜出炉的雷达目标点云了。
        // process_radar_points(rx_span); 
    }

    return 0;
}