#!/bin/bash

OUT_SUFFIX="pb_hw"

$(dirname $0)/../etc/scripts/test_panda.py --tool=bambu \
   -lpanda_bench_hw_list \
   -o "out${OUT_SUFFIX}" -b$(dirname $0) \
   --name="${OUT_SUFFIX}" "$@"
exit $?