set moduleName stage_process_fft_real_Pipeline_drain_loop
set isTopModule 0
set isCombinational 0
set isDatapathOnly 0
set isPipelined 1
set isPipelined_legacy 1
set pipeline_type loop_auto_rewind
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
set C_modelName {stage_process_fft_real_Pipeline_drain_loop}
set C_modelType { void 0 }
set ap_memory_interface_dict [dict create]
dict set ap_memory_interface_dict out_bufI { MEM_WIDTH 16 MEM_SIZE 2048 MASTER_TYPE BRAM_CTRL MEM_ADDRESS_MODE WORD_ADDRESS PACKAGE_IO port READ_LATENCY 0 }
dict set ap_memory_interface_dict out_bufQ { MEM_WIDTH 16 MEM_SIZE 2048 MASTER_TYPE BRAM_CTRL MEM_ADDRESS_MODE WORD_ADDRESS PACKAGE_IO port READ_LATENCY 0 }
set C_modelArgList {
	{ fft_out_stream int 32 regular {fifo 0 volatile }  }
	{ out_bufI int 16 regular {array 1024 { 3 0 } 0 1 }  }
	{ out_bufQ int 16 regular {array 1024 { 3 0 } 0 1 }  }
}
set hasAXIMCache 0
set l_AXIML2Cache [list]
set AXIMCacheInstDict [dict create]
set C_modelArgMapList {[ 
	{ "Name" : "fft_out_stream", "interface" : "fifo", "bitwidth" : 32, "direction" : "READONLY"} , 
 	{ "Name" : "out_bufI", "interface" : "memory", "bitwidth" : 16, "direction" : "WRITEONLY"} , 
 	{ "Name" : "out_bufQ", "interface" : "memory", "bitwidth" : 16, "direction" : "WRITEONLY"} ]}
