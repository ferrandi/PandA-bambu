#!/bin/bash
ARGS="-c=--clock-period=10 -c=--device=EP2C70F896C6-R -c=--speculative-sdc-scheduling -c=--experimental-setup=BAMBU-PERFORMANCE-MP"
script=$(readlink -e $0)
root_dir=$(dirname $script)
NAME=$(basename $0 .sh)
DIRNAME=${root_dir##*/}
$root_dir/altera.sh $ARGS -ooutput_${DIRNAME}_$NAME --name=${DIRNAME}_$NAME --table=${DIRNAME}_$NAME.tex -l$root_dir/EP2C70F896C6-R_O3_list $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
exit 0
