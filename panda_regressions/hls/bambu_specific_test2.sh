#!/bin/bash
script_dir="$(dirname $(readlink -e $0))"

BATCH_ARGS=("-lm" "--simulate" "--expose-globals")
OUT_SUFFIX="bambu_specific_test2"

python3 $script_dir/../../etc/scripts/test_panda.py --tool=bambu  \
   --args="--compiler=I386_GCC49 --configuration-name=GCC49_O0 -O0 ${BATCH_ARGS[*]}" \
   --args="--compiler=I386_GCC49 --configuration-name=GCC49_O1 -O1 ${BATCH_ARGS[*]}" \
   --args="--compiler=I386_GCC49 --configuration-name=GCC49_O2 -O2 ${BATCH_ARGS[*]}" \
   --args="--compiler=I386_GCC49 --configuration-name=GCC49_O3 -O3 ${BATCH_ARGS[*]}" \
   --args="--compiler=I386_CLANG13 --configuration-name=CLANG13_O0 -O0 ${BATCH_ARGS[*]}" \
   --args="--compiler=I386_CLANG13 --configuration-name=CLANG13_O1 -O1 ${BATCH_ARGS[*]}" \
   --args="--compiler=I386_CLANG13 --configuration-name=CLANG13_O2 -O2 ${BATCH_ARGS[*]}" \
   --args="--compiler=I386_CLANG13 --configuration-name=CLANG13_O3 -O3 ${BATCH_ARGS[*]}" \
   -lbambu_specific_test2_list \
   -o "out_${OUT_SUFFIX}" -b$script_dir \
   --name="${OUT_SUFFIX}" "$@"
exit $?
