#!/usr/bin/env bash
# Measure the three controlled copy baselines with ModelSim.
# Usage: BURST_MODELSIM_OUTSIDE_SANDBOX=1 run_copy_baseline_tests.sh BAMBU OUTPUT_DIR
set -Eeuo pipefail

script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)
repo_root=$(realpath "$script_dir/../../..")
source_file="$script_dir/copy_baseline.cpp"
testbench="$script_dir/copy_baseline_tb.cpp"
expectations="$script_dir/copy_cycle_expectations.tsv"

if (($# != 2)); then
  echo "usage: BURST_MODELSIM_OUTSIDE_SANDBOX=1 $0 BAMBU FRESH_OUTPUT_DIR" >&2
  exit 2
fi
if [[ ${BURST_MODELSIM_OUTSIDE_SANDBOX:-} != 1 ]]; then
  echo "refusing compiler/simulation unless BURST_MODELSIM_OUTSIDE_SANDBOX=1 is set; run outside the sandbox" >&2
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
command -v vsim >/dev/null || { echo "ModelSim vsim is required" >&2; exit 2; }
command -v vlog >/dev/null || { echo "ModelSim vlog is required" >&2; exit 2; }

prefix=$(dirname "$(dirname "$bambu")")
settings="$prefix/settings.sh"
plugin="$prefix/lib/panda/clang-16/ASTAnalyzer.so"
[[ -r $settings && -r $plugin ]] || { echo "installed settings/plugin missing below $prefix" >&2; exit 2; }
# shellcheck disable=SC1090
source "$settings"
[[ $(realpath -m "${BAMBU_HLS:-}") == $(realpath "$prefix") ]] || {
  echo "settings.sh selected an unexpected BAMBU_HLS=${BAMBU_HLS:-<unset>}" >&2; exit 2;
}

out_arg=$2
if [[ -e $out_arg || -L $out_arg ]]; then
  echo "output directory must be fresh and not a symlink: $out_arg" >&2; exit 2
fi
out=$(realpath -m -- "$out_arg")
mkdir -p "$out/tmp"
export TMPDIR="$out/tmp" TMP="$out/tmp" TEMP="$out/tmp"
export PYTHONDONTWRITEBYTECODE=1 PYTHONPYCACHEPREFIX="$out/tmp/pycache"

device="$repo_root/etc/libtech/xilinx/xcu250-2Lfigd2104.xml"
common=(--compiler=I386_CLANG16 --generate-interface=INFER --device-name=xcu250,-2L,figd2104
  --AXI-burst-type=INCREMENTAL --mem-delay-read=64 --mem-delay-write=2 --tb-queue-size=32
  --memory-allocation-policy=NO_BRAM --distram-threshold=0 --clock-period=3.33
  --no-clean --simulate --simulator=MODELSIM "--generate-tb=$testbench"
  --bambu-parameter=experimental-m-axi-burst=1 --extra-cc-options=-DNDEBUG
  --extra-cc-options=-DBAMBU)
{
  printf 'repo_root=%s\noutput=%s\nbambu=%s\nprefix=%s\nsettings=%s\nplugin=%s\n' \
    "$repo_root" "$out" "$bambu" "$prefix" "$(realpath "$settings")" "$(realpath "$plugin")"
  printf 'simulator=%s\nsimulator_version=' "$(realpath "$(command -v vsim)")"
  vsim -version 2>&1 | sed -n '1p' || true
  printf 'source=%s\ntestbench=%s\nexpectations=%s\n' "$source_file" "$testbench" "$expectations"
  printf 'argv='; printf '%q ' "$bambu" "$source_file" "${common[@]}"; printf '\n'
  printf 'TMPDIR=%s\nTMP=%s\nTEMP=%s\n' "$TMPDIR" "$TMP" "$TEMP"
  "$bambu" --version 2>&1 | sed -n '1,2p'
  sha256sum "$bambu" "$settings" "$plugin" "$source_file" "$testbench" "$expectations" \
    "$(command -v vsim)" "$(command -v vlog)"
} >"$out/provenance.txt"

run_case() {
  local case_name=$1 profile=$2 macro=$3
  local case_dir="$out/$case_name"
  mkdir "$case_dir"
  local args=("--top-fname=kernel" "${common[@]}" "--extra-cc-options=-DCOPY_BASELINE_PROFILE=$macro")
  {
    printf 'case=%s\nprofile=%s\nmacro=%s\nsource=%s\ntestbench=%s\nargv=' \
      "$case_name" "$profile" "$macro" "$source_file" "$testbench"
    printf '%q ' "$bambu" "$source_file" "${args[@]}"; printf '\n'
    sha256sum "$source_file" "$testbench" "$bambu" "$settings" "$plugin"
  } >"$case_dir/provenance.txt"
  if ! (cd "$case_dir" && "$bambu" "$source_file" "${args[@]}" >stdout.log 2>stderr.log); then
    tail -n 80 "$case_dir/stderr.log" >&2 || true
    tail -n 80 "$case_dir/stdout.log" >&2 || true
    echo "$case_name failed; see $case_dir" >&2
    return 1
  fi
  [[ -s $case_dir/bambu_results.xml ]] || { echo "$case_name missing bambu_results.xml" >&2; return 1; }
  local rtl_files
  rtl_files=$(find "$case_dir" -type f \( -name '*.v' -o -name '*.sv' \) ! -name panda_libtech.v -print -quit)
  [[ -n $rtl_files ]] || { echo "$case_name missing generated RTL" >&2; return 1; }
  while IFS= read -r -d '' artifact; do
    printf '%s  %s\n' "$(sha256sum "$artifact" | cut -d' ' -f1)" "$artifact"
  done < <(find "$case_dir" -type f \( -name '*.v' -o -name '*.sv' -o -name 'bambu_results.xml' \
    -o -path '*/HLS_output/simulation/testbench.log' \) -print0 | sort -z) \
    >"$case_dir/artifact-sha256.txt"
}

run_case copy-nonrestrict-two-bundles nonrestrict-two 1
run_case copy-legacy-two-bundles legacy-two 2
run_case copy-legacy-shared-bundle legacy-shared 3
python3 -B "$script_dir/check_copy_baseline_results.py" --results "$out" \
  --expectations "$expectations" --source "$source_file" --testbench "$testbench" \
  --bambu "$bambu" --settings "$settings" --plugin "$plugin"
printf 'COPY_BASELINES_PASS output=%s\n' "$out"
