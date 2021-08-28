#!/bin/bash

script_dir="$(dirname $(readlink -e $0))"

BATCH_ARGS=("--simulate" "-O0" "-fwhole-program" "--clock-period=15" "-D'printf(fmt, ...)='" "--channels-type=MEM_ACC_NN" "--experimental-setup=BAMBU")
OUT_SUFFIX="multi_CHStone-frontend"

$script_dir/../../etc/scripts/test_panda.py --tool=bambu \
   --args="--configuration-name=GCC49-O0-wp-NN ${BATCH_ARGS[*]}" \
   --args="--configuration-name=CLANG4-O0-wp-NN ${BATCH_ARGS[*]}" \
   --args="--configuration-name=CLANG5-O0-wp-NN ${BATCH_ARGS[*]}" \
   --args="--configuration-name=CLANG6-O0-wp-NN ${BATCH_ARGS[*]}" \
   --args="--configuration-name=CLANG7-O0-wp-NN ${BATCH_ARGS[*]}" \
   -lCHStone_list \
   -o "output_${OUT_SUFFIX}" -b$script_dir \
   --name="${OUT_SUFFIX}" $@
