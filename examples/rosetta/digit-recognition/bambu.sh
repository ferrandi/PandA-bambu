#!/bin/bash
script=$(readlink -e $0)
root_dir=$(dirname $script)

mkdir -p bambu
cd bambu
bambu --generate-interface=INFER --simulate --simulator=VERILATOR $root_dir/digitrec_sw.c --top-fname=DigitRec_sw -s --do-not-expose-globals --generate-tb=$root_dir/test.xml --std=c99 --compiler=I386_CLANG4 --panda-parameter=enable-CSROA=0 
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
cd ..
exit 0
