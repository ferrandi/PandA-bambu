#!/bin/bash
script_dir="$(dirname $(readlink -e $0))"
. $script_dir/../../panda_regressions/hls/generic_getopt.sh

BATCH_ARGS=("--no-iob" "--simulate" "--experimental-setup=BAMBU")
OUT_SUFFIX="output_arf"

python3 $script_dir/../../etc/scripts/test_panda.py --tool=bambu  \
   --args="--configuration-name=arf ${BATCH_ARGS[*]}"\
   -larf_list \
   -o "out${OUT_SUFFIX}" -b$script_dir \
   --name="${OUT_SUFFIX}" "$@"
