#!/bin/bash
script=$(readlink -e $0)
root_dir=$(dirname $script)

mkdir -p IP_integration_hls
cd IP_integration_hls
echo "#synthesis with VIVADO RTL and simulation"
timeout 2h bambu $root_dir/main_test.c $root_dir/top.c $root_dir/module_lib.xml --top-fname=main --top-rtldesign-name=my_ip \
   $root_dir/constraints_STD.xml --experimental-setup=BAMBU --no-iob --memory-allocation-policy=ALL_BRAM -O3 \
   --C-no-parse=$root_dir/module1.c,$root_dir/module2.c,$root_dir/printer1.c,$root_dir/printer2.c \
   --file-input-data=$root_dir/module1.v,$root_dir/module2.v,$root_dir/printer1.v,$root_dir/printer2.v \
   --generate-tb=$root_dir/test.xml --simulator=MODELSIM --print-dot --evaluation -DBAMBU_PROFILING "$@"
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
cd ..
exit 0
