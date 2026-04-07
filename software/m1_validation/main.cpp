#include <iostream>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <span>      
#include <cstdint>   

constexpr uintptr_t AXI_DMA_BASE = 0x43000000; 
// 🛑 致命错误已修正：AX7020 只有 512MB 内存（上限 0x1FFFFFFF）
// 将读写地址改到 480MB 的安全区域
constexpr uintptr_t TX_PHYS_BASE = 0x1E000000; 
constexpr uintptr_t RX_PHYS_BASE = 0x1E400000; 

int main() {
    int fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd < 0) { perror("open /dev/mem"); return 1; }

    void* dma_map = mmap(nullptr, 0x10000, PROT_READ | PROT_WRITE, MAP_SHARED, fd, AXI_DMA_BASE);
    volatile uint32_t* dma = static_cast<volatile uint32_t*>(dma_map);

    void* tx_map = mmap(nullptr, 0x400000, PROT_READ | PROT_WRITE, MAP_SHARED, fd, TX_PHYS_BASE);
    void* rx_map = mmap(nullptr, 0x400000, PROT_READ | PROT_WRITE, MAP_SHARED, fd, RX_PHYS_BASE);

    uint32_t* tx_ptr = static_cast<uint32_t*>(tx_map);
    uint32_t* rx_ptr = static_cast<uint32_t*>(rx_map);

    std::span<uint32_t> tx_buf(tx_ptr, 1024);
    for (size_t i = 0; i < tx_buf.size(); ++i)
        tx_buf[i] = static_cast<uint32_t>(0xDEADBEEF + i);

    // 🌟 新增：启动前对 DMA 进行硬件级软复位，清空上次崩溃留下的 Error 标志
    dma[0x00/4] = 0x4; // Reset MM2S
    dma[0x30/4] = 0x4; // Reset S2MM
    usleep(1000);      // 等待复位完成

    dma[0x30/4] = 0x1;                                    
    dma[0x48/4] = static_cast<uint32_t>(RX_PHYS_BASE);    
    dma[0x58/4] = 4096;                                   

    dma[0x00/4] = 0x1;                                    
    dma[0x18/4] = static_cast<uint32_t>(TX_PHYS_BASE);    
    dma[0x28/4] = 4096;                                   

    std::cout << "Transferring..." << std::endl;
    int timeout = 10000;
    // 轮询 S2MM_DMASR 的 Idle 位 (Bit 1)
    while (!(dma[0x34/4] & 0x02) && timeout > 0) {
        usleep(100); 
        timeout--;
    }
    
    if (timeout <= 0) {
        std::cout << "M1 FAILURE: DMA Timeout!" << std::endl;
        // 🌟 新增：如果锁死，直接从硬件读出死因！
        std::cout << "MM2S Status (0x04): 0x" << std::hex << dma[0x04/4] << std::endl;
        std::cout << "S2MM Status (0x34): 0x" << std::hex << dma[0x34/4] << std::endl;
        return 1;
    }

    // 清除缓存，强迫 CPU 读取 DDR 里的真实数据
    __builtin___clear_cache(rx_map, (char*)rx_map + 4096);

    std::span<uint32_t> rx_buf(rx_ptr, 1024);
    if (rx_buf[0] == 0xDEADBEEF) {
        std::cout << "ZENITH M1 SUCCESS: Loopback Verified!" << std::endl;
    } else {
        std::cout << "M1 FAILURE: Data Mismatch. Rx[0] = 0x" << std::hex << rx_buf[0] << std::endl;
    }

    close(fd);
    return 0;
}