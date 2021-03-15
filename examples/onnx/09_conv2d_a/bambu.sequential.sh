#!/bin/bash
script=$(readlink -e $0)
root_dir=$(dirname $script)

export PATH=../../../../../panda/bin:../../src:../../../src:/opt/panda/bin:$PATH
rm -rf bambu
mkdir bambu
cd bambu
timeout 2h bambu $root_dir/09_conv2d_a.parallel.c \
      -I $root_dir/../common/ \
      -DBAMBU_PROFILING\
      -fno-inline -fno-inline-functions\
      -v4 --compiler=I386_GCC49 --print-dot \
      --top-fname=fused_conv2d_wrapper --top-rtldesign-name=fused_nn_layout_transform_2 \
      --channels-type=MEM_ACC_11 \
      --memory-allocation-policy=NO_BRAM \
      --mem-delay-read=20 --mem-delay-write=20 --simulator=VERILATOR \
      --generate-tb=$root_dir/test.xml \
      --pretty-print=a.c --no-iob --device-name=xc7vx690t-3ffg1930-VVD --clock-period=3.3 --evaluation 
