open_project net_hls

add_files net.c
add_files input.data
add_files check.data
add_files -tb ../../common/harness.c

set_top backprop

open_solution -reset solution
set_part virtex7
create_clock -period 10

#source ./net_dir
config_rtl -reset all -reset_level low

csynth_design
cosim_design -rtl verilog -tool modelsim -trace_level all

exit
