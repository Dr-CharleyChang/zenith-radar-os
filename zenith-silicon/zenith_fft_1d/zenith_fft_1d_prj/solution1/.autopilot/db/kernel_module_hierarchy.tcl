set ModuleHierarchy {[{
"Name" : "fft_1d_top", "RefName" : "fft_1d_top","ID" : "0","Type" : "dataflow",
"SubInsts" : [
	{"Name" : "Block_entry_in_bufI_wr_proc_U0", "RefName" : "Block_entry_in_bufI_wr_proc","ID" : "1","Type" : "sequential",
		"SubInsts" : [
		{"Name" : "grp_stage_read_input_fu_50", "RefName" : "stage_read_input","ID" : "2","Type" : "sequential",
			"SubLoops" : [
			{"Name" : "VITIS_LOOP_41_1","RefName" : "VITIS_LOOP_41_1","ID" : "3","Type" : "pipeline"},]},
		{"Name" : "grp_stage_read_input_1_fu_72", "RefName" : "stage_read_input_1","ID" : "4","Type" : "sequential",
			"SubLoops" : [
			{"Name" : "VITIS_LOOP_41_1","RefName" : "VITIS_LOOP_41_1","ID" : "5","Type" : "pipeline"},]},]},
	{"Name" : "entry_proc_U0", "RefName" : "entry_proc","ID" : "6","Type" : "sequential"},
	{"Name" : "stage_process_fft_real_U0", "RefName" : "stage_process_fft_real","ID" : "7","Type" : "sequential",
		"SubInsts" : [
		{"Name" : "grp_stage_process_fft_real_Pipeline_feed_loop_fu_92", "RefName" : "stage_process_fft_real_Pipeline_feed_loop","ID" : "8","Type" : "sequential",
			"SubLoops" : [
			{"Name" : "feed_loop","RefName" : "feed_loop","ID" : "9","Type" : "pipeline"},]},
		{"Name" : "grp_fft_ZenithFFTConfig_s_fu_101", "RefName" : "fft_ZenithFFTConfig_s","ID" : "10","Type" : "sequential"},
		{"Name" : "grp_stage_process_fft_real_Pipeline_drain_loop_fu_108", "RefName" : "stage_process_fft_real_Pipeline_drain_loop","ID" : "11","Type" : "sequential",
			"SubLoops" : [
			{"Name" : "drain_loop","RefName" : "drain_loop","ID" : "12","Type" : "pipeline"},]},]},
	{"Name" : "stage_write_output_U0", "RefName" : "stage_write_output","ID" : "13","Type" : "sequential",
		"SubLoops" : [
		{"Name" : "VITIS_LOOP_100_1","RefName" : "VITIS_LOOP_100_1","ID" : "14","Type" : "pipeline"},]},]
}]}