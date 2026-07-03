#!/bin/bash

script_dir="$(dirname $(readlink -e $0))"
ggo_require_compiler=1
. $script_dir/generic_getopt.sh

BATCH_ARGS=("--simulate" "--reset-type=sync" "--experimental-setup=BAMBU")
OUT_SUFFIX="${compiler}_libm_tests"

python3 $script_dir/../../etc/scripts/mantis.py --tool=bambu \
   --args="--configuration-name=${compiler}-libm-hls -lm --max-ulp=1 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=${compiler}-libm-newlib -lm_newlib --max-ulp=0 ${BATCH_ARGS[*]}" \
   -llibm-tests_list \
   -o "out_${OUT_SUFFIX}" -b "$script_dir/libm-tests" \
   $ARGS

#   --args="--configuration-name=${compiler}-libm-musl -lm_musl --max-ulp=1 ${BATCH_ARGS[*]}" \
