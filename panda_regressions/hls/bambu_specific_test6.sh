#!/bin/bash
script_dir="$(dirname $(readlink -e $0))"
BATCH_ARGS=("--simulate" "--experimental-setup=BAMBU" "--expose-globals")
OUT_SUFFIX="bambu_specific_test6"

python3 $script_dir/../../etc/scripts/mantis.py --tool=bambu  \
   --args="--configuration-name=CLANG11_O0 -O0 --compiler=I386_CLANG11 ${BATCH_ARGS[*]}" \
   -lbambu_specific_test6_list \
   -o "out_${OUT_SUFFIX}" -b "$script_dir/bambu_specific_test6" \
   "$@"
exit $?

