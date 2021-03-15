#!/bin/bash
ARGS="-c=--clock-period=10 -c=--speculative-sdc-scheduling -c=--device=xc7vx330t-1ffg1157"
script=$(readlink -e $0)
root_dir=$(dirname $script)
NAME=$(basename $0 .sh)
DIRNAME=${root_dir##*/}
$root_dir/xilinx_ISE.sh $ARGS -ooutput_${DIRNAME}_$NAME --name=${DIRNAME}_$NAME --table=${DIRNAME}_$NAME.tex $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
exit 0
