set moduleName stage_process_fft_real
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
set C_modelName {stage_process_fft_real}
set C_modelType { void 0 }
set ap_memory_interface_dict [dict create]
dict set ap_memory_interface_dict in_bufI { MEM_WIDTH 16 MEM_SIZE 2048 MASTER_TYPE BRAM_CTRL MEM_ADDRESS_MODE WORD_ADDRESS PACKAGE_IO port READ_LATENCY 1 }
dict set ap_memory_interface_dict in_bufQ { MEM_WIDTH 16 MEM_SIZE 2048 MASTER_TYPE BRAM_CTRL MEM_ADDRESS_MODE WORD_ADDRESS PACKAGE_IO port READ_LATENCY 1 }
dict set ap_memory_interface_dict out_bufI { MEM_WIDTH 16 MEM_SIZE 2048 MASTER_TYPE BRAM_CTRL MEM_ADDRESS_MODE WORD_ADDRESS PACKAGE_IO port READ_LATENCY 1 }
dict set ap_memory_interface_dict out_bufQ { MEM_WIDTH 16 MEM_SIZE 2048 MASTER_TYPE BRAM_CTRL MEM_ADDRESS_MODE WORD_ADDRESS PACKAGE_IO port READ_LATENCY 1 }
set C_modelArgList {
	{ scaling_schedule int 32 regular {fifo 0}  }
	{ in_bufI int 16 regular {array 1024 { 1 3 } 1 1 }  }
	{ in_bufQ int 16 regular {array 1024 { 1 3 } 1 1 }  }
	{ out_bufI int 16 regular {array 1024 { 3 0 } 0 1 }  }
	{ out_bufQ int 16 regular {array 1024 { 3 0 } 0 1 }  }
}
set hasAXIMCache 0
set l_AXIML2Cache [list]
set AXIMCacheInstDict [dict create]
set C_modelArgMapList {[ 
	{ "Name" : "scaling_schedule", "interface" : "fifo", "bitwidth" : 32, "direction" : "READONLY"} , 
 	{ "Name" : "in_bufI", "interface" : "memory", "bitwidth" : 16, "direction" : "READONLY"} , 
 	{ "Name" : "in_bufQ", "interface" : "memory", "bitwidth" : 16, "direction" : "READONLY"} , 
 	{ "Name" : "out_bufI", "interface" : "memory", "bitwidth" : 16, "direction" : "WRITEONLY"} , 
 	{ "Name" : "out_bufQ", "interface" : "memory", "bitwidth" : 16, "direction" : "WRITEONLY"} ]}
