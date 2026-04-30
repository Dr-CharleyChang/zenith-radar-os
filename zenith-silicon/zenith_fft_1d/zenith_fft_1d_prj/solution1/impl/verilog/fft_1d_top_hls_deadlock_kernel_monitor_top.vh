
wire kernel_monitor_reset;
wire kernel_monitor_clock;
wire kernel_monitor_report;
assign kernel_monitor_reset = ~ap_rst_n;
assign kernel_monitor_clock = ap_clk;
assign kernel_monitor_report = 1'b0;
wire [2:0] axis_block_sigs;
wire [8:0] inst_idle_sigs;
wire [3:0] inst_block_sigs;
wire kernel_block;

assign axis_block_sigs[0] = ~Block_entry_in_bufI_wr_proc_U0.grp_stage_read_input_fu_50.in_stream_TDATA_blk_n;
assign axis_block_sigs[1] = ~Block_entry_in_bufI_wr_proc_U0.grp_stage_read_input_1_fu_72.in_stream_TDATA_blk_n;
assign axis_block_sigs[2] = ~stage_write_output_U0.out_stream_TDATA_blk_n;

assign inst_idle_sigs[0] = entry_proc_U0.ap_idle;
assign inst_block_sigs[0] = (entry_proc_U0.ap_done & ~entry_proc_U0.ap_continue) | ~entry_proc_U0.scaling_schedule_c_blk_n;
assign inst_idle_sigs[1] = Block_entry_in_bufI_wr_proc_U0.ap_idle;
assign inst_block_sigs[1] = (Block_entry_in_bufI_wr_proc_U0.ap_done & ~Block_entry_in_bufI_wr_proc_U0.ap_continue);
assign inst_idle_sigs[2] = stage_process_fft_real_U0.ap_idle;
assign inst_block_sigs[2] = (stage_process_fft_real_U0.ap_done & ~stage_process_fft_real_U0.ap_continue) | ~stage_process_fft_real_U0.scaling_schedule_blk_n;
assign inst_idle_sigs[3] = stage_write_output_U0.ap_idle;
assign inst_block_sigs[3] = (stage_write_output_U0.ap_done & ~stage_write_output_U0.ap_continue);

assign inst_idle_sigs[4] = 1'b0;
assign inst_idle_sigs[5] = Block_entry_in_bufI_wr_proc_U0.ap_idle;
assign inst_idle_sigs[6] = Block_entry_in_bufI_wr_proc_U0.grp_stage_read_input_fu_50.ap_idle;
assign inst_idle_sigs[7] = Block_entry_in_bufI_wr_proc_U0.grp_stage_read_input_1_fu_72.ap_idle;
assign inst_idle_sigs[8] = stage_write_output_U0.ap_idle;

fft_1d_top_hls_deadlock_idx0_monitor fft_1d_top_hls_deadlock_idx0_monitor_U (
    .clock(kernel_monitor_clock),
    .reset(kernel_monitor_reset),
    .axis_block_sigs(axis_block_sigs),
    .inst_idle_sigs(inst_idle_sigs),
    .inst_block_sigs(inst_block_sigs),
    .block(kernel_block)
);


always @ (kernel_block or kernel_monitor_reset) begin
    if (kernel_block == 1'b1 && kernel_monitor_reset == 1'b0) begin
        find_kernel_block = 1'b1;
    end
    else begin
        find_kernel_block = 1'b0;
    end
end
