transcript file ""
transcript file simulation.log
transcript on

#set XILINX_MODE 1
#set define_macro +define+ASYNC_WFIFO_DEPTH=$ASYNC_WFIFO_DEPTH+define+XILINX_MODE=$XILINX_MODE

set HLS_PATH ../hls
set RTL_PATH ../rtl
set TB_PATH ../tb

vlog -64 -incr -lint -sv  $HLS_PATH/calc_mvd_cost_int64_hls_mul_41ns_64s_64_5_1.v
vlog -64 -incr -lint -sv  $HLS_PATH/calc_mvd_cost_int64_hls.v
vlog -64 -incr -lint -sv  $RTL_PATH/calc_mvd_cost.sv
vlog -64 -incr -lint -sv  $TB_PATH/calc_mvd_cost_tb.sv

vsim -vopt -voptargs="+acc" -coverage calc_mvd_cost_tb

do wave.do

run -all