#!/bin/bash
script=$(readlink -e $0)
root_dir=$(dirname $script)

rm -rf keccak_CycloneII_5ns-INFER
mkdir keccak_CycloneII_5ns-INFER
cd keccak_CycloneII_5ns-INFER
timeout 2h bambu --simulator=MODELSIM -O3 --top-fname=kekka_coproc --clock-period=6 --device-name=EP2C70F896C6-R $root_dir/Keccak.c --generate-tb=$root_dir/test.xml  --evaluation --generate-interface=INFER --compiler=I386_CLANG11 --parallel-backend --no-iob --print-dot "$@"
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
cd ..
exit 0
