onerror {resume}
quietly WaveActivateNextPane {} 0
add wave -noupdate /calc_mvd_cost_tb/test_count
add wave -noupdate /calc_mvd_cost_tb/test_done
add wave -noupdate /calc_mvd_cost_tb/test_state
add wave -noupdate /calc_mvd_cost_tb/dut/ap_rst
add wave -noupdate /calc_mvd_cost_tb/dut/ap_clk
add wave -noupdate /calc_mvd_cost_tb/dut/ap_start
add wave -noupdate /calc_mvd_cost_tb/dut/ap_idle
add wave -noupdate /calc_mvd_cost_tb/dut/ap_ready
add wave -noupdate /calc_mvd_cost_tb/dut/ap_done
add wave -noupdate /calc_mvd_cost_tb/dut/mvd_cost_int64_ap_vld
add wave -noupdate /calc_mvd_cost_tb/dut/mvd_cost_int64
add wave -noupdate /calc_mvd_cost_tb/dut/bitcost_ap_vld
add wave -noupdate /calc_mvd_cost_tb/dut/bitcost
add wave -noupdate /calc_mvd_cost_tb/dut/lambda_sqrt_decimal_int64
add wave -noupdate /calc_mvd_cost_tb/dut/lambda_sqrt_integer_int64
add wave -noupdate /calc_mvd_cost_tb/dut/mv_cand_0
add wave -noupdate /calc_mvd_cost_tb/dut/mv_cand_1
add wave -noupdate /calc_mvd_cost_tb/dut/mv_cand_2
add wave -noupdate /calc_mvd_cost_tb/dut/mv_cand_3
add wave -noupdate /calc_mvd_cost_tb/dut/mv_shift
add wave -noupdate /calc_mvd_cost_tb/dut/x
add wave -noupdate /calc_mvd_cost_tb/dut/y
add wave -noupdate -group hls /calc_mvd_cost_tb/hls/ap_ce
add wave -noupdate -group hls /calc_mvd_cost_tb/hls/ap_clk
add wave -noupdate -group hls /calc_mvd_cost_tb/hls/ap_core
add wave -noupdate -group hls /calc_mvd_cost_tb/hls/ap_done
add wave -noupdate -group hls /calc_mvd_cost_tb/hls/ap_idle
add wave -noupdate -group hls /calc_mvd_cost_tb/hls/ap_parent
add wave -noupdate -group hls /calc_mvd_cost_tb/hls/ap_part
add wave -noupdate -group hls /calc_mvd_cost_tb/hls/ap_ready
add wave -noupdate -group hls /calc_mvd_cost_tb/hls/ap_rst
add wave -noupdate -group hls /calc_mvd_cost_tb/hls/ap_start
add wave -noupdate -group hls /calc_mvd_cost_tb/hls/bitcost
add wave -noupdate -group hls /calc_mvd_cost_tb/hls/bitcost_ap_vld
add wave -noupdate -group hls /calc_mvd_cost_tb/hls/lambda_sqrt_decimal_int64
add wave -noupdate -group hls /calc_mvd_cost_tb/hls/lambda_sqrt_integer_int64
add wave -noupdate -group hls /calc_mvd_cost_tb/hls/mv_cand_0
add wave -noupdate -group hls /calc_mvd_cost_tb/hls/mv_cand_1
add wave -noupdate -group hls /calc_mvd_cost_tb/hls/mv_cand_2
add wave -noupdate -group hls /calc_mvd_cost_tb/hls/mv_cand_3
add wave -noupdate -group hls /calc_mvd_cost_tb/hls/mv_shift
add wave -noupdate -group hls /calc_mvd_cost_tb/hls/mvd_cost_int64
add wave -noupdate -group hls /calc_mvd_cost_tb/hls/mvd_cost_int64_ap_vld
add wave -noupdate -group hls /calc_mvd_cost_tb/hls/x
add wave -noupdate -group hls /calc_mvd_cost_tb/hls/y
TreeUpdate [SetDefaultTree]
WaveRestoreCursors {{Cursor 13} {100000 ps} 1} {{Cursor 3} {998533264 ps} 0} {{Cursor 4} {7943395804 ps} 0}
quietly wave cursor active 2
configure wave -namecolwidth 201
configure wave -valuecolwidth 260
configure wave -justifyvalue left
configure wave -signalnamewidth 1
configure wave -snapdistance 10
configure wave -datasetprefix 0
configure wave -rowmargin 4
configure wave -childrowmargin 2
configure wave -gridoffset 0
configure wave -gridperiod 1
configure wave -griddelta 40
configure wave -timeline 0
configure wave -timelineunits ps
update
WaveRestoreZoom {0 ps} {1624374 ps}
