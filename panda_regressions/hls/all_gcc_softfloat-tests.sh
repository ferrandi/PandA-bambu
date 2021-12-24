#!/bin/bash

script_dir="$(dirname $(readlink -e $0))"

BATCH_ARGS=("--simulate" "--soft-float" "--max-ulp=0" "--experimental-setup=BAMBU-PERFORMANCE-MP")
OUT_SUFFIX="all_gcc_softfloat-tests"

$script_dir/../../etc/scripts/test_panda.py --tool=bambu \
   --args="--configuration-name=GCC45-O0-wp-NN --compiler=I386_GCC45 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=GCC46-O0-wp-NN --compiler=I386_GCC46 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=GCC47-O0-wp-NN --compiler=I386_GCC47 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=GCC48-O0-wp-NN --compiler=I386_GCC48 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=GCC49-O0-wp-NN --compiler=I386_GCC49 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=GCC5-O0-wp-NN  --compiler=I386_GCC5  ${BATCH_ARGS[*]}" \
   --args="--configuration-name=GCC6-O0-wp-NN  --compiler=I386_GCC6  ${BATCH_ARGS[*]}" \
   --args="--configuration-name=GCC7-O0-wp-NN  --compiler=I386_GCC7  ${BATCH_ARGS[*]}" \
   --args="--configuration-name=GCC8-O0-wp-NN  --compiler=I386_GCC8  ${BATCH_ARGS[*]}" \
   -lsoftfloat-tests_list \
   -o "output_${OUT_SUFFIX}" -b$script_dir \
   --name="${OUT_SUFFIX}" "$@"
