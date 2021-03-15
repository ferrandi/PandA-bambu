#!/bin/bash
script=$(readlink -e $0)
root_dir=$(dirname $script)
basename_dir=$(basename $root_dir)
current_dir=$(pwd)
export PATH=../../src:../../../src:../../../../src:../../../../../src:/opt/panda/bin:$PATH
mkdir -p $basename_dir
mkdir -p $basename_dir/hls1
cd $basename_dir/hls1
echo "#synthesis and simulation CORDIC"
timeout 2h bambu -v4 --print-dot $root_dir/cordic_atan2.cpp --compiler=I386_CLANG6 --no-iob --evaluation --clock-period=2.5 -DDB_CORDIC --do-not-expose-globals --device-name=xc7vx690t-3ffg1930-VVD --generate-interface=INFER --top-fname=top_atan2 --generate-tb="y0=4,x0=54,zn={0}" -DBIT_ACCURATE
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
cd $current_dir
mkdir -p $basename_dir
mkdir -p $basename_dir/hls2
cd $basename_dir/hls2
echo "#synthesis and simulation CORDIC"
timeout 2h bambu -v4 --print-dot $root_dir/cordic_atan2.cpp --compiler=I386_CLANG6 --no-iob --evaluation --clock-period=2.5 -DDB_CORDIC --do-not-expose-globals --device-name=xc7vx690t-3ffg1930-VVD --generate-interface=INFER --top-fname=top_atan2 --generate-tb="y0=4,x0=54,zn={0}"
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
cd $current_dir
mkdir -p $basename_dir/hls3
cd $basename_dir/hls3
echo "#synthesis and simulation + single precision"
timeout 2h bambu -v4 --print-dot $root_dir/cordic_atan2.cpp --compiler=I386_CLANG6 --no-iob --evaluation --clock-period=2.5 -DDB_SINGLE_PRECISION --do-not-expose-globals --device-name=xc7vx690t-3ffg1930-VVD --generate-interface=INFER --top-fname=top_atan2 --generate-tb="y0=4,x0=54,zn={0}" -lm
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
cd $current_dir
mkdir -p $basename_dir
mkdir -p $basename_dir/hls4
cd $basename_dir/hls4
echo "#synthesis and simulation double precision"
timeout 2h bambu -v4 --print-dot $root_dir/cordic_atan2.cpp --compiler=I386_CLANG6 --no-iob --evaluation --clock-period=2.5 -DDB_DOUBLE_PRECISION --do-not-expose-globals --device-name=xc7vx690t-3ffg1930-VVD --generate-interface=INFER --top-fname=top_atan2 --generate-tb="y0=4,x0=54,zn={0}" -lm
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
exit 0
