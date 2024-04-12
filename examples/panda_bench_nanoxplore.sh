#!/bin/bash
script_dir="$(dirname $(readlink -e $0))"
BATCH_ARGS=("--clock-period=20" "--evaluation" "--compiler=I386_GCC49")
OUT_SUFFIX="pb_nanoxplore"

python3 $script_dir/../etc/scripts/test_panda.py --tool=bambu  \
   --args="${BATCH_ARGS[*]}"\
   -lpanda_bench_nanoxplore_list \
   -o "out${OUT_SUFFIX}" -b$script_dir \
   --name="${OUT_SUFFIX}" "$@"
