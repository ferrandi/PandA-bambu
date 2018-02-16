#!/bin/bash
script=$(readlink -e $0)
root_dir=$(dirname $script)
ARGS="--compiler=I386_GCC49 --evaluation --no-iob --device-name=5SGXEA7N2F45C1 -v3 --print-dot --experimental-setup=BAMBU-PERFORMANCE-MP"
NAME=$(basename $0 .sh)
DIRNAME=${root_dir##*/}
$root_dir/../../etc/scripts/test_panda.py --spider-style="$root_dir/latex_format_bambu_results_altera_synth.xml" --tool=bambu -l$root_dir/list --args="$ARGS" -t120m --benchmarks_root=$root_dir -ooutput_${DIRNAME}_$NAME --name=${DIRNAME}_$NAME --table=${DIRNAME}_$NAME.tex $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
exit 0

