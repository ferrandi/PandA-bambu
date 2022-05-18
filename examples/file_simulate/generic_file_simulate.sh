#!/bin/bash
script_dir="$(dirname $(readlink -e $0))"
. $script_dir/../../panda_regressions/hls/generic_getopt.sh

BATCH_ARGS=("--no-iob" "--simulate" "--experimental-setup=BAMBU" "--generate-tb=$script_dir/test.xml" "--file-input-data=$script_dir/test.xml")
OUT_SUFFIX="output_file_simulate"

python3 $script_dir/../../etc/scripts/test_panda.py --tool=bambu \
   --args="--configuration-name=mem-11 --channels-type=MEM_ACC_11 ${BATCH_ARGS[*]}"\
   --args="--configuration-name=mem-NN --channels-type=MEM_ACC_NN ${BATCH_ARGS[*]}"\
   $script_dir -t120m \
   --table=file_simulate.tex --spider-style="latex_format_bambu_results_xilinx.xml" \
   -o "out${OUT_SUFFIX}" -b$script_dir \
   --name="${OUT_SUFFIX}" "$@"
