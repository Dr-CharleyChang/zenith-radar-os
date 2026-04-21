// ==============================================================
// Vitis HLS - High-Level Synthesis from C, C++ and OpenCL v2025.2 (64-bit)
// Tool Version Limit: 2025.11
// Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
// Copyright 2022-2025 Advanced Micro Devices, Inc. All Rights Reserved.
// 
// ==============================================================
/***************************** Include Files *********************************/
#include "xfft_1d_top.h"

/************************** Function Implementation *************************/
#ifndef __linux__
int XFft_1d_top_CfgInitialize(XFft_1d_top *InstancePtr, XFft_1d_top_Config *ConfigPtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(ConfigPtr != NULL);

    InstancePtr->Ctrl_BaseAddress = ConfigPtr->Ctrl_BaseAddress;
    InstancePtr->IsReady = XIL_COMPONENT_IS_READY;

    return XST_SUCCESS;
}
#endif

void XFft_1d_top_Start(XFft_1d_top *InstancePtr) {
    u32 Data;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XFft_1d_top_ReadReg(InstancePtr->Ctrl_BaseAddress, XFFT_1D_TOP_CTRL_ADDR_AP_CTRL) & 0x80;
    XFft_1d_top_WriteReg(InstancePtr->Ctrl_BaseAddress, XFFT_1D_TOP_CTRL_ADDR_AP_CTRL, Data | 0x01);
}

u32 XFft_1d_top_IsDone(XFft_1d_top *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XFft_1d_top_ReadReg(InstancePtr->Ctrl_BaseAddress, XFFT_1D_TOP_CTRL_ADDR_AP_CTRL);
    return (Data >> 1) & 0x1;
}

u32 XFft_1d_top_IsIdle(XFft_1d_top *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XFft_1d_top_ReadReg(InstancePtr->Ctrl_BaseAddress, XFFT_1D_TOP_CTRL_ADDR_AP_CTRL);
    return (Data >> 2) & 0x1;
}

u32 XFft_1d_top_IsReady(XFft_1d_top *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XFft_1d_top_ReadReg(InstancePtr->Ctrl_BaseAddress, XFFT_1D_TOP_CTRL_ADDR_AP_CTRL);
    // check ap_start to see if the pcore is ready for next input
    return !(Data & 0x1);
}

void XFft_1d_top_EnableAutoRestart(XFft_1d_top *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XFft_1d_top_WriteReg(InstancePtr->Ctrl_BaseAddress, XFFT_1D_TOP_CTRL_ADDR_AP_CTRL, 0x80);
}

void XFft_1d_top_DisableAutoRestart(XFft_1d_top *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XFft_1d_top_WriteReg(InstancePtr->Ctrl_BaseAddress, XFFT_1D_TOP_CTRL_ADDR_AP_CTRL, 0);
}

void XFft_1d_top_Set_scaling_schedule(XFft_1d_top *InstancePtr, u32 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XFft_1d_top_WriteReg(InstancePtr->Ctrl_BaseAddress, XFFT_1D_TOP_CTRL_ADDR_SCALING_SCHEDULE_DATA, Data);
}

u32 XFft_1d_top_Get_scaling_schedule(XFft_1d_top *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XFft_1d_top_ReadReg(InstancePtr->Ctrl_BaseAddress, XFFT_1D_TOP_CTRL_ADDR_SCALING_SCHEDULE_DATA);
    return Data;
}

void XFft_1d_top_InterruptGlobalEnable(XFft_1d_top *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XFft_1d_top_WriteReg(InstancePtr->Ctrl_BaseAddress, XFFT_1D_TOP_CTRL_ADDR_GIE, 1);
}

void XFft_1d_top_InterruptGlobalDisable(XFft_1d_top *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XFft_1d_top_WriteReg(InstancePtr->Ctrl_BaseAddress, XFFT_1D_TOP_CTRL_ADDR_GIE, 0);
}

void XFft_1d_top_InterruptEnable(XFft_1d_top *InstancePtr, u32 Mask) {
    u32 Register;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Register =  XFft_1d_top_ReadReg(InstancePtr->Ctrl_BaseAddress, XFFT_1D_TOP_CTRL_ADDR_IER);
    XFft_1d_top_WriteReg(InstancePtr->Ctrl_BaseAddress, XFFT_1D_TOP_CTRL_ADDR_IER, Register | Mask);
}

void XFft_1d_top_InterruptDisable(XFft_1d_top *InstancePtr, u32 Mask) {
    u32 Register;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Register =  XFft_1d_top_ReadReg(InstancePtr->Ctrl_BaseAddress, XFFT_1D_TOP_CTRL_ADDR_IER);
    XFft_1d_top_WriteReg(InstancePtr->Ctrl_BaseAddress, XFFT_1D_TOP_CTRL_ADDR_IER, Register & (~Mask));
}

void XFft_1d_top_InterruptClear(XFft_1d_top *InstancePtr, u32 Mask) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XFft_1d_top_WriteReg(InstancePtr->Ctrl_BaseAddress, XFFT_1D_TOP_CTRL_ADDR_ISR, Mask);
}

u32 XFft_1d_top_InterruptGetEnabled(XFft_1d_top *InstancePtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    return XFft_1d_top_ReadReg(InstancePtr->Ctrl_BaseAddress, XFFT_1D_TOP_CTRL_ADDR_IER);
}

u32 XFft_1d_top_InterruptGetStatus(XFft_1d_top *InstancePtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    return XFft_1d_top_ReadReg(InstancePtr->Ctrl_BaseAddress, XFFT_1D_TOP_CTRL_ADDR_ISR);
}

