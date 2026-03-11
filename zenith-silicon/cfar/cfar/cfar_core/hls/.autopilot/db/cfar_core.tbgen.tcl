set moduleName cfar_core
set isTopModule 1
set isCombinational 0
set isDatapathOnly 0
set isPipelined 0
set isPipelined_legacy 0
set pipeline_type function
set FunctionProtocol ap_ctrl_hs
set restart_counter_num 0
set isOneStateSeq 0
set ProfileFlag 0
set StallSigGenFlag 0
set isEnableWaveformDebug 1
set hasInterrupt 0
set DLRegFirstOffset 0
set DLRegItemOffset 0
set svuvm_can_support 1
set cdfgNum 2
set C_modelName {cfar_core}
set C_modelType { void 0 }
set ap_memory_interface_dict [dict create]
set C_modelArgList {
	{ in_stream_V_data_V int 16 regular {axi_s 0 volatile  { in_stream Data } }  }
	{ in_stream_V_keep_V int 2 regular {axi_s 0 volatile  { in_stream Keep } }  }
	{ in_stream_V_strb_V int 2 regular {axi_s 0 volatile  { in_stream Strb } }  }
	{ in_stream_V_last_V int 1 regular {axi_s 0 volatile  { in_stream Last } }  }
	{ out_stream_V_data_V int 16 regular {axi_s 1 volatile  { out_stream Data } }  }
	{ out_stream_V_keep_V int 2 regular {axi_s 1 volatile  { out_stream Keep } }  }
	{ out_stream_V_strb_V int 2 regular {axi_s 1 volatile  { out_stream Strb } }  }
	{ out_stream_V_last_V int 1 regular {axi_s 1 volatile  { out_stream Last } }  }
	{ threshold_alpha int 16 regular {axi_slave 0}  }
}
set hasAXIMCache 0
set l_AXIML2Cache [list]
set AXIMCacheInstDict [dict create]
set C_modelArgMapList {[ 
	{ "Name" : "in_stream_V_data_V", "interface" : "axis", "bitwidth" : 16, "direction" : "READONLY"} , 
 	{ "Name" : "in_stream_V_keep_V", "interface" : "axis", "bitwidth" : 2, "direction" : "READONLY"} , 
 	{ "Name" : "in_stream_V_strb_V", "interface" : "axis", "bitwidth" : 2, "direction" : "READONLY"} , 
 	{ "Name" : "in_stream_V_last_V", "interface" : "axis", "bitwidth" : 1, "direction" : "READONLY"} , 
 	{ "Name" : "out_stream_V_data_V", "interface" : "axis", "bitwidth" : 16, "direction" : "WRITEONLY"} , 
 	{ "Name" : "out_stream_V_keep_V", "interface" : "axis", "bitwidth" : 2, "direction" : "WRITEONLY"} , 
 	{ "Name" : "out_stream_V_strb_V", "interface" : "axis", "bitwidth" : 2, "direction" : "WRITEONLY"} , 
 	{ "Name" : "out_stream_V_last_V", "interface" : "axis", "bitwidth" : 1, "direction" : "WRITEONLY"} , 
 	{ "Name" : "threshold_alpha", "interface" : "axi_slave", "bundle":"CTRL","type":"ap_none","bitwidth" : 16, "direction" : "READONLY", "offset" : {"in":16}, "offset_end" : {"in":23}} ]}
