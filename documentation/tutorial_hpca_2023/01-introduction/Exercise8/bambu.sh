#!/bin/bash
script=$(readlink -e $0)
root_dir=$(dirname $script)

rm -rf keccak_V7
mkdir keccak_V7
cd keccak_V7
clang-6.0 -O3 -fno-slp-vectorize -fno-vectorize $root_dir/Keccak.c -emit-llvm -S -o test.ll
bambu test.ll --top-fname=kekka_coproc \
   --clock-period=2.5 --device-name=xc7vx690t-3ffg1930-VVD \
   --generate-tb=$root_dir/test.xml --simulate \
   --compiler=I386_CLANG6  \
   --print-dot --pretty-print=a.c -v4 "$@" |& tee log.txt
