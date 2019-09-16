#!/bin/bash
script=$(readlink -e $0)
root_dir=$(dirname $script)

export PATH=../../../../../panda/bin:../../src:../../../src:/opt/panda/bin:$PATH
rm -rf bambu
mkdir bambu
cd bambu
bambu $root_dir/e1_mlp.wrapper.c \
      -I $root_dir/../common/ \
      -DBAMBU_PROFILING\
      -fno-inline -fno-inline-functions\
      -lm -v4 --compiler=I386_GCC49 --print-dot \
      --top-fname=mlp_wrapper --top-rtldesign-name=fused_nn_softmax \
      --channels-type=MEM_ACC_11 \
      --memory-allocation-policy=NO_BRAM \
      --mem-delay-read=20 --mem-delay-write=20 --simulator=VERILATOR \
      --generate-tb=$root_dir/test.xml \
      --no-iob --device-name=xc7vx690t-3ffg1930-VVD --clock-period=3.3 --evaluation 
