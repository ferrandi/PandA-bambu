############################################################
## ATAN2() 

## Copyright (C) 2015 Xilinx Inc. All rights reserved.

## daniele.bagni@xilinx.com

## 22 July 2015
############################################################


################## SOLUTION1_DOUBLE: 64-bit floating point ####################

open_project hls_atan2_prj
set_top top_atan2
add_files cordic_atan2.cpp -cflags "-DDB_DOUBLE_PRECISION"
add_files -tb cordic_test.cpp -cflags "-DDB_DOUBLE_PRECISION"
add_files -tb test_data

open_solution "solution1_double"
set_part {xc7z045ffg900-2}
create_clock -period 2.5 -name default
csim_design -clean
csynth_design
#cosim_design
#export_design -evaluate verilog -format ip_catalog

close_project


################## SOLUTION1_FLOAT: 32-bit floating point ####################

open_project hls_atan2_prj
set_top top_atan2
add_files cordic_atan2.cpp -cflags "-DDB_SINGLE_PRECISION"
add_files -tb cordic_test.cpp -cflags "-DDB_SINGLE_PRECISION"
add_files -tb test_data

open_solution "solution1_single"
set_part {xc7z045ffg900-2}
create_clock -period 2.5 -name default
csim_design -clean
csynth_design
#cosim_design
#export_design -evaluate verilog -format ip_catalog

close_project

################## SOLUTION1_CORDIC: cordic algorithm ####################

open_project hls_atan2_prj
set_top top_atan2
add_files cordic_atan2.cpp -cflags "-DDB_CORDIC"
add_files -tb cordic_test.cpp -cflags "-DDB_CORDIC"
add_files -tb test_data

open_solution "solution1_cordic"
set_part {xc7z045ffg900-2}
create_clock -period 2.5 -name default
csim_design -clean
csynth_design
#cosim_design
#export_design -evaluate verilog -format ip_catalog

close_project


################## SOLUTION2_CORDIC: simplified cordic algorithm ####################

open_project hls_atan2_prj
set_top top_atan2
add_files cordic_atan2.cpp -cflags "-DDB_CORDIC -DBIT_ACCURATE"
add_files -tb cordic_test.cpp -cflags "-DDB_CORDIC -DBIT_ACCURATE"
add_files -tb test_data

open_solution "solution2_cordic_bitaccurate"
set_part {xc7z045ffg900-2}
create_clock -period 2.5 -name default
csim_design -clean
csynth_design
cosim_design
export_design -evaluate verilog -format ip_catalog
close_project
