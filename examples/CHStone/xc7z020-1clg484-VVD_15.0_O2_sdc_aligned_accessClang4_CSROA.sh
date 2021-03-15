#!/bin/bash
ARGS="-c=--evaluation -c=-fwhole-program -c=-fno-delete-null-pointer-checks -c=--no-iob -c=--clock-period=15 -c=--speculative-sdc-scheduling -c=--experimental-setup=BAMBU-BALANCED-MP -c=--device=xc7z020-1clg484-VVD -c=--aligned-access -c=--compiler=I386_CLANG4 -c=--panda-parameter=enable-CSROA=1"
script=$(readlink -e $0)
root_dir=$(dirname $script)
NAME=$(basename $0 .sh)
DIRNAME=${root_dir##*/}
$root_dir/../../etc/scripts/test_panda.py --spider-style="../lib/latex_format_bambu_results_xilinx.xml" --tool=bambu  -t300m --benchmarks_root=$root_dir  $ARGS -ooutput_${DIRNAME}_$NAME --name=${DIRNAME}_$NAME --table=${DIRNAME}_$NAME.tex  -l$root_dir/list_CSROA $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
exit 0
