#!/bin/bash
script_dir="$(dirname "$(readlink -e "$0")")"
ggo_require_compiler=1

BATCH_ARGS=("--std=c++17" "-lm" "-fopenmp" "--compiler=I386_CLANG19" "--simulate" "--bambu-parameter=enable-task=1" "--generate-interface=INFER" "--max-sim-cycles=10000000")
OUT_SUFFIX="openmp_task_banked"

python3 "$script_dir/../../../etc/scripts/mantis.py" --tool=bambu  \
   --args="--configuration-name=NB --memory-allocation-policy=NO_BRAM ${BATCH_ARGS[*]}"\
   --args="--configuration-name=NB_P --memory-allocation-policy=NO_BRAM --bus-pipelined ${BATCH_ARGS[*]}"\
   --args="--configuration-name=GLSS --memory-allocation-policy=GLSS ${BATCH_ARGS[*]}"\
   --args="--configuration-name=TREE --priority-encoder=tree --memory-allocation-policy=NO_BRAM ${BATCH_ARGS[*]}"\
   --args="--configuration-name=GATES --priority-encoder=gates --memory-allocation-policy=NO_BRAM ${BATCH_ARGS[*]}"\
   -ltask_list_banked \
   -o "out_${OUT_SUFFIX}" -b "$script_dir" \
   "$@"
exit $?
