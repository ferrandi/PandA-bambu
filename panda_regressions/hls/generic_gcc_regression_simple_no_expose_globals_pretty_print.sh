#!/bin/bash

. ./generic_getopt.sh

BATCH_ARGS=("-lm" "--simulate" "--experimental-setup=BAMBU" "--do-not-expose-globals" "--pretty-print=output.c")
OUT_SUFFIX="${COMPILER}_gcc_regression_simple_no_expose_globals_pretty_preint"

$(dirname $0)/../../etc/scripts/test_panda.py --tool=bambu \
   --args="--configuration-name=${COMPILER}_O0 -O0 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=${COMPILER}_O1 -O1 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=${COMPILER}_O2 -O2 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=${COMPILER}_O3 -O3 ${BATCH_ARGS[*]}" \
   -ldiscrepancy_list \
   -o "output_${OUT_SUFFIX}" -b$(dirname $0) \
   --table="${OUT_SUFFIX}.tex" \
   --csv="${OUT_SUFFIX}.csv" \
   --name="${OUT_SUFFIX}" $ARGS
exit $?
