#!/bin/bash
script=$(readlink -e $0)
root_dir=$(dirname $script)
basename_dir=$(basename $root_dir)
current_dir=$(pwd)
mkdir -p $basename_dir/hls1
cd $basename_dir/hls1
echo "#simulation and discrepancy"
timeout 2h bambu --simulator=MODELSIM --print-dot $root_dir/gcd.cc --compiler=I386_GCC5 --generate-tb=$root_dir/test.xml --no-iob --simulate --discrepancy --top-fname=gcd "$@"
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
cd $current_dir
mkdir -p $basename_dir/hls2
cd $basename_dir/hls2
echo "#just simulation"
timeout 2h bambu --simulator=MODELSIM --print-dot $root_dir/gcd.cc --compiler=I386_GCC5 --generate-tb=$root_dir/test.xml --no-iob --simulate --top-fname=gcd "$@"
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
exit 0
