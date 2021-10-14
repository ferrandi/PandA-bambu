#!/bin/bash

script_dir="$(dirname $(readlink -e $0))"
. $script_dir/../../panda_regressions/hls/generic_getopt.sh

BATCH_ARGS=("--simulator=MODELSIM" "--compiler=I386_GCC49" "--std=c99" "--experimental-setup=BAMBU" "-O3" "-fno-delete-null-pointer-checks" "-fopenmp" "--pragma-parse" "--mem-delay-read=20" "--mem-delay-write=20" "--channels-type=MEM_ACC_11" "--memory-allocation-policy=NO_BRAM" "--no-iob" "--device-name=xc7vx690t-3ffg1930-VVD" "--clock-period=10" "-DMAX_VERTEX_NUMBER=26455" "-DMAX_EDGE_NUMBER=100573" "--simulate")
OUT_SUFFIX="parallel_queries_40DB"

$(dirname $0)/../../etc/scripts/test_panda.py --tool=bambu  \
   --args="--configuration-name=02W-04CH-2C-01CS -DN_THREADS=2  --num-accelerators=2  --memory-banks-number=4  --channels-number=2 --context_switch=1 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=02W-04CH-2C-02CS -DN_THREADS=2  --num-accelerators=2  --memory-banks-number=4  --channels-number=2 --context_switch=2 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=02W-04CH-2C-04CS -DN_THREADS=2  --num-accelerators=2  --memory-banks-number=4  --channels-number=2 --context_switch=4 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=02W-04CH-2C-08CS -DN_THREADS=2  --num-accelerators=2  --memory-banks-number=4  --channels-number=2 --context_switch=8 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=02W-04CH-2C-16CS -DN_THREADS=2  --num-accelerators=2  --memory-banks-number=4  --channels-number=2 --context_switch=16 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=02W-08CH-2C-01CS -DN_THREADS=2  --num-accelerators=2  --memory-banks-number=8  --channels-number=2 --context_switch=1 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=02W-08CH-2C-02CS -DN_THREADS=2  --num-accelerators=2  --memory-banks-number=8  --channels-number=2 --context_switch=2 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=02W-08CH-2C-04CS -DN_THREADS=2  --num-accelerators=2  --memory-banks-number=8  --channels-number=2 --context_switch=4 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=02W-08CH-2C-08CS -DN_THREADS=2  --num-accelerators=2  --memory-banks-number=8  --channels-number=2 --context_switch=8 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=02W-08CH-2C-16CS -DN_THREADS=2  --num-accelerators=2  --memory-banks-number=8  --channels-number=2 --context_switch=16 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=02W-16CH-2C-01CS -DN_THREADS=2  --num-accelerators=2  --memory-banks-number=16 --channels-number=2 --context_switch=1 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=02W-16CH-2C-02CS -DN_THREADS=2  --num-accelerators=2  --memory-banks-number=16 --channels-number=2 --context_switch=2 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=02W-16CH-2C-04CS -DN_THREADS=2  --num-accelerators=2  --memory-banks-number=16 --channels-number=2 --context_switch=4 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=02W-16CH-2C-08CS -DN_THREADS=2  --num-accelerators=2  --memory-banks-number=16 --channels-number=2 --context_switch=8 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=02W-16CH-2C-16CS -DN_THREADS=2  --num-accelerators=2  --memory-banks-number=16 --channels-number=2 --context_switch=16 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=04W-04CH-2C-02CS -DN_THREADS=4  --num-accelerators=4  --memory-banks-number=4  --channels-number=2 --context_switch=2 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=04W-04CH-2C-04CS -DN_THREADS=4  --num-accelerators=4  --memory-banks-number=4  --channels-number=2 --context_switch=4 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=04W-04CH-2C-08CS -DN_THREADS=4  --num-accelerators=4  --memory-banks-number=4  --channels-number=2 --context_switch=8 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=04W-04CH-2C-16CS -DN_THREADS=4  --num-accelerators=4  --memory-banks-number=4  --channels-number=2 --context_switch=16 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=08W-04CH-2C-02CS -DN_THREADS=8  --num-accelerators=8  --memory-banks-number=4  --channels-number=2 --context_switch=2 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=08W-04CH-2C-04CS -DN_THREADS=8  --num-accelerators=8  --memory-banks-number=4  --channels-number=2 --context_switch=4 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=08W-04CH-2C-08CS -DN_THREADS=8  --num-accelerators=8  --memory-banks-number=4  --channels-number=2 --context_switch=8 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=08W-04CH-2C-16CS -DN_THREADS=8  --num-accelerators=8  --memory-banks-number=4  --channels-number=2 --context_switch=16 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=16W-04CH-2C-02CS -DN_THREADS=16 --num-accelerators=16 --memory-banks-number=4  --channels-number=2 --context_switch=2 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=16W-04CH-2C-04CS -DN_THREADS=16 --num-accelerators=16 --memory-banks-number=4  --channels-number=2 --context_switch=4 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=16W-04CH-2C-08CS -DN_THREADS=16 --num-accelerators=16 --memory-banks-number=4  --channels-number=2 --context_switch=8 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=16W-04CH-2C-16CS -DN_THREADS=16 --num-accelerators=16 --memory-banks-number=4  --channels-number=2 --context_switch=16 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=32W-04CH-2C-02CS -DN_THREADS=32  --num-accelerators=32  --memory-banks-number=4  --channels-number=2 --context_switch=2 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=32W-04CH-2C-04CS -DN_THREADS=32  --num-accelerators=32  --memory-banks-number=4  --channels-number=2 --context_switch=4 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=32W-04CH-2C-08CS -DN_THREADS=32  --num-accelerators=32  --memory-banks-number=4  --channels-number=2 --context_switch=8 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=32W-04CH-2C-16CS -DN_THREADS=32  --num-accelerators=32  --memory-banks-number=4  --channels-number=2 --context_switch=16 ${BATCH_ARGS[*]}" \
   -llist_40DB \
   --spider-style="latex_format_bambu_results_xilinx.xml" \
   -o "out${OUT_SUFFIX}" -b$(dirname $0) \
   --name="${OUT_SUFFIX}" "$@"
