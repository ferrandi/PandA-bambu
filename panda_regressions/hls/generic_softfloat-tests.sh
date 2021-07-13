#!/bin/bash

script_dir="$(dirname $(readlink -e $0))"
. $script_dir/generic_getopt.sh

BATCH_ARGS=("--soft-float" "--max-ulp=0" "--experimental-setup=BAMBU-PERFORMANCE-MP")
OUT_SUFFIX="${COMPILER}_softfloat-tests"

$script_dir/../../etc/scripts/test_panda.py --tool=bambu \
   --args="--configuration-name=${COMPILER} ${BATCH_ARGS[*]}"\
   -lsoftfloat-tests_list \
   -o "output_${OUT_SUFFIX}" -b$script_dir \
   --table="${OUT_SUFFIX}.tex" \
   --csv="${OUT_SUFFIX}.csv" \
   --name="${OUT_SUFFIX}" $ARGS
exit $?
