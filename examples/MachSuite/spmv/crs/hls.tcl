open_project crs_syn

add_files crs.c
add_files input.data
add_files check.data
add_files -tb ../../common/harness.c

set_top spmv

open_solution -reset solution
set_part virtex7
create_clock -period 10
source ./crs_dir
csynth_design
cosim_design -rtl verilog -tool modelsim 

exit
