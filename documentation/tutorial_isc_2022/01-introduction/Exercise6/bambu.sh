#!/bin/bash
script=$(readlink -e $0)
root_dir=$(dirname $script)

rm -rf hls
mkdir -p hls
cd hls
echo "# integrating IP simulation"
bambu $root_dir/main_test.c $root_dir/top.c --top-fname=main --top-rtldesign-name=my_ip \
   --C-no-parse=$root_dir/module1.c,$root_dir/module2.c,$root_dir/printer1.c,$root_dir/printer2.c \
   --file-input-data=$root_dir/module1.v,$root_dir/module2.v,$root_dir/printer1.v,$root_dir/printer2.v \
   $root_dir/module_lib.xml $root_dir/constraints_STD.xml \
   --experimental-setup=BAMBU -O3 \
   --memory-allocation-policy=ALL_BRAM \
   --generate-tb=$root_dir/test.xml --simulate --simulator=VERILATOR \
   --print-dot -v4 "$@" |& tee log.txt
