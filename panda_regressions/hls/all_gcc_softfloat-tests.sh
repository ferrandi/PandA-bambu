#!/bin/bash

script_dir="$(dirname $(readlink -e $0))"

BATCH_ARGS=("--simulate" "--soft-float" "--max-ulp=0" "--experimental-setup=BAMBU-PERFORMANCE-MP")
OUT_SUFFIX="all_gcc_softfloat-tests"

python3 $script_dir/../../etc/scripts/test_panda.py --tool=bambu \
   --args="--configuration-name=GCC45-wp-NN --compiler=I386_GCC45 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=GCC46-wp-NN --compiler=I386_GCC46 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=GCC47-wp-NN --compiler=I386_GCC47 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=GCC48-wp-NN --compiler=I386_GCC48 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=GCC49-wp-NN --compiler=I386_GCC49 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=GCC5-wp-NN  --compiler=I386_GCC5  ${BATCH_ARGS[*]}" \
   --args="--configuration-name=GCC6-wp-NN  --compiler=I386_GCC6  ${BATCH_ARGS[*]}" \
   --args="--configuration-name=GCC7-wp-NN  --compiler=I386_GCC7  ${BATCH_ARGS[*]}" \
   --args="--configuration-name=GCC8-wp-NN  --compiler=I386_GCC8  ${BATCH_ARGS[*]}" \
   -lsoftfloat-tests_list \
   -o "out_${OUT_SUFFIX}" -b$script_dir \
   --name="${OUT_SUFFIX}" "$@"