# RTL Port declarations: 
set portNum 32
set portList { 
	{ ap_clk sc_in sc_logic 1 clock -1 } 
	{ ap_rst_n sc_in sc_logic 1 reset -1 active_low_sync } 
	{ out_stream_TREADY sc_in sc_logic 1 outacc 7 } 
	{ in_stream_TDATA sc_in sc_lv 16 signal 0 } 
	{ in_stream_TVALID sc_in sc_logic 1 invld 3 } 
	{ in_stream_TREADY sc_out sc_logic 1 inacc 3 } 
	{ in_stream_TKEEP sc_in sc_lv 2 signal 1 } 
	{ in_stream_TSTRB sc_in sc_lv 2 signal 2 } 
	{ in_stream_TLAST sc_in sc_lv 1 signal 3 } 
	{ out_stream_TDATA sc_out sc_lv 16 signal 4 } 
	{ out_stream_TVALID sc_out sc_logic 1 outvld 7 } 
	{ out_stream_TKEEP sc_out sc_lv 2 signal 5 } 
	{ out_stream_TSTRB sc_out sc_lv 2 signal 6 } 
	{ out_stream_TLAST sc_out sc_lv 1 signal 7 } 
	{ s_axi_CTRL_AWVALID sc_in sc_logic 1 signal -1 } 
	{ s_axi_CTRL_AWREADY sc_out sc_logic 1 signal -1 } 
	{ s_axi_CTRL_AWADDR sc_in sc_lv 5 signal -1 } 
	{ s_axi_CTRL_WVALID sc_in sc_logic 1 signal -1 } 
	{ s_axi_CTRL_WREADY sc_out sc_logic 1 signal -1 } 
	{ s_axi_CTRL_WDATA sc_in sc_lv 32 signal -1 } 
	{ s_axi_CTRL_WSTRB sc_in sc_lv 4 signal -1 } 
	{ s_axi_CTRL_ARVALID sc_in sc_logic 1 signal -1 } 
	{ s_axi_CTRL_ARREADY sc_out sc_logic 1 signal -1 } 
	{ s_axi_CTRL_ARADDR sc_in sc_lv 5 signal -1 } 
	{ s_axi_CTRL_RVALID sc_out sc_logic 1 signal -1 } 
	{ s_axi_CTRL_RREADY sc_in sc_logic 1 signal -1 } 
	{ s_axi_CTRL_RDATA sc_out sc_lv 32 signal -1 } 
	{ s_axi_CTRL_RRESP sc_out sc_lv 2 signal -1 } 
	{ s_axi_CTRL_BVALID sc_out sc_logic 1 signal -1 } 
	{ s_axi_CTRL_BREADY sc_in sc_logic 1 signal -1 } 
	{ s_axi_CTRL_BRESP sc_out sc_lv 2 signal -1 } 
	{ interrupt sc_out sc_logic 1 signal -1 } 
}
set NewPortList {[ 
	{ "name": "s_axi_CTRL_AWADDR", "direction": "in", "datatype": "sc_lv", "bitwidth":5, "type": "signal", "bundle":{"name": "CTRL", "role": "AWADDR" },"address":[{"name":"cfar_core","role":"start","value":"0","valid_bit":"0"},{"name":"cfar_core","role":"continue","value":"0","valid_bit":"4"},{"name":"cfar_core","role":"auto_start","value":"0","valid_bit":"7"},{"name":"threshold_alpha","role":"data","value":"16"}] },
	{ "name": "s_axi_CTRL_AWVALID", "direction": "in", "datatype": "sc_logic", "bitwidth":1, "type": "signal", "bundle":{"name": "CTRL", "role": "AWVALID" } },
	{ "name": "s_axi_CTRL_AWREADY", "direction": "out", "datatype": "sc_logic", "bitwidth":1, "type": "signal", "bundle":{"name": "CTRL", "role": "AWREADY" } },
	{ "name": "s_axi_CTRL_WVALID", "direction": "in", "datatype": "sc_logic", "bitwidth":1, "type": "signal", "bundle":{"name": "CTRL", "role": "WVALID" } },
	{ "name": "s_axi_CTRL_WREADY", "direction": "out", "datatype": "sc_logic", "bitwidth":1, "type": "signal", "bundle":{"name": "CTRL", "role": "WREADY" } },
	{ "name": "s_axi_CTRL_WDATA", "direction": "in", "datatype": "sc_lv", "bitwidth":32, "type": "signal", "bundle":{"name": "CTRL", "role": "WDATA" } },
	{ "name": "s_axi_CTRL_WSTRB", "direction": "in", "datatype": "sc_lv", "bitwidth":4, "type": "signal", "bundle":{"name": "CTRL", "role": "WSTRB" } },
	{ "name": "s_axi_CTRL_ARADDR", "direction": "in", "datatype": "sc_lv", "bitwidth":5, "type": "signal", "bundle":{"name": "CTRL", "role": "ARADDR" },"address":[{"name":"cfar_core","role":"start","value":"0","valid_bit":"0"},{"name":"cfar_core","role":"done","value":"0","valid_bit":"1"},{"name":"cfar_core","role":"idle","value":"0","valid_bit":"2"},{"name":"cfar_core","role":"ready","value":"0","valid_bit":"3"},{"name":"cfar_core","role":"auto_start","value":"0","valid_bit":"7"}] },
	{ "name": "s_axi_CTRL_ARVALID", "direction": "in", "datatype": "sc_logic", "bitwidth":1, "type": "signal", "bundle":{"name": "CTRL", "role": "ARVALID" } },
	{ "name": "s_axi_CTRL_ARREADY", "direction": "out", "datatype": "sc_logic", "bitwidth":1, "type": "signal", "bundle":{"name": "CTRL", "role": "ARREADY" } },
	{ "name": "s_axi_CTRL_RVALID", "direction": "out", "datatype": "sc_logic", "bitwidth":1, "type": "signal", "bundle":{"name": "CTRL", "role": "RVALID" } },
	{ "name": "s_axi_CTRL_RREADY", "direction": "in", "datatype": "sc_logic", "bitwidth":1, "type": "signal", "bundle":{"name": "CTRL", "role": "RREADY" } },
	{ "name": "s_axi_CTRL_RDATA", "direction": "out", "datatype": "sc_lv", "bitwidth":32, "type": "signal", "bundle":{"name": "CTRL", "role": "RDATA" } },
	{ "name": "s_axi_CTRL_RRESP", "direction": "out", "datatype": "sc_lv", "bitwidth":2, "type": "signal", "bundle":{"name": "CTRL", "role": "RRESP" } },
	{ "name": "s_axi_CTRL_BVALID", "direction": "out", "datatype": "sc_logic", "bitwidth":1, "type": "signal", "bundle":{"name": "CTRL", "role": "BVALID" } },
	{ "name": "s_axi_CTRL_BREADY", "direction": "in", "datatype": "sc_logic", "bitwidth":1, "type": "signal", "bundle":{"name": "CTRL", "role": "BREADY" } },
	{ "name": "s_axi_CTRL_BRESP", "direction": "out", "datatype": "sc_lv", "bitwidth":2, "type": "signal", "bundle":{"name": "CTRL", "role": "BRESP" } },
	{ "name": "interrupt", "direction": "out", "datatype": "sc_logic", "bitwidth":1, "type": "signal", "bundle":{"name": "CTRL", "role": "interrupt" } }, 
 	{ "name": "ap_clk", "direction": "in", "datatype": "sc_logic", "bitwidth":1, "type": "clock", "bundle":{"name": "ap_clk", "role": "default" }} , 
 	{ "name": "ap_rst_n", "direction": "in", "datatype": "sc_logic", "bitwidth":1, "type": "reset", "bundle":{"name": "ap_rst_n", "role": "default" }} , 
 	{ "name": "out_stream_TREADY", "direction": "in", "datatype": "sc_logic", "bitwidth":1, "type": "outacc", "bundle":{"name": "out_stream_V_last_V", "role": "default" }} , 
 	{ "name": "in_stream_TDATA", "direction": "in", "datatype": "sc_lv", "bitwidth":16, "type": "signal", "bundle":{"name": "in_stream_V_data_V", "role": "default" }} , 
 	{ "name": "in_stream_TVALID", "direction": "in", "datatype": "sc_logic", "bitwidth":1, "type": "invld", "bundle":{"name": "in_stream_V_last_V", "role": "default" }} , 
 	{ "name": "in_stream_TREADY", "direction": "out", "datatype": "sc_logic", "bitwidth":1, "type": "inacc", "bundle":{"name": "in_stream_V_last_V", "role": "default" }} , 
 	{ "name": "in_stream_TKEEP", "direction": "in", "datatype": "sc_lv", "bitwidth":2, "type": "signal", "bundle":{"name": "in_stream_V_keep_V", "role": "default" }} , 
 	{ "name": "in_stream_TSTRB", "direction": "in", "datatype": "sc_lv", "bitwidth":2, "type": "signal", "bundle":{"name": "in_stream_V_strb_V", "role": "default" }} , 
 	{ "name": "in_stream_TLAST", "direction": "in", "datatype": "sc_lv", "bitwidth":1, "type": "signal", "bundle":{"name": "in_stream_V_last_V", "role": "default" }} , 
 	{ "name": "out_stream_TDATA", "direction": "out", "datatype": "sc_lv", "bitwidth":16, "type": "signal", "bundle":{"name": "out_stream_V_data_V", "role": "default" }} , 
 	{ "name": "out_stream_TVALID", "direction": "out", "datatype": "sc_logic", "bitwidth":1, "type": "outvld", "bundle":{"name": "out_stream_V_last_V", "role": "default" }} , 
 	{ "name": "out_stream_TKEEP", "direction": "out", "datatype": "sc_lv", "bitwidth":2, "type": "signal", "bundle":{"name": "out_stream_V_keep_V", "role": "default" }} , 
 	{ "name": "out_stream_TSTRB", "direction": "out", "datatype": "sc_lv", "bitwidth":2, "type": "signal", "bundle":{"name": "out_stream_V_strb_V", "role": "default" }} , 
 	{ "name": "out_stream_TLAST", "direction": "out", "datatype": "sc_lv", "bitwidth":1, "type": "signal", "bundle":{"name": "out_stream_V_last_V", "role": "default" }}  ]}

