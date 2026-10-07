#!/bin/bash
script_dir="$(dirname $(readlink -e $0))"

BATCH_ARGS=("--compiler=I386_CLANG13" "-lm" "-fopenmp" "--generate-interface=INFER" "--channels-type=MEM_ACC_NN" "--memory-allocation-policy=NO_BRAM" "--bus-pipelined" "--simulate")
OUT_SUFFIX="banked_allocation"

python3 $script_dir/../../../etc/scripts/mantis.py --tool=bambu \
   --args="--configuration-name=all_banks23 -DBANK_ALLOCATION=1 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=a03_b0_c03 -DBANK_ALLOCATION=2 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=a2_b0_c0 -DBANK_ALLOCATION=3 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=a0_b2_c3 -DBANK_ALLOCATION=4 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=all_3bank_allocations -DBANK_ALLOCATION=5 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=a_default_bc_5bank -DBANK_ALLOCATION=6 -DBANK_NUMBER=8 ${BATCH_ARGS[*]}" \
   -lbanked_list_allocation \
   -o "out_${OUT_SUFFIX}" -b "$script_dir" \
   "$@"
exit $?
