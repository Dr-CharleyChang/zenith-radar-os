set moduleName stage_process_fft_real_Pipeline_feed_loop
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
set C_modelName {stage_process_fft_real_Pipeline_feed_loop}
set C_modelType { void 0 }
set ap_memory_interface_dict [dict create]
dict set ap_memory_interface_dict in_bufI { MEM_WIDTH 16 MEM_SIZE 2048 MASTER_TYPE BRAM_CTRL MEM_ADDRESS_MODE WORD_ADDRESS PACKAGE_IO port READ_LATENCY 1 }
dict set ap_memory_interface_dict in_bufQ { MEM_WIDTH 16 MEM_SIZE 2048 MASTER_TYPE BRAM_CTRL MEM_ADDRESS_MODE WORD_ADDRESS PACKAGE_IO port READ_LATENCY 1 }
set C_modelArgList {
	{ in_bufI int 16 regular {array 1024 { 1 3 } 1 1 }  }
	{ in_bufQ int 16 regular {array 1024 { 1 3 } 1 1 }  }
	{ fft_in_stream int 32 regular {fifo 1 volatile }  }
}
set hasAXIMCache 0
set l_AXIML2Cache [list]
set AXIMCacheInstDict [dict create]
set C_modelArgMapList {[ 
	{ "Name" : "in_bufI", "interface" : "memory", "bitwidth" : 16, "direction" : "READONLY"} , 
 	{ "Name" : "in_bufQ", "interface" : "memory", "bitwidth" : 16, "direction" : "READONLY"} , 
 	{ "Name" : "fft_in_stream", "interface" : "fifo", "bitwidth" : 32, "direction" : "WRITEONLY"} ]}
# RTL Port declarations: 
set portNum 15
set portList { 
	{ ap_clk sc_in sc_logic 1 clock -1 } 
	{ ap_rst sc_in sc_logic 1 reset -1 active_high_sync } 
	{ ap_start sc_in sc_logic 1 start -1 } 
	{ ap_done sc_out sc_logic 1 predone -1 } 
	{ ap_idle sc_out sc_logic 1 done -1 } 
	{ ap_ready sc_out sc_logic 1 ready -1 } 
	{ fft_in_stream_din sc_out sc_lv 32 signal 2 } 
	{ fft_in_stream_full_n sc_in sc_logic 1 signal 2 } 
	{ fft_in_stream_write sc_out sc_logic 1 signal 2 } 
	{ in_bufI_address0 sc_out sc_lv 10 signal 0 } 
	{ in_bufI_ce0 sc_out sc_logic 1 signal 0 } 
	{ in_bufI_q0 sc_in sc_lv 16 signal 0 } 
	{ in_bufQ_address0 sc_out sc_lv 10 signal 1 } 
	{ in_bufQ_ce0 sc_out sc_logic 1 signal 1 } 
	{ in_bufQ_q0 sc_in sc_lv 16 signal 1 } 
}
set NewPortList {[ 
	{ "name": "ap_clk", "direction": "in", "datatype": "sc_logic", "bitwidth":1, "type": "clock", "bundle":{"name": "ap_clk", "role": "default" }} , 
 	{ "name": "ap_rst", "direction": "in", "datatype": "sc_logic", "bitwidth":1, "type": "reset", "bundle":{"name": "ap_rst", "role": "default" }} , 
 	{ "name": "ap_start", "direction": "in", "datatype": "sc_logic", "bitwidth":1, "type": "start", "bundle":{"name": "ap_start", "role": "default" }} , 
 	{ "name": "ap_done", "direction": "out", "datatype": "sc_logic", "bitwidth":1, "type": "predone", "bundle":{"name": "ap_done", "role": "default" }} , 
 	{ "name": "ap_idle", "direction": "out", "datatype": "sc_logic", "bitwidth":1, "type": "done", "bundle":{"name": "ap_idle", "role": "default" }} , 
 	{ "name": "ap_ready", "direction": "out", "datatype": "sc_logic", "bitwidth":1, "type": "ready", "bundle":{"name": "ap_ready", "role": "default" }} , 
 	{ "name": "fft_in_stream_din", "direction": "out", "datatype": "sc_lv", "bitwidth":32, "type": "signal", "bundle":{"name": "fft_in_stream", "role": "din" }} , 
 	{ "name": "fft_in_stream_full_n", "direction": "in", "datatype": "sc_logic", "bitwidth":1, "type": "signal", "bundle":{"name": "fft_in_stream", "role": "full_n" }} , 
 	{ "name": "fft_in_stream_write", "direction": "out", "datatype": "sc_logic", "bitwidth":1, "type": "signal", "bundle":{"name": "fft_in_stream", "role": "write" }} , 
 	{ "name": "in_bufI_address0", "direction": "out", "datatype": "sc_lv", "bitwidth":10, "type": "signal", "bundle":{"name": "in_bufI", "role": "address0" }} , 
 	{ "name": "in_bufI_ce0", "direction": "out", "datatype": "sc_logic", "bitwidth":1, "type": "signal", "bundle":{"name": "in_bufI", "role": "ce0" }} , 
 	{ "name": "in_bufI_q0", "direction": "in", "datatype": "sc_lv", "bitwidth":16, "type": "signal", "bundle":{"name": "in_bufI", "role": "q0" }} , 
 	{ "name": "in_bufQ_address0", "direction": "out", "datatype": "sc_lv", "bitwidth":10, "type": "signal", "bundle":{"name": "in_bufQ", "role": "address0" }} , 
 	{ "name": "in_bufQ_ce0", "direction": "out", "datatype": "sc_logic", "bitwidth":1, "type": "signal", "bundle":{"name": "in_bufQ", "role": "ce0" }} , 
 	{ "name": "in_bufQ_q0", "direction": "in", "datatype": "sc_lv", "bitwidth":16, "type": "signal", "bundle":{"name": "in_bufQ", "role": "q0" }}  ]}

set ArgLastReadFirstWriteLatency {
	stage_process_fft_real_Pipeline_feed_loop {
		in_bufI {Type I LastRead 0 FirstWrite -1}
		in_bufQ {Type I LastRead 0 FirstWrite -1}
		fft_in_stream {Type O LastRead -1 FirstWrite 2}}}

set hasDtUnsupportedChannel 0

set PerformanceInfo {[
	{"Name" : "Latency", "Min" : "1027", "Max" : "1027"}
	, {"Name" : "Interval", "Min" : "1025", "Max" : "1025"}
]}

set PipelineEnableSignalInfo {[
	{"Pipeline" : "0", "EnableSignal" : "ap_enable_pp0"}
]}

set Spec2ImplPortList { 
	in_bufI { ap_memory {  { in_bufI_address0 mem_address 1 10 }  { in_bufI_ce0 mem_ce 1 1 }  { in_bufI_q0 mem_dout 0 16 } } }
	in_bufQ { ap_memory {  { in_bufQ_address0 mem_address 1 10 }  { in_bufQ_ce0 mem_ce 1 1 }  { in_bufQ_q0 mem_dout 0 16 } } }
	fft_in_stream { ap_fifo {  { fft_in_stream_din fifo_data_out 1 32 }  { fft_in_stream_full_n fifo_status_empty 0 1 }  { fft_in_stream_write fifo_data_in 1 1 } } }
}
