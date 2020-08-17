#!/bin/bash
script=$(readlink -e $0)
root_dir=$(dirname $script)
export PATH=../../src:../../../src:$root_dir/../../../../bin/:/opt/panda/bin:$PATH

mkdir -p arf_hls
cd arf_hls
echo "#synthesis script generation"
timeout 2h bambu -v2 --print-dot $root_dir/module.c --no-iob
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
cd ..

mkdir -p arf_testbench
cd arf_testbench
echo "#synthesis script generation and testbench simulation"
timeout 2h bambu -v5 --print-dot $root_dir/module.c --generate-tb=$root_dir/test.xml --no-iob --simulate
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
cd ..
exit 0
