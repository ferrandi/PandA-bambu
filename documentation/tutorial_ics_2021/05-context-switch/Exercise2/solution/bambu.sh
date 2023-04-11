#!/bin/bash
script=$(readlink -e $0)
root_dir=$(dirname $script)/..

bambu ${root_dir}/trinityq4/lubm_trinityq4.c --top-fname=search \
   ${root_dir}/common/atominIncrement.c ${root_dir}/common/data.c -I${root_dir}/common/ \
   --compiler=I386_GCC49 --experimental-set=BAMBU -O3 --std=c99 -fno-delete-null-pointer-checks \
   --channels-type=MEM_ACC_11 --memory-allocation-policy=NO_BRAM \
   --device-name=xc7vx690t-3ffg1930-VVD --clock-period=10 \
   -DMAX_VERTEX_NUMBER=26455 -DMAX_EDGE_NUMBER=100573 -DN_THREADS=2  \
   --mem-delay-read=20 --mem-delay-write=20 \
   --generate-tb=${root_dir}/test-1.xml --simulator=VERILATOR --simulate \
   --pragma-parse --num-accelerators=2 --memory-banks-number=4 --channels-number=2 --context_switch=1 \
   -v3 "$@" |& tee log.txt
