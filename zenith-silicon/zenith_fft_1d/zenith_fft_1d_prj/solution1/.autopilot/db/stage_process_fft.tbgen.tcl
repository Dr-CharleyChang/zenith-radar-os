set moduleName stage_process_fft
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
set cdfgNum 6
set C_modelName {stage_process_fft}
set C_modelType { void 0 }
set ap_memory_interface_dict [dict create]
dict set ap_memory_interface_dict bufI { MEM_WIDTH 16 MEM_SIZE 2048 MASTER_TYPE BRAM_CTRL MEM_ADDRESS_MODE WORD_ADDRESS PACKAGE_IO port READ_LATENCY 1 }
dict set ap_memory_interface_dict outI { MEM_WIDTH 16 MEM_SIZE 2048 MASTER_TYPE BRAM_CTRL MEM_ADDRESS_MODE WORD_ADDRESS PACKAGE_IO port READ_LATENCY 0 }
dict set ap_memory_interface_dict outQ { MEM_WIDTH 1 MEM_SIZE 1024 MASTER_TYPE BRAM_CTRL MEM_ADDRESS_MODE WORD_ADDRESS PACKAGE_IO port READ_LATENCY 0 }
set C_modelArgList {
	{ bufI int 16 regular {array 1024 { 1 3 } 1 1 }  }
	{ outI int 16 regular {array 1024 { 3 0 } 0 1 }  }
	{ outQ int 1 regular {array 1024 { 3 0 } 0 1 }  }
}
set hasAXIMCache 0
set l_AXIML2Cache [list]
set AXIMCacheInstDict [dict create]
set C_modelArgMapList {[ 
	{ "Name" : "bufI", "interface" : "memory", "bitwidth" : 16, "direction" : "READONLY"} , 
 	{ "Name" : "outI", "interface" : "memory", "bitwidth" : 16, "direction" : "WRITEONLY"} , 
 	{ "Name" : "outQ", "interface" : "memory", "bitwidth" : 1, "direction" : "WRITEONLY"} ]}
