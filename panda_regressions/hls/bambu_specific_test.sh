#!/bin/bash
script_dir="$(dirname $(readlink -e $0))"
BATCH_ARGS=("-lm" "--simulate" "--experimental-setup=BAMBU" "--compiler=I386_GCC49")
OUT_SUFFIX="bambu_specific_test"

$script_dir/../../etc/scripts/test_panda.py --tool=bambu  \
   --args="${BATCH_ARGS[*]}" \
   -lbambu_specific_test_list \
   -o "output_${OUT_SUFFIX}" -b$script_dir \
   --name="${OUT_SUFFIX}" "$@"
exit $?

