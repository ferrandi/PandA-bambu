#!/bin/bash
ARGS="-c=--clock-period=5 -c=--speculative-sdc-scheduling -c=--device=xc7vx690t-3ffg1930-VVD"
script=$(readlink -e $0)
root_dir=$(dirname $script)
NAME=$(basename $0 .sh)
DIRNAME=${root_dir##*/}
$root_dir/xilinx_VVD.sh $ARGS -ooutput_${DIRNAME}_$NAME --name=${DIRNAME}_$NAME --table=${DIRNAME}_$NAME.tex $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
exit 0
