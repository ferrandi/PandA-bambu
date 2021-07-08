#!/bin/bash

BATCH_ARGS=("--no-iob" "--simulate")
OUT_SUFFIX="pb_sim"

$(dirname $0)/../etc/scripts/test_panda.py --tool=bambu  \
   --args="${BATCH_ARGS[*]}"\
   -lpanda_bench_sim_list \
   -o "out${OUT_SUFFIX}" -b$(dirname $0) \
   --table="${OUT_SUFFIX}.tex" \
   --csv="${OUT_SUFFIX}.csv" \
   --name="${OUT_SUFFIX}" "$@"
exit $?