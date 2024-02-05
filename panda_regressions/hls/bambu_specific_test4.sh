#!/bin/bash
script_dir="$(dirname $(readlink -e $0))"
BATCH_ARGS=("--simulate" "--generate-interface=INFER")
OUT_SUFFIX="bambu_specific_test4"

python3 $script_dir/../../etc/scripts/test_panda.py --tool=bambu  \
   --args="--configuration-name=BASE_CLANG6  --compiler=I386_CLANG6  --experimental-setup=BAMBU ${BATCH_ARGS[*]}" \
   --args="--configuration-name=BASE_CLANG16 --compiler=I386_CLANG16 --experimental-setup=BAMBU ${BATCH_ARGS[*]}" \
   --args="--configuration-name=BALANCED_MP_CLANG6  --compiler=I386_CLANG6  --experimental-setup=BAMBU-BALANCED-MP ${BATCH_ARGS[*]}" \
   --args="--configuration-name=BALANCED_MP_CLANG16 --compiler=I386_CLANG16 --experimental-setup=BAMBU-BALANCED-MP ${BATCH_ARGS[*]}" \
   -lbambu_specific_test4_list \
   -o "out_${OUT_SUFFIX}" -b$script_dir \
   --name="${OUT_SUFFIX}" "$@"
exit $?