set ArgLastReadFirstWriteLatency {
	cfar_core {
		in_stream_V_data_V {Type I LastRead 0 FirstWrite -1}
		in_stream_V_keep_V {Type I LastRead 0 FirstWrite -1}
		in_stream_V_strb_V {Type I LastRead 0 FirstWrite -1}
		in_stream_V_last_V {Type I LastRead 0 FirstWrite -1}
		out_stream_V_data_V {Type O LastRead -1 FirstWrite 5}
		out_stream_V_keep_V {Type O LastRead -1 FirstWrite 5}
		out_stream_V_strb_V {Type O LastRead -1 FirstWrite 5}
		out_stream_V_last_V {Type O LastRead -1 FirstWrite 5}
		threshold_alpha {Type I LastRead 3 FirstWrite -1}
		cfar_core_stream_stream_axis_0_ap_fixed_16_8_5_3_0_window_data_19 {Type IO LastRead -1 FirstWrite -1}
		cfar_core_stream_stream_axis_0_ap_fixed_16_8_5_3_0_window_data_18 {Type IO LastRead -1 FirstWrite -1}
		cfar_core_stream_stream_axis_0_ap_fixed_16_8_5_3_0_window_data_17 {Type IO LastRead -1 FirstWrite -1}
		cfar_core_stream_stream_axis_0_ap_fixed_16_8_5_3_0_window_data_16 {Type IO LastRead -1 FirstWrite -1}
		cfar_core_stream_stream_axis_0_ap_fixed_16_8_5_3_0_window_data_15 {Type IO LastRead -1 FirstWrite -1}
		cfar_core_stream_stream_axis_0_ap_fixed_16_8_5_3_0_window_data_14 {Type IO LastRead -1 FirstWrite -1}
		cfar_core_stream_stream_axis_0_ap_fixed_16_8_5_3_0_window_data_13 {Type IO LastRead -1 FirstWrite -1}
		cfar_core_stream_stream_axis_0_ap_fixed_16_8_5_3_0_window_data_12 {Type IO LastRead -1 FirstWrite -1}
		cfar_core_stream_stream_axis_0_ap_fixed_16_8_5_3_0_window_data_11 {Type IO LastRead -1 FirstWrite -1}
		cfar_core_stream_stream_axis_0_ap_fixed_16_8_5_3_0_window_data_10 {Type IO LastRead -1 FirstWrite -1}
		cfar_core_stream_stream_axis_0_ap_fixed_16_8_5_3_0_window_data_9 {Type IO LastRead -1 FirstWrite -1}
		cfar_core_stream_stream_axis_0_ap_fixed_16_8_5_3_0_window_data_8 {Type IO LastRead -1 FirstWrite -1}
		cfar_core_stream_stream_axis_0_ap_fixed_16_8_5_3_0_window_data_7 {Type IO LastRead -1 FirstWrite -1}
		cfar_core_stream_stream_axis_0_ap_fixed_16_8_5_3_0_window_data_6 {Type IO LastRead -1 FirstWrite -1}
		cfar_core_stream_stream_axis_0_ap_fixed_16_8_5_3_0_window_data_5 {Type IO LastRead -1 FirstWrite -1}
		cfar_core_stream_stream_axis_0_ap_fixed_16_8_5_3_0_window_data_4 {Type IO LastRead -1 FirstWrite -1}
		cfar_core_stream_stream_axis_0_ap_fixed_16_8_5_3_0_window_data_3 {Type IO LastRead -1 FirstWrite -1}
		cfar_core_stream_stream_axis_0_ap_fixed_16_8_5_3_0_window_data_2 {Type IO LastRead -1 FirstWrite -1}
		cfar_core_stream_stream_axis_0_ap_fixed_16_8_5_3_0_window_data_1 {Type IO LastRead -1 FirstWrite -1}
		cfar_core_stream_stream_axis_0_ap_fixed_16_8_5_3_0_window_data_0 {Type IO LastRead -1 FirstWrite -1}
		cfar_core_stream_stream_axis_0_ap_fixed_16_8_5_3_0_window_keep_7 {Type IO LastRead -1 FirstWrite -1}
		cfar_core_stream_stream_axis_0_ap_fixed_16_8_5_3_0_window_keep_6 {Type IO LastRead -1 FirstWrite -1}
		cfar_core_stream_stream_axis_0_ap_fixed_16_8_5_3_0_window_keep_5 {Type IO LastRead -1 FirstWrite -1}
		cfar_core_stream_stream_axis_0_ap_fixed_16_8_5_3_0_window_keep_4 {Type IO LastRead -1 FirstWrite -1}
		cfar_core_stream_stream_axis_0_ap_fixed_16_8_5_3_0_window_keep_3 {Type IO LastRead -1 FirstWrite -1}
		cfar_core_stream_stream_axis_0_ap_fixed_16_8_5_3_0_window_keep_2 {Type IO LastRead -1 FirstWrite -1}
		cfar_core_stream_stream_axis_0_ap_fixed_16_8_5_3_0_window_keep_1 {Type IO LastRead -1 FirstWrite -1}
		cfar_core_stream_stream_axis_0_ap_fixed_16_8_5_3_0_window_keep_0 {Type IO LastRead -1 FirstWrite -1}
		cfar_core_stream_stream_axis_0_ap_fixed_16_8_5_3_0_window_keep_8 {Type O LastRead -1 FirstWrite -1}
		cfar_core_stream_stream_axis_0_ap_fixed_16_8_5_3_0_window_strb_7 {Type IO LastRead -1 FirstWrite -1}
		cfar_core_stream_stream_axis_0_ap_fixed_16_8_5_3_0_window_strb_6 {Type IO LastRead -1 FirstWrite -1}
		cfar_core_stream_stream_axis_0_ap_fixed_16_8_5_3_0_window_strb_5 {Type IO LastRead -1 FirstWrite -1}
		cfar_core_stream_stream_axis_0_ap_fixed_16_8_5_3_0_window_strb_4 {Type IO LastRead -1 FirstWrite -1}
		cfar_core_stream_stream_axis_0_ap_fixed_16_8_5_3_0_window_strb_3 {Type IO LastRead -1 FirstWrite -1}
		cfar_core_stream_stream_axis_0_ap_fixed_16_8_5_3_0_window_strb_2 {Type IO LastRead -1 FirstWrite -1}
		cfar_core_stream_stream_axis_0_ap_fixed_16_8_5_3_0_window_strb_1 {Type IO LastRead -1 FirstWrite -1}
		cfar_core_stream_stream_axis_0_ap_fixed_16_8_5_3_0_window_strb_0 {Type IO LastRead -1 FirstWrite -1}
		cfar_core_stream_stream_axis_0_ap_fixed_16_8_5_3_0_window_strb_8 {Type O LastRead -1 FirstWrite -1}
		cfar_core_stream_stream_axis_0_ap_fixed_16_8_5_3_0_window_last_7 {Type IO LastRead -1 FirstWrite -1}
		cfar_core_stream_stream_axis_0_ap_fixed_16_8_5_3_0_window_last_6 {Type IO LastRead -1 FirstWrite -1}
		cfar_core_stream_stream_axis_0_ap_fixed_16_8_5_3_0_window_last_5 {Type IO LastRead -1 FirstWrite -1}
		cfar_core_stream_stream_axis_0_ap_fixed_16_8_5_3_0_window_last_4 {Type IO LastRead -1 FirstWrite -1}
		cfar_core_stream_stream_axis_0_ap_fixed_16_8_5_3_0_window_last_3 {Type IO LastRead -1 FirstWrite -1}
		cfar_core_stream_stream_axis_0_ap_fixed_16_8_5_3_0_window_last_2 {Type IO LastRead -1 FirstWrite -1}
		cfar_core_stream_stream_axis_0_ap_fixed_16_8_5_3_0_window_last_1 {Type IO LastRead -1 FirstWrite -1}
		cfar_core_stream_stream_axis_0_ap_fixed_16_8_5_3_0_window_last_0 {Type IO LastRead -1 FirstWrite -1}
		cfar_core_stream_stream_axis_0_ap_fixed_16_8_5_3_0_window_last_8 {Type O LastRead -1 FirstWrite -1}}}

