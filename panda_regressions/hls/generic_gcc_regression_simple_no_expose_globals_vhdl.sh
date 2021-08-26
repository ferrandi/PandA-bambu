#!/bin/bash

script_dir="$(dirname $(readlink -e $0))"
ggo_require_compiler=1
. $script_dir/generic_getopt.sh

BATCH_ARGS=("-lm" "--simulate" "--experimental-setup=BAMBU" "--do-not-expose-globals" "-wH")
OUT_SUFFIX="${COMPILER}_grs_no_expose_globals_vhdl"

$script_dir/../../etc/scripts/test_panda.py gcc_regression_simple --tool=bambu \
   --args="--configuration-name=${COMPILER}_O0 -O0 ${BATCH_ARGS[*]}" \
   -o "output_${OUT_SUFFIX}" -b$script_dir \
   --table="${REPORT_DIR}${OUT_SUFFIX}.tex" \
   --csv="${REPORT_DIR}${OUT_SUFFIX}.csv" \
   --name="${OUT_SUFFIX}" $ARGS
exit $?
