#!/bin/bash
script_dir="$(dirname $(readlink -e $0))"
ARGS="--simulator=MODELSIM --experimental-setup=BAMBU --simulate --generate-tb=$script_dir/test.xml --print-dot --file-input-data=$script_dir/test.xml"
$script_dir/../../etc/scripts/test_panda.py $script_dir --spider-style="latex_format_bambu_results_xilinx.xml" --tool=bambu  -ooutput_file_simulate \
   --args="$ARGS --channels-type=MEM_ACC_11 --configuration-name=MEM_ACC_11" \
   --args="$ARGS --channels-type=MEM_ACC_NN --configuration-name=MEM_ACC_NN" \
   -t120m --table=file_simulate.tex --benchmarks_root=$script_dir --name="file_simulate"  "$@"
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
exit 0
