#!/bin/bash
script=$(readlink -e $0)
root_dir=$(dirname $script)
export PATH=../../src:../../../src:/opt/panda/bin:$PATH

mkdir -p add_device_simple_sim
cd add_device_simple_sim
echo "# HLS synthesis, testbench generation and simulation"
timeout 2h bambu -v2 -O2 $root_dir/module.c --generate-tb=$root_dir/test.xml --evaluation=CYCLES --simulator=XSIM --pretty-print=a.c --experimental-setup=BAMBU --target-file=$root_dir/xc7z045-2ffg900-VVD.xml
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
cd ..

mkdir -p add_device_simple_synth
cd add_device_simple_synth
echo "# HLS synthesis, testbench generation, simulation with XSIM and RTL synthesis with VIVADO RTL"
timeout 2h bambu -v2 -O2 $root_dir/module.c --generate-tb=$root_dir/test.xml --evaluation --simulator=XSIM --pretty-print=a.c --experimental-setup=BAMBU --target-file=$root_dir/xc7z045-2ffg900-VVD.xml
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
cd ..
exit 0
