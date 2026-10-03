#!/bin/bash
script_dir="$(dirname $(readlink -e $0))"
ggo_require_compiler=1
ggo_require_device=1
ggo_require_period=1
. ${script_dir}/../../panda_regressions/hls/generic_getopt.sh

DATASET_SIZE="mini"
DATASET_SIZE_U="$(echo $DATASET_SIZE|tr '[:lower:]' '[:upper:]')"
BATCH_ARGS=("-D${DATASET_SIZE_U}_DATASET" "--std=gnu11")
BATCH_ARGS+=("--generate-tb=BENCHMARKS_ROOT/utilities/polybench.c")
BATCH_ARGS+=("--generate-interface=INFER" "--bambu-parameter=simple-benchmark-name=1" "--max-sim-cycles=2000000000" "--simulate" "-p=__float,expf" "--hls-fpdiv=SRT4U" "-s")
BATCH_ARGS+=("-DPOLYBENCH_USE_RESTRICT" "-DDATA_TYPE_IS_FLOAT")
configuration="${device}_$(printf "%04.1f" $period)_$(echo $compiler | tr '[:upper:]' '[:lower:]')"
OUT_SUFFIX="_${configuration}_Polybench.${DATASET_SIZE}"
BENCHMARKS_ROOT="${script_dir}/PolyBenchC"

python3 ${script_dir}/../../etc/scripts/mantis.py --tool=bambu  \
   --args="--configuration-name=${configuration}-float0-c -funroll-loops -C='*__float' ${BATCH_ARGS[*]}"\
   --args="--configuration-name=${configuration}-float1-u -funroll-loops -C='*=u' ${BATCH_ARGS[*]}"\
   --args="--configuration-name=${configuration}-float2-no-unroll-c --channels-number=1 -fno-unroll-loops -C='*__float' ${BATCH_ARGS[*]}"\
   --args="--configuration-name=${configuration}-float3-no-unroll-u --channels-number=1 -fno-unroll-loops -C='*=u' ${BATCH_ARGS[*]}"\
   --args="--configuration-name=${configuration}-float4-no-lp-c -funroll-loops --bambu-parameter=LP-BB-list=0 -C='*__float' ${BATCH_ARGS[*]}"\
   --args="--configuration-name=${configuration}-float5-no-lp-u -funroll-loops --bambu-parameter=LP-BB-list=0 -C='*=u' ${BATCH_ARGS[*]}"\
   --args="--configuration-name=${configuration}-float6-no-unroll-no-lp-c --channels-number=1 -fno-unroll-loops --bambu-parameter=LP-BB-list=0 -C='*__float' ${BATCH_ARGS[*]}"\
   --args="--configuration-name=${configuration}-float7-no-unroll-no-lp-u --channels-number=1 -fno-unroll-loops --bambu-parameter=LP-BB-list=0 -C='*=u' ${BATCH_ARGS[*]}"\
   -l${script_dir}/polybench_list \
   -o "out_${OUT_SUFFIX}" -b${BENCHMARKS_ROOT} \
   "$@"
   

