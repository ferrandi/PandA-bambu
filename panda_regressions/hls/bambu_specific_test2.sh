#!/bin/bash
script_dir="$(dirname $(readlink -e $0))"

BATCH_ARGS=("-lm" "--simulate" "--expose-globals" "--compiler=I386_GCC49")
OUT_SUFFIX="bambu_specific_test2"

$script_dir/../../etc/scripts/test_panda.py --tool=bambu  \
   --args="--configuration-name=O0 -O0 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=O1 -O1 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=O2 -O2 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=O3 -O3 ${BATCH_ARGS[*]}" \
   -lbambu_specific_test2_list \
   -o "out_${OUT_SUFFIX}" -b$script_dir \
   --name="${OUT_SUFFIX}" "$@"
exit $?
