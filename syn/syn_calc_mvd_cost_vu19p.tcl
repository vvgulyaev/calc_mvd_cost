set RTL_PATH ../rtl
set XDC_PATH ../xdc

# ###############
# create project
# ###############
create_project calc_mvd_cost -part xcvu19p-fsva3824-2-e -force

# #############
# set property
# #############
set_property source_mgmt_mode DisplayOnly [current_project]
set_property coreContainer.enable 0 [current_project]
set_param project.singleFileAddWarning.Threshold 1000
set_property -name {STEPS.SYNTH_DESIGN.ARGS.MORE OPTIONS} -value {-mode out_of_context} -objects [get_runs synth_1]
set_property target_language Verilog [current_project]
set_param synth.elaboration.rodinMoreOptions {rt::set_parameter dissolveMemorySizeLimit 327680}

# #################################
# add systemVerilog /Verilog files
# #################################
add_files -norecurse $RTL_PATH/calc_mvd_cost.sv

# #############
# add xdc file
# #############
read_xdc $XDC_PATH/constr_calc_mvd_cost.xdc

# ###############
# set top module
# ###############
set_property top calc_mvd_cost [current_fileset]

# #########################
# Suprress warning message
# #########################
# [Synth 8-7129] Port in module is either unconnected or has no load
set_msg_config -suppress -id {Synth 8-7129}
