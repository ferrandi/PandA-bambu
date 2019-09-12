#!/bin/bash
script=$(readlink -e $0)
root_dir=$(dirname $script)

./scalarize.sh
export PATH=../../src:../../../src:/opt/panda/bin:$PATH
rm -rf bambu
mkdir bambu
cd bambu
bambu $root_dir/01_vecmul_a.scalarized.ll $root_dir/01_vecmul_a.wrapper.c \
      -I $root_dir/../common/ \
      -DBAMBU_PROFILING\
      -fno-inline -fno-inline-functions\
      -v4 --compiler=I386_CLANG6 --print-dot \
      --top-fname=fused_multiply_wrapper --top-rtldesign-name=fused_multiply\
      --memory-allocation-policy=EXT_PIPELINED_BRAM \
      --generate-tb=$root_dir/test.xml \
      --pretty-print=a.c --no-iob --device-name=EP4SGX530KH40C2 --evaluation
