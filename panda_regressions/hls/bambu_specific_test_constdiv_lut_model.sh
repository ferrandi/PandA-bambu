#!/bin/bash
script_dir="$(dirname $(readlink -e $0))"
EXTRA_ARGS=("$@")
BASE_ARGS=("-lm" "--simulate" "--experimental-setup=BAMBU" "--compiler=I386_CLANG13" "-v1")
OUT_SUFFIX="bambu_constdiv_lut_model_test"

expected=(
   "udiv_5_u32"
   "urem_5_u32"
   "udiv_23_u32"
   "urem_23_u32"
   "udiv_9_u64"
   "urem_9_u64"
)

run_case() {
   local case_name="$1"
   local mode="$2"
   local lut_model="$3"
   local expected_lowering="$4"
   local out_dir="out_${OUT_SUFFIX}_${case_name}"
   rm -rf "${out_dir}"

   python3 "$script_dir/../../etc/scripts/mantis.py" --tool=bambu \
      --args="${BASE_ARGS[*]} --bambu-parameter=constdiv_lowering_mode=${mode} --bambu-parameter=constmultdiv_lut_cost_model=${lut_model}" \
      --summary-suffix="_${case_name}" \
      -lbambu_constdiv_lut_model_test_list \
      -o "out_${OUT_SUFFIX}_${case_name}" -b "$script_dir/bambu_constdiv_lut_model_test" \
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
      if grep -Eq "i(div|rem)_node_FU" "$log"; then
         echo "Unexpected trunc div/mod FU in ${bench} (${case_name})" >&2
         missing=1
      fi
      if ! grep -Fq "${expected_lowering}" "$log"; then
         echo "Expected lowering trace '${expected_lowering}' not found for ${bench} (${case_name})" >&2
         missing=1
      fi
   done

   if [ $missing -ne 0 ]; then
      exit 1
   fi
}

run_case "linarch_mockturtle" "linarch" "mockturtle" "constdiv(u): linarch D="
run_case "linarch_mockturtle_full" "linarch" "mockturtle_full" "constdiv(u): linarch D="
run_case "btcd_mockturtle" "btcd" "mockturtle" "constdiv(u): btcd D="
run_case "btcd_mockturtle_full" "btcd" "mockturtle_full" "constdiv(u): btcd D="

echo "Constdiv LUT cost model checks passed."
exit 0
