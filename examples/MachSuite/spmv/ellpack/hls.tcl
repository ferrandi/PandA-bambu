open_project ellpack_syn

add_files ellpack.c
add_files input.data
add_files check.data
add_files -tb ../../common/harness.c

set_top ellpack

open_solution -reset solution
set_part virtex7
create_clock -period 10
source ./ellpack_dir
csynth_design
cosim_design -rtl verilog -tool modelsim -trace_level all

exit
