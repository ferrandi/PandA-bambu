#!/bin/bash
abs_script=$(readlink -e $0)
dir_script=$(dirname $abs_script)

mkdir -p all_bram
cd all_bram
bambu $dir_script/adpcm.c --simulate --clock-period=15  --memory-allocation-policy=ALL_BRAM
cd ..
mkdir -p lss
cd lss
bambu $dir_script/adpcm.c --simulate --clock-period=15 --memory-allocation-policy=LSS
cd ..
mkdir -p gss
cd gss
bambu $dir_script/adpcm.c --simulate --clock-period=15 --memory-allocation-policy=GSS
cd ..
mkdir -p no_bram
cd no_bram
bambu $dir_script/adpcm.c --simulate --clock-period=15 --memory-allocation-policy=NO_BRAM
cd ..
mkdir -p ext_pipelined
cd ext_pipelined
bambu $dir_script/adpcm.c --simulate --clock-period=15 --memory-allocation-policy=EXT_PIPELINED_BRAM
cd ..
