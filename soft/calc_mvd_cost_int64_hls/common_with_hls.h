#define IMPL(func)      func


#if __VITIS_HLS__ || __SYNTHESIS__
    #define HLS_COMMON_INIT_VAR()	uint8_t ap_core=0; uint8_t ap_part = 0; uint8_t ap_parent = 0;
    #define HLS_COMMON_ARG          uint8_t ap_core, uint8_t ap_part, uint8_t ap_parent, 
    #define HLS_COMMON_ARG_CALL     ap_core, ap_part, ap_parent, 
#else
    #define HLS_COMMON_INIT_VAR()
    #define HLS_COMMON_ARG
    #define HLS_COMMON_ARG_CALL
#endif

