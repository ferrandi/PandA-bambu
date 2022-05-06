#!/bin/bash
script_dir="$(dirname $(readlink -e $0))"
BATCH_ARGS=("--no-iob" "--evaluation")
OUT_SUFFIX="pb_lattice"

python3 $script_dir/../etc/scripts/test_panda.py --tool=bambu  \
   --args="${BATCH_ARGS[*]}"\
   -lpanda_bench_lattice_list \
   -o "out${OUT_SUFFIX}" -b$script_dir \
   --name="${OUT_SUFFIX}" "$@"
