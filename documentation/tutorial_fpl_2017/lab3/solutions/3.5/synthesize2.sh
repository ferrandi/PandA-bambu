#!/bin/bash
abs_script=$(readlink -e $0)
dir_script=$(dirname $abs_script)
mkdir -p hls2
cd hls2
bambu $dir_script/mips.c --aligned-access -fwhole-program --clock-period=15 --simulate -v3
cd ..
