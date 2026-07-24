#!/bin/bash
script_dir="$(dirname "$(readlink -e "$0")")"
BATCH_ARGS=("--generate-interface=INFER" "--device-name=nx2h540tsc" "--clock-period=20")
OUT_SUFFIX="bambu2hls4ml"

python3 "$script_dir/../../etc/scripts/mantis.py" --tool=bambu \
   --args="--configuration-name=CLANG16 --compiler=I386_CLANG16 ${BATCH_ARGS[*]}" \
   -lbambu2hls4ml_list \
   -o "out_${OUT_SUFFIX}" -b "$script_dir/bambu2hls4ml" \
   "$@"
exit $?
