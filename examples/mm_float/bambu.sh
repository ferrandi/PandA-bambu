#!/bin/bash
script=$(readlink -e $0)
root_dir=$(dirname $script)

export NPROC=8
export NCORE=8
export DIE_AREA="0 0 180 180"
export CORE_AREA="5.08 5.08 170 170"

mkdir -p mm_float_synth
cd mm_float_synth
echo "# HLS synthesis, testbench generation, simulation with VERILATOR and RTL synthesis with OpenRoad"
timeout 2h bambu $root_dir/module.c --clock-period=1.5 --std=c99 -s --top-fname=mm --generate-tb=$root_dir/test.xml \
                 --evaluation --simulator=VERILATOR --device-name=asap7-BC --memory-allocation-policy=NO_BRAM \
                 --channels-number=8 --experimental-setup=BAMBU-PERFORMANCE-MP --compiler=I386_CLANG12 -v4 "$@"
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
cd ..
exit 0
