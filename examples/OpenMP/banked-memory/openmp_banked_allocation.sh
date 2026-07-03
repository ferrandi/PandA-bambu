#!/bin/bash
script_dir="$(dirname $(readlink -e $0))"

BATCH_ARGS=("--compiler=I386_CLANG13" "-lm" "-fopenmp" "--generate-interface=INFER" "--channels-type=MEM_ACC_NN" "--memory-allocation-policy=NO_BRAM" "--bus-pipelined" "--simulate")
OUT_SUFFIX="banked_allocation"

python3 $script_dir/../../../etc/scripts/mantis.py --tool=bambu \
   --args="--configuration-name=a0_b0_c1 -DBANK_ALLOCATION=1 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=a0_b0_c0 -DBANK_ALLOCATION=2 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=a2_b2_c2 -DBANK_ALLOCATION=3 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=a0/3_b0/3_c0/3 -DBANK_ALLOCATION=4 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=3000_byte -DBANK_ALLOCATION=5 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=4096_byte -DBANK_ALLOCATION=6 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=5000_byte -DBANK_ALLOCATION=7 ${BATCH_ARGS[*]}" \
   -lbanked_list \
   -o "out_${OUT_SUFFIX}" -b "$script_dir" \
   "$@"
exit $?
