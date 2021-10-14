#!/bin/bash
script=$(readlink -e $0)
root_dir=$(dirname $script)

rm -rf keccak_V7
mkdir keccak_V7
cd keccak_V7
bambu ../Keccak.c --top-fname=kekka_coproc \ 
   --clock-period=2.5 --device-name=xc7vx690t-3ffg1930-VVD \
   --generate-tb=$root_dir/test.xml --simulate \ 
   --compiler=I386_CLANG12 --no-iob \
   --print-dot -v4 "$@" |& tee log.txt
