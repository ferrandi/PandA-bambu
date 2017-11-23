#!/bin/bash
script=$(readlink -e $0)
root_dir=$(dirname $script)
export PATH=../../src:../../../src:/opt/panda/bin:$PATH

mkdir -p hls
cd hls
echo "#synthesis"
bambu -v2 --print-dot $root_dir/module.c --no-iob
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
cd ..

mkdir -p testbench
cd testbench
echo "#synthesis and generation of testbench with ICARUS"
bambu -v5 --print-dot $root_dir/module.c --generate-tb=$root_dir/test.xml --simulator=ICARUS --no-iob
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
cd ..
exit 0
