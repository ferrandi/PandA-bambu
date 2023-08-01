#!/bin/bash

script_dir="$(dirname $(readlink -e $0))"

BATCH_ARGS=("--simulate" "--soft-float" "--max-ulp=0" "--experimental-setup=BAMBU-PERFORMANCE-MP")
OUT_SUFFIX="multi_softfloat-tests"

python3 $script_dir/../../etc/scripts/test_panda.py --tool=bambu \
   --args="--configuration-name=GCC49   --compiler=I386_GCC49   ${BATCH_ARGS[*]}" \
   --args="--configuration-name=CLANG4  --compiler=I386_CLANG4  ${BATCH_ARGS[*]}" \
   --args="--configuration-name=CLANG6  --compiler=I386_CLANG6  ${BATCH_ARGS[*]}" \
   --args="--configuration-name=CLANG7  --compiler=I386_CLANG7  ${BATCH_ARGS[*]}" \
   --args="--configuration-name=CLANG11 --compiler=I386_CLANG11 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=CLANG13 --compiler=I386_CLANG13 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=CLANG16 --compiler=I386_CLANG16 ${BATCH_ARGS[*]}" \
   -lsoftfloat-tests_list \
   -o "out_${OUT_SUFFIX}" -b$script_dir \
   --name="${OUT_SUFFIX}" "$@"
