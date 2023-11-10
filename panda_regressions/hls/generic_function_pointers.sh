#!/bin/bash
script_dir="$(dirname $(readlink -e $0))"
ggo_require_compiler=1
. $script_dir/../../panda_regressions/hls/generic_getopt.sh

BATCH_ARGS=("--simulate" "--experimental-setup=BAMBU")
configuration="$(echo $compiler | tr '[:upper:]' '[:lower:]')"
OUT_SUFFIX="${configuration}_function_pointers"
BENCHMARKS_ROOT="${script_dir}/../../examples/function_pointers"

python3 $script_dir/../../etc/scripts/test_panda.py --tool=bambu \
   --args="--configuration-name=base" \
   --args="--configuration-name=nn --channels-type=MEM_ACC_NN" \
   --args="--configuration-name=hl --bram-high-latency" \
   --args="--configuration-name=nn-hl --channels-type=MEM_ACC_NN --bram-high-latency" \
   -l${BENCHMARKS_ROOT}/function_pointers_list \
   -o "out${OUT_SUFFIX}" -b${BENCHMARKS_ROOT} \
   --name="${OUT_SUFFIX}" "$@"