# RTL Port declarations: 
set portNum 17
set portList { 
	{ ap_clk sc_in sc_logic 1 clock -1 } 
	{ ap_rst sc_in sc_logic 1 reset -1 active_high_sync } 
	{ ap_start sc_in sc_logic 1 start -1 } 
	{ ap_done sc_out sc_logic 1 predone -1 } 
	{ ap_idle sc_out sc_logic 1 done -1 } 
	{ ap_ready sc_out sc_logic 1 ready -1 } 
	{ fft_out_stream_dout sc_in sc_lv 32 signal 0 } 
	{ fft_out_stream_empty_n sc_in sc_logic 1 signal 0 } 
	{ fft_out_stream_read sc_out sc_logic 1 signal 0 } 
	{ out_bufI_address1 sc_out sc_lv 10 signal 1 } 
	{ out_bufI_ce1 sc_out sc_logic 1 signal 1 } 
	{ out_bufI_we1 sc_out sc_logic 1 signal 1 } 
	{ out_bufI_d1 sc_out sc_lv 16 signal 1 } 
	{ out_bufQ_address1 sc_out sc_lv 10 signal 2 } 
	{ out_bufQ_ce1 sc_out sc_logic 1 signal 2 } 
	{ out_bufQ_we1 sc_out sc_logic 1 signal 2 } 
	{ out_bufQ_d1 sc_out sc_lv 16 signal 2 } 
}
set NewPortList {[ 
	{ "name": "ap_clk", "direction": "in", "datatype": "sc_logic", "bitwidth":1, "type": "clock", "bundle":{"name": "ap_clk", "role": "default" }} , 
 	{ "name": "ap_rst", "direction": "in", "datatype": "sc_logic", "bitwidth":1, "type": "reset", "bundle":{"name": "ap_rst", "role": "default" }} , 
 	{ "name": "ap_start", "direction": "in", "datatype": "sc_logic", "bitwidth":1, "type": "start", "bundle":{"name": "ap_start", "role": "default" }} , 
 	{ "name": "ap_done", "direction": "out", "datatype": "sc_logic", "bitwidth":1, "type": "predone", "bundle":{"name": "ap_done", "role": "default" }} , 
 	{ "name": "ap_idle", "direction": "out", "datatype": "sc_logic", "bitwidth":1, "type": "done", "bundle":{"name": "ap_idle", "role": "default" }} , 
 	{ "name": "ap_ready", "direction": "out", "datatype": "sc_logic", "bitwidth":1, "type": "ready", "bundle":{"name": "ap_ready", "role": "default" }} , 
 	{ "name": "fft_out_stream_dout", "direction": "in", "datatype": "sc_lv", "bitwidth":32, "type": "signal", "bundle":{"name": "fft_out_stream", "role": "dout" }} , 
 	{ "name": "fft_out_stream_empty_n", "direction": "in", "datatype": "sc_logic", "bitwidth":1, "type": "signal", "bundle":{"name": "fft_out_stream", "role": "empty_n" }} , 
 	{ "name": "fft_out_stream_read", "direction": "out", "datatype": "sc_logic", "bitwidth":1, "type": "signal", "bundle":{"name": "fft_out_stream", "role": "read" }} , 
 	{ "name": "out_bufI_address1", "direction": "out", "datatype": "sc_lv", "bitwidth":10, "type": "signal", "bundle":{"name": "out_bufI", "role": "address1" }} , 
 	{ "name": "out_bufI_ce1", "direction": "out", "datatype": "sc_logic", "bitwidth":1, "type": "signal", "bundle":{"name": "out_bufI", "role": "ce1" }} , 
 	{ "name": "out_bufI_we1", "direction": "out", "datatype": "sc_logic", "bitwidth":1, "type": "signal", "bundle":{"name": "out_bufI", "role": "we1" }} , 
 	{ "name": "out_bufI_d1", "direction": "out", "datatype": "sc_lv", "bitwidth":16, "type": "signal", "bundle":{"name": "out_bufI", "role": "d1" }} , 
 	{ "name": "out_bufQ_address1", "direction": "out", "datatype": "sc_lv", "bitwidth":10, "type": "signal", "bundle":{"name": "out_bufQ", "role": "address1" }} , 
 	{ "name": "out_bufQ_ce1", "direction": "out", "datatype": "sc_logic", "bitwidth":1, "type": "signal", "bundle":{"name": "out_bufQ", "role": "ce1" }} , 
 	{ "name": "out_bufQ_we1", "direction": "out", "datatype": "sc_logic", "bitwidth":1, "type": "signal", "bundle":{"name": "out_bufQ", "role": "we1" }} , 
 	{ "name": "out_bufQ_d1", "direction": "out", "datatype": "sc_lv", "bitwidth":16, "type": "signal", "bundle":{"name": "out_bufQ", "role": "d1" }}  ]}

set ArgLastReadFirstWriteLatency {
	stage_process_fft_real_Pipeline_drain_loop {
		fft_out_stream {Type I LastRead 1 FirstWrite -1}
		out_bufI {Type O LastRead -1 FirstWrite 2}
		out_bufQ {Type O LastRead -1 FirstWrite 2}}}

set hasDtUnsupportedChannel 0

set PerformanceInfo {[
	{"Name" : "Latency", "Min" : "1027", "Max" : "1027"}
	, {"Name" : "Interval", "Min" : "1025", "Max" : "1025"}
]}

set PipelineEnableSignalInfo {[
	{"Pipeline" : "0", "EnableSignal" : "ap_enable_pp0"}
]}

set Spec2ImplPortList { 
	fft_out_stream { ap_fifo {  { fft_out_stream_dout fifo_data_out 0 32 }  { fft_out_stream_empty_n fifo_status_empty 0 1 }  { fft_out_stream_read fifo_data_in 1 1 } } }
	out_bufI { ap_memory {  { out_bufI_address1 MemPortADDR2 1 10 }  { out_bufI_ce1 MemPortCE2 1 1 }  { out_bufI_we1 MemPortWE2 1 1 }  { out_bufI_d1 MemPortDIN2 1 16 } } }
	out_bufQ { ap_memory {  { out_bufQ_address1 MemPortADDR2 1 10 }  { out_bufQ_ce1 MemPortCE2 1 1 }  { out_bufQ_we1 MemPortWE2 1 1 }  { out_bufQ_d1 MemPortDIN2 1 16 } } }
}
