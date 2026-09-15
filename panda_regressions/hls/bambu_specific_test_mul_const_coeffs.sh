#!/bin/bash
script_dir="$(dirname $(readlink -e $0))"
EXTRA_ARGS=("$@")
BASE_ARGS=("-lm" "--simulate" "--experimental-setup=BAMBU" "--compiler=I386_CLANG13" "-v1" \
   "--bambu-parameter=constmul_enable=1" \
   "--bambu-parameter=lut-transformation=1")
OUT_SUFFIX="bambu_mul_const_coeff_test"

run_case() {
   local case_name="$1"
   shift
   local out_dir="out_${OUT_SUFFIX}_${case_name}"

   python3 "$script_dir/../../etc/scripts/mantis.py" --tool=bambu \
      --args="${BASE_ARGS[*]} $*" \
      -lbambu_mul_const_coeff_list \
      -o "out_${OUT_SUFFIX}_${case_name}" -b "$script_dir/bambu_mul_const_coeff_test" \
      "${EXTRA_ARGS[@]}"
   local status=$?
   if [ $status -ne 0 ]; then
      exit $status
   fi
}

run_case "shiftadd_default" \
   "--bambu-parameter=constmul_kcm_enable=0" \
   "--bambu-parameter=constmul_try_factor_forms=1" \
   "--bambu-parameter=constmul_enable_small_factor_chains=1"

run_case "shiftadd_no_factors" \
   "--bambu-parameter=constmul_kcm_enable=0" \
   "--bambu-parameter=constmul_try_factor_forms=0" \
   "--bambu-parameter=constmul_enable_small_factor_chains=0"

run_case "kcm_tree" \
   "--bambu-parameter=constmul_kcm_enable=1" \
   "--bambu-parameter=constmul_max_terms=0" \
   "--bambu-parameter=constmul_dsp_scale_k=500" \
   "--bambu-parameter=constmul_kcm_alpha=6" \
   "--bambu-parameter=constmul_kcm_sum_strategy=tree" \
   "--bambu-parameter=constmul_kcm_merge_table_add=0"

run_case "kcm_rake_merge" \
   "--bambu-parameter=constmul_kcm_enable=1" \
   "--bambu-parameter=constmul_max_terms=0" \
   "--bambu-parameter=constmul_dsp_scale_k=500" \
   "--bambu-parameter=constmul_kcm_alpha=6" \
   "--bambu-parameter=constmul_kcm_sum_strategy=rake" \
   "--bambu-parameter=constmul_kcm_merge_table_add=1"

echo "Constmul coefficient regression checks completed."
exit 0
