#!/bin/bash
script=$(readlink -e $0)
root_dir=$(dirname $script)

rm -rf ludecomp
mkdir -p ludecomp
cd ludecomp
echo "#synthesis of fun"
bambu $root_dir/LUdecomposition.c --top-fname=fun \
   -O1 \
   --generate-tb=$root_dir/test.xml --simulate --simulator=VERILATOR \
   -v2 --print-dot "$@" |& tee log.txt
