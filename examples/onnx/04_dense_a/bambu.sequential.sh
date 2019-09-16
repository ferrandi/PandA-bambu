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
      --pretty-print=a.c --no-iob --device-name=EP4SGX530KH40C2 --evaluation 
