set SynModuleInfo {
  {SRCNAME entry_proc MODELNAME entry_proc RTLNAME fft_1d_top_entry_proc}
  {SRCNAME stage_read_input MODELNAME stage_read_input RTLNAME fft_1d_top_stage_read_input
    SUBMODULES {
      {MODELNAME fft_1d_top_flow_control_loop_pipe_sequential_init RTLNAME fft_1d_top_flow_control_loop_pipe_sequential_init BINDTYPE interface TYPE internal_upc_flow_control INSTNAME fft_1d_top_flow_control_loop_pipe_sequential_init_U}
    }
  }
  {SRCNAME stage_read_input.1 MODELNAME stage_read_input_1 RTLNAME fft_1d_top_stage_read_input_1}
  {SRCNAME Block_entry_in_bufI_wr_proc MODELNAME Block_entry_in_bufI_wr_proc RTLNAME fft_1d_top_Block_entry_in_bufI_wr_proc
    SUBMODULES {
      {MODELNAME fft_1d_top_regslice_both RTLNAME fft_1d_top_regslice_both BINDTYPE interface TYPE adapter IMPL reg_slice}
    }
  }
  {SRCNAME stage_process_fft_real_Pipeline_feed_loop MODELNAME stage_process_fft_real_Pipeline_feed_loop RTLNAME fft_1d_top_stage_process_fft_real_Pipeline_feed_loop}
  {SRCNAME fft<ZenithFFTConfig> MODELNAME fft_ZenithFFTConfig_s RTLNAME fft_1d_top_fft_ZenithFFTConfig_s
    SUBMODULES {
      {MODELNAME fft_1d_top_fft_ZenithFFTConfig_s RTLNAME fft_1d_top_fft_ZenithFFTConfig_s BINDTYPE op TYPE ip_block_Vivado_FFT}
    }
  }
  {SRCNAME stage_process_fft_real_Pipeline_drain_loop MODELNAME stage_process_fft_real_Pipeline_drain_loop RTLNAME fft_1d_top_stage_process_fft_real_Pipeline_drain_loop}
  {SRCNAME stage_process_fft_real MODELNAME stage_process_fft_real RTLNAME fft_1d_top_stage_process_fft_real
    SUBMODULES {
      {MODELNAME fft_1d_top_fifo_w32_d1024_A RTLNAME fft_1d_top_fifo_w32_d1024_A BINDTYPE storage TYPE fifo IMPL memory ALLOW_PRAGMA 1 INSTNAME fft_in_stream_U}
      {MODELNAME fft_1d_top_fifo_w32_d1024_A RTLNAME fft_1d_top_fifo_w32_d1024_A BINDTYPE storage TYPE fifo IMPL memory ALLOW_PRAGMA 1 INSTNAME fft_out_stream_U}
      {MODELNAME fft_1d_top_fifo_w16_d2_S RTLNAME fft_1d_top_fifo_w16_d2_S BINDTYPE storage TYPE fifo IMPL srl ALLOW_PRAGMA 1 INSTNAME fft_config_stream_U}
    }
  }
  {SRCNAME stage_write_output MODELNAME stage_write_output RTLNAME fft_1d_top_stage_write_output
    SUBMODULES {
      {MODELNAME fft_1d_top_flow_control_loop_pipe RTLNAME fft_1d_top_flow_control_loop_pipe BINDTYPE interface TYPE internal_upc_flow_control INSTNAME fft_1d_top_flow_control_loop_pipe_U}
    }
  }
  {SRCNAME fft_1d_top MODELNAME fft_1d_top RTLNAME fft_1d_top IS_TOP 1
    SUBMODULES {
      {MODELNAME fft_1d_top_in_bufI_RAM_2P_BRAM_1R1W_memcore RTLNAME fft_1d_top_in_bufI_RAM_2P_BRAM_1R1W_memcore BINDTYPE storage TYPE ram_2p IMPL bram LATENCY 2 ALLOW_PRAGMA 1}
      {MODELNAME fft_1d_top_in_bufI_RAM_2P_BRAM_1R1W RTLNAME fft_1d_top_in_bufI_RAM_2P_BRAM_1R1W BINDTYPE storage TYPE ram_2p IMPL bram LATENCY 2 ALLOW_PRAGMA 1}
      {MODELNAME fft_1d_top_fifo_w32_d3_S RTLNAME fft_1d_top_fifo_w32_d3_S BINDTYPE storage TYPE fifo IMPL srl ALLOW_PRAGMA 1 INSTNAME scaling_schedule_c_U}
      {MODELNAME fft_1d_top_CTRL_s_axi RTLNAME fft_1d_top_CTRL_s_axi BINDTYPE interface TYPE interface_s_axilite}
    }
  }
}
