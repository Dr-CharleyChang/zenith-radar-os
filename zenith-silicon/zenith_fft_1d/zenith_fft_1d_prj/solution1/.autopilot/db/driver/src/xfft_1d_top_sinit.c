// ==============================================================
// Vitis HLS - High-Level Synthesis from C, C++ and OpenCL v2025.2 (64-bit)
// Tool Version Limit: 2025.11
// Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
// Copyright 2022-2025 Advanced Micro Devices, Inc. All Rights Reserved.
// 
// ==============================================================
#ifndef __linux__

#include "xstatus.h"
#ifdef SDT
#include "xparameters.h"
#endif
#include "xfft_1d_top.h"

extern XFft_1d_top_Config XFft_1d_top_ConfigTable[];

#ifdef SDT
XFft_1d_top_Config *XFft_1d_top_LookupConfig(UINTPTR BaseAddress) {
	XFft_1d_top_Config *ConfigPtr = NULL;

	int Index;

	for (Index = (u32)0x0; XFft_1d_top_ConfigTable[Index].Name != NULL; Index++) {
		if (!BaseAddress || XFft_1d_top_ConfigTable[Index].Ctrl_BaseAddress == BaseAddress) {
			ConfigPtr = &XFft_1d_top_ConfigTable[Index];
			break;
		}
	}

	return ConfigPtr;
}

int XFft_1d_top_Initialize(XFft_1d_top *InstancePtr, UINTPTR BaseAddress) {
	XFft_1d_top_Config *ConfigPtr;

	Xil_AssertNonvoid(InstancePtr != NULL);

	ConfigPtr = XFft_1d_top_LookupConfig(BaseAddress);
	if (ConfigPtr == NULL) {
		InstancePtr->IsReady = 0;
		return (XST_DEVICE_NOT_FOUND);
	}

	return XFft_1d_top_CfgInitialize(InstancePtr, ConfigPtr);
}
#else
XFft_1d_top_Config *XFft_1d_top_LookupConfig(u16 DeviceId) {
	XFft_1d_top_Config *ConfigPtr = NULL;

	int Index;

	for (Index = 0; Index < XPAR_XFFT_1D_TOP_NUM_INSTANCES; Index++) {
		if (XFft_1d_top_ConfigTable[Index].DeviceId == DeviceId) {
			ConfigPtr = &XFft_1d_top_ConfigTable[Index];
			break;
		}
	}

	return ConfigPtr;
}

int XFft_1d_top_Initialize(XFft_1d_top *InstancePtr, u16 DeviceId) {
	XFft_1d_top_Config *ConfigPtr;

	Xil_AssertNonvoid(InstancePtr != NULL);

	ConfigPtr = XFft_1d_top_LookupConfig(DeviceId);
	if (ConfigPtr == NULL) {
		InstancePtr->IsReady = 0;
		return (XST_DEVICE_NOT_FOUND);
	}

	return XFft_1d_top_CfgInitialize(InstancePtr, ConfigPtr);
}
#endif

#endif

