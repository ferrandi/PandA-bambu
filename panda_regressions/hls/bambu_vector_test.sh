#!/bin/bash
script_dir="$(dirname "$(readlink -e "$0")")"
BATCH_ARGS=("--generate-interface=INFER" "--simulate")

python3 "$script_dir/../../etc/scripts/mantis.py" --tool=bambu \
   --args="--configuration-name=O2-VECTOR-CLANG6 --compiler=I386_CLANG6 -O2 --experimental-setup=BAMBU ${BATCH_ARGS[*]}" \
   --args="--configuration-name=O2-VECTOR-MP-CLANG6 --compiler=I386_CLANG6 -O2 --experimental-setup=BAMBU-BALANCED-MP ${BATCH_ARGS[*]}" \
   --args="--configuration-name=O2-VECTOR-CLANG16 --compiler=I386_CLANG16 -O2 --experimental-setup=BAMBU ${BATCH_ARGS[*]}" \
   --args="--configuration-name=O2-VECTOR-MP-CLANG16 --compiler=I386_CLANG16 -O2 --experimental-setup=BAMBU-BALANCED-MP ${BATCH_ARGS[*]}" \
   -l bambu_vector_test_list -b "$script_dir/bambu_vector_test" \
   -o out_bambu_vector_test "$@"
exit $?
