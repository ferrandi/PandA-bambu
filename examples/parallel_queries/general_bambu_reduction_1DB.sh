#!/bin/bash
script_dir="$(dirname $(readlink -e $0))"
ggo_require_compiler=1
. $script_dir/../../panda_regressions/hls/generic_getopt.sh

BATCH_ARGS=("-lm" "-fno-delete-null-pointer-checks" "-fopenmp" "--mem-delay-read=20" "--mem-delay-write=20" "--tb-queue-size=20" "--channels-type=MEM_ACC_NN" "--generate-interface=INFER" "--memory-allocation-policy=NO_BRAM" "--bus-pipelined" "-DMAX_VERTEX_NUMBER=26455" "-DMAX_EDGE_NUMBER=100573")
OUT_SUFFIX="parallel_queries_1DB"

python3 $script_dir/../../etc/scripts/mantis.py --tool=bambu  \
   --args="--configuration-name=02A-01CS -DN_THREADS=2     --channels-number=2 --context_switch=1 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=02A-02CS -DN_THREADS=4     --channels-number=2 --context_switch=2 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=02A-04CS -DN_THREADS=8     --channels-number=2 --context_switch=4 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=02A-08CS -DN_THREADS=16    --channels-number=2 --context_switch=8 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=04A-01CS -DN_THREADS=4     --channels-number=4 --context_switch=1 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=04A-02CS -DN_THREADS=8     --channels-number=4 --context_switch=2 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=04A-04CS -DN_THREADS=16    --channels-number=4 --context_switch=4 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=04A-08CS -DN_THREADS=32    --channels-number=4 --context_switch=8 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=08A-01CS -DN_THREADS=8     --channels-number=8 --context_switch=1 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=08A-02CS -DN_THREADS=16    --channels-number=8 --context_switch=2 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=08A-04CS -DN_THREADS=32    --channels-number=8 --context_switch=4 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=08A-08CS -DN_THREADS=64    --channels-number=8 --context_switch=8 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=16A-01CS -DN_THREADS=16    --channels-number=16 --context_switch=1 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=16A-02CS -DN_THREADS=32    --channels-number=16 --context_switch=2 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=16A-04CS -DN_THREADS=64    --channels-number=16 --context_switch=4 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=16A-08CS -DN_THREADS=128   --channels-number=16 --context_switch=8 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=32A-01CS -DN_THREADS=32    --channels-number=32 --context_switch=1 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=32A-02CS -DN_THREADS=64    --channels-number=32 --context_switch=2 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=32A-04CS -DN_THREADS=128   --channels-number=32 --context_switch=4 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=32A-08CS -DN_THREADS=256   --channels-number=32 --context_switch=8 ${BATCH_ARGS[*]}" \
   -llist_reduction_1DB \
   --timeout=3000m \
   -o "out_${OUT_SUFFIX}" -b$script_dir \
   "$@"
exit $?
