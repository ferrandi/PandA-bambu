#!/bin/bash
script_dir="$(dirname $(readlink -e $0))"
BATCH_ARGS=("--generate-interface=INFER" "-lm" "--simulate")
OUT_SUFFIX="bambu_specific_test4"

python3 $script_dir/../../etc/scripts/test_panda.py --tool=bambu  \
   --args="--configuration-name=O2-INFER-CLANG6 --compiler=I386_CLANG6 -O2 --experimental-setup=BAMBU ${BATCH_ARGS[*]}" \
   --args="--configuration-name=O2-INFER-MP-CLANG6 --compiler=I386_CLANG6 -O2 --experimental-setup=BAMBU-BALANCED-MP ${BATCH_ARGS[*]}" \
   --args="--configuration-name=O2-INFER-CLANG16 --compiler=I386_CLANG16 -O2 --experimental-setup=BAMBU ${BATCH_ARGS[*]}" \
   --args="--configuration-name=O2-INFER-MP-CLANG16 --compiler=I386_CLANG16 -O2 --experimental-setup=BAMBU-BALANCED-MP ${BATCH_ARGS[*]}" \
   -lbambu_specific_test4_list \
   -o "out_${OUT_SUFFIX}" -b$script_dir \
   --name="${OUT_SUFFIX}" "$@"
exit $?