# RTL Port declarations: 
set portNum 26
set portList { 
	{ ap_clk sc_in sc_logic 1 clock -1 } 
	{ ap_rst sc_in sc_logic 1 reset -1 active_high_sync } 
	{ ap_start sc_in sc_logic 1 start -1 } 
	{ ap_done sc_out sc_logic 1 predone -1 } 
	{ ap_continue sc_in sc_logic 1 continue -1 } 
	{ ap_idle sc_out sc_logic 1 done -1 } 
	{ ap_ready sc_out sc_logic 1 ready -1 } 
	{ scaling_schedule_dout sc_in sc_lv 32 signal 0 } 
	{ scaling_schedule_empty_n sc_in sc_logic 1 signal 0 } 
	{ scaling_schedule_read sc_out sc_logic 1 signal 0 } 
	{ scaling_schedule_num_data_valid sc_in sc_lv 3 signal 0 } 
	{ scaling_schedule_fifo_cap sc_in sc_lv 3 signal 0 } 
	{ in_bufI_address0 sc_out sc_lv 10 signal 1 } 
	{ in_bufI_ce0 sc_out sc_logic 1 signal 1 } 
	{ in_bufI_q0 sc_in sc_lv 16 signal 1 } 
	{ in_bufQ_address0 sc_out sc_lv 10 signal 2 } 
	{ in_bufQ_ce0 sc_out sc_logic 1 signal 2 } 
	{ in_bufQ_q0 sc_in sc_lv 16 signal 2 } 
	{ out_bufI_address1 sc_out sc_lv 10 signal 3 } 
	{ out_bufI_ce1 sc_out sc_logic 1 signal 3 } 
	{ out_bufI_we1 sc_out sc_logic 1 signal 3 } 
	{ out_bufI_d1 sc_out sc_lv 16 signal 3 } 
	{ out_bufQ_address1 sc_out sc_lv 10 signal 4 } 
	{ out_bufQ_ce1 sc_out sc_logic 1 signal 4 } 
	{ out_bufQ_we1 sc_out sc_logic 1 signal 4 } 
	{ out_bufQ_d1 sc_out sc_lv 16 signal 4 } 
}
set NewPortList {[ 
	{ "name": "ap_clk", "direction": "in", "datatype": "sc_logic", "bitwidth":1, "type": "clock", "bundle":{"name": "ap_clk", "role": "default" }} , 
 	{ "name": "ap_rst", "direction": "in", "datatype": "sc_logic", "bitwidth":1, "type": "reset", "bundle":{"name": "ap_rst", "role": "default" }} , 
 	{ "name": "ap_start", "direction": "in", "datatype": "sc_logic", "bitwidth":1, "type": "start", "bundle":{"name": "ap_start", "role": "default" }} , 
 	{ "name": "ap_done", "direction": "out", "datatype": "sc_logic", "bitwidth":1, "type": "predone", "bundle":{"name": "ap_done", "role": "default" }} , 
 	{ "name": "ap_continue", "direction": "in", "datatype": "sc_logic", "bitwidth":1, "type": "continue", "bundle":{"name": "ap_continue", "role": "default" }} , 
 	{ "name": "ap_idle", "direction": "out", "datatype": "sc_logic", "bitwidth":1, "type": "done", "bundle":{"name": "ap_idle", "role": "default" }} , 
 	{ "name": "ap_ready", "direction": "out", "datatype": "sc_logic", "bitwidth":1, "type": "ready", "bundle":{"name": "ap_ready", "role": "default" }} , 
 	{ "name": "scaling_schedule_dout", "direction": "in", "datatype": "sc_lv", "bitwidth":32, "type": "signal", "bundle":{"name": "scaling_schedule", "role": "dout" }} , 
 	{ "name": "scaling_schedule_empty_n", "direction": "in", "datatype": "sc_logic", "bitwidth":1, "type": "signal", "bundle":{"name": "scaling_schedule", "role": "empty_n" }} , 
 	{ "name": "scaling_schedule_read", "direction": "out", "datatype": "sc_logic", "bitwidth":1, "type": "signal", "bundle":{"name": "scaling_schedule", "role": "read" }} , 
 	{ "name": "scaling_schedule_num_data_valid", "direction": "in", "datatype": "sc_lv", "bitwidth":3, "type": "signal", "bundle":{"name": "scaling_schedule", "role": "num_data_valid" }} , 
 	{ "name": "scaling_schedule_fifo_cap", "direction": "in", "datatype": "sc_lv", "bitwidth":3, "type": "signal", "bundle":{"name": "scaling_schedule", "role": "fifo_cap" }} , 
 	{ "name": "in_bufI_address0", "direction": "out", "datatype": "sc_lv", "bitwidth":10, "type": "signal", "bundle":{"name": "in_bufI", "role": "address0" }} , 
 	{ "name": "in_bufI_ce0", "direction": "out", "datatype": "sc_logic", "bitwidth":1, "type": "signal", "bundle":{"name": "in_bufI", "role": "ce0" }} , 
 	{ "name": "in_bufI_q0", "direction": "in", "datatype": "sc_lv", "bitwidth":16, "type": "signal", "bundle":{"name": "in_bufI", "role": "q0" }} , 
 	{ "name": "in_bufQ_address0", "direction": "out", "datatype": "sc_lv", "bitwidth":10, "type": "signal", "bundle":{"name": "in_bufQ", "role": "address0" }} , 
 	{ "name": "in_bufQ_ce0", "direction": "out", "datatype": "sc_logic", "bitwidth":1, "type": "signal", "bundle":{"name": "in_bufQ", "role": "ce0" }} , 
 	{ "name": "in_bufQ_q0", "direction": "in", "datatype": "sc_lv", "bitwidth":16, "type": "signal", "bundle":{"name": "in_bufQ", "role": "q0" }} , 
 	{ "name": "out_bufI_address1", "direction": "out", "datatype": "sc_lv", "bitwidth":10, "type": "signal", "bundle":{"name": "out_bufI", "role": "address1" }} , 
 	{ "name": "out_bufI_ce1", "direction": "out", "datatype": "sc_logic", "bitwidth":1, "type": "signal", "bundle":{"name": "out_bufI", "role": "ce1" }} , 
 	{ "name": "out_bufI_we1", "direction": "out", "datatype": "sc_logic", "bitwidth":1, "type": "signal", "bundle":{"name": "out_bufI", "role": "we1" }} , 
 	{ "name": "out_bufI_d1", "direction": "out", "datatype": "sc_lv", "bitwidth":16, "type": "signal", "bundle":{"name": "out_bufI", "role": "d1" }} , 
 	{ "name": "out_bufQ_address1", "direction": "out", "datatype": "sc_lv", "bitwidth":10, "type": "signal", "bundle":{"name": "out_bufQ", "role": "address1" }} , 
 	{ "name": "out_bufQ_ce1", "direction": "out", "datatype": "sc_logic", "bitwidth":1, "type": "signal", "bundle":{"name": "out_bufQ", "role": "ce1" }} , 
 	{ "name": "out_bufQ_we1", "direction": "out", "datatype": "sc_logic", "bitwidth":1, "type": "signal", "bundle":{"name": "out_bufQ", "role": "we1" }} , 
 	{ "name": "out_bufQ_d1", "direction": "out", "datatype": "sc_lv", "bitwidth":16, "type": "signal", "bundle":{"name": "out_bufQ", "role": "d1" }}  ]}

