#!/bin/bash
script=$(readlink -e $0)
root_dir=$(dirname $script)
export PATH=../../src:../../../src:/opt/panda/bin:$PATH

rm -rf keccak_CycloneII_10ns
mkdir keccak_CycloneII_10ns
cd keccak_CycloneII_10ns
timeout 2h bambu -O3 --pretty-print=a.c --top-fname=kekka_coproc -v4 --addr-bus-bitsize=9 --base-address=256 --sparse-memory=off --memory-allocation-policy=ALL_BRAM --channels-type=MEM_ACC_11 -finline-limit=10000 --clock-period=10 --device-name=EP2C70F896C6-R $root_dir/Keccak.c --generate-tb=$root_dir/test.xml --experimental-setup=BAMBU --evaluation --mux-margins=2 --print-dot -fno-ivopts
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
cd ..
exit 0
