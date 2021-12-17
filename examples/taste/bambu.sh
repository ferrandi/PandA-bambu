#!/bin/bash
script=$(readlink -e $0)
root_dir=$(dirname $script)

mkdir -p taste_synth
cd taste_synth
timeout 2h bambu --simulator=MODELSIM --evaluation=AREA --benchmark-name=InterfaceView.aadl $root_dir/InterfaceView.aadl --top-fname=first_function,second_function "$@"
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
cd ..
