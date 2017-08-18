open_project radix_syn

set_top ss_sort

add_files radix.c
add_files input.data
add_files check.data
add_files -tb ../../common/harness.c

set clock 10
set part virtex7


open_solution solution
set_part $part
create_clock -period $clock
source ./inline_dir

#config_rtl -reset all -reset_level low
set_clock_uncertainty 0
csynth_design 
cosim_design -rtl verilog -tool modelsim -trace_level all

exit
