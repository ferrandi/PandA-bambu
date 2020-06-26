#!/bin/bash
script=$(readlink -e $0)
root_dir=$(dirname $script)
basename_dir=$(basename $root_dir)
current_dir=$(pwd)
export PATH=../../src:../../../src:../../../../src:../../../../../src:/opt/panda/bin:$PATH
mkdir -p $basename_dir
mkdir -p $basename_dir/hls1
cd $basename_dir/hls1
echo "#complete synthesis and simulation"
timeout 2h bambu --panda-parameter=disable-pragma-parsing=1 -v5 --print-dot $root_dir/coef1.cpp --compiler=I386_GCC5 --no-iob --evaluation --std=c++1y --top-fname=main --top-rtldesign-name=firFixed --file-input-data=$root_dir/input.pcm -v2 -DNDEBUG -DWITH_EXTERNC
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
diff outputFixed.pcm $root_dir/outputFixed.gold.pcm
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
cd $current_dir
mkdir -p $basename_dir/hls2
cd $basename_dir/hls2
echo "#focused synthesis and simulation"
timeout 2h bambu --panda-parameter=disable-pragma-parsing=1 -v5 --print-dot $root_dir/coef1.cpp --compiler=I386_GCC5 --no-iob --evaluation --std=c++1y --top-fname=firFixed --generate-tb=$root_dir/test.xml -v2
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
exit 0
