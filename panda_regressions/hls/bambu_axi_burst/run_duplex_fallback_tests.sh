#!/usr/bin/env bash
# Manually qualify generated fallback RTL for duplex-ineligible kernels.
# Usage: BURST_DUPLEX_FALLBACK_SIM_OUTSIDE_SANDBOX=1 run_duplex_fallback_tests.sh BAMBU [FRESH_OUTPUT_DIR]
set -Eeuo pipefail

script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)
repo_root=$(git -C "$script_dir/../../.." rev-parse --show-toplevel)
runner_file=$(realpath "${BASH_SOURCE[0]}")
source_file="$script_dir/bambu_axi_burst_duplex_fallback.cpp"
testbench="$script_dir/duplex_fallback_tb.cpp"
checker="$script_dir/check_duplex_fallback_results.py"
helpers="$script_dir/parallel_helpers.sh"

usage() {
  echo "usage: BURST_DUPLEX_FALLBACK_SIM_OUTSIDE_SANDBOX=1 $0 BAMBU [FRESH_OUTPUT_DIR]" >&2
}

if (($# < 1 || $# > 2)); then usage; exit 2; fi
if [[ ${BURST_DUPLEX_FALLBACK_SIM_OUTSIDE_SANDBOX:-} != 1 ]]; then
  echo "refusing compiler/simulation unless BURST_DUPLEX_FALLBACK_SIM_OUTSIDE_SANDBOX=1 is set; run outside the sandbox" >&2
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
command -v verilator >/dev/null || { echo "Verilator is required" >&2; exit 2; }
command -v rg >/dev/null || { echo "ripgrep is required to validate scenario markers" >&2; exit 2; }
for tool in python3 sha256sum; do command -v "$tool" >/dev/null || { echo "$tool is required" >&2; exit 2; }; done
[[ -r $source_file && -r $testbench && -r $checker && -r $helpers ]] || {
  echo "duplex fallback source, testbench, checker, or parallel helper is missing" >&2; exit 2;
}

prefix=$(dirname "$(dirname "$bambu")")
settings="$prefix/settings.sh"
[[ -r $settings ]] || { echo "installed settings.sh missing below $prefix" >&2; exit 2; }
# shellcheck disable=SC1090
source "$settings"
[[ $(realpath -m "${BAMBU_HLS:-}") == $(realpath "$prefix") ]] || {
  echo "settings.sh selected an unexpected BAMBU_HLS=${BAMBU_HLS:-<unset>}" >&2; exit 2;
}
plugin="$(realpath "$BAMBU_HLS")/lib/panda/clang-16/ASTAnalyzer.so"
[[ -r $plugin ]] || { echo "selected ASTAnalyzer plugin is missing: $plugin" >&2; exit 2; }
simulator=$(realpath "$(command -v verilator)")

requested_jobs=${DUPLEX_FALLBACK_JOBS:-2}
if [[ ! $requested_jobs =~ ^[0-9]+$ ]]; then
  echo "DUPLEX_FALLBACK_JOBS must be a positive integer (got '$requested_jobs')" >&2; exit 2
fi
while [[ $requested_jobs == 0* && ${#requested_jobs} -gt 1 ]]; do requested_jobs=${requested_jobs#0}; done
if [[ $requested_jobs == 0 || ${#requested_jobs} -gt 10 ]] || \
  { ((${#requested_jobs} == 10)) && [[ $requested_jobs > 2147483647 ]]; }; then
  echo "DUPLEX_FALLBACK_JOBS must be between 1 and 2147483647 (got '${DUPLEX_FALLBACK_JOBS:-}')" >&2; exit 2
fi
requested_jobs=$((10#$requested_jobs))

out_arg=${2:-"$PWD/out_duplex_fallback"}
if [[ -e $out_arg || -L $out_arg ]]; then
  echo "output directory must be fresh and not a symlink: $out_arg" >&2; exit 2
fi
out=$(realpath -m -- "$out_arg")

device="$repo_root/etc/libtech/xilinx/xcu250-2Lfigd2104.xml"
[[ -r $device ]] || { echo "device description missing: $device" >&2; exit 2; }
common=(--top-fname=bambu_axi_burst_duplex_fallback --compiler=I386_CLANG16
  --generate-interface=INFER --device-name=xcu250,-2L,figd2104
  --AXI-burst-type=INCREMENTAL --mem-delay-read=64 --mem-delay-write=2 --tb-queue-size=32
  --debug-classes=InterfaceInfer,BuildVirtualPhi,parametric_list_based
  --memory-allocation-policy=NO_BRAM --distram-threshold=0 --clock-period=3.33
  --no-clean --simulate --simulator=VERILATOR "--generate-tb=$testbench"
  --tb-param-size=data:16384 --extra-cc-options=-DNDEBUG --extra-cc-options=-DBAMBU)

mkdir "$out"
out=$(realpath "$out")
mkdir "$out/job_logs" "$out/tmp"
export TMPDIR="$out/tmp" TMP="$out/tmp" TEMP="$out/tmp"
export PYTHONDONTWRITEBYTECODE=1

digest() { sha256sum "$1" | cut -d' ' -f1; }
git_head=$(git -C "$repo_root" rev-parse HEAD)
{
  printf 'git_head=%s\ninvocation=' "$git_head"
  printf '%q ' "$0" "$bambu_arg" "${2:-$PWD/out_duplex_fallback}"
  printf '\nrepo_root=%s\noutput=%s\nbambu=%s\nsettings=%s\nplugin=%s\nsimulator=%s\njobs=%s\n' \
    "$repo_root" "$out" "$bambu" "$(realpath "$settings")" "$plugin" "$simulator" "$requested_jobs"
  printf 'source=%s\nsource_sha256=%s\ntestbench=%s\ntestbench_sha256=%s\nchecker=%s\nchecker_sha256=%s\nrunner=%s\nrunner_sha256=%s\n' \
    "$source_file" "$(digest "$source_file")" "$testbench" "$(digest "$testbench")" \
    "$checker" "$(digest "$checker")" "$runner_file" "$(digest "$runner_file")"
  printf 'bambu_sha256=%s\nsettings_sha256=%s\nplugin_sha256=%s\nsimulator_sha256=%s\n' \
    "$(digest "$bambu")" "$(digest "$settings")" "$(digest "$plugin")" "$(digest "$simulator")"
  printf 'simulator_version='; "$simulator" --version 2>&1 | sed -n '1p'
  printf 'bambu_version_begin\n'; "$bambu" --version 2>&1 | sed -n '1,3p'; printf 'bambu_version_end\n'
} >"$out/provenance.txt"

source "$helpers"
BURST_JOB_LOG_DIR="$out"
BURST_QUIET_JOB_STATUS=0
run_profile() {
  local profile=$1 macro profile_dir marker
  case "$profile" in
    phased_erew_reverse_same_bundle) macro=1 ;;
    overlap_rw_same_bundle) macro=2 ;;
    conditional_read_or_write_same_bundle) macro=3 ;;
    residual_store_same_bundle) macro=4 ;;
    *) echo "unknown duplex fallback profile: $profile" >&2; return 2 ;;
  esac
  profile_dir="$out/$profile"
  marker="DUPLEX_FALLBACK_GOLDEN_PASS PROFILE=$profile N=38"
  mkdir "$profile_dir" "$profile_dir/tmp"
  export TMPDIR="$profile_dir/tmp" TMP="$profile_dir/tmp" TEMP="$profile_dir/tmp"
  local -a args=("${common[@]}" --bambu-parameter=experimental-m-axi-burst=1
    "--extra-cc-options=-DDUPLEX_FALLBACK_PROFILE=$macro")
  {
    printf 'git_head=%s\nprofile=%s\nmacro=%s\nfeature=1\noutput=%s\n' \
      "$git_head" "$profile" "$macro" "$profile_dir"
    printf 'source=%s\ntestbench=%s\nchecker=%s\nrunner=%s\nbambu=%s\nsettings=%s\nplugin=%s\nsimulator=%s\n' \
      "$source_file" "$testbench" "$checker" "$runner_file" "$bambu" "$(realpath "$settings")" "$plugin" "$simulator"
    printf 'source_sha256=%s\ntestbench_sha256=%s\nchecker_sha256=%s\nrunner_sha256=%s\nbambu_sha256=%s\nsettings_sha256=%s\nplugin_sha256=%s\nsimulator_sha256=%s\n' \
      "$(digest "$source_file")" "$(digest "$testbench")" "$(digest "$checker")" "$(digest "$runner_file")" \
      "$(digest "$bambu")" "$(digest "$settings")" "$(digest "$plugin")" "$(digest "$simulator")"
    printf 'cwd=%s\nTMPDIR=%s\nargv=' "$profile_dir" "$TMPDIR"
    printf '%q ' "$bambu" "$source_file" "${args[@]}"; printf '\n'
  } >"$profile_dir/provenance.txt"
  if ! (cd "$profile_dir" && "$bambu" "$source_file" "${args[@]}" >stdout.log 2>stderr.log); then
    tail -n 80 "$profile_dir/stderr.log" >&2 || true
    tail -n 80 "$profile_dir/stdout.log" >&2 || true
    echo "$profile failed; see $profile_dir" >&2
    return 1
  fi
  local testbench_log="$profile_dir/HLS_output/simulation/testbench.log"
  [[ -s $testbench_log ]] || { echo "$profile missing testbench log: $testbench_log" >&2; return 1; }
  if ! rg -F -x -q "$marker" "$testbench_log"; then
    echo "$profile missing exact testbench marker: $marker" >&2
    tail -n 80 "$testbench_log" >&2 || true
    return 1
  fi
  find "$profile_dir" -type f \( -name '*.v' -o -name '*.sv' -o -name '*.xml' -o -name '*.log' \) -print0 | sort -z |
    while IFS= read -r -d '' artifact; do
      printf '%s  %s\n' "$(digest "$artifact")" "$artifact"
    done >"$profile_dir/artifact-sha256.txt"
}

profiles=(phased_erew_reverse_same_bundle overlap_rw_same_bundle
  conditional_read_or_write_same_bundle residual_store_same_bundle)
if ! run_bounded_jobs "$requested_jobs" run_profile "${profiles[@]}"; then
  echo "DUPLEX_FALLBACK_FAILED output=$out" | tee -a "$out/aggregate.log" >&2
  exit 1
fi

if ! python3 -B "$checker" "$out" --source "$source_file" --testbench "$testbench" \
  --bambu "$bambu" --settings "$settings" --plugin "$plugin" --simulator "$simulator" \
  --runner "$runner_file" --report "$out/fallback.json" >"$out/checker.log" 2>&1; then
  cat "$out/checker.log" >&2
  printf 'DUPLEX_FALLBACK_FAILED output=%s\n' "$out" | tee -a "$out/aggregate.log" >&2
  exit 1
fi
cat "$out/checker.log"
profile_list=$(IFS=,; printf '%s' "${profiles[*]}")
printf 'DUPLEX_FALLBACK_PASS profiles=%s output=%s\n' "$profile_list" "$out" | tee "$out/aggregate.log"
