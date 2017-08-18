open_project queue_syn

add_files queue.c
add_files input.data
add_files check.data
add_files -tb ../../common/harness.c

set_top bfs

open_solution -reset solution
set_part virtex7
create_clock -period 10
source ./queue_dir
#config_rtl -reset all -reset_level low
csynth_design
cosim_design -rtl verilog -tool modelsim -trace_level all

exit
