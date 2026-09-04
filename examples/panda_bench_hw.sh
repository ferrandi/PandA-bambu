#!/bin/bash
script_dir="$(dirname $(readlink -e $0))"
OUT_SUFFIX="pb_hw"

python3 $script_dir/../etc/scripts/mantis.py --tool=bambu \
   -c=--connect-iob \
   -lpanda_bench_hw_list \
   -o "out_${OUT_SUFFIX}" -b$script_dir \
   "$@"
