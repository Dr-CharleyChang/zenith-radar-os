set ModuleHierarchy {[{
"Name" : "fft_1d_top", "RefName" : "fft_1d_top","ID" : "0","Type" : "dataflow",
"SubInsts" : [
	{"Name" : "stage_read_input_U0", "RefName" : "stage_read_input","ID" : "1","Type" : "sequential",
		"SubLoops" : [
		{"Name" : "VITIS_LOOP_23_1","RefName" : "VITIS_LOOP_23_1","ID" : "2","Type" : "pipeline"},]},
	{"Name" : "stage_process_fft_U0", "RefName" : "stage_process_fft","ID" : "3","Type" : "sequential",
		"SubLoops" : [
		{"Name" : "VITIS_LOOP_54_1","RefName" : "VITIS_LOOP_54_1","ID" : "4","Type" : "pipeline"},]},
	{"Name" : "stage_write_output_U0", "RefName" : "stage_write_output","ID" : "5","Type" : "sequential",
		"SubLoops" : [
		{"Name" : "VITIS_LOOP_69_1","RefName" : "VITIS_LOOP_69_1","ID" : "6","Type" : "pipeline"},]},]
}]}