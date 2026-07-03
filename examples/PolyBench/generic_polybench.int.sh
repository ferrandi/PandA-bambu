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
BATCH_ARGS+=("--generate-interface=INFER" "--bambu-parameter=simple-benchmark-name=1" "--max-sim-cycles=2000000000" "--simulate")
BATCH_ARGS+=("-DPOLYBENCH_USE_RESTRICT" "-DDATA_TYPE_IS_INT")
configuration="${device}_$(printf "%04.1f" $period)_$(echo $compiler | tr '[:upper:]' '[:lower:]')"
OUT_SUFFIX="_${configuration}_Polybench.int_${DATASET_SIZE}"
BENCHMARKS_ROOT="${script_dir}/PolyBenchC"

python3 ${script_dir}/../../etc/scripts/mantis.py --tool=bambu  \
   --args="--configuration-name=${configuration}-bal-int0                     --experimental-setup=BAMBU-BALANCED       -funroll-loops ${BATCH_ARGS[*]}"\
   --args="--configuration-name=${configuration}-bal-int1-no-unroll           --experimental-setup=BAMBU-BALANCED       -fno-unroll-loops ${BATCH_ARGS[*]}"\
   --args="--configuration-name=${configuration}-bal-int2-no-lp               --experimental-setup=BAMBU-BALANCED       -funroll-loops --bambu-parameter=LP-BB-list=0 ${BATCH_ARGS[*]}"\
   --args="--configuration-name=${configuration}-bal-int3-no-unroll-no-lp     --experimental-setup=BAMBU-BALANCED       -fno-unroll-loops --bambu-parameter=LP-BB-list=0 ${BATCH_ARGS[*]}"\
   --args="--configuration-name=${configuration}-bal-int4-NN                  --experimental-setup=BAMBU-BALANCED-MP    -funroll-loops ${BATCH_ARGS[*]}"\
   --args="--configuration-name=${configuration}-bal-int5-no-unroll-NN        --experimental-setup=BAMBU-BALANCED-MP    -fno-unroll-loops ${BATCH_ARGS[*]}"\
   --args="--configuration-name=${configuration}-bal-int6-no-lp-NN            --experimental-setup=BAMBU-BALANCED-MP    -funroll-loops --bambu-parameter=LP-BB-list=0 ${BATCH_ARGS[*]}"\
   --args="--configuration-name=${configuration}-bal-int7-no-unroll-no-lp-NN  --experimental-setup=BAMBU-BALANCED-MP    -fno-unroll-loops --bambu-parameter=LP-BB-list=0 ${BATCH_ARGS[*]}"\
   --args="--configuration-name=${configuration}-perf-int0                    --experimental-setup=BAMBU-PERFORMANCE    -funroll-loops ${BATCH_ARGS[*]}"\
   --args="--configuration-name=${configuration}-perf-int1-no-unroll          --experimental-setup=BAMBU-PERFORMANCE    -fno-unroll-loops ${BATCH_ARGS[*]}"\
   --args="--configuration-name=${configuration}-perf-int2-no-lp              --experimental-setup=BAMBU-PERFORMANCE    -funroll-loops --bambu-parameter=LP-BB-list=0 ${BATCH_ARGS[*]}"\
   --args="--configuration-name=${configuration}-perf-int3-no-unroll-no-lp    --experimental-setup=BAMBU-PERFORMANCE    -fno-unroll-loops --bambu-parameter=LP-BB-list=0 ${BATCH_ARGS[*]}"\
   --args="--configuration-name=${configuration}-perf-int4-NN                 --experimental-setup=BAMBU-PERFORMANCE-MP -funroll-loops ${BATCH_ARGS[*]}"\
   --args="--configuration-name=${configuration}-perf-int5-no-unroll-NN       --experimental-setup=BAMBU-PERFORMANCE-MP -fno-unroll-loops ${BATCH_ARGS[*]}"\
   --args="--configuration-name=${configuration}-perf-int6-no-lp-NN           --experimental-setup=BAMBU-PERFORMANCE-MP -funroll-loops --bambu-parameter=LP-BB-list=0 ${BATCH_ARGS[*]}"\
   --args="--configuration-name=${configuration}-perf-int7-no-unroll-no-lp-NN --experimental-setup=BAMBU-PERFORMANCE-MP -fno-unroll-loops --bambu-parameter=LP-BB-list=0 ${BATCH_ARGS[*]}"\
   -l${script_dir}/polybench_list.int \
   -o "out_${OUT_SUFFIX}" -b${BENCHMARKS_ROOT} \
   "$@"

