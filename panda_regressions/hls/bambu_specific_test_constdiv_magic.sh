#!/bin/bash
script_dir="$(dirname $(readlink -e $0))"
BATCH_ARGS=("-lm" "--simulate" "--experimental-setup=BAMBU" "--compiler=I386_CLANG13" "-v1" "--bambu-parameter=constdiv_lowering_mode=llvm_magic")
OUT_SUFFIX="bambu_constdiv_magic_test"
out_dir="out_${OUT_SUFFIX}"

python3 $script_dir/../../etc/scripts/mantis.py --tool=bambu  \
   --args="${BATCH_ARGS[*]}" \
   -lbambu_constdiv_magic_test_list \
   -o "out_${OUT_SUFFIX}" -b "$script_dir/bambu_constdiv_magic_test" \
   "$@"
status=$?
if [ $status -ne 0 ]; then
   exit $status
fi

conf_dir="${out_dir}/conf_0"
expected=(
   "udiv_pow2_u32"
   "urem_pow2_u32"
   "udiv_3_u32"
   "urem_3_u32"
   "udiv_10_u32"
   "urem_10_u32"
   "udiv_12_u32"
   "urem_12_u32"
   "udiv_255_u32"
   "urem_255_u32"
   "udiv_5_u64"
   "urem_5_u64"
   "udiv_1024_u64"
   "urem_1024_u64"
   "sdiv_3_s32"
   "srem_3_s32"
   "sdiv_neg3_s32"
   "srem_neg3_s32"
   "sdiv_7_s32"
   "srem_7_s32"
   "sdiv_5_s64"
   "srem_5_s64"
   "sdiv_neg7_s64"
   "srem_neg7_s64"
)

missing=0
for bench in "${expected[@]}"; do
   log="${conf_dir}/${bench}/execution.log"
   if [ ! -f "$log" ]; then
      echo "Missing execution log for ${bench}: ${log}" >&2
      missing=1
      continue
   fi
   if grep -Eq "i(div|rem)_node_FU" "$log"; then
      echo "Unexpected trunc div/mod FU in ${bench}" >&2
      missing=1
   fi
done

if [ $missing -ne 0 ]; then
   exit 1
fi

echo "Constdiv llvm-magic checks passed."
exit 0
