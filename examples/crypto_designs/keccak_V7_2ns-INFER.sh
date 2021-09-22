#!/bin/bash
script=$(readlink -e $0)
root_dir=$(dirname $script)

rm -rf keccak_V7_2ns-INFER
mkdir keccak_V7_2ns-INFER
cd keccak_V7_2ns-INFER
timeout 2h bambu --simulator=MODELSIM -O3 --top-fname=kekka_coproc --clock-period=2.8 --device-name=xc7vx690t-3ffg1930-VVD $root_dir/Keccak.c --generate-tb=$root_dir/test.xml --evaluation --compiler=I386_CLANG11 --parallel-backend --no-iob --generate-interface=INFER "$@"
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
cd ..
exit 0
