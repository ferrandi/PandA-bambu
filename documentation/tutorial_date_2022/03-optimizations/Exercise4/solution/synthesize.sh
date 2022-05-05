#!/bin/bash
abs_script=$(readlink -e $0)
dir_script=$(dirname $abs_script)
python3 $dir_script/../../test_panda.py --tool=bambu --bambu=bambu --spider=spider \
   --args="--configuration-name=pow_square " \
   --args="--configuration-name=mult_square -DMULT_SQUARE" \
   --args="--configuration-name=single_pow_square -DFP_SINGLE" \
   --args="--configuration-name=single_mult_square -DFP_SINGLE -DMULT_SQUARE" \
   -c=--simulate -c=-lm -c=--generate-tb=$dir_script/testbench.xml -c=--speculative-sdc-scheduling \
   -c=--top-fname=awesome_math \
   -b$dir_script -l$dir_script/list "$@"
