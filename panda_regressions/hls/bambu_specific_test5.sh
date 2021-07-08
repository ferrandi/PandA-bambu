#!/bin/bash

BATCH_ARGS=("--evaluation=TOTAL_CYCLES,CYCLES" "--expose-globals")
OUT_SUFFIX="bambu_specific_test5"

$(dirname $0)/../../etc/scripts/test_panda.py --tool=bambu  \
   --args="--configuration-name=CLANG4 --compiler=I386_CLANG4 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=CLANG11 --compiler=I386_CLANG11 ${BATCH_ARGS[*]}" \
   -lbambu_specific_test5_list \
   -o "output_${OUT_SUFFIX}" -b$(dirname $0) \
   --table="${OUT_SUFFIX}.tex" \
   --csv="${OUT_SUFFIX}.csv" \
   --name="${OUT_SUFFIX}" "$@"
exit $?