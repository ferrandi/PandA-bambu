#!/bin/bash
script_dir="$(dirname $(readlink -e $0))"
ggo_require_compiler=1

BATCH_ARGS=("-lm" "-fopenmp" "--simulate" "--memory-allocation-policy=GLSS")
OUT_SUFFIX="openmp_gemmm"

python3 $script_dir/../../../etc/scripts/mantis.py --tool=bambu  \
   --args="--configuration-name=GLSS ${BATCH_ARGS[*]}"\
   -lgemm_list \
   -o "out_${OUT_SUFFIX}" -b "$script_dir" \
   "$@"
exit $?
