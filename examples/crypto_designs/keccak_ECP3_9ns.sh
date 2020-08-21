#!/bin/bash
script=$(readlink -e $0)
root_dir=$(dirname $script)
export PATH=../../src:../../../src:/opt/panda/bin:$PATH

rm -rf keccak_ECP3_9ns
mkdir keccak_ECP3_9ns
cd keccak_ECP3_9ns
timeout 2h bambu -O3 --pretty-print=a.c --top-fname=kekka_coproc -v4 --addr-bus-bitsize=9 --base-address=256 --sparse-memory=off --memory-allocation-policy=ALL_BRAM --channels-type=MEM_ACC_11 -finline-limit=10000 --clock-period=9 --device-name=LFE335EA8FN484C $root_dir/Keccak.c --generate-tb=$root_dir/test.xml  --experimental-setup=BAMBU --evaluation -fno-ivopts
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
cd ..
exit 0
