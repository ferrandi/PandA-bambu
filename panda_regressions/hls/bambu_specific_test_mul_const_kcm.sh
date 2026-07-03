#!/bin/bash
script_dir="$(dirname $(readlink -e $0))"
EXTRA_ARGS=("$@")
BASE_ARGS=("-lm" "--simulate" "--experimental-setup=BAMBU" "--compiler=I386_CLANG13" "-v1" \
   "--bambu-parameter=constmul_kcm_enable=1" \
   "--bambu-parameter=constmul_max_terms=0" \
   "--bambu-parameter=constmul_dsp_scale_k=500" \
   "--bambu-parameter=lut-transformation=1")
OUT_SUFFIX="bambu_mul_const_kcm_test"

expected=(
   "kcm_mul_small_3"
   "kcm_mul_small_3_negop"
   "kcm_mul_neg_3"
   "kcm_mul_small_5"
   "kcm_mul_small_7"
)

run_case() {
   local case_name="$1"
   shift
   local out_dir="out_${OUT_SUFFIX}_${case_name}"
   rm -rf "${out_dir}"

   python3 "$script_dir/../../etc/scripts/mantis.py" --tool=bambu \
      --args="${BASE_ARGS[*]} $*" \
      --summary-suffix="_${case_name}" \
      -lbambu_mul_const_kcm_test_list \
      -o "out_${OUT_SUFFIX}_${case_name}" -b "$script_dir/bambu_mul_const_test" \
      "${EXTRA_ARGS[@]}"
   local status=$?
   if [ $status -ne 0 ]; then
      exit $status
   fi

   local conf_dir="${out_dir}/conf_0"
   local missing=0
   for bench in "${expected[@]}"; do
      local log="${conf_dir}/${bench}/execution.log"
      if [ ! -f "$log" ]; then
         echo "Missing execution log for ${bench}: ${log}" >&2
         missing=1
         continue
      fi
      if ! grep -Eq "constmul: coeff=.*-> kcm" "$log"; then
         echo "Expected KCM selection not observed for ${bench} (${case_name})" >&2
         missing=1
      fi
      if ! grep -Eq "lut_node_FU" "$log"; then
         echo "Expected lut_node usage not observed for ${bench} (${case_name})" >&2
         missing=1
      fi
   done

   if [ $missing -ne 0 ]; then
      exit 1
   fi
}

run_case "alpha6_tree" \
   "--bambu-parameter=constmul_kcm_alpha=6" \
   "--bambu-parameter=constmul_kcm_sum_strategy=tree" \
   "--bambu-parameter=constmul_kcm_merge_table_add=0"

run_case "alpha6_rake" \
   "--bambu-parameter=constmul_kcm_alpha=6" \
   "--bambu-parameter=constmul_kcm_sum_strategy=rake" \
   "--bambu-parameter=constmul_kcm_merge_table_add=0"

run_case "alpha6_merge" \
   "--bambu-parameter=constmul_kcm_alpha=6" \
   "--bambu-parameter=constmul_kcm_sum_strategy=rake" \
   "--bambu-parameter=constmul_kcm_merge_table_add=1"

run_case "alpha4_tree" \
   "--bambu-parameter=constmul_kcm_alpha=4" \
   "--bambu-parameter=constmul_kcm_sum_strategy=tree" \
   "--bambu-parameter=constmul_kcm_merge_table_add=0"

echo "Constmul KCM checks passed."
exit 0
