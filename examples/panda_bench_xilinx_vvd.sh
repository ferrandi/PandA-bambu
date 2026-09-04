#!/bin/bash
script_dir="$(dirname $(readlink -e $0))"
OUT_SUFFIX="pb_xvvd"

python3 $script_dir/../etc/scripts/mantis.py --tool=bambu  \
   -lpanda_bench_xilinx_vvd_list \
   -o "out_${OUT_SUFFIX}" -b$script_dir \
   "$@"
