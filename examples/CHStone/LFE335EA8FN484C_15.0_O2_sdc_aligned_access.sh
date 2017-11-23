#!/bin/bash
ARGS="-c=--clock-period=15 -c=--device=LFE335EA8FN484C -c=--speculative-sdc-scheduling -c=--aligned-access"
script=$(readlink -e $0)
root_dir=$(dirname $script)
NAME=$(basename $0 .sh)
DIRNAME=${root_dir##*/}
$root_dir/lattice.sh $ARGS -ooutput_${DIRNAME}_$NAME --name=${DIRNAME}_$NAME --table=${DIRNAME}_$NAME.tex $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
exit 0
