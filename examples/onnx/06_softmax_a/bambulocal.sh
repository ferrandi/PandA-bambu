#!/bin/bash
script=$(readlink -e $0)
root_dir=$(dirname $script)

./scalarize.sh
export PATH=../../../../../panda/bin:../../src:../../../src:/opt/panda/bin:$PATH
rm -rf bambu
mkdir bambu
cd bambu
bambu $root_dir/06_softmax_a.scalarized.ll $root_dir/06_softmax_a.wrapper.c \
      -I $root_dir/../common/ \
      -DBAMBU_PROFILING\
      -fno-inline -fno-inline-functions\
      -lm -v4 --compiler=I386_CLANG6 --print-dot \
      --top-fname=fused_nn_softmax_wrapper --top-rtldesign-name=fused_nn_softmax \
      --memory-allocation-policy=EXT_PIPELINED_BRAM \
      --generate-tb=$root_dir/test.xml --simulator=VERILATOR \
      --pretty-print=a.c --no-iob --device-name=xc7vx690t-3ffg1930-VVD --clock-period=3.3 --evaluation
