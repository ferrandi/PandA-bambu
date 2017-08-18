open_project kmp_syn

add_files kmp.c
add_files input.data
add_files check.data
add_files -tb ../../common/harness.c

#add_files -tb kmp_test.c

set_top kmp

open_solution -reset solution
set_part virtex7
create_clock -period 10
#source ./kmp_dir
#config_rtl -reset all -reset_level low
csynth_design
cosim_design -rtl verilog -tool modelsim

exit
