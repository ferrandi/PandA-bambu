#!/bin/bash
script_dir="$(dirname $(readlink -e $0))"
BATCH_ARGS=("--simulate" "--expose-globals" "--clock-period=5")
OUT_SUFFIX="bambu_specific_test5"

python3 $script_dir/../../etc/scripts/test_panda.py --tool=bambu  \
   --args="--configuration-name=CLANG4 --compiler=I386_CLANG4 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=CLANG11 --compiler=I386_CLANG11 ${BATCH_ARGS[*]}" \
   -lbambu_specific_test5_list \
   -o "out_${OUT_SUFFIX}" -b$script_dir \
   --name="${OUT_SUFFIX}" "$@"
exit $?