set ArgLastReadFirstWriteLatency {
	stage_process_fft_real {
		scaling_schedule {Type I LastRead 0 FirstWrite -1}
		in_bufI {Type I LastRead 0 FirstWrite -1}
		in_bufQ {Type I LastRead 0 FirstWrite -1}
		out_bufI {Type O LastRead -1 FirstWrite 2}
		out_bufQ {Type O LastRead -1 FirstWrite 2}}
	stage_process_fft_real_Pipeline_feed_loop {
		in_bufI {Type I LastRead 0 FirstWrite -1}
		in_bufQ {Type I LastRead 0 FirstWrite -1}
		fft_in_stream {Type O LastRead -1 FirstWrite 2}}
	fft_ZenithFFTConfig_s {
		fft_in_stream {Type I LastRead -1 FirstWrite -1}
		fft_out_stream {Type O LastRead -1 FirstWrite -1}
		fft_config_stream {Type I LastRead -1 FirstWrite -1}}
	stage_process_fft_real_Pipeline_drain_loop {
		fft_out_stream {Type I LastRead 1 FirstWrite -1}
		out_bufI {Type O LastRead -1 FirstWrite 2}
		out_bufQ {Type O LastRead -1 FirstWrite 2}}}

set hasDtUnsupportedChannel 0

set PerformanceInfo {[
	{"Name" : "Latency", "Min" : "5254", "Max" : "5254"}
	, {"Name" : "Interval", "Min" : "5254", "Max" : "5254"}
]}

set PipelineEnableSignalInfo {[
]}

set Spec2ImplPortList { 
	scaling_schedule { ap_fifo {  { scaling_schedule_dout fifo_data_out 0 32 }  { scaling_schedule_empty_n fifo_status_empty 0 1 }  { scaling_schedule_read fifo_data_in 1 1 }  { scaling_schedule_num_data_valid fifo_update 0 3 }  { scaling_schedule_fifo_cap fifo_data 0 3 } } }
	in_bufI { ap_memory {  { in_bufI_address0 mem_address 1 10 }  { in_bufI_ce0 mem_ce 1 1 }  { in_bufI_q0 mem_dout 0 16 } } }
	in_bufQ { ap_memory {  { in_bufQ_address0 mem_address 1 10 }  { in_bufQ_ce0 mem_ce 1 1 }  { in_bufQ_q0 mem_dout 0 16 } } }
	out_bufI { ap_memory {  { out_bufI_address1 MemPortADDR2 1 10 }  { out_bufI_ce1 MemPortCE2 1 1 }  { out_bufI_we1 MemPortWE2 1 1 }  { out_bufI_d1 MemPortDIN2 1 16 } } }
	out_bufQ { ap_memory {  { out_bufQ_address1 MemPortADDR2 1 10 }  { out_bufQ_ce1 MemPortCE2 1 1 }  { out_bufQ_we1 MemPortWE2 1 1 }  { out_bufQ_d1 MemPortDIN2 1 16 } } }
}
