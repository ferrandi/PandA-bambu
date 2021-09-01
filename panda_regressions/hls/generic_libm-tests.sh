#!/bin/bash

script_dir="$(dirname $(readlink -e $0))"
ggo_require_compiler=1
. $script_dir/generic_getopt.sh

BATCH_ARGS=("--soft-float" "--simulate" "-lm" "--reset-type=sync" "--experimental-setup=BAMBU")
OUT_SUFFIX="${COMPILER}_libm_tests"

$script_dir/../../etc/scripts/test_panda.py --tool=bambu \
   --args="--configuration-name=${COMPILER}-soft-float -O0 -DFAITHFULLY_ROUNDED ${BATCH_ARGS[*]}" \
   --args="--configuration-name=${COMPILER}-soft-float-std --libm-std-rounding ${BATCH_ARGS[*]}" \
   -llibm-tests_list \
   -o "output_${OUT_SUFFIX}" -b$script_dir \
   --name="${OUT_SUFFIX}" $ARGS
