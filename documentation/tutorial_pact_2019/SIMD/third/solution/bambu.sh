#!/bin/bash
script=$(readlink -e $0)
root_dir=$(dirname $script)/..
mkdir test2
cd test2
bambu --compiler=I386_GCC49 --device-name=5SGXEA7N2F45C1 --simulate -fwhole-program -fno-delete-null-pointer-checks --clock-period=10 --experimental-setup=BAMBU-BALANCED-MP -fdisable-tree-cunroll -fdisable-tree-ivopts --param max-inline-insns-auto=1000  ${root_dir}/histogram.c -fopenmp-simd=2 --pretty-print=output.c
cd ..
mkdir test3
cd test3
bambu --compiler=I386_GCC49 --device-name=5SGXEA7N2F45C1 --simulate -fwhole-program -fno-delete-null-pointer-checks --clock-period=10 --experimental-setup=BAMBU-BALANCED-MP -fdisable-tree-cunroll -fdisable-tree-ivopts --param max-inline-insns-auto=1000  ${root_dir}/histogram.c -fopenmp-simd=3 --pretty-print=output.c
cd ..
mkdir test4
cd test4
bambu --compiler=I386_GCC49 --device-name=5SGXEA7N2F45C1 --simulate -fwhole-program -fno-delete-null-pointer-checks --clock-period=10 --experimental-setup=BAMBU-BALANCED-MP -fdisable-tree-cunroll -fdisable-tree-ivopts --param max-inline-insns-auto=1000  ${root_dir}/histogram.c -fopenmp-simd=4 --pretty-print=output.c
cd ..
mkdir test8
cd test8
bambu --compiler=I386_GCC49 --device-name=5SGXEA7N2F45C1 --simulate -fwhole-program -fno-delete-null-pointer-checks --clock-period=10 --experimental-setup=BAMBU-BALANCED-MP -fdisable-tree-cunroll -fdisable-tree-ivopts --param max-inline-insns-auto=1000  ${root_dir}/histogram.c -fopenmp-simd=8 --pretty-print=output.c
cd ..
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
exit 0
