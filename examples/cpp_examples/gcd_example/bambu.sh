#!/bin/bash
script=$(readlink -e $0)
root_dir=$(dirname $script)
export PATH=../../src:../../../src:/opt/panda/bin:$PATH


mkdir -p hls
cd hls
echo "#synthesis and simulation"
bambu -v5 --print-dot $root_dir/gcd.cc --compiler=I386_GCC49 --generate-tb=$root_dir/test.xml --simulator=VERILATOR --no-iob --evaluation
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
cd ..
exit 0
