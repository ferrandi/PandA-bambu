#!/bin/bash
script=$(readlink -e $0)
root_dir=$(dirname $script)
export PATH=../../src:../../../src:/opt/panda/bin:$PATH

mkdir -p hls
cd hls
echo "#synthesis script generation"
bambu -v2 --print-dot $root_dir/module.c --no-iob
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
cd ..

mkdir -p testbench
cd testbench
echo "#synthesis script generation and testbench simulation with VERILATOR"
bambu -v5 --print-dot $root_dir/module.c --generate-tb=$root_dir/test.xml --simulator=VERILATOR --no-iob --simulate
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
cd ..
exit 0
