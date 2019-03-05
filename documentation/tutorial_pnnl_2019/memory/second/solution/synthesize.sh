#!/bin/bash
abs_script=$(readlink -e $0)
dir_script=$(dirname $abs_script)

mkdir -p nn
cd nn
bambu $dir_script/adpcm.c --simulate --clock-period=15  --channels-type=MEM_ACC_NN -v3 --aligned-access
cd ..
mkdir -p n1
cd n1
bambu $dir_script/adpcm.c --simulate --clock-period=15  --channels-type=MEM_ACC_N1 -v3 --aligned-access
cd ..
mkdir -p 11
cd 11
bambu $dir_script/adpcm.c --simulate --clock-period=15  --channels-type=MEM_ACC_11 -v3 --aligned-access
cd ..
mkdir -p nn_ext_pipelined1
cd nn_ext_pipelined1
bambu $dir_script/adpcm.c --simulate --clock-period=15  --channels-type=MEM_ACC_11 --memory-allocation-policy=EXT_PIPELINED_BRAM -v3 --aligned-access
cd ..
mkdir -p nn_ext_pipelined2
cd nn_ext_pipelined2
bambu $dir_script/adpcm.c --simulate --clock-period=15  --channels-type=MEM_ACC_NN --memory-allocation-policy=EXT_PIPELINED_BRAM --channels-number=2 -v3 --aligned-access
cd ..
mkdir -p nn_ext_pipelined4
cd nn_ext_pipelined4
bambu $dir_script/adpcm.c --simulate --clock-period=15  --channels-type=MEM_ACC_NN --memory-allocation-policy=EXT_PIPELINED_BRAM --channels-number=4 -v3 --aligned-access
cd ..
mkdir -p nn_ext_pipelined8
cd nn_ext_pipelined8
bambu $dir_script/adpcm.c --simulate --clock-period=15  --channels-type=MEM_ACC_NN --memory-allocation-policy=EXT_PIPELINED_BRAM --channels-number=8 -v3 --aligned-access
cd ..
mkdir -p nn_ext_pipelined16
cd nn_ext_pipelined16
bambu $dir_script/adpcm.c --simulate --clock-period=15  --channels-type=MEM_ACC_NN --memory-allocation-policy=EXT_PIPELINED_BRAM --channels-number=8 -v3 --aligned-access
cd ..
