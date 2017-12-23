#!/bin/bash
abs_script=$(readlink -e $0)
dir_script=$(dirname $abs_script)
mkdir -p hls2
cd hls2
bambu $dir_script/mips.c --distram-threshold=1024 -fwhole-program --clock-period=15 --simulate -v3
cd ..
