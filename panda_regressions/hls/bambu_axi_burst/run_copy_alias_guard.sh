#!/usr/bin/env bash
# Compiler-only A/B guard for cross-bundle burst coalescing (P1 review item).
#
# copy_baseline.cpp profile 1 declares two distinct m_axi bundles (input reads, output writes) with
# burst attributes, but its signature carries no __restrict: the two pointer arguments may overlap,
# so the disjointness of the two ranges is not proven and the burst engines must stay inactive.
# copy_baseline_restrict.cpp is the same kernel with the caller's restrict contract, and there both
# engines must be instantiated. The guard only runs Bambu and inspects the generated Verilog.
#
# Usage: BURST_ALIAS_GUARD_OUTSIDE_SANDBOX=1 run_copy_alias_guard.sh BAMBU OUTPUT_DIR
set -Eeuo pipefail

script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)
if (($# != 2)); then
  echo "usage: BURST_ALIAS_GUARD_OUTSIDE_SANDBOX=1 $0 BAMBU FRESH_OUTPUT_DIR" >&2
  exit 2
fi
if [[ ${BURST_ALIAS_GUARD_OUTSIDE_SANDBOX:-} != 1 ]]; then
  echo "refusing to run Bambu unless BURST_ALIAS_GUARD_OUTSIDE_SANDBOX=1 is set; run outside the sandbox" >&2
  exit 2
fi

bambu_arg=$1
if [[ $bambu_arg == */* ]]; then
  bambu=$(realpath "$bambu_arg" 2>/dev/null || true)
else
  bambu=$(command -v "$bambu_arg" || true)
  [[ -z $bambu ]] || bambu=$(realpath "$bambu")
fi
[[ -n ${bambu:-} && -x $bambu ]] || { echo "Bambu executable not found: $bambu_arg" >&2; exit 2; }
out=$2
if [[ -e $out ]]; then
  echo "output directory must be fresh: $out" >&2
  exit 2
fi
bambu_prefix=$(dirname "$(dirname "$bambu")")
# shellcheck disable=SC1090
source "$bambu_prefix/settings.sh"
[[ $(realpath -m "${BAMBU_HLS:-}") == $(realpath "$bambu_prefix") ]] || {
  echo "settings.sh selected BAMBU_HLS='${BAMBU_HLS:-<unset>}' instead of $bambu_prefix" >&2
  exit 2
}

common=(--top-fname=kernel --compiler=I386_CLANG16 --generate-interface=INFER
  --device-name=xcu250,-2L,figd2104 --clock-period=3.33 --AXI-burst-type=INCREMENTAL
  --memory-allocation-policy=NO_BRAM --distram-threshold=0
  --bambu-parameter=experimental-m-axi-burst=1 --extra-cc-options=-DNDEBUG
  --extra-cc-options=-DBAMBU)

run_variant() {
  local name=$1 source=$2 macro=$3
  mkdir -p "$out/$name"
  (cd "$out/$name" && TMPDIR="$out/$name" "$bambu" "$source" "${common[@]}" \
    --extra-cc-options="-D$macro" > bambu.log 2>&1) || {
    tail -n 40 "$out/$name/bambu.log" >&2
    echo "$name: Bambu failed" >&2
    return 1
  }
  local read write
  read=$(grep -c "MinimalAXI4MasterPipelined #" "$out/$name/kernel.v" || true)
  write=$(grep -c "MinimalAXI4MasterPipelinedWrite #" "$out/$name/kernel.v" || true)
  printf '%s engines=%s\n' "$name" "$((read + write))"
  grep -oE "m_axi_(input|output)_awvalid" "$out/$name/kernel.v" | sort -u | tr '\n' ' '
  printf '\n'
  [[ $name == non-restrict ]] && (( read == 0 && write == 0 )) && return 0
  [[ $name == restrict ]] && (( read == 1 && write == 1 )) && return 0
  echo "$name: unexpected burst engine population read=$read write=$write" >&2
  return 1
}

run_variant non-restrict "$script_dir/copy_baseline.cpp" COPY_BASELINE_PROFILE=1
run_variant restrict "$script_dir/copy_baseline_restrict.cpp" COPY_BASELINE_RESTRICT_PROFILE=1
printf 'COPY_ALIAS_GUARD_PASS non_restrict_engines=0 restrict_engines=2\n'
