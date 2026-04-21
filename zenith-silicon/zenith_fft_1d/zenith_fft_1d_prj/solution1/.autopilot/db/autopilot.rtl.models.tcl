set SynModuleInfo {
  {SRCNAME stage_read_input MODELNAME stage_read_input RTLNAME fft_1d_top_stage_read_input
    SUBMODULES {
      {MODELNAME fft_1d_top_regslice_both RTLNAME fft_1d_top_regslice_both BINDTYPE interface TYPE adapter IMPL reg_slice}
      {MODELNAME fft_1d_top_flow_control_loop_pipe RTLNAME fft_1d_top_flow_control_loop_pipe BINDTYPE interface TYPE internal_upc_flow_control INSTNAME fft_1d_top_flow_control_loop_pipe_U}
    }
  }
  {SRCNAME stage_process_fft MODELNAME stage_process_fft RTLNAME fft_1d_top_stage_process_fft}
  {SRCNAME stage_write_output MODELNAME stage_write_output RTLNAME fft_1d_top_stage_write_output}
  {SRCNAME fft_1d_top MODELNAME fft_1d_top RTLNAME fft_1d_top IS_TOP 1
    SUBMODULES {
      {MODELNAME fft_1d_top_in_bufI_RAM_2P_BRAM_1R1W_memcore RTLNAME fft_1d_top_in_bufI_RAM_2P_BRAM_1R1W_memcore BINDTYPE storage TYPE ram_2p IMPL bram LATENCY 2 ALLOW_PRAGMA 1}
      {MODELNAME fft_1d_top_in_bufI_RAM_2P_BRAM_1R1W RTLNAME fft_1d_top_in_bufI_RAM_2P_BRAM_1R1W BINDTYPE storage TYPE ram_2p IMPL bram LATENCY 2 ALLOW_PRAGMA 1}
      {MODELNAME fft_1d_top_out_bufQ_RAM_2P_BRAM_1R1W_memcore RTLNAME fft_1d_top_out_bufQ_RAM_2P_BRAM_1R1W_memcore BINDTYPE storage TYPE ram_2p IMPL bram LATENCY 2 ALLOW_PRAGMA 1}
      {MODELNAME fft_1d_top_out_bufQ_RAM_2P_BRAM_1R1W RTLNAME fft_1d_top_out_bufQ_RAM_2P_BRAM_1R1W BINDTYPE storage TYPE ram_2p IMPL bram LATENCY 2 ALLOW_PRAGMA 1}
      {MODELNAME fft_1d_top_CTRL_s_axi RTLNAME fft_1d_top_CTRL_s_axi BINDTYPE interface TYPE interface_s_axilite}
    }
  }
}