set hasDtUnsupportedChannel 0

set PerformanceInfo {[
	{"Name" : "Latency", "Min" : "6", "Max" : "6"}
	, {"Name" : "Interval", "Min" : "1", "Max" : "1"}
]}

set PipelineEnableSignalInfo {[
	{"Pipeline" : "0", "EnableSignal" : "ap_enable_pp0"}
]}

set Spec2ImplPortList { 
	in_stream_V_data_V { axis {  { in_stream_TDATA in_data 0 16 } } }
	in_stream_V_keep_V { axis {  { in_stream_TKEEP in_data 0 2 } } }
	in_stream_V_strb_V { axis {  { in_stream_TSTRB in_data 0 2 } } }
	in_stream_V_last_V { axis {  { in_stream_TVALID in_vld 0 1 }  { in_stream_TREADY in_acc 1 1 }  { in_stream_TLAST in_data 0 1 } } }
	out_stream_V_data_V { axis {  { out_stream_TDATA out_data 1 16 } } }
	out_stream_V_keep_V { axis {  { out_stream_TKEEP out_data 1 2 } } }
	out_stream_V_strb_V { axis {  { out_stream_TSTRB out_data 1 2 } } }
	out_stream_V_last_V { axis {  { out_stream_TREADY out_acc 0 1 }  { out_stream_TVALID out_vld 1 1 }  { out_stream_TLAST out_data 1 1 } } }
}

set maxi_interface_dict [dict create]

# RTL port scheduling information:
set fifoSchedulingInfoList { 
}

# RTL bus port read request latency information:
set busReadReqLatencyList { 
}

# RTL bus port write response latency information:
set busWriteResLatencyList { 
}

# RTL array port load latency information:
set memoryLoadLatencyList { 
}
