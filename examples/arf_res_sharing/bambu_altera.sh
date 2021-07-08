#!/bin/bash
script=$(readlink -e $0)
root_dir=$(dirname $script)

$root_dir/constrained_synth_altera.sh
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi

$root_dir/unconstrained_synth_altera.sh
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi

$root_dir/constrained_synth_altera_5.sh
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi

$root_dir/unconstrained_synth_altera_5.sh
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi

