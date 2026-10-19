#!/bin/bash
script_dir="$(dirname $(readlink -e $0))"
ggo_require_compiler=1
. $script_dir/../../panda_regressions/hls/generic_getopt.sh

BENCHMARKS_ROOT="${script_dir}/MachSuite"
BATCH_ARGS=("-I${BENCHMARKS_ROOT}/common" "--generate-tb=${BENCHMARKS_ROOT}/common/harness.cpp" "--generate-tb=${BENCHMARKS_ROOT}/common/support.cpp")
BATCH_ARGS+=("-m32" "-fno-tree-vectorize")
BATCH_ARGS+=("--generate-interface=INFER" "--hls-div=NR")
BATCH_ARGS+=("--simulate" "--memory-allocation-policy=NO_BRAM" "--bus-pipelined" "--mem-delay-read=20" "--mem-delay-write=20" "--tb-queue-size=20" "-fopenmp")
OUT_SUFFIX="${compiler}_MachSuite"

python3 $script_dir/../../etc/scripts/mantis.py --tool=bambu  \
   --args="--configuration-name=1co_1cs    -DTHREAD_NUMBER=1                       --channels-number=1  --channels-type=MEM_ACC_11 ${BATCH_ARGS[*]}"\
   --args="--configuration-name=1co_2cs    -DTHREAD_NUMBER=2    --context_switch=2 --channels-number=1  --channels-type=MEM_ACC_11 ${BATCH_ARGS[*]}"\
   --args="--configuration-name=1co_4cs    -DTHREAD_NUMBER=4    --context_switch=4 --channels-number=1  --channels-type=MEM_ACC_11 ${BATCH_ARGS[*]}"\
   --args="--configuration-name=1co_8cs    -DTHREAD_NUMBER=8    --context_switch=8 --channels-number=1  --channels-type=MEM_ACC_11 ${BATCH_ARGS[*]}"\
   --args="--configuration-name=2co_1cs    -DTHREAD_NUMBER=2                       --channels-number=2  --channels-type=MEM_ACC_NN ${BATCH_ARGS[*]}"\
   --args="--configuration-name=2co_2cs    -DTHREAD_NUMBER=4    --context_switch=2 --channels-number=2  --channels-type=MEM_ACC_NN ${BATCH_ARGS[*]}"\
   --args="--configuration-name=2co_4cs    -DTHREAD_NUMBER=8    --context_switch=4 --channels-number=2  --channels-type=MEM_ACC_NN ${BATCH_ARGS[*]}"\
   --args="--configuration-name=2co_8cs    -DTHREAD_NUMBER=16   --context_switch=8 --channels-number=2  --channels-type=MEM_ACC_NN ${BATCH_ARGS[*]}"\
   --args="--configuration-name=4co_1cs    -DTHREAD_NUMBER=4                       --channels-number=4  --channels-type=MEM_ACC_NN ${BATCH_ARGS[*]}"\
   --args="--configuration-name=4co_2cs    -DTHREAD_NUMBER=8    --context_switch=2 --channels-number=4  --channels-type=MEM_ACC_NN ${BATCH_ARGS[*]}"\
   --args="--configuration-name=4co_4cs    -DTHREAD_NUMBER=16   --context_switch=4 --channels-number=4  --channels-type=MEM_ACC_NN ${BATCH_ARGS[*]}"\
   --args="--configuration-name=4co_8cs    -DTHREAD_NUMBER=32   --context_switch=8 --channels-number=4  --channels-type=MEM_ACC_NN ${BATCH_ARGS[*]}"\
   --args="--configuration-name=8co_1cs    -DTHREAD_NUMBER=8                       --channels-number=8  --channels-type=MEM_ACC_NN ${BATCH_ARGS[*]}"\
   --args="--configuration-name=8co_2cs    -DTHREAD_NUMBER=16   --context_switch=2 --channels-number=8  --channels-type=MEM_ACC_NN ${BATCH_ARGS[*]}"\
   --args="--configuration-name=8co_4cs    -DTHREAD_NUMBER=32   --context_switch=4 --channels-number=8  --channels-type=MEM_ACC_NN ${BATCH_ARGS[*]}"\
   --args="--configuration-name=8co_8cs    -DTHREAD_NUMBER=64   --context_switch=8 --channels-number=8  --channels-type=MEM_ACC_NN ${BATCH_ARGS[*]}"\
   --args="--configuration-name=16co_1cs   -DTHREAD_NUMBER=16                      --channels-number=16 --channels-type=MEM_ACC_NN ${BATCH_ARGS[*]}"\
   --args="--configuration-name=16co_2cs   -DTHREAD_NUMBER=32   --context_switch=2 --channels-number=16 --channels-type=MEM_ACC_NN ${BATCH_ARGS[*]}"\
   --args="--configuration-name=16co_4cs   -DTHREAD_NUMBER=64   --context_switch=4 --channels-number=16 --channels-type=MEM_ACC_NN ${BATCH_ARGS[*]}"\
   --args="--configuration-name=16co_8cs   -DTHREAD_NUMBER=128  --context_switch=8 --channels-number=16 --channels-type=MEM_ACC_NN ${BATCH_ARGS[*]}"\
   --args="--configuration-name=32co_1cs   -DTHREAD_NUMBER=32                      --channels-number=32 --channels-type=MEM_ACC_NN ${BATCH_ARGS[*]}"\
   --args="--configuration-name=32co_2cs   -DTHREAD_NUMBER=64   --context_switch=2 --channels-number=32 --channels-type=MEM_ACC_NN ${BATCH_ARGS[*]}"\
   --args="--configuration-name=32co_4cs   -DTHREAD_NUMBER=128  --context_switch=4 --channels-number=32 --channels-type=MEM_ACC_NN ${BATCH_ARGS[*]}"\
   --args="--configuration-name=32co_8cs   -DTHREAD_NUMBER=256  --context_switch=8 --channels-number=32 --channels-type=MEM_ACC_NN ${BATCH_ARGS[*]}"\
   -l${script_dir}/machsuite_list_parallel \
   --timeout=3000m \
   -o "out_${OUT_SUFFIX}" -b${BENCHMARKS_ROOT} \
   "$@"
exit $?
