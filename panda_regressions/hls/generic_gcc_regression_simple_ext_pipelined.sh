#!/bin/bash

. ./generic_getopt.sh

BATCH_ARGS=("-lm" "--simulate" "--experimental-setup=BAMBU" "--expose-globals" "--channels-type=MEM_ACC_NN" "--memory-allocation-policy=EXT_PIPELINED_BRAM")
OUT_SUFFIX="${COMPILER}_gcc_regression_simple_ext_pipelined"

$(dirname $0)/../../etc/scripts/test_panda.py gcc_regression_simple --tool=bambu \
   --args="--configuration-name=${COMPILER}_O0 -O0 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=${COMPILER}_O1 -O1 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=${COMPILER}_O2 -O2 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=${COMPILER}_O3 -O3 ${BATCH_ARGS[*]}" \
   -o "output_${OUT_SUFFIX}" -b$(dirname $0) \
   --table="${OUT_SUFFIX}.tex" \
   --csv="${OUT_SUFFIX}.csv" \
   --name="${OUT_SUFFIX}" $ARGS
exit $?
