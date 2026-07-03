#!/bin/bash
script_dir="$(dirname $(readlink -e $0))"
BATCH_ARGS=("--simulate --generate-interface=INFER")
BATCH_ARGS_OPAQUE=("--simulate --generate-interface=INFER" --bambu-parameter=opaque-pointers=1)
OUT_SUFFIX="bambu_specific_test7"

python3 $script_dir/../../etc/scripts/mantis.py --tool=bambu  \
   --args="--configuration-name=CLANG16 --compiler=I386_CLANG16 ${BATCH_ARGS[*]}" \
   --args="--configuration-name=CLANG16_OPAQUE --compiler=I386_CLANG16 ${BATCH_ARGS_OPAQUE[*]}" \
   -lbambu_specific_test7_list \
   -o "out_${OUT_SUFFIX}" -b "$script_dir/bambu_specific_test7" \
   "$@"

status=$?
if [[ $status -ne 0 ]]; then
   exit $status
fi

for conf in CLANG16 CLANG16_OPAQUE; do
   log="out_${OUT_SUFFIX}/${conf}/template_inline_pragma/execution.log"
   if [[ -f "$log" ]] && grep -Fq "HLS pragma not associated to any decl" "$log"; then
      echo "Unexpected HLS pragma association warning in ${log}" >&2
      exit 1
   fi
done

exit 0
