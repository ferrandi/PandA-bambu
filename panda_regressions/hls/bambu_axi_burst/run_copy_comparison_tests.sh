#!/usr/bin/env bash
# Run the four copy transport modes twice each with isolated Verilator outputs.
# Usage: BURST_COPY_COMPARISON_OUTSIDE_SANDBOX=1 run_copy_comparison_tests.sh BAMBU OUTPUT_DIR
set -Eeuo pipefail

script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)
repo_root=$(git -C "$script_dir/../../.." rev-parse --show-toplevel)
runner_file=$(realpath "${BASH_SOURCE[0]}")
source_file="$script_dir/bambu_axi_burst_copy_comparison.cpp"
testbench="$script_dir/copy_comparison_tb.cpp"
checker="$script_dir/check_copy_comparison_results.py"
helpers="$script_dir/parallel_helpers.sh"

usage() {
  echo "usage: BURST_COPY_COMPARISON_OUTSIDE_SANDBOX=1 $0 BAMBU FRESH_OUTPUT_DIR" >&2
}

if (($# != 2)); then usage; exit 2; fi
if [[ ${BURST_COPY_COMPARISON_OUTSIDE_SANDBOX:-} != 1 ]]; then
  echo "refusing compiler/simulation unless BURST_COPY_COMPARISON_OUTSIDE_SANDBOX=1 is set; run outside the sandbox" >&2
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
command -v grep >/dev/null || { echo "grep is required to validate scenario markers" >&2; exit 2; }
[[ -r $source_file && -r $testbench && -r $checker && -r $helpers ]] || {
  echo "copy-comparison source, testbench, checker, or parallel helper is missing" >&2; exit 2;
}

prefix=$(dirname "$(dirname "$bambu")")
settings="$prefix/settings.sh"
plugin="$prefix/lib/panda/clang-16/ASTAnalyzer.so"
[[ -r $settings && -r $plugin ]] || { echo "installed settings/plugin missing below $prefix" >&2; exit 2; }
# shellcheck disable=SC1090
source "$settings"
[[ $(realpath -m "${BAMBU_HLS:-}") == $(realpath "$prefix") ]] || {
  echo "settings.sh selected an unexpected BAMBU_HLS=${BAMBU_HLS:-<unset>}" >&2; exit 2;
}

requested_jobs=${COPY_COMPARISON_JOBS:-}
if [[ -z $requested_jobs ]]; then
  detected_jobs=$(getconf _NPROCESSORS_ONLN 2>/dev/null || printf '1')
  [[ $detected_jobs =~ ^[1-9][0-9]*$ ]] || detected_jobs=1
  if (( detected_jobs > 4 )); then requested_jobs=4; else requested_jobs=$detected_jobs; fi
else
  if [[ ! $requested_jobs =~ ^[0-9]+$ ]]; then
    echo "COPY_COMPARISON_JOBS must be a positive integer (got '$requested_jobs')" >&2; exit 2
  fi
  while [[ $requested_jobs == 0* && ${#requested_jobs} -gt 1 ]]; do requested_jobs=${requested_jobs#0}; done
  if [[ $requested_jobs == 0 || ${#requested_jobs} -gt 10 ]] || \
    { ((${#requested_jobs} == 10)) && [[ $requested_jobs > 2147483647 ]]; }; then
    echo "COPY_COMPARISON_JOBS must be between 1 and 2147483647 (got '${COPY_COMPARISON_JOBS}')" >&2; exit 2
  fi
  requested_jobs=$((10#$requested_jobs))
fi

out_arg=$2
if [[ -e $out_arg || -L $out_arg ]]; then
  echo "output directory must be fresh and not a symlink: $out_arg" >&2; exit 2
fi
out=$(realpath -m -- "$out_arg")

device="$repo_root/etc/libtech/xilinx/xcu250-2Lfigd2104.xml"
[[ -r $device ]] || { echo "device description missing: $device" >&2; exit 2; }
common=(--top-fname=kernel --compiler=I386_CLANG16 --generate-interface=INFER
  --device-name=xcu250,-2L,figd2104 --AXI-burst-type=INCREMENTAL
  --mem-delay-read=64 --mem-delay-write=2 --tb-queue-size=32
  --memory-allocation-policy=NO_BRAM --distram-threshold=0 --clock-period=3.33
  # The fixtures declare the streamed data as unbounded pointers, so the extent of the memory
  # behind them is not visible to the back-end; size the co-simulation image explicitly.
  --tb-param-size=src:16384 --tb-param-size=dst:16384
  --no-clean --simulate --simulator=VERILATOR "--generate-tb=$testbench"
  --extra-cc-options=-DNDEBUG --extra-cc-options=-DBAMBU)

mkdir -p "$out/jobs"
out=$(realpath "$out")
export PYTHONDONTWRITEBYTECODE=1

git_head=$(git -C "$repo_root" rev-parse HEAD)
simulator=$(realpath "$(command -v verilator)")
{
  printf 'git_head=%s\ninvocation=' "$git_head"
  printf '%q ' "$0" "$bambu_arg" "$out_arg"
  printf '\nrepo_root=%s\noutput=%s\nbambu=%s\nprefix=%s\nsettings=%s\nplugin=%s\n' \
    "$repo_root" "$out" "$bambu" "$prefix" "$(realpath "$settings")" "$(realpath "$plugin")"
  printf 'simulator=%s\nsimulator_version=' "$simulator"
  "$simulator" --version 2>&1 | sed -n '1p'
  printf 'jobs=%s\nsource=%s\ntestbench=%s\nchecker=%s\n' \
    "$requested_jobs" "$source_file" "$testbench" "$checker"
  printf 'source_sha256='; sha256sum "$source_file"
  printf 'testbench_sha256='; sha256sum "$testbench"
  printf 'runner_sha256='; sha256sum "$runner_file"
  printf 'bambu_sha256='; sha256sum "$bambu"
  printf 'settings_sha256='; sha256sum "$settings"
  printf 'plugin_sha256='; sha256sum "$plugin"
  printf 'simulator_sha256='; sha256sum "$simulator"
  printf 'bambu_version_begin\n'; "$bambu" --version 2>&1 | sed -n '1,3p'; printf 'bambu_version_end\n'
  printf 'common_argv='; printf '%q ' "$bambu" "$source_file" "${common[@]}"; printf '\n'
} >"$out/provenance.txt"

source "$helpers"
BURST_JOB_LOG_DIR="$out/jobs"
BURST_QUIET_JOB_STATUS=0
run_copy_job() {
  local job=$1 profile=${1%%__run*} run=${1##*__} macro feature case_dir marker
  case $profile in
    burst-two-bundles) macro=1; feature=1 ;;
    legacy-two-bundles) macro=2; feature=0 ;;
    legacy-shared) macro=3; feature=0 ;;
    duplex-shared) macro=4; feature=1 ;;
    *) echo "unknown copy-comparison job: $job" >&2; return 2 ;;
  esac
  case_dir="$out/$profile/$run"
  mkdir -p "$case_dir"
  mkdir "$case_dir/tmp"
  export TMPDIR="$case_dir/tmp" TMP="$case_dir/tmp" TEMP="$case_dir/tmp"
  marker="COPY_COMPARISON_GOLDEN_PASS PROFILE=$profile N=1024"
  local -a args=("${common[@]}" "--bambu-parameter=experimental-m-axi-burst=$feature"
    "--extra-cc-options=-DCOPY_COMPARISON_PROFILE=$macro")
  {
    printf 'profile=%s\nrun=%s\nmacro=%s\nfeature=%s\nsource=%s\ntestbench=%s\nmarker=%s\n' \
      "$profile" "$run" "$macro" "$feature" "$source_file" "$testbench" "$marker"
    printf 'cwd=%s\nTMPDIR=%s\nargv=' "$case_dir" "$TMPDIR"
    printf '%q ' "$bambu" "$source_file" "${args[@]}"; printf '\n'
    sha256sum "$source_file" "$testbench" "$bambu" "$settings" "$plugin" "$simulator" "$runner_file"
  } >"$case_dir/provenance.txt"
  if ! (cd "$case_dir" && "$bambu" "$source_file" "${args[@]}" >stdout.log 2>stderr.log); then
    tail -n 80 "$case_dir/stderr.log" >&2 || true
    tail -n 80 "$case_dir/stdout.log" >&2 || true
    echo "$job failed; see $case_dir" >&2
    return 1
  fi
  [[ -s $case_dir/bambu_results.xml ]] || { echo "$job missing nonempty bambu_results.xml" >&2; return 1; }
  local rtl
  rtl=$(find "$case_dir" -type f \( -name '*.v' -o -name '*.sv' \) ! -name panda_libtech.v -print -quit)
  [[ -n $rtl ]] || { echo "$job missing generated RTL" >&2; return 1; }
  local tb_log="$case_dir/HLS_output/simulation/testbench.log"
  [[ -s $tb_log ]] || { echo "$job missing nonempty testbench log: $tb_log" >&2; return 1; }
  if ! grep -F -x -q "$marker" "$tb_log"; then
    echo "$job missing exact testbench marker: $marker" >&2
    tail -n 80 "$tb_log" >&2 || true
    return 1
  fi
  printf '%s\n' "$marker" >"$case_dir/scenario-marker.txt"
  find "$case_dir" -type f \( -name '*.v' -o -name '*.sv' -o -name '*.xml' -o -name '*.log' \) -print0 | sort -z |
    while IFS= read -r -d '' artifact; do
      printf '%s  %s\n' "$(sha256sum "$artifact" | cut -d' ' -f1)" "$artifact"
    done >"$case_dir/artifact-sha256.txt"
}

jobs=()
for profile in burst-two-bundles legacy-two-bundles legacy-shared duplex-shared; do
  jobs+=("${profile}__run1" "${profile}__run2")
done
if ! run_bounded_jobs "$requested_jobs" run_copy_job "${jobs[@]}"; then
  echo "COPY_COMPARISON_FAILED output=$out" >&2
  exit 1
fi

python3 -B "$checker" "$out" --source "$source_file" --testbench "$testbench" \
  --bambu "$bambu" --settings "$settings" --plugin "$plugin" --runner "$runner_file" \
  --report "$out/comparison.json"
printf 'COPY_COMPARISON_PASS output=%s\n' "$out"
