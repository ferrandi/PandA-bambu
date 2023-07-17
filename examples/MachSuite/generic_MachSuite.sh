#!/bin/bash
script_dir="$(dirname $(readlink -e $0))"
ggo_require_compiler=1
. $script_dir/../../panda_regressions/hls/generic_getopt.sh

BATCH_ARGS=("--hls-div" "--experimental-setup=BAMBU-PERFORMANCE-MP" "--top-fname=run_benchmark" "--top-rtldesign-name=run_benchmark" "-mx32" "-fno-tree-vectorize" "--simulate" "--simulator=VERILATOR" "-s" "-DBAMBU_PROFILING")
OUT_SUFFIX="${compiler}_MachSuite"

python3 $script_dir/../../etc/scripts/test_panda.py --tool=bambu  \
   --args="--configuration-name=${compiler} ${BATCH_ARGS[*]}"\
   -lmachsuite_list \
   -o "out${OUT_SUFFIX}" -b$script_dir \
   --name="${OUT_SUFFIX}" "$@"
