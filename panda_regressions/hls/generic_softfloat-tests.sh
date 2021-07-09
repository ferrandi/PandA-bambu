#!/bin/bash

. ./generic_getopt.sh

BATCH_ARGS=("--soft-float" "--max-ulp=0" "--experimental-setup=BAMBU-PERFORMANCE-MP")
OUT_SUFFIX="${COMPILER}_softfloat_tests"

$(dirname $0)/../../etc/scripts/test_panda.py --tool=bambu \
   --args="--configuration-name=${COMPILER} ${BATCH_ARGS[*]}"\
   -lsoftfloat-tests_list \
   -o "output_${OUT_SUFFIX}" -b$(dirname $0) \
   --table="${OUT_SUFFIX}.tex" \
   --csv="${OUT_SUFFIX}.csv" \
   --name="${OUT_SUFFIX}" $ARGS
exit $?
