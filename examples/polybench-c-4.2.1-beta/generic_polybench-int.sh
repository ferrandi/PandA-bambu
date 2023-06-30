#!/bin/bash
script_dir="$(dirname $(readlink -e $0))"
ggo_require_compiler=1
ggo_require_device=1
ggo_require_period=1
. $script_dir/../../panda_regressions/hls/generic_getopt.sh

BATCH_ARGS=(" --generate-interface=INFER" "--simulate" "-DPOLYBENCH_USE_RESTRICT" "-DPOLYBENCH_STACK_ARRAYS" "-DMINI_DATASET" "--panda-parameter=simple-benchmark-name=1")
configuration="${device}_$(printf "%04.1f" $period)_$(echo $compiler | tr '[:upper:]' '[:lower:]')"
OUT_SUFFIX="${configuration}_Polybench-int"

python3 $script_dir/../../etc/scripts/test_panda.py --tool=bambu  \
   --args="--configuration-name=${configuration}-int0 -DDATA_TYPE_IS_INT ${BATCH_ARGS[*]}"\
   --args="--configuration-name=${configuration}-int1-no-unroll -DDATA_TYPE_IS_INT --channels-number=1 -fno-unroll-loops ${BATCH_ARGS[*]}"\
   --args="--configuration-name=${configuration}-int2-no-lp -DDATA_TYPE_IS_INT --panda-parameter=LP-BB-list=0 ${BATCH_ARGS[*]}"\
   --args="--configuration-name=${configuration}-int3-no-unroll-no-lp -DDATA_TYPE_IS_INT --channels-number=1 -fno-unroll-loops --panda-parameter=LP-BB-list=0 ${BATCH_ARGS[*]}"\
   -lpolybench_list-int \
   -o "out${OUT_SUFFIX}" -b$script_dir \
   --name="${OUT_SUFFIX}" "$@"

