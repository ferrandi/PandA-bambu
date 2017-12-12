#!/bin/bash
abs_script=$(readlink -e $0)
dir_script=$(dirname $abs_script)
mkdir -p hls1
cd hls1
bambu $dir_script/mips.c --base-address=1024 --memory-allocation-policy=LSS --clock-period=15 --simulate -v3
cd ..
