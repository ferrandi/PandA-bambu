#!/bin/bash
script_dir="$(dirname $(readlink -e $0))"
ggo_require_compiler=1

BATCH_ARGS=("-lm" "-fopenmp" "--simulate" "--generate-interface=INFER" "--memory-allocation-policy=NO_BRAM")
OUT_SUFFIX="openmp_axi"

python3 $script_dir/../../../etc/scripts/mantis.py --tool=bambu  \
   --args="--configuration-name=PIPELINED_BUS --bus-pipelined ${BATCH_ARGS[*]}"\
   --args="--configuration-name=SEQUENTIAL_BUS ${BATCH_ARGS[*]}"\
   -laxi_list \
   -o "out_${OUT_SUFFIX}" -b "$script_dir" \
   "$@"
exit $?
