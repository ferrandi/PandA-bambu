#!/bin/bash
script=$(readlink -e $0)
root_dir=$(dirname $script)

rm -rf  read
mkdir -p read
cd read
echo "#simulating read"
bambu ../read.c --top-fname=read \
   --generate-tb=../test_read.xml --simulator=VERILATOR --simulate \
   -v4 --generate-interface=INFER --compiler=I386_CLANG13 "$@" |& tee read.log
