#include <cstdint>
#include <span>

// AXI DMA S2MM(接收通道)的物理寄存器映射（严格按照Xilinx官方手册）
struct AxiDmaS2mmRegs {
  volatile uint32_t cr; // 0x30: 控制寄存器
  volatile uint32_t sr; // 0x34: 状态寄存器
  uint32_t reserved1[4]; // 0x38 - 0x44: 物理空洞填充 (4个32位 = 16字节)
  volatile uint32_t da;     // 0x48: 目标地址
  volatile uint32_t da_msb; // 0x4C: 目标地址高位
  uint32_t reserved2[2]; // 0x50 - 0x54: 物理空洞填充 (2个32位 = 8字节)
  volatile uint32_t length; // 0x58: 传输长度
};

class AxiDmaController {

private:
  AxiDmaS2mmRegs *const s2mm_regs_;

public:
  // 构造函数：基地址加上 0x30 偏移量，直接指向 S2MM 通道寄存器组
  constexpr explicit AxiDmaController(uintptr_t base_addr) noexcept
      : s2mm_regs_(reinterpret_cast<AxiDmaS2mmRegs *>(base_addr + 0x30)){};

  // 启动接收传输 (Zero-Copy)
  [[nodiscard]] bool start_rx_transfer(std::span<uint32_t> rx_buffer) noexcept {
    if (rx_buffer.empty())
      return false;

    // 1. 确保 DMA 处于运行状态（CR寄存器 bit 0 设为 1）
    s2mm_regs_->cr |= 0x00000001;

    // 2. 检查DMA是否繁忙（SR 寄存器 bit 0 若为0，表示没有停机，可能正在传）
    // 严格来说，应该检查idle位，这里做简要防护
    if ((s2mm_regs_->sr & 0x00000002) == 0)
      return false;

    // 3. 写入目标物理地址
    s2mm_regs_->da = reinterpret_cast<uint32_t>(rx_buffer.data());

    // 4. 写入传输字节数（此写入动作会直接扣动DMA的物理扳机，开始搬运）
    s2mm_regs_->length = rx_buffer.size_bytes();

    return true;
  }
};