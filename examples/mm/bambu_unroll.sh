#!/bin/bash
script=$(readlink -e $0)
root_dir=$(dirname $script)

mkdir -p mm_synth_unroll
cd mm_synth_unroll
echo "# full unroll HLS synthesis"
timeout 2h bambu $root_dir/module.c --generate-tb=$root_dir/test.xml --compiler=I386_CLANG11 --print-dot --simulator=VERILATOR --generate-interface=INFER --simulate --register-allocation=COLORING -DUNROLL_LOOPS3 -DUNROLL_LOOPS2 
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
cd ..
exit 0
