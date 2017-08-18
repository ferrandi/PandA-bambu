#!/bin/bash
script=`readlink -e $0`
root_dir=`dirname $script`
export PATH=../../src:../../../src:/opt/panda/bin:$PATH

$root_dir/constrained_synth_lattice.sh
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi

$root_dir/unconstrained_synth_lattice.sh
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
