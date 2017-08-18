open_project grid_syn

add_files md.c
add_files input.data
add_files check.data
add_files -tb ../../common/harness.c

set_top md

open_solution -reset solution
set_part virtex7
create_clock -period 10
#source ./grid_dir
#config_rtl -reset all -reset_level low
csynth_design
cosim_design -rtl verilog -tool modelsim -trace_level all

exit
