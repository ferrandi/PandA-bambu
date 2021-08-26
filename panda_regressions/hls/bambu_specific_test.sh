#!/bin/bash

. $script_dir/generic_getopt.sh

BATCH_ARGS=("-lm" "--simulate" "--experimental-setup=BAMBU" "--compiler=I386_GCC49")
OUT_SUFFIX="bambu_specific_test"

$(dirname $0)/../../etc/scripts/test_panda.py --tool=bambu  \
   --args="${BATCH_ARGS[*]}" \
   -lbambu_specific_test_list \
   -o "output_${OUT_SUFFIX}" -b$(dirname $0) \
   --table="${REPORT_DIR}${OUT_SUFFIX}.tex" \
   --csv="${REPORT_DIR}${OUT_SUFFIX}.csv" \
   --name="${OUT_SUFFIX}" "$@"
exit $?

