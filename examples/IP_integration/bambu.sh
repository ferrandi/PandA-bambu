#!/bin/bash
script=$(readlink -e $0)
root_dir=$(dirname $script)
export PATH=../../src:../../../src:/opt/panda/bin:$PATH

mkdir -p IP_integration_hls
cd IP_integration_hls
echo "#synthesis with VIVADO RTL and simulation"
timeout 2h bambu --print-dot $root_dir/main_test.c $root_dir/top.c --generate-tb=$root_dir/test.xml --C-no-parse=$root_dir/module1.c,$root_dir/module2.c,$root_dir/printer1.c,$root_dir/printer2.c --file-input-data=$root_dir/module1.v,$root_dir/module2.v,$root_dir/printer1.v,$root_dir/printer2.v $root_dir/module_lib.xml -v4  --experimental-setup=BAMBU --top-fname=main --top-rtldesign-name=my_ip --evaluation --no-iob -DBAMBU_PROFILING $root_dir/constraints_STD.xml --memory-allocation-policy=ALL_BRAM -O3
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
cd ..
exit 0
