#!/bin/bash

. ./generic_getopt.sh

BATCH_ARGS=("-lm" "--simulate" "--experimental-setup=BAMBU" "--expose-globals" "--discrepancy")
OUT_SUFFIX="${COMPILER}_gcc_regression_simple_discrepancy"

$(dirname $0)/../../etc/scripts/test_panda.py --tool=bambu  \
   --args="--configuration-name=${COMPILER}_O2 -O2 ${BATCH_ARGS[*]}" \
   -ldiscrepancy_list \
   -o "output_${OUT_SUFFIX}" -b$(dirname $0) \
   --table="${OUT_SUFFIX}.tex" \
   --csv="${OUT_SUFFIX}.csv" \
   --name="${OUT_SUFFIX}" $ARGS
exit $?
