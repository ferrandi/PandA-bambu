#!/bin/bash
script=$(readlink -e $0)
root_dir=$(dirname $script)
export PATH=/opt/panda/bin:$PATH

mkdir -p hls
cd hls
echo "#synthesis"
timeout 2h bambu -v3 --print-dot $root_dir/aes.c --evaluation --generate-tb=$root_dir/decrypt.xml --no-iob --device-name=xc7vx690t,-3,ffg1930,VVD --clock-period=5.3 --top-fname=main --top-rtldesign-name=decrypt -ftree-vectorize -finline-limit=10000 --simulator=VERILATOR
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
cd ..


