#!/bin/bash
script_dir="$(dirname $(readlink -e $0))"
BATCH_ARGS=("--simulate" "--experimental-setup=BAMBU" "--expose-globals")
OUT_SUFFIX="bambu_specific_test_modelsim"

python3 $script_dir/../../etc/scripts/test_panda.py --tool=bambu  \
   --args="--configuration-name=CLANG11_O0 -O0 --compiler=I386_CLANG11 ${BATCH_ARGS[*]}" \
   -lbambu_specific_test_modelsim_list \
   -o "out_${OUT_SUFFIX}" -b$script_dir \
   --name="${OUT_SUFFIX}" "$@"
exit $?

