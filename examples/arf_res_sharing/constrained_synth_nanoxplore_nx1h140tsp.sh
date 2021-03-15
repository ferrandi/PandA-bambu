#!/bin/bash
script=$(readlink -e $0)
root_dir=$(dirname $script)
export PATH=../../src:../../../src:/opt/panda/bin:$PATH

mkdir -p constrained_synth_nanoxplore_nx1h140tsp
cd constrained_synth_nanoxplore_nx1h140tsp
echo "# Diamond synthesis and ICARUS simulation"
timeout 2h bambu -v4 $root_dir/module.c --generate-tb=$root_dir/test.xml --simulator=ICARUS --device-name=nx1h140tsp --evaluation --experimental-setup=BAMBU --generate-interface=WB4 $root_dir/constraints_STD.xml  --cprf=0.9 --skip-pipe-parameter=1
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
cd ..
