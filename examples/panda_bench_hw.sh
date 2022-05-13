#!/bin/bash
script_dir="$(dirname $(readlink -e $0))"
OUT_SUFFIX="pb_hw"

python3 $script_dir/../etc/scripts/test_panda.py --tool=bambu \
   -lpanda_bench_hw_list \
   -o "out${OUT_SUFFIX}" -b$script_dir \
   --name="${OUT_SUFFIX}" "$@"
