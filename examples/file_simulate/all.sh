#!/bin/bash
script=$(readlink -e $0)
root_dir=$(dirname $script)
ARGS="--simulator=MODELSIM --experimental-setup=BAMBU --simulate --generate-tb=$root_dir/test.xml --print-dot --file-input-data=$root_dir/test.xml"
$(dirname $0)/../../etc/scripts/test_panda.py $root_dir --spider-style="../lib/latex_format_bambu_results_xilinx.xml" --tool=bambu  -ooutput_file_simulate \
   --args="$ARGS --channels-type=MEM_ACC_11 --configuration-name=MEM_ACC_11" \
   --args="$ARGS --channels-type=MEM_ACC_NN --configuration-name=MEM_ACC_NN" \
   -t120m --table=file_simulate.tex --benchmarks_root=$(dirname $0) --name="file_simulate"  "$@"
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
exit 0
