#!/bin/bash
script_dir="$(dirname $(readlink -e $0))"
ggo_require_compiler=1
ggo_require_device=1
ggo_require_period=1
. $script_dir/../../panda_regressions/hls/generic_getopt.sh

BATCH_ARGS=(" --generate-interface=INFER" "--simulate" "-DPOLYBENCH_USE_RESTRICT" "-DPOLYBENCH_STACK_ARRAYS" "-DMINI_DATASET" "-p=__float_mul,__float_add,__float_sub,sqrtf,expf" "-lm" "-s" "--panda-parameter=simple-benchmark-name=1")
configuration="${device}_$(printf "%04.1f" $period)_$(echo $compiler | tr '[:upper:]' '[:lower:]')"
OUT_SUFFIX="${configuration}_Polybench"

python3 $script_dir/../../etc/scripts/test_panda.py --tool=bambu  \
   --args="--configuration-name=${configuration}-float0 -DDATA_TYPE_IS_FLOAT ${BATCH_ARGS[*]}"\
   --args="--configuration-name=${configuration}-float1-dfp -DDATA_TYPE_IS_FLOAT --disable-function-proxy ${BATCH_ARGS[*]}"\
   --args="--configuration-name=${configuration}-float2-no-unroll -DDATA_TYPE_IS_FLOAT --channels-number=1 -fno-unroll-loops ${BATCH_ARGS[*]}"\
   --args="--configuration-name=${configuration}-float3-no-unroll-dfp -DDATA_TYPE_IS_FLOAT --channels-number=1 -fno-unroll-loops --disable-function-proxy ${BATCH_ARGS[*]}"\
   --args="--configuration-name=${configuration}-float4-no-lp -DDATA_TYPE_IS_FLOAT --panda-parameter=LP-BB-list=0 ${BATCH_ARGS[*]}"\
   --args="--configuration-name=${configuration}-float5-no-lp-dfp -DDATA_TYPE_IS_FLOAT --panda-parameter=LP-BB-list=0 ${BATCH_ARGS[*]}"\
   --args="--configuration-name=${configuration}-float6-no-unroll-no-lp -DDATA_TYPE_IS_FLOAT --channels-number=1 -fno-unroll-loops --panda-parameter=LP-BB-list=0 ${BATCH_ARGS[*]}"\
   --args="--configuration-name=${configuration}-float7-no-unroll-no-lp-dfp -DDATA_TYPE_IS_FLOAT --channels-number=1 -fno-unroll-loops --panda-parameter=LP-BB-list=0 --disable-function-proxy ${BATCH_ARGS[*]}"\
   -lpolybench_list \
   -o "out${OUT_SUFFIX}" -b$script_dir \
   --name="${OUT_SUFFIX}" "$@"
   

