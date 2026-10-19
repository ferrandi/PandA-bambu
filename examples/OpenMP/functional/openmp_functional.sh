#!/bin/bash
script_dir="$(dirname $(readlink -e $0))"
ggo_require_compiler=1

BATCH_ARGS=("-lm" "-fopenmp" "--simulate" "--memory-allocation-policy=GLSS")
OUT_SUFFIX="openmp_functional"

python3 $script_dir/../../../etc/scripts/mantis.py --tool=bambu  \
   --args="--configuration-name=GLSS ${BATCH_ARGS[*]}"\
   --args="--configuration-name=latency_3 --bram-high-latency=3 ${BATCH_ARGS[*]}"\
   --args="--configuration-name=latency_4 --bram-high-latency=4 ${BATCH_ARGS[*]}"\
   -lfunctional_list \
   -o "out_${OUT_SUFFIX}" -b "$script_dir" \
   "$@"
exit $?
