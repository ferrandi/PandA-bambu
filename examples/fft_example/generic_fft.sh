#!/bin/bash
script_dir="$(dirname $(readlink -e $0))"
. $script_dir/../../panda_regressions/hls/generic_getopt.sh

BATCH_ARGS=("--simulate" "-Os" "-ffast-math" "-lm_newlib" "--experimental-setup=BAMBU")
OUT_SUFFIX="output_fft"

python3 $script_dir/../../etc/scripts/mantis.py --tool=bambu  \
   --args="--configuration-name=fft ${BATCH_ARGS[*]}" \
   -lfft_list \
   -o "out_${OUT_SUFFIX}" -b$script_dir \
   "$@"
