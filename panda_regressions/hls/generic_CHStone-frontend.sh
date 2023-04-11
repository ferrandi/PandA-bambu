#!/bin/bash

script_dir="$(dirname $(readlink -e $0))"
ggo_require_compiler=1
. $script_dir/generic_getopt.sh

BATCH_ARGS=("--simulate" "-fwhole-program" "--clock-period=15" "-D'printf(fmt, ...)='" "--channels-type=MEM_ACC_NN" "--experimental-setup=BAMBU")
OUT_SUFFIX="${compiler}_CHStone-frontend"

python3 $script_dir/../../etc/scripts/test_panda.py --tool=bambu \
   --args="--configuration-name=${compiler}-O0-wp-NN -O0 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=${compiler}-O1-wp-NN -O1 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=${compiler}-O2-wp-NN -O2 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=${compiler}-O3-wp-NN -O3 ${BATCH_ARGS[*]}" \
   -lCHStone_list \
   -o "out_${OUT_SUFFIX}" -b$script_dir \
   --name="${OUT_SUFFIX}" "$@"
