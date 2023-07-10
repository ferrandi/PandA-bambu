#!/bin/bash
script=`readlink -e $0`
root_dir=`dirname $script`
mkdir temp
cd temp
timeout 2h bambu --simulator=MODELSIM -fno-delete-null-pointer-checks -fopenmp --pragma-parse --mem-delay-read=20 --mem-delay-write=20 --channels-type=MEM_ACC_11 --memory-allocation-policy=NO_BRAM --device-name=xc7vx690t-3ffg1930-VVD --clock-period=10 --num-accelerators=2  --memory-banks-number=4  --channels-number=2 --context_switch=2 $root_dir/bfs.c --top-fname=parallel "$@"
if test $return_value != 0; then
   exit $return_value
fi
cd ..
