#!/bin/bash
script_dir="$(dirname $(readlink -e $0))"
BATCH_ARGS=("--compiler=I386_CLANG16")
OUT_SUFFIX="pb_lattice"

python3 $script_dir/../etc/scripts/mantis.py --tool=bambu  \
   --args="${BATCH_ARGS[*]}"\
   -lpanda_bench_lattice_list \
   -o "out_${OUT_SUFFIX}" -b$script_dir \
   "$@"
