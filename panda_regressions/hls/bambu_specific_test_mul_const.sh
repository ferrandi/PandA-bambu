#!/bin/bash
script_dir="$(dirname $(readlink -e $0))"
BATCH_ARGS=("-lm" "--simulate" "--experimental-setup=BAMBU" "--compiler=I386_CLANG13" "-v1")
OUT_SUFFIX="bambu_mul_const_test"
out_dir="out_${OUT_SUFFIX}"

python3 $script_dir/../../etc/scripts/mantis.py --tool=bambu  \
   --args="${BATCH_ARGS[*]}" \
   -lbambu_mul_const_test_list \
   -o "out_${OUT_SUFFIX}" -b "$script_dir/bambu_mul_const_test" \
   "$@"
status=$?
if [ $status -ne 0 ]; then
   exit $status
fi

conf_dir="${out_dir}/conf_0"
expected_expanded=(
   "mul_pow2"
   "mul_neg_pow2"
   "mul_neg_pow2_negop"
   "mul_small_3"
   "mul_small_3_negop"
   "mul_neg_3"
   "mul_neg_3_negop"
   "mul_small_5"
   "mul_small_7"
   "mul_small_9"
   "mul_pow2_minus1"
   "mul_neg_pow2_minus1"
   "mul_pow2_plus1"
   "mul_neg_pow2_plus1"
   "mul_sparse"
   "mul_balanced_case"
   "mul_balanced_case_neg"
   "umul_pow2"
   "umul_pow2_minus1"
   "umul_pow2_plus1"
)
expected_balanced=(
   "mul_balanced_case"
   "mul_balanced_case_neg"
)
expected_simplified=(
   "mul_zero"
   "mul_one"
   "mul_neg_one"
)
expected_terms2=(
   "mul_small_3"
   "mul_small_3_negop"
   "mul_neg_3"
   "mul_neg_3_negop"
   "mul_small_5"
   "mul_small_7"
   "mul_small_9"
)

missing=0
for bench in "${expected_expanded[@]}"; do
   log="${conf_dir}/${bench}/execution.log"
   if [ ! -f "$log" ]; then
      echo "Missing execution log for ${bench}: ${log}" >&2
      missing=1
      continue
   fi
   if grep -Eq "constmul: coeff=.*-> (shift|expand)" "$log"; then
      continue
   fi
   if grep -Eq "mul_node_FU" "$log"; then
      echo "Expected constmul expansion not observed for ${bench}" >&2
      missing=1
      continue
   fi
   if ! grep -Eq "(shl_node_FU|add_node_FU|sub_node_FU)" "$log"; then
      echo "Expected constmul expansion evidence not found for ${bench}" >&2
      missing=1
   fi
done

for bench in "${expected_simplified[@]}"; do
   log="${conf_dir}/${bench}/execution.log"
   if [ ! -f "$log" ]; then
      echo "Missing execution log for ${bench}: ${log}" >&2
      missing=1
      continue
   fi
   if grep -Eq "mul_node_FU" "$log"; then
      echo "Expected constmul simplification not observed for ${bench}" >&2
      missing=1
   fi
   if [[ "$bench" == "mul_neg_one" ]] && ! grep -Eq "neg_node_FU" "$log"; then
      echo "Expected negate lowering not observed for ${bench}" >&2
      missing=1
   fi
done

for bench in "${expected_balanced[@]}"; do
   log="${conf_dir}/${bench}/execution.log"
   if [ ! -f "$log" ]; then
      echo "Missing execution log for ${bench}: ${log}" >&2
      missing=1
      continue
   fi
   if ! grep -Eq "constmul: coeff=.*balanced yes" "$log"; then
      echo "Expected balanced constmul expansion not observed for ${bench}" >&2
      missing=1
   fi
done

for bench in "${expected_terms2[@]}"; do
   log="${conf_dir}/${bench}/execution.log"
   if [ ! -f "$log" ]; then
      echo "Missing execution log for ${bench}: ${log}" >&2
      missing=1
      continue
   fi
   if ! grep -Eq "constmul: coeff=.*terms 2" "$log"; then
      echo "Expected 2-term constmul expansion not observed for ${bench}" >&2
      missing=1
   fi
done

if [ $missing -ne 0 ]; then
   exit 1
fi

echo "Constmul expansion checks passed."
exit 0
