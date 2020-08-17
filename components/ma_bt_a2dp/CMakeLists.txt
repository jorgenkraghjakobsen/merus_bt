idf_component_register(SRCS "bt_app_av.c" 
                            "bt_app_core.c" 
                            "ma_bt_a2dp.c"  
                       INCLUDE_DIRS "include"  
                       REQUIRES MerusAudio dsp_processor)
