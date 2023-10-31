#!/bin/bash

script_dir="$(dirname $(readlink -e $0))"
ggo_require_compiler=1
. $script_dir/generic_getopt.sh

BATCH_ARGS=("-lm" "--simulate" "--experimental-setup=BAMBU" "--pretty-print=output.c" "--std=gnu89")
OUT_SUFFIX="${compiler}_pp"

python3 $script_dir/../../etc/scripts/test_panda.py --tool=bambu \
   --args="--configuration-name=${compiler}_O0 -O0 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=${compiler}_O1 -O1 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=${compiler}_O2 -O2 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=${compiler}_O3 -O3 ${BATCH_ARGS[*]}" \
   -ldiscrepancy_list \
   -o "out_${OUT_SUFFIX}" -b$script_dir \
   --name="${OUT_SUFFIX}" $ARGS
