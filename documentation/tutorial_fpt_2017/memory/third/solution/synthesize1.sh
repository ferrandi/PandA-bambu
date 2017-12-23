#!/bin/bash
abs_script=$(readlink -e $0)
dir_script=$(dirname $abs_script)

mkdir -p D00
cd D00
bambu $dir_script/mips.c --memory-ctrl-type=D00 --channels-type=MEM_ACC_NN --memory-allocation-policy=EXT_PIPELINED_BRAM --channels-number=4 --clock-period=15 --simulate -v3
cd ..
mkdir -p D10
cd D10
bambu $dir_script/mips.c --memory-ctrl-type=D10 --channels-type=MEM_ACC_NN --memory-allocation-policy=EXT_PIPELINED_BRAM --channels-number=4 --clock-period=15 --simulate -v3
cd ..
mkdir -p D11
cd D11
bambu $dir_script/mips.c --memory-ctrl-type=D11 --channels-type=MEM_ACC_NN --memory-allocation-policy=EXT_PIPELINED_BRAM --channels-number=4 --clock-period=15 --simulate -v3
cd ..
mkdir -p D21
cd D21
bambu $dir_script/mips.c --memory-ctrl-type=D21 --channels-type=MEM_ACC_NN --memory-allocation-policy=EXT_PIPELINED_BRAM --channels-number=4 --clock-period=15 --simulate -v3
cd ..


