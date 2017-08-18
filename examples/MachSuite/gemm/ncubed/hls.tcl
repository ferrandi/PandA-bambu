open_project gemm_hls

add_files gemm.c
add_files input.data
add_files check.data
add_files -tb ../../common/harness.c
#add_files -tb gemm_test.c

set_top gemm

open_solution -reset solution
set_part virtex7
create_clock -period 10
source ./gemm_dir
csynth_design
cosim_design -rtl verilog -tool modelsim -trace_level all

exit
