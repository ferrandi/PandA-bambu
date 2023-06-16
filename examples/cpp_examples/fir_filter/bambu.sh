#!/bin/bash
script=$(readlink -e $0)
root_dir=$(dirname $script)
basename_dir=$(basename $root_dir)
current_dir=$(pwd)

mkdir -p $basename_dir/hls1
cd $basename_dir/hls1
echo "#complete synthesis and simulation"
timeout 2h bambu --simulator=MODELSIM --panda-parameter=disable-pragma-parsing=1 --print-dot $root_dir/coef1.cpp --generate-tb=$root_dir/coef1.cpp --compiler=I386_GCC5 --evaluation --std=c++1y --top-fname=firFixed --file-input-data=$root_dir/input.pcm -DNDEBUG -DWITH_EXTERNC "$@"
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
diff outputFixed.pcm $root_dir/outputFixed.gold.pcm
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
exit 0
