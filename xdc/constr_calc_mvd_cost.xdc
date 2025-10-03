create_clock -period 3.3 -name clk     -waveform {0.000 1.67} [get_ports {ap_clk}]

set  aclk_inps    [get_ports {\
    ap_start \
    x* \
    y* \
    mv_shift* \
    mv_cand* \
    lambda_sqrt_integer_int64* \
    lambda_sqrt_decimal_int64* \
}]

set aclk_outps  [get_ports {\
    ap_done \
    ap_ready \
    ap_idle \
    bitcost* \
    bitcost_ap_vld \
    mvd_cost_int64* \
    mvd_cost_int64_ap_vld \
}]

set_input_delay -clock [get_clocks clk] -max 0.100 $aclk_inps
set_input_delay -clock [get_clocks clk] -min 0.000 $aclk_inps

set_output_delay -clock [get_clocks clk] -max 0.100 $aclk_outps
set_output_delay -clock [get_clocks clk] -min 0.000 $aclk_outps

set_false_path -from [get_ports {\
                                    ap_rst\
                                }]

#set_property HD.CLK_SRC BUFGCTRL_X0Y2 [get_ports {clk} ]
#set_property HD.PARTPIN_RANGE SLICE_X1Y1:SLICE_X78Y298 $aclk_inps
#set_property HD.PARTPIN_RANGE SLICE_X1Y1:SLICE_X78Y298 $aclk_outps