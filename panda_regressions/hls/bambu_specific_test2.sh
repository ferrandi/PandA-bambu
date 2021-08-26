#!/bin/bash

. $script_dir/generic_getopt.sh

BATCH_ARGS=("-lm" "--evaluation=TOTAL_CYCLES,CYCLES" "--expose-globals" "--compiler=I386_GCC49")
OUT_SUFFIX="bambu_specific_test2"

$(dirname $0)/../../etc/scripts/test_panda.py --tool=bambu  \
   --args="--configuration-name=O0 -O0 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=O1 -O1 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=O2 -O2 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=O3 -O3 ${BATCH_ARGS[*]}" \
   -lbambu_specific_test2_list \
   -o "output_${OUT_SUFFIX}" -b$(dirname $0) \
   --table="${REPORT_DIR}${OUT_SUFFIX}.tex" \
   --csv="${REPORT_DIR}${OUT_SUFFIX}.csv" \
   --name="${OUT_SUFFIX}" "$@"
exit $?
