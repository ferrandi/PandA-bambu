#!/bin/bash
script_dir="$(dirname $(readlink -e $0))"
BATCH_ARGS=("-lm" "--simulate" "--experimental-setup=BAMBU" "--compiler=I386_CLANG13")
OUT_SUFFIX="bambu_specific_test"

python3 $script_dir/../../etc/scripts/mantis.py --tool=bambu  \
   --args="${BATCH_ARGS[*]}" \
   -lbambu_specific_test_list \
   -o "out_${OUT_SUFFIX}" -b$script_dir \
   "$@"
exit $?

