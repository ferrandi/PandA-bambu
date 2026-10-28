#!/bin/bash
script_dir="$(dirname $(readlink -e $0))"
BATCH_ARGS=("-lm" "--simulate" "--experimental-setup=BAMBU" "--compiler=I386_CLANG13" "-v1"
   "--bambu-parameter=constdiv_lowering_mode=linarch"
   "--bambu-parameter=constdiv_composite_enable=1"
   "--bambu-parameter=constdiv_composite_margin=-1000000.0")
OUT_SUFFIX="bambu_constdiv_composite_test"
out_dir="out_${OUT_SUFFIX}"

python3 $script_dir/../../etc/scripts/mantis.py --tool=bambu  \
   --args="${BATCH_ARGS[*]}" \
   -lbambu_constdiv_composite_test_list \
   -o "out_${OUT_SUFFIX}" -b "$script_dir/bambu_constdiv_composite_test" \
   "$@"
status=$?
if [ $status -ne 0 ]; then
   exit $status
fi

conf_dir="${out_dir}/conf_0"
expected=(
   "udiv_9_u32"
   "urem_9_u32"
   "udiv_15_u32"
   "urem_15_u32"
   "udiv_21_u32"
   "urem_21_u32"
   "udiv_25_u32"
   "urem_25_u32"
   "udiv_45_u32"
   "urem_45_u32"
   "udiv_9_u64"
   "urem_9_u64"
)

missing=0
composite_hits=0
mod_hits=0
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
   if grep -Fq "constdiv composite(u): split D=" "$log"; then
      composite_hits=$((composite_hits + 1))
      if [[ "$bench" == urem_* ]]; then
         mod_hits=$((mod_hits + 1))
      fi
   fi
done

if [ $missing -ne 0 ] || [ $composite_hits -eq 0 ] || [ $mod_hits -eq 0 ]; then
   if [ $composite_hits -eq 0 ]; then
      echo "Missing composite split in all benchmarks." >&2
   fi
   if [ $mod_hits -eq 0 ]; then
      echo "Missing composite split in modulo benchmarks." >&2
   fi
   exit 1
fi

echo "Constdiv composite checks passed."
exit 0
