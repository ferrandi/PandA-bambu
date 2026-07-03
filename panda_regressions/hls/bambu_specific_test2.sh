#!/bin/bash
script_dir="$(dirname $(readlink -e $0))"

BATCH_ARGS=("-lm" "--simulate" "--expose-globals")
OUT_SUFFIX="bambu_specific_test2"

python3 $script_dir/../../etc/scripts/mantis.py --tool=bambu  \
   --args="--configuration-name=CLANG13_O0 --compiler=I386_CLANG13 -O0 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=CLANG13_O1 --compiler=I386_CLANG13 -O1 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=CLANG13_O2 --compiler=I386_CLANG13 -O2 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=CLANG13_O3 --compiler=I386_CLANG13 -O3 ${BATCH_ARGS[*]}" \
   -lbambu_specific_test2_list \
   -o "out_${OUT_SUFFIX}" -b "$script_dir/bambu_specific_test2" \
   "$@"
exit $?
