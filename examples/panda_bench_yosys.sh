#!/bin/bash
script_dir="$(dirname $(readlink -e $0))"
BATCH_ARGS=("--no-iob" "--clock-period=5" "--compiler=I386_CLANG13" "--evaluation")
OUT_SUFFIX="pb_yosys"

python3 $script_dir/../etc/scripts/test_panda.py --tool=bambu \
   --args="--configuration-name=nangate45 --device-name=nangate45 BENCHMARK_ENV=DIE_AREA='0 0 2020 1980',CORE_AREA='10 12 2010 1971' ${BATCH_ARGS[*]}" \
   --args="--configuration-name=asap7-BC  --device-name=asap7-BC  BENCHMARK_ENV=DIE_AREA='0 0 190 190',CORE_AREA='5.08 5.08 180 180' ${BATCH_ARGS[*]}" \
   -lpanda_bench_yosys_list \
   -o "out${OUT_SUFFIX}" -b$script_dir \
   --name="${OUT_SUFFIX}" "$@"
