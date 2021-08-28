#!/bin/bash

. $script_dir/generic_getopt.sh

BATCH_ARGS=("--generate-interface=INFER" "-lm" "--simulate" "--expose-globals" "--compiler=I386_CLANG6")
OUT_SUFFIX="bambu_specific_test4"

$(dirname $0)/../../etc/scripts/test_panda.py --tool=bambu  \
   --args="--configuration-name=O2-INFER -O2 --experimental-setup=BAMBU ${BATCH_ARGS[*]}" \
   --args="--configuration-name=O2-INFER-MP -O2 --experimental-setup=BAMBU-BALANCED-MP ${BATCH_ARGS[*]}" \
   -lbambu_specific_test4_list \
   -o "output_${OUT_SUFFIX}" -b$(dirname $0) \
   --name="${OUT_SUFFIX}" "$@"
exit $?
