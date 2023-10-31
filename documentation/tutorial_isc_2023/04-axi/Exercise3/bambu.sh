#!/bin/bash
script=$(readlink -e $0)
root_dir=$(dirname $script)

rm -rf  maxNumbers
mkdir -p maxNumbers
cd maxNumbers
echo "#simulating maxNumbers"
bambu ../maxNumbers.c --top-fname=maxNumbers \
   --generate-tb=../test_maxNumbers.xml --simulator=VERILATOR --simulate \
   -v4 --generate-interface=INFER --compiler=I386_CLANG13 "$@" |& tee maxNumbers.log
