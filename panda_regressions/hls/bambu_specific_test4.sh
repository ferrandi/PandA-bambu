#!/bin/bash

BATCH_ARGS=("--generate-interface=INFER" "-lm" "--simulate" "--expose-globals" "--compiler=I386_CLANG6")
OUT_SUFFIX="bambu_specific_test4"

$(dirname $0)/../../etc/scripts/test_panda.py --tool=bambu  \
   --args="--configuration-name=O2-INFER -O2 --experimental-setup=BAMBU ${BATCH_ARGS[*]}" \
   --args="--configuration-name=O2-INFER-MP -O2 --experimental-setup=BAMBU-BALANCED-MP -funroll-loops ${BATCH_ARGS[*]}" \
   -lbambu_specific_test4_list \
   -o "output_${OUT_SUFFIX}" -b$(dirname $0) \
   --table="${OUT_SUFFIX}.tex" \
   --csv="${OUT_SUFFIX}.csv" \
   --name="${OUT_SUFFIX}" "$@"
exit $?