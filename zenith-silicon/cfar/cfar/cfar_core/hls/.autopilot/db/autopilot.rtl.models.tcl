set SynModuleInfo {
  {SRCNAME cfar_core MODELNAME cfar_core RTLNAME cfar_core IS_TOP 1
    SUBMODULES {
      {MODELNAME cfar_core_mul_16s_16s_24_2_1 RTLNAME cfar_core_mul_16s_16s_24_2_1 BINDTYPE op TYPE mul IMPL auto LATENCY 1 ALLOW_PRAGMA 1}
      {MODELNAME cfar_core_CTRL_s_axi RTLNAME cfar_core_CTRL_s_axi BINDTYPE interface TYPE interface_s_axilite}
      {MODELNAME cfar_core_regslice_both RTLNAME cfar_core_regslice_both BINDTYPE interface TYPE adapter IMPL reg_slice}
    }
  }
}
