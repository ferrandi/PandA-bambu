#!/bin/bash

. $script_dir/generic_getopt.sh

BATCH_ARGS=("--no-iob" "--evaluation")
OUT_SUFFIX="pb_xise"

$(dirname $0)/../etc/scripts/test_panda.py --tool=bambu  \
   --args="${BATCH_ARGS[*]}"\
   -lpanda_bench_xilinx_ise_list \
   -o "out${OUT_SUFFIX}" -b$(dirname $0) \
   --table="${REPORT_DIR}${OUT_SUFFIX}.tex" \
   --csv="${REPORT_DIR}${OUT_SUFFIX}.csv" \
   --name="${OUT_SUFFIX}" "$@"
exit $?