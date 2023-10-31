#!/bin/bash
script=$(readlink -e $0)
root_dir=$(dirname $script)

rm -rf  sum
mkdir -p sum
cd sum
echo "#simulating sum"
bambu ../sum.c --top-fname=sum \
   --generate-tb=../test_sum.xml --simulator=VERILATOR --simulate \
   -v4 --generate-interface=INFER --compiler=I386_CLANG13 "$@" |& tee sum.log
