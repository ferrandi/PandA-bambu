#!/bin/bash
script=$(readlink -e $0)
root_dir=$(dirname $script)
export PATH=../../src:../../../src:/opt/panda/bin:$PATH

mkdir -p fortran_hls1
cd fortran_hls1
echo "#synthesis"
timeout 2h bambu -v2 --print-dot $root_dir/euclid.copy.f -fno-underscoring --top-fname=ngcd --generate-tb=$root_dir/test.copy.xml --print-dot --evaluation --speculative-sdc-scheduling --no-iob --pretty-print=a.c
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
cd ..

mkdir -p fortran_hls2
cd fortran_hls2
echo "#synthesis"
timeout 2h bambu -v2 --print-dot $root_dir/euclid.f -fno-underscoring --top-fname=ngcd --generate-tb=$root_dir/test.xml --print-dot --evaluation --speculative-sdc-scheduling --no-iob --pretty-print=a.c
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
cd ..


exit 0
