#!/bin/bash
script=$(readlink -e $0)
root_dir=$(dirname $script)
export PATH=../../src:../../../src:/opt/panda/bin:$PATH

mkdir -p function_pointers_synth
cd function_pointers_synth
echo "# HLS synthesis, testbench generation and simulation"
timeout 2h bambu --top-fname=test --experimental-setup=BAMBU --generate-tb=$root_dir/qsort_glibc/test.xml -Os --device-name=xc7z020-1clg484-VVD --no-iob -v4 --evaluation --soft-float $root_dir/qsort_glibc/test.c $root_dir/qsort_glibc/less.c $root_dir/qsort_glibc/qsort.c
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
cd ..
exit 0
