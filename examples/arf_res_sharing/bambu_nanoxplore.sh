#!/bin/bash
script=$(readlink -e $0)
root_dir=$(dirname $script)

$root_dir/constrained_synth_nanoxplore_nx1h35S.sh
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi

$root_dir/constrained_synth_nanoxplore_nx1h140tsp.sh
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi

$root_dir/unconstrained_synth_nanoxplore_nx1h35S.sh
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi

$root_dir/unconstrained_synth_nanoxplore_nx1h140tsp.sh
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
