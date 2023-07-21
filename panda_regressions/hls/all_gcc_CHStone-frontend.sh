#!/bin/bash

script_dir="$(dirname $(readlink -e $0))"

BATCH_ARGS=("--simulate" "-O0" "-fwhole-program" "--clock-period=15" "-D'printf(fmt, ...)='" "--channels-type=MEM_ACC_NN" "--experimental-setup=BAMBU")
OUT_SUFFIX="all_gcc_CHStone-frontend"

python3 $script_dir/../../etc/scripts/test_panda.py --tool=bambu \
   --args="--configuration-name=GCC45-O0-wp-NN --compiler=I386_GCC45 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=GCC46-O0-wp-NN --compiler=I386_GCC46 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=GCC47-O0-wp-NN --compiler=I386_GCC47 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=GCC48-O0-wp-NN --compiler=I386_GCC48 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=GCC49-O0-wp-NN --compiler=I386_GCC49 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=GCC5-O0-wp-NN  --compiler=I386_GCC5  ${BATCH_ARGS[*]}" \
   --args="--configuration-name=GCC6-O0-wp-NN  --compiler=I386_GCC6  ${BATCH_ARGS[*]}" \
   --args="--configuration-name=GCC7-O0-wp-NN  --compiler=I386_GCC7  ${BATCH_ARGS[*]}" \
   --args="--configuration-name=GCC8-O0-wp-NN  --compiler=I386_GCC8  ${BATCH_ARGS[*]}" \
   -l../chstone_list \
   -o "out_${OUT_SUFFIX}" -b${script_dir}/../../examples/CHStone/CHStone \
   --name="${OUT_SUFFIX}" "$@"
