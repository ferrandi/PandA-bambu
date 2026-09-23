#!/bin/bash
script_dir="$(dirname $(readlink -e $0))"

BATCH_ARGS=("-lm" "-fopenmp" "--generate-interface=INFER" "--channels-type=MEM_ACC_NN" "--simulate")
OUT_SUFFIX="banked"

python3 $script_dir/../../../etc/scripts/mantis.py --tool=bambu \
   --args="--configuration-name=banked_2b  -DBANK_NUMBER=2 --memory-allocation-policy=GLSS ${BATCH_ARGS[*]}" \
   --args="--configuration-name=banked_4b  -DBANK_NUMBER=4 --memory-allocation-policy=GLSS ${BATCH_ARGS[*]}" \
   --args="--configuration-name=banked_8b  -DBANK_NUMBER=8 --memory-allocation-policy=GLSS ${BATCH_ARGS[*]}" \
   --args="--configuration-name=banked_2b_simple_memory -DBANK_NUMBER=2 --bus-pipelined --memory-allocation-policy=NO_BRAM --tb-queue-size=1 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=banked_4b_simple_memory -DBANK_NUMBER=4 --bus-pipelined --memory-allocation-policy=NO_BRAM --tb-queue-size=1 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=banked_8b_simple_memory -DBANK_NUMBER=8 --bus-pipelined --memory-allocation-policy=NO_BRAM --tb-queue-size=1 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=banked_axi --bus-pipelined --memory-allocation-policy=NO_BRAM -DUSE_AXI=1 ${BATCH_ARGS[*]}" \
   -lbanked_list \
   -o "out_${OUT_SUFFIX}" -b "$script_dir" \
   "$@"
exit $?
