#!/bin/bash

BATCH_ARGS=("--no-iob" "--evaluation")
OUT_SUFFIX="pb_xise"

$(dirname $0)/../etc/scripts/test_panda.py --tool=bambu  \
   --args="${BATCH_ARGS[*]}"\
   -lpanda_bench_xilinx_ise_list \
   -o "out${OUT_SUFFIX}" -b$(dirname $0) \
   --name="${OUT_SUFFIX}" "$@"
exit $?