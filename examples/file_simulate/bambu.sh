#!/bin/bash
script=$(readlink -e $0)
root_dir=$(dirname $script)
export PATH=../../src:../../../src:/opt/panda/bin:$PATH

mkdir -p 11
cd 11
timeout 2h bambu --experimental-setup=BAMBU --simulate --generate-tb=$root_dir/test.xml -v2 --print-dot --file-input-data=$root_dir/test.xml --channels-type=MEM_ACC_11 --configuration-name=MEM_ACC_NN $root_dir/module.c
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
cd ..

mkdir -p NN
cd NN
timeout 2h bambu --experimental-setup=BAMBU --simulate --generate-tb=$root_dir/test.xml -v2 --print-dot --file-input-data=$root_dir/test.xml --channels-type=MEM_ACC_11 --configuration-name=MEM_ACC_NN $root_dir/module.c
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
cd ..
exit 0