# RTL Port declarations: 
set portNum 18
set portList { 
	{ ap_clk sc_in sc_logic 1 clock -1 } 
	{ ap_rst sc_in sc_logic 1 reset -1 active_high_sync } 
	{ ap_start sc_in sc_logic 1 start -1 } 
	{ ap_done sc_out sc_logic 1 predone -1 } 
	{ ap_continue sc_in sc_logic 1 continue -1 } 
	{ ap_idle sc_out sc_logic 1 done -1 } 
	{ ap_ready sc_out sc_logic 1 ready -1 } 
	{ bufI_address0 sc_out sc_lv 10 signal 0 } 
	{ bufI_ce0 sc_out sc_logic 1 signal 0 } 
	{ bufI_q0 sc_in sc_lv 16 signal 0 } 
	{ outI_address1 sc_out sc_lv 10 signal 1 } 
	{ outI_ce1 sc_out sc_logic 1 signal 1 } 
	{ outI_we1 sc_out sc_logic 1 signal 1 } 
	{ outI_d1 sc_out sc_lv 16 signal 1 } 
	{ outQ_address1 sc_out sc_lv 10 signal 2 } 
	{ outQ_ce1 sc_out sc_logic 1 signal 2 } 
	{ outQ_we1 sc_out sc_logic 1 signal 2 } 
	{ outQ_d1 sc_out sc_lv 1 signal 2 } 
}
set NewPortList {[ 
	{ "name": "ap_clk", "direction": "in", "datatype": "sc_logic", "bitwidth":1, "type": "clock", "bundle":{"name": "ap_clk", "role": "default" }} , 
 	{ "name": "ap_rst", "direction": "in", "datatype": "sc_logic", "bitwidth":1, "type": "reset", "bundle":{"name": "ap_rst", "role": "default" }} , 
 	{ "name": "ap_start", "direction": "in", "datatype": "sc_logic", "bitwidth":1, "type": "start", "bundle":{"name": "ap_start", "role": "default" }} , 
 	{ "name": "ap_done", "direction": "out", "datatype": "sc_logic", "bitwidth":1, "type": "predone", "bundle":{"name": "ap_done", "role": "default" }} , 
 	{ "name": "ap_continue", "direction": "in", "datatype": "sc_logic", "bitwidth":1, "type": "continue", "bundle":{"name": "ap_continue", "role": "default" }} , 
 	{ "name": "ap_idle", "direction": "out", "datatype": "sc_logic", "bitwidth":1, "type": "done", "bundle":{"name": "ap_idle", "role": "default" }} , 
 	{ "name": "ap_ready", "direction": "out", "datatype": "sc_logic", "bitwidth":1, "type": "ready", "bundle":{"name": "ap_ready", "role": "default" }} , 
 	{ "name": "bufI_address0", "direction": "out", "datatype": "sc_lv", "bitwidth":10, "type": "signal", "bundle":{"name": "bufI", "role": "address0" }} , 
 	{ "name": "bufI_ce0", "direction": "out", "datatype": "sc_logic", "bitwidth":1, "type": "signal", "bundle":{"name": "bufI", "role": "ce0" }} , 
 	{ "name": "bufI_q0", "direction": "in", "datatype": "sc_lv", "bitwidth":16, "type": "signal", "bundle":{"name": "bufI", "role": "q0" }} , 
 	{ "name": "outI_address1", "direction": "out", "datatype": "sc_lv", "bitwidth":10, "type": "signal", "bundle":{"name": "outI", "role": "address1" }} , 
 	{ "name": "outI_ce1", "direction": "out", "datatype": "sc_logic", "bitwidth":1, "type": "signal", "bundle":{"name": "outI", "role": "ce1" }} , 
 	{ "name": "outI_we1", "direction": "out", "datatype": "sc_logic", "bitwidth":1, "type": "signal", "bundle":{"name": "outI", "role": "we1" }} , 
 	{ "name": "outI_d1", "direction": "out", "datatype": "sc_lv", "bitwidth":16, "type": "signal", "bundle":{"name": "outI", "role": "d1" }} , 
 	{ "name": "outQ_address1", "direction": "out", "datatype": "sc_lv", "bitwidth":10, "type": "signal", "bundle":{"name": "outQ", "role": "address1" }} , 
 	{ "name": "outQ_ce1", "direction": "out", "datatype": "sc_logic", "bitwidth":1, "type": "signal", "bundle":{"name": "outQ", "role": "ce1" }} , 
 	{ "name": "outQ_we1", "direction": "out", "datatype": "sc_logic", "bitwidth":1, "type": "signal", "bundle":{"name": "outQ", "role": "we1" }} , 
 	{ "name": "outQ_d1", "direction": "out", "datatype": "sc_lv", "bitwidth":1, "type": "signal", "bundle":{"name": "outQ", "role": "d1" }}  ]}

set ArgLastReadFirstWriteLatency {
	stage_process_fft {
		bufI {Type I LastRead 0 FirstWrite -1}
		outI {Type O LastRead -1 FirstWrite 2}
		outQ {Type O LastRead -1 FirstWrite 2}}}

set hasDtUnsupportedChannel 0

set PerformanceInfo {[
	{"Name" : "Latency", "Min" : "1027", "Max" : "1027"}
	, {"Name" : "Interval", "Min" : "1024", "Max" : "1024"}
]}

set PipelineEnableSignalInfo {[
	{"Pipeline" : "0", "EnableSignal" : "ap_enable_pp0"}
]}

set Spec2ImplPortList { 
	bufI { ap_memory {  { bufI_address0 mem_address 1 10 }  { bufI_ce0 mem_ce 1 1 }  { bufI_q0 mem_dout 0 16 } } }
	outI { ap_memory {  { outI_address1 MemPortADDR2 1 10 }  { outI_ce1 MemPortCE2 1 1 }  { outI_we1 MemPortWE2 1 1 }  { outI_d1 MemPortDIN2 1 16 } } }
	outQ { ap_memory {  { outQ_address1 MemPortADDR2 1 10 }  { outQ_ce1 MemPortCE2 1 1 }  { outQ_we1 MemPortWE2 1 1 }  { outQ_d1 MemPortDIN2 1 1 } } }
}
