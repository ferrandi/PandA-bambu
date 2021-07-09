#!/bin/bash

. ./generic_getopt.sh

BATCH_ARGS=("--soft-float" "--simulate" "-lm" "--reset-type=sync" "-DNO_MAIN" "--max-ulp=0" "--experimental-setup=BAMBU")
OUT_SUFFIX="${COMPILER}_libm_tests"

$(dirname $0)/../../etc/scripts/test_panda.py --tool=bambu \
   --args="--configuration-name=${COMPILER}-soft-float -O0 -DFAITHFULLY_ROUNDED ${BATCH_ARGS[*]}" \
   --args="--configuration-name=${COMPILER}-soft-float-libm --libm-std-rounding ${BATCH_ARGS[*]}" \
   -llibm-tests_list \
   -o "output_${OUT_SUFFIX}" -b$(dirname $0) \
   --table="${OUT_SUFFIX}.tex" \
   --csv="${OUT_SUFFIX}.csv" \
   --name="${OUT_SUFFIX}" $ARGS
exit $?
