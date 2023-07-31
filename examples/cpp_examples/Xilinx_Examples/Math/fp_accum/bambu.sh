#!/bin/bash
script=$(readlink -e $0)
root_dir=$(dirname $script)
basename_dir=$(basename $root_dir)
current_dir=$(pwd)
mkdir -p $basename_dir
mkdir -p $basename_dir/hls1
cd $basename_dir/hls1
echo "#synthesis and simulation + unroll"
timeout 2h bambu --simulator=MODELSIM --print-dot $root_dir/fp_accum.cpp --compiler=I386_CLANG6 --evaluation --clock-period=2.5 -fno-unroll-loops --device-name=xc7vx690t-3ffg1930-VVD --generate-interface=INFER --top-fname=hls_fp_accumulator --generate-tb=$root_dir/fp_accum.cpp -DCUSTOM_VERIFICATION "$@"
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
cd $current_dir
mkdir -p $basename_dir/hls2
cd $basename_dir/hls2
echo "#synthesis and simulation + no-unroll"
timeout 2h bambu --simulator=MODELSIM --print-dot $root_dir/fp_accum.cpp --compiler=I386_CLANG6 --evaluation --clock-period=2.5 --device-name=xc7vx690t-3ffg1930-VVD --generate-interface=INFER --top-fname=hls_fp_accumulator --generate-tb=$root_dir/fp_accum.cpp -DCUSTOM_VERIFICATION "$@"
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
exit 0
