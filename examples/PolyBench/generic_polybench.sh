#!/bin/bash
script_dir="$(dirname $(readlink -e $0))"
ggo_require_compiler=1
ggo_require_device=1
ggo_require_period=1
. ${script_dir}/../../panda_regressions/hls/generic_getopt.sh

BATCH_ARGS=(" --generate-interface=INFER" "--simulate" "-DPOLYBENCH_USE_RESTRICT" "-DPOLYBENCH_STACK_ARRAYS" "-DMINI_DATASET" "-p=__float_mul,__float_add,__float_sub,expf" "-lm" "-s" "--panda-parameter=simple-benchmark-name=1")
configuration="${device}_$(printf "%04.1f" $period)_$(echo $compiler | tr '[:upper:]' '[:lower:]')"
OUT_SUFFIX="${configuration}_Polybench"
BENCHMARKS_ROOT="${script_dir}/PolyBenchC"

python3 ${script_dir}/../../etc/scripts/test_panda.py --tool=bambu  \
   --args="--configuration-name=${configuration}-float0-c -DDATA_TYPE_IS_FLOAT -C='*__float' ${BATCH_ARGS[*]}"\
   --args="--configuration-name=${configuration}-float1-u -DDATA_TYPE_IS_FLOAT -C='*=u' ${BATCH_ARGS[*]}"\
   --args="--configuration-name=${configuration}-float2-no-unroll-c -DDATA_TYPE_IS_FLOAT --channels-number=1 -fno-unroll-loops -C='*__float' ${BATCH_ARGS[*]}"\
   --args="--configuration-name=${configuration}-float3-no-unroll-u -DDATA_TYPE_IS_FLOAT --channels-number=1 -fno-unroll-loops  -C='*=u' ${BATCH_ARGS[*]}"\
   --args="--configuration-name=${configuration}-float4-no-lp-c -DDATA_TYPE_IS_FLOAT --panda-parameter=LP-BB-list=0 -C='*__float' ${BATCH_ARGS[*]}"\
   --args="--configuration-name=${configuration}-float5-no-lp-u -DDATA_TYPE_IS_FLOAT --panda-parameter=LP-BB-list=0  -C='*=u' ${BATCH_ARGS[*]}"\
   --args="--configuration-name=${configuration}-float6-no-unroll-no-lp-c -DDATA_TYPE_IS_FLOAT --channels-number=1 -fno-unroll-loops --panda-parameter=LP-BB-list=0  -C='*__float' ${BATCH_ARGS[*]}"\
   --args="--configuration-name=${configuration}-float7-no-unroll-no-lp-u -DDATA_TYPE_IS_FLOAT --channels-number=1 -fno-unroll-loops --panda-parameter=LP-BB-list=0  -C='*=u' ${BATCH_ARGS[*]}"\
   -l${script_dir}/polybench_list \
   -o "out${OUT_SUFFIX}" -b${BENCHMARKS_ROOT} \
   --name="${OUT_SUFFIX}" "$@"
   

