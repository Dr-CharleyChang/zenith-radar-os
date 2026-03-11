// ==============================================================
// Vitis HLS - High-Level Synthesis from C, C++ and OpenCL v2025.2 (64-bit)
// Tool Version Limit: 2025.11
// Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
// Copyright 2022-2025 Advanced Micro Devices, Inc. All Rights Reserved.
// 
// ==============================================================
#ifndef XCFAR_CORE_H
#define XCFAR_CORE_H

#ifdef __cplusplus
extern "C" {
#endif

/***************************** Include Files *********************************/
#ifndef __linux__
#include "xil_types.h"
#include "xil_assert.h"
#include "xstatus.h"
#include "xil_io.h"
#else
#include <stdint.h>
#include <assert.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stddef.h>
#endif
#include "xcfar_core_hw.h"

/**************************** Type Definitions ******************************/
#ifdef __linux__
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
#else
typedef struct {
#ifdef SDT
    char *Name;
#else
    u16 DeviceId;
#endif
    u64 Ctrl_BaseAddress;
} XCfar_core_Config;
#endif

typedef struct {
    u64 Ctrl_BaseAddress;
    u32 IsReady;
} XCfar_core;

typedef u32 word_type;

/***************** Macros (Inline Functions) Definitions *********************/
#ifndef __linux__
#define XCfar_core_WriteReg(BaseAddress, RegOffset, Data) \
    Xil_Out32((BaseAddress) + (RegOffset), (u32)(Data))
#define XCfar_core_ReadReg(BaseAddress, RegOffset) \
    Xil_In32((BaseAddress) + (RegOffset))
#else
#define XCfar_core_WriteReg(BaseAddress, RegOffset, Data) \
    *(volatile u32*)((BaseAddress) + (RegOffset)) = (u32)(Data)
#define XCfar_core_ReadReg(BaseAddress, RegOffset) \
    *(volatile u32*)((BaseAddress) + (RegOffset))

#define Xil_AssertVoid(expr)    assert(expr)
#define Xil_AssertNonvoid(expr) assert(expr)

#define XST_SUCCESS             0
#define XST_DEVICE_NOT_FOUND    2
#define XST_OPEN_DEVICE_FAILED  3
#define XIL_COMPONENT_IS_READY  1
#endif

/************************** Function Prototypes *****************************/
#ifndef __linux__
#ifdef SDT
int XCfar_core_Initialize(XCfar_core *InstancePtr, UINTPTR BaseAddress);
XCfar_core_Config* XCfar_core_LookupConfig(UINTPTR BaseAddress);
#else
int XCfar_core_Initialize(XCfar_core *InstancePtr, u16 DeviceId);
XCfar_core_Config* XCfar_core_LookupConfig(u16 DeviceId);
#endif
int XCfar_core_CfgInitialize(XCfar_core *InstancePtr, XCfar_core_Config *ConfigPtr);
#else
int XCfar_core_Initialize(XCfar_core *InstancePtr, const char* InstanceName);
int XCfar_core_Release(XCfar_core *InstancePtr);
#endif

void XCfar_core_Start(XCfar_core *InstancePtr);
u32 XCfar_core_IsDone(XCfar_core *InstancePtr);
u32 XCfar_core_IsIdle(XCfar_core *InstancePtr);
u32 XCfar_core_IsReady(XCfar_core *InstancePtr);
void XCfar_core_EnableAutoRestart(XCfar_core *InstancePtr);
void XCfar_core_DisableAutoRestart(XCfar_core *InstancePtr);

void XCfar_core_Set_threshold_alpha(XCfar_core *InstancePtr, u32 Data);
u32 XCfar_core_Get_threshold_alpha(XCfar_core *InstancePtr);

void XCfar_core_InterruptGlobalEnable(XCfar_core *InstancePtr);
void XCfar_core_InterruptGlobalDisable(XCfar_core *InstancePtr);
void XCfar_core_InterruptEnable(XCfar_core *InstancePtr, u32 Mask);
void XCfar_core_InterruptDisable(XCfar_core *InstancePtr, u32 Mask);
void XCfar_core_InterruptClear(XCfar_core *InstancePtr, u32 Mask);
u32 XCfar_core_InterruptGetEnabled(XCfar_core *InstancePtr);
u32 XCfar_core_InterruptGetStatus(XCfar_core *InstancePtr);

#ifdef __cplusplus
}
#endif

#endif
