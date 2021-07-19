#!/bin/bash

script_dir="$(dirname $(readlink -e $0))"
. $script_dir/generic_getopt.sh

BATCH_ARGS=("--soft-float" "--simulate" "-lm" "--reset-type=sync" "--experimental-setup=BAMBU")
OUT_SUFFIX="${COMPILER}_libm_tests"

$script_dir/../../etc/scripts/test_panda.py --tool=bambu \
   --args="--configuration-name=${COMPILER}-soft-float -O0 -DFAITHFULLY_ROUNDED ${BATCH_ARGS[*]}" \
   --args="--configuration-name=${COMPILER}-soft-float-libm --libm-std-rounding ${BATCH_ARGS[*]}" \
   -llibm-tests_list \
   -o "output_${OUT_SUFFIX}" -b$script_dir \
   --table="${OUT_SUFFIX}.tex" \
   --csv="${OUT_SUFFIX}.csv" \
   --name="${OUT_SUFFIX}" $ARGS
exit $?
