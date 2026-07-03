#!/bin/bash
script_dir="$(dirname $(readlink -e $0))"
BATCH_ARGS=("--simulate" "--expose-globals" "--clock-period=5")
OUT_SUFFIX="bambu_specific_test5"

python3 $script_dir/../../etc/scripts/mantis.py --tool=bambu  \
   --args="--configuration-name=CLANG13 --compiler=I386_CLANG13 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=CLANG11 --compiler=I386_CLANG11 ${BATCH_ARGS[*]}" \
   -lbambu_specific_test5_list \
   -o "out_${OUT_SUFFIX}" -b "$script_dir/bambu_specific_test5" \
   "$@"
exit $?
