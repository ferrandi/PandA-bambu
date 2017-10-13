#!/bin/bash
script=$(readlink -e $0)
root_dir=$(dirname $script)
export PATH=../../src:../../../src:../../../../src:/opt/panda/bin:$PATH

mkdir -p $root_dir/hls1
cd $root_dir/hls1
echo "#complete synthesis and simulation"
bambu -v5 --print-dot $root_dir/coef1.cpp --compiler=I386_GCC5 --no-iob --evaluation --std=c++1y --top-fname=main --top-rtldesign-name=firFixed --file-input-data=$root_dir/input.pcm -v2 -DNDEBUG -DWITH_EXTERNC
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
diff $root_dir/hls1/outputFixed.pcm $root_dir/outputFixed.gold.pcm
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi

mkdir -p $root_dir/hls2
cd $root_dir/hls2
echo "#focused synthesis and simulation"
bambu -v5 --print-dot $root_dir/coef1.cpp --compiler=I386_GCC5 --no-iob --evaluation --std=c++1y --top-fname=firFixed --generate-tb=$root_dir/test.xml -v2
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi

exit 0
