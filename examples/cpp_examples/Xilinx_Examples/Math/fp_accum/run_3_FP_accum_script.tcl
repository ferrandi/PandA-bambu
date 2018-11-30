############################################################
## Single Precision Floating Point Accumulator

## Copyright (C) 2015 Xilinx Inc. All rights reserved.

## daniele.bagni@xilinx.com

## 22 July 2015
############################################################


############## ORIGINAL C CODE PURELY SEQUENTIAL #####################
open_project hls_fp_acc_prj
set_top hls_fp_accumulator
add_files fp_accum.cpp
add_files -tb fp_accum.cpp

# baseline solution1: no directives
open_solution "solution1"
set_part {xc7k325tffg900-2}
create_clock -period 2.5 -name default
csim_design -clean
csynth_design

# solution2: as solution1 plus PIPELINE the inner loop
open_solution "solution2"
set_part {xc7k325tffg900-2}
create_clock -period 2.5 -name default
set_directive_pipeline "hls_fp_accumulator/L1"
csynth_design

close_project

############## OPTIMIZED C CODE WITH PARALLEL ADDER TREE #####################


# solution3: all loops are PIPELINED each one  separately
open_project hls_fp_acc_prj
set_top hls_fp_accumulator
add_files fp_accum.cpp -cflags "-DDB_OPTIMIZED"
add_files -tb fp_accum.cpp
open_solution "solution3"
set_part {xc7k325tffg900-2}
create_clock -period 2.5 -name default
csim_design -clean
csynth_design

# solution4: as solution3 plus PIPELINE at top level 
open_solution "solution4"
set_part {xc7k325tffg900-2}
create_clock -period 2.5 -name default
set_directive_pipeline "hls_fp_accumulator"
csynth_design
cosim_design
export_design -evaluate verilog -format ip_catalog

close_project
