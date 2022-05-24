#!/bin/bash
abs_script=$(readlink -e $0)
dir_script=$(dirname $abs_script)
bambu $dir_script/module.c --top-fname=awesome_math \
   -O3 -lm --speculative-sdc-scheduling --libm-std-rounding --soft-float \
   --simulate --generate-tb="a=3.0,b=4.0,c=5.0" \
   "$@" |& tee log.txt
