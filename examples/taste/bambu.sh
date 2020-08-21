#!/bin/bash
script=$(readlink -e $0)
root_dir=$(dirname $script)
export PATH=../../src:../../../src:/opt/panda/bin:$PATH

mkdir -p taste_synth
cd taste_synth
timeout 2h bambu  --evaluation=AREA --benchmark-name=InterfaceView.aadl $root_dir/InterfaceView.aadl
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
cd ..
