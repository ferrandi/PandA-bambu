#!/bin/bash
script=$(readlink -e $0)
root_dir=$(dirname $script)/..
bambu --device-name=5SGXEA7N2F45C1 --simulate -fwhole-program -fno-delete-null-pointer-checks --clock-period=10 --experimental-setup=BAMBU-BALANCED-MP -fdisable-tree-cunroll -fdisable-tree-ivopts --param max-inline-insns-auto=1000  ${root_dir}/histogram.c -fopenmp-simd=4
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
exit 0
