#include <cstdint>

// 严格按照 FPGA 端 AXI-Lite 寄存器地址的偏移量定义结构体
// 注意：必须保证不被编译器随意进行内存对齐填充 (Padding)
struct CfarHardwareMap {
    volatile uint32_t threshold;       // Offset 0x00
    volatile uint32_t status_flags;    // Offset 0x04
    volatile uint32_t target_count;    // Offset 0x08
    volatile uint32_t control_cmd;     // Offset 0x0C
};

class CfarEngineController {
private:
    CfarHardwareMap* const hw_regs_;

public:
    // 构造函数：强转为结构体指针
    constexpr explicit CfarEngineController(uintptr_t base_addr) noexcept
        : hw_regs_(reinterpret_cast<CfarHardwareMap*>(base_addr)) {}

    // 现代安全特性：[[nodiscard]] 强制检查，noexcept 拒绝异常开销
    [[nodiscard]] bool set_cfar_threshold(const uint32_t threshold_val) noexcept {
        hw_regs_->threshold = threshold_val; // 像操作普通变量一样操作底层硬件
        
        // 物理直觉：确认硬件并未处于 Busy 状态
        if ((hw_regs_->status_flags & 0x01) != 0) {
            return false; // 硬件正忙，设置失败
        }
        return true;
    }
};