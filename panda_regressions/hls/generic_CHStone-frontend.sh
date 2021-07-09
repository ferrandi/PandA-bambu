#!/bin/bash

. ./generic_getopt.sh

BATCH_ARGS=("--simulate" "-fwhole-program" "--clock-period=15" "-D'printf(fmt, ...)='" "--channels-type=MEM_ACC_NN" "--experimental-setup=BAMBU")
OUT_SUFFIX="${COMPILER}_CHStone-frontend"

$(dirname $0)/../../etc/scripts/test_panda.py --tool=bambu \
   --args="--configuration-name=GCC49-O0-wp-NN -O0 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=GCC49-O1-wp-NN -O1 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=GCC49-O2-wp-NN -O2 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=GCC49-O3-wp-NN -O3 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=GCC49-O4-wp-NN -O4 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=GCC49-O5-wp-NN -O5 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=GCC49-Os-wp-NN -Os ${BATCH_ARGS[*]}" \
   -lCHStone_list \
   -o "output_${OUT_SUFFIX}" -b$(dirname $0) \
   --table="${OUT_SUFFIX}.tex" \
   --csv="${OUT_SUFFIX}.csv" \
   --name="${OUT_SUFFIX}" $ARGS
exit $?
