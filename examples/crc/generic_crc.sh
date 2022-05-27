#!/bin/bash
script_dir="$(dirname $(readlink -e $0))"
. $script_dir/../../panda_regressions/hls/generic_getopt.sh

BATCH_ARGS=("--no-iob" "--simulate" "--experimental-setup=BAMBU")
OUT_SUFFIX="output_crc"

python3 $script_dir/../../etc/scripts/test_panda.py --tool=bambu  \
   --args="--configuration-name=base   --channels-type=MEM_ACC_11 ${BATCH_ARGS[*]}"\
   --args="--configuration-name=mem-N1 --channels-type=MEM_ACC_N1 ${BATCH_ARGS[*]}"\
   --args="--configuration-name=mem-NN --channels-type=MEM_ACC_NN ${BATCH_ARGS[*]}"\
   -lcrc_list \
   -o "out${OUT_SUFFIX}" -b$script_dir \
   --name="${OUT_SUFFIX}" "$@"
