open_project stencil3d_syn

add_files stencil3d.c
add_files input.data
add_files check.data
add_files -tb ../../common/harness.c 

set_top stencil3d
open_solution -reset solution

set_part virtex7
create_clock -period 10
source ./stencil3d_dir

csynth_design
cosim_design -rtl verilog -tool modelsim -trace_level all

exit
