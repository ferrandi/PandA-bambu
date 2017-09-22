#!/bin/bash
abs_script=$(readlink -e $0)
dir_script=$(dirname $abs_script)
bambu $dir_script/module.c -O3 -lm --simulate --top-fname=formula-pow --generate-tb="a=3.0,b=4.0,c=5.0" --speculative-sdc-scheduling --libm-std-rounding --hls-div=none --soft-float
