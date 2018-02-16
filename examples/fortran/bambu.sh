#!/bin/bash
script=$(readlink -e $0)
root_dir=$(dirname $script)
export PATH=../../src:../../../src:/opt/panda/bin:$PATH

mkdir -p hls1
cd hls1
echo "#synthesis"
bambu -v2 --print-dot $root_dir/euclid.copy.f -fno-underscoring --top-fname=ngcd --generate-tb=$root_dir/test.xml --print-dot --evaluation --speculative-sdc-scheduling --no-iob --pretty-print=a.c
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
cd ..

mkdir -p hls2
cd hls2
echo "#synthesis"
bambu -v2 --print-dot $root_dir/euclid.f -fno-underscoring --top-fname=ngcd --generate-tb=$root_dir/test.xml --print-dot --evaluation --speculative-sdc-scheduling --no-iob --pretty-print=a.c
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
cd ..


exit 0
