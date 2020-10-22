#!/bin/bash
script=$(readlink -e $0)
root_dir=$(dirname $script)
export PATH=../../src:../../../src:/opt/panda/bin:$PATH

mkdir -p C_testbench
cd C_testbench
echo "#synthesis of icrc with main working as C testbench"
timeout 2h bambu $root_dir/spec.c --top-fname=main --top-rtldesign-name=icrc1 --simulator=VERILATOR --device-name=xc7z020,-1,clg484,YOSYS-VVD --generate-tb=$root_dir/test.xml --evaluation --experimental-setup=BAMBU -O3 -v3 --print-dot -DBAMBU_PROFILING
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
cd ..



