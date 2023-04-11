#!/bin/bash
script=$(readlink -e $0)
root_dir=$(dirname $script)/..

mkdir test2
cd test2
bambu ${root_dir}/histogram.c \
   --compiler=I386_GCC49 --experimental-setup=BAMBU-BALANCED-MP \
   --device-name=5SGXEA7N2F45C1 --clock-period=10 \
   -fopenmp-simd=2 \
   -fwhole-program -fno-delete-null-pointer-checks -fdisable-tree-cunroll -fdisable-tree-ivopts --param max-inline-insns-auto=1000 \
   --simulate --pretty-print=output.c \
   "$@" |& tee log.txt
cd ..
mkdir test3
cd test3
bambu ${root_dir}/histogram.c \
   --compiler=I386_GCC49 --experimental-setup=BAMBU-BALANCED-MP \
   --device-name=5SGXEA7N2F45C1 --clock-period=10 \
   -fopenmp-simd=3 \
   -fwhole-program -fno-delete-null-pointer-checks -fdisable-tree-cunroll -fdisable-tree-ivopts --param max-inline-insns-auto=1000 \
   --simulate --pretty-print=output.c \
   "$@" |& tee log.txt
cd ..
mkdir test4
cd test4
bbambu ${root_dir}/histogram.c \
   --compiler=I386_GCC49 --experimental-setup=BAMBU-BALANCED-MP \
   --device-name=5SGXEA7N2F45C1 --clock-period=10 \
   -fopenmp-simd=4 \
   -fwhole-program -fno-delete-null-pointer-checks -fdisable-tree-cunroll -fdisable-tree-ivopts --param max-inline-insns-auto=1000 \
   --simulate --pretty-print=output.c \
   "$@" |& tee log.txt
cd ..
mkdir test8
cd test8
bambu ${root_dir}/histogram.c \
   --compiler=I386_GCC49 --experimental-setup=BAMBU-BALANCED-MP \
   --device-name=5SGXEA7N2F45C1 --clock-period=10 \
   -fopenmp-simd=8 \
   -fwhole-program -fno-delete-null-pointer-checks -fdisable-tree-cunroll -fdisable-tree-ivopts --param max-inline-insns-auto=1000 \
   --simulate --pretty-print=output.c \
   "$@" |& tee log.txt
cd ..
