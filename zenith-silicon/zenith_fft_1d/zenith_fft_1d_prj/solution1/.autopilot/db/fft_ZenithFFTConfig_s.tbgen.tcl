set moduleName fft_ZenithFFTConfig_s
set isTopModule 0
set isCombinational 0
set isDatapathOnly 0
set isPipelined 0
set isPipelined_legacy 0
set pipeline_type none
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
set cdfgNum 12
set C_modelName {fft<ZenithFFTConfig>}
set C_modelType { void 0 }
set ap_memory_interface_dict [dict create]
set C_modelArgList {
	{ fft_in_stream int 32 regular {fifo 0 volatile }  }
	{ fft_out_stream int 32 regular {fifo 1 volatile }  }
	{ fft_config_stream int 16 regular {fifo 0 volatile }  }
}
set hasAXIMCache 0
set l_AXIML2Cache [list]
set AXIMCacheInstDict [dict create]
set C_modelArgMapList {[ 
	{ "Name" : "fft_in_stream", "interface" : "fifo", "bitwidth" : 32, "direction" : "READONLY"} , 
 	{ "Name" : "fft_out_stream", "interface" : "fifo", "bitwidth" : 32, "direction" : "WRITEONLY"} , 
 	{ "Name" : "fft_config_stream", "interface" : "fifo", "bitwidth" : 16, "direction" : "READONLY"} ]}
# RTL Port declarations: 
set portNum 16
set portList { 
	{ ap_clk sc_in sc_logic 1 clock -1 } 
	{ ap_rst sc_in sc_logic 1 reset -1 active_high_sync } 
	{ ap_ce sc_in sc_logic 1 ce -1 } 
	{ ap_start sc_in sc_logic 1 start -1 } 
	{ ap_done sc_out sc_logic 1 predone -1 } 
	{ ap_idle sc_out sc_logic 1 done -1 } 
	{ ap_ready sc_out sc_logic 1 ready -1 } 
	{ fft_in_stream_dout sc_in sc_lv 32 signal 0 } 
	{ fft_in_stream_empty_n sc_in sc_logic 1 signal 0 } 
	{ fft_in_stream_read sc_out sc_logic 1 signal 0 } 
	{ fft_out_stream_din sc_out sc_lv 32 signal 1 } 
	{ fft_out_stream_full_n sc_in sc_logic 1 signal 1 } 
	{ fft_out_stream_write sc_out sc_logic 1 signal 1 } 
	{ fft_config_stream_dout sc_in sc_lv 16 signal 2 } 
	{ fft_config_stream_empty_n sc_in sc_logic 1 signal 2 } 
	{ fft_config_stream_read sc_out sc_logic 1 signal 2 } 
}
set NewPortList {[ 
	{ "name": "ap_clk", "direction": "in", "datatype": "sc_logic", "bitwidth":1, "type": "clock", "bundle":{"name": "ap_clk", "role": "default" }} , 
 	{ "name": "ap_rst", "direction": "in", "datatype": "sc_logic", "bitwidth":1, "type": "reset", "bundle":{"name": "ap_rst", "role": "default" }} , 
 	{ "name": "ap_ce", "direction": "in", "datatype": "sc_logic", "bitwidth":1, "type": "ce", "bundle":{"name": "ap_ce", "role": "default" }} , 
 	{ "name": "ap_start", "direction": "in", "datatype": "sc_logic", "bitwidth":1, "type": "start", "bundle":{"name": "ap_start", "role": "default" }} , 
 	{ "name": "ap_done", "direction": "out", "datatype": "sc_logic", "bitwidth":1, "type": "predone", "bundle":{"name": "ap_done", "role": "default" }} , 
 	{ "name": "ap_idle", "direction": "out", "datatype": "sc_logic", "bitwidth":1, "type": "done", "bundle":{"name": "ap_idle", "role": "default" }} , 
 	{ "name": "ap_ready", "direction": "out", "datatype": "sc_logic", "bitwidth":1, "type": "ready", "bundle":{"name": "ap_ready", "role": "default" }} , 
 	{ "name": "fft_in_stream_dout", "direction": "in", "datatype": "sc_lv", "bitwidth":32, "type": "signal", "bundle":{"name": "fft_in_stream", "role": "dout" }} , 
 	{ "name": "fft_in_stream_empty_n", "direction": "in", "datatype": "sc_logic", "bitwidth":1, "type": "signal", "bundle":{"name": "fft_in_stream", "role": "empty_n" }} , 
 	{ "name": "fft_in_stream_read", "direction": "out", "datatype": "sc_logic", "bitwidth":1, "type": "signal", "bundle":{"name": "fft_in_stream", "role": "read" }} , 
 	{ "name": "fft_out_stream_din", "direction": "out", "datatype": "sc_lv", "bitwidth":32, "type": "signal", "bundle":{"name": "fft_out_stream", "role": "din" }} , 
 	{ "name": "fft_out_stream_full_n", "direction": "in", "datatype": "sc_logic", "bitwidth":1, "type": "signal", "bundle":{"name": "fft_out_stream", "role": "full_n" }} , 
 	{ "name": "fft_out_stream_write", "direction": "out", "datatype": "sc_logic", "bitwidth":1, "type": "signal", "bundle":{"name": "fft_out_stream", "role": "write" }} , 
 	{ "name": "fft_config_stream_dout", "direction": "in", "datatype": "sc_lv", "bitwidth":16, "type": "signal", "bundle":{"name": "fft_config_stream", "role": "dout" }} , 
 	{ "name": "fft_config_stream_empty_n", "direction": "in", "datatype": "sc_logic", "bitwidth":1, "type": "signal", "bundle":{"name": "fft_config_stream", "role": "empty_n" }} , 
 	{ "name": "fft_config_stream_read", "direction": "out", "datatype": "sc_logic", "bitwidth":1, "type": "signal", "bundle":{"name": "fft_config_stream", "role": "read" }}  ]}

set ArgLastReadFirstWriteLatency {
	fft_ZenithFFTConfig_s {
		fft_in_stream {Type I LastRead -1 FirstWrite -1}
		fft_out_stream {Type O LastRead -1 FirstWrite -1}
		fft_config_stream {Type I LastRead -1 FirstWrite -1}}}

set hasDtUnsupportedChannel 0

set PerformanceInfo {[
	{"Name" : "Latency", "Min" : "3195", "Max" : "3195"}
	, {"Name" : "Interval", "Min" : "3195", "Max" : "3195"}
]}

set PipelineEnableSignalInfo {[
]}

set Spec2ImplPortList { 
	fft_in_stream { ap_fifo {  { fft_in_stream_dout fifo_data_out 0 32 }  { fft_in_stream_empty_n fifo_status_empty 0 1 }  { fft_in_stream_read fifo_data_in 1 1 } } }
	fft_out_stream { ap_fifo {  { fft_out_stream_din fifo_data_out 1 32 }  { fft_out_stream_full_n fifo_status_empty 0 1 }  { fft_out_stream_write fifo_data_in 1 1 } } }
	fft_config_stream { ap_fifo {  { fft_config_stream_dout fifo_data_out 0 16 }  { fft_config_stream_empty_n fifo_status_empty 0 1 }  { fft_config_stream_read fifo_data_in 1 1 } } }
}
