#!/bin/bash
abs_script=$(readlink -e $0)
dir_script=$(dirname $abs_script)

mkdir -p lat2
cd lat2
bambu $dir_script/mips.c --channels-type=MEM_ACC_NN --clock-period=15 --simulate -v3
cd ..
mkdir -p lat3
cd lat3
bambu $dir_script/mips.c --bram-high-latency=3 --channels-type=MEM_ACC_NN --clock-period=15 --simulate -v3
cd ..
mkdir -p lat4
cd lat4
bambu $dir_script/mips.c --bram-high-latency=4 --channels-type=MEM_ACC_NN --clock-period=15 --simulate -v3
cd ..

