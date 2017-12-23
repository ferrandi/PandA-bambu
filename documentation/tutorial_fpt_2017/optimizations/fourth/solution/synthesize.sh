#!/bin/bash
abs_script=$(readlink -e $0)
dir_script=$(dirname $abs_script)
rm -rf output
$dir_script/test_panda.py --tool=bambu --bambu=bambu --spider=spider \
   --args="--configuration-name=pow --top-fname=formula_pow" \
   --args="--configuration-name=mult --top-fname=formula_mult " \
   --args="--configuration-name=double_pow --top-fname=double_formula_pow" \
   --args="--configuration-name=double_mult --top-fname=double_formula_mult " \
   -c=--simulate -c=-lm -c=--generate-tb=$dir_script/testbench.xml -c=--speculative-sdc-scheduling -b$dir_script -l$dir_script/list $@
