#!/bin/bash
script=$(readlink -e $0)
root_dir=$(dirname $script)

export PATH=../../src:../../../src:/opt/panda/bin:$PATH
rm -rf bambu
mkdir bambu
cd bambu
bambu $root_dir/04_dense_a.parallel.c \
      -I $root_dir/../common/ \
      -DBAMBU_PROFILING\
      -fno-inline -fno-inline-functions\
      -v4 --compiler=I386_GCC49 --print-dot \
      --top-fname=fused_nn_dense_add_wrapper --top-rtldesign-name=fused_nn_dense_add\
      --channels-type=MEM_ACC_11 \
      --memory-allocation-policy=NO_BRAM \
      --mem-delay-read=20 --mem-delay-write=20 \
      --generate-tb=$root_dir/test.xml \
      --no-iob --device-name=EP4SGX530KH40C2 --evaluation \
      --simulator=VERILATOR\
      -fopenmp --pragma-parse --std=c99\
      --num-threads=2  --memory-banks-number=4  --channels-number=2 --context_switch=4 --debug-classes=conn_binding_cs,top_entity_cs,fu_binding_cs
