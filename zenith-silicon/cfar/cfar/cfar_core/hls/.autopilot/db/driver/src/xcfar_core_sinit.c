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
#include "xcfar_core.h"

extern XCfar_core_Config XCfar_core_ConfigTable[];

#ifdef SDT
XCfar_core_Config *XCfar_core_LookupConfig(UINTPTR BaseAddress) {
	XCfar_core_Config *ConfigPtr = NULL;

	int Index;

	for (Index = (u32)0x0; XCfar_core_ConfigTable[Index].Name != NULL; Index++) {
		if (!BaseAddress || XCfar_core_ConfigTable[Index].Ctrl_BaseAddress == BaseAddress) {
			ConfigPtr = &XCfar_core_ConfigTable[Index];
			break;
		}
	}

	return ConfigPtr;
}

int XCfar_core_Initialize(XCfar_core *InstancePtr, UINTPTR BaseAddress) {
	XCfar_core_Config *ConfigPtr;

	Xil_AssertNonvoid(InstancePtr != NULL);

	ConfigPtr = XCfar_core_LookupConfig(BaseAddress);
	if (ConfigPtr == NULL) {
		InstancePtr->IsReady = 0;
		return (XST_DEVICE_NOT_FOUND);
	}

	return XCfar_core_CfgInitialize(InstancePtr, ConfigPtr);
}
#else
XCfar_core_Config *XCfar_core_LookupConfig(u16 DeviceId) {
	XCfar_core_Config *ConfigPtr = NULL;

	int Index;

	for (Index = 0; Index < XPAR_XCFAR_CORE_NUM_INSTANCES; Index++) {
		if (XCfar_core_ConfigTable[Index].DeviceId == DeviceId) {
			ConfigPtr = &XCfar_core_ConfigTable[Index];
			break;
		}
	}

	return ConfigPtr;
}

int XCfar_core_Initialize(XCfar_core *InstancePtr, u16 DeviceId) {
	XCfar_core_Config *ConfigPtr;

	Xil_AssertNonvoid(InstancePtr != NULL);

	ConfigPtr = XCfar_core_LookupConfig(DeviceId);
	if (ConfigPtr == NULL) {
		InstancePtr->IsReady = 0;
		return (XST_DEVICE_NOT_FOUND);
	}

	return XCfar_core_CfgInitialize(InstancePtr, ConfigPtr);
}
#endif

#endif

