#!/bin/bash
script=$(readlink -e $0)
root_dir=$(dirname $script)
export PATH=../../src:../../../src:/opt/panda/bin:$PATH
ARGS="--device-name=xc7z020,-1,clg484,VVD --simulate -fwhole-program -fno-delete-null-pointer-checks --clock-period=15 --experimental-setup=BAMBU-PERFORMANCE-MP --speculative-sdc-scheduling --no-iob --host-profiling"
mkdir CHSTONE_test
cd CHSTONE_test
timeout 2h bambu $ARGS $root_dir/dfadd/dfadd.c
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
cd ..

exit 0
