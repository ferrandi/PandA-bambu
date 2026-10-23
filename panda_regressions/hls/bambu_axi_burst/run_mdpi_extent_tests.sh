#!/usr/bin/env bash
# Verify co-simulation extents for source arrays and runtime-sized pointers.
# Usage: MDPI_EXTENT_TESTS_OUTSIDE_SANDBOX=1 run_mdpi_extent_tests.sh BAMBU [FRESH_OUTPUT_DIR]
set -Eeuo pipefail

script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)
repo_root=$(git -C "$script_dir/../../.." rev-parse --show-toplevel)
source_file="$script_dir/mdpi_extent_case.cpp"
testbench="$script_dir/mdpi_extent_tb.cpp"
runner_file=$(realpath "${BASH_SOURCE[0]}")
helpers="$script_dir/parallel_helpers.sh"

if (($# < 1 || $# > 2)); then
  echo "usage: MDPI_EXTENT_TESTS_OUTSIDE_SANDBOX=1 $0 BAMBU [FRESH_OUTPUT_DIR]" >&2
  exit 2
fi
if [[ ${MDPI_EXTENT_TESTS_OUTSIDE_SANDBOX:-} != 1 ]]; then
  echo "refusing compiler/simulation unless MDPI_EXTENT_TESTS_OUTSIDE_SANDBOX=1 is set; run outside the sandbox" >&2
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
for tool in rg sha256sum timeout; do command -v "$tool" >/dev/null || { echo "$tool is required" >&2; exit 2; }; done
[[ -r $source_file && -r $testbench && -r $helpers ]] || {
  echo "MDPI extent source, testbench, or parallel helper is missing" >&2; exit 2;
}

prefix=$(dirname "$(dirname "$bambu")")
settings="$prefix/settings.sh"
[[ -r $settings ]] || { echo "installed settings.sh missing below $prefix" >&2; exit 2; }
# shellcheck disable=SC1090
source "$settings"
[[ $(realpath -m "${BAMBU_HLS:-}") == $(realpath "$prefix") ]] || {
  echo "settings.sh selected an unexpected BAMBU_HLS=${BAMBU_HLS:-<unset>}" >&2; exit 2;
}

out_arg=${2:-"$PWD/out_mdpi_extent_tests"}
if [[ -e $out_arg || -L $out_arg ]]; then
  echo "output directory must be fresh and not a symlink: $out_arg" >&2
  exit 2
fi
out=$(realpath -m -- "$out_arg")
mkdir -p "$out/jobs"
out=$(realpath "$out")
export PYTHONDONTWRITEBYTECODE=1

device="$repo_root/etc/libtech/xilinx/xcu250-2Lfigd2104.xml"
[[ -r $device ]] || { echo "device description missing: $device" >&2; exit 2; }
plugin="$(realpath "$BAMBU_HLS")/lib/panda/clang-16/ASTAnalyzer.so"
[[ -r $plugin ]] || { echo "selected ASTAnalyzer plugin is missing: $plugin" >&2; exit 2; }
simulator=$(realpath "$(command -v verilator)")
requested_jobs=${MDPI_EXTENT_JOBS:-4}
if [[ ! $requested_jobs =~ ^[1-9][0-9]*$ ]] || (( requested_jobs > 32 )); then
  echo "MDPI_EXTENT_JOBS must be an integer in [1,32] (got '${MDPI_EXTENT_JOBS:-}')" >&2
  exit 2
fi
timeout_seconds=${MDPI_EXTENT_TIMEOUT_SECONDS:-300}
if [[ ! $timeout_seconds =~ ^[1-9][0-9]*$ ]] || (( timeout_seconds > 86400 )); then
  echo "MDPI_EXTENT_TIMEOUT_SECONDS must be an integer in [1,86400] (got '${MDPI_EXTENT_TIMEOUT_SECONDS:-}')" >&2
  exit 2
fi
digest() { sha256sum "$1" | cut -d' ' -f1; }
{
  printf 'git_head=%s\ninvocation=' "$(git -C "$repo_root" rev-parse HEAD)"
  printf '%q ' "$0" "$bambu_arg" "${2:-$PWD/out_mdpi_extent_tests}"
  printf '\nrepo_root=%s\noutput=%s\nbambu=%s\nsettings=%s\nplugin=%s\nsimulator=%s\n' \
    "$repo_root" "$out" "$bambu" "$(realpath "$settings")" "$plugin" "$simulator"
  printf 'jobs=%s\n' "$requested_jobs"
  printf 'timeout_seconds=%s\n' "$timeout_seconds"
  printf 'source=%s\nsource_sha256=%s\ntestbench=%s\ntestbench_sha256=%s\nrunner=%s\nrunner_sha256=%s\n' \
    "$source_file" "$(digest "$source_file")" "$testbench" "$(digest "$testbench")" "$runner_file" "$(digest "$runner_file")"
  printf 'bambu_sha256=%s\nsettings_sha256=%s\nplugin_sha256=%s\nsimulator_sha256=%s\n' \
    "$(digest "$bambu")" "$(digest "$settings")" "$(digest "$plugin")" "$(digest "$simulator")"
  printf 'simulator_version='; "$simulator" --version 2>&1 | sed -n '1p'
  printf 'bambu_version_begin\n'; "$bambu" --version 2>&1 | sed -n '1,3p'; printf 'bambu_version_end\n'
} >"$out/provenance.txt"

source "$helpers"
BURST_JOB_LOG_DIR="$out/jobs"
BURST_QUIET_JOB_STATUS=0
run_case() {
  local job=$1 case_name case_id expected case_dir marker
  IFS=: read -r case_name case_id expected <<<"$job"
  case_dir="$out/$case_name"
  mkdir -p "$case_dir/tmp"
  export TMPDIR="$case_dir/tmp" TMP="$case_dir/tmp" TEMP="$case_dir/tmp"
  marker="MDPI_EXTENT_GOLDEN_PASS CASE=$case_id"
  local tb_log="$case_dir/HLS_output/simulation/testbench.log"
  local -a args=("$source_file" --top-fname=kernel --compiler=I386_CLANG16 --generate-interface=INFER
    --device-name=xcu250,-2L,figd2104 --memory-allocation-policy=NO_BRAM --distram-threshold=0
    --clock-period=3.33 --no-clean --simulate --simulator=VERILATOR "--generate-tb=$testbench"
    "--extra-cc-options=-DMDPI_EXTENT_CASE=$case_id")
  if [[ $case_id == 1 ]]; then args+=(--tb-param-size=data:4); fi
  if [[ $case_id == 6 ]]; then args+=(--tb-param-size=data:8); fi
  if [[ $case_id == 7 ]]; then args+=(--tb-param-size=data:0); fi
  if [[ $case_id == 10 ]]; then args+=(--tb-param-size=data:32); fi
  {
    printf 'case=%s\ncase_id=%s\nexpected=%s\nsource=%s\ntestbench=%s\n' \
      "$case_name" "$case_id" "$expected" "$source_file" "$testbench"
    printf 'cwd=%s\nTMPDIR=%s\nargv=' "$case_dir" "$TMPDIR"
    printf '%q ' "$bambu" "${args[@]}"; printf '\n'
    sha256sum "$source_file" "$testbench" "$bambu" "$settings" "$plugin" "$simulator" "$runner_file"
  } >"$case_dir/provenance.txt"

  local status=0
  (cd "$case_dir" && timeout --foreground --kill-after=10s "${timeout_seconds}s" \
    "$bambu" "${args[@]}" >stdout.log 2>stderr.log) || status=$?
  if (( status == 124 || status == 137 )); then
    echo "$case_name timed out after ${timeout_seconds}s; see $case_dir" >&2
    return 1
  fi
  if [[ $expected == failure ]]; then
    if (( status == 0 )); then
      echo "$case_name unexpectedly succeeded; zero memory extents must fail closed" >&2
      return 1
    fi
    if [[ ! -f $tb_log || -L $tb_log || ! -s $tb_log ]]; then
      echo "$case_name missing nonempty regular testbench log: $tb_log" >&2
      return 1
    fi
    if ! rg -F -q \
      "MDPI parameter extent is zero: pointer parameter 'data' of function 'kernel' must resolve to a positive size from the source extent, --tb-param-size, or m_param_alloc before the top-function call." \
      "$tb_log"; then
      echo "$case_name failed without the targeted missing-extent diagnostic" >&2
      tail -n 80 "$case_dir/stdout.log" "$case_dir/stderr.log" "$tb_log" >&2 || true
      return 1
    fi
    if ! rg -F -x -q "Sim: Driver terminated with code: 1" "$tb_log"; then
      echo "$case_name failed without the simulator failure marker" >&2
      tail -n 80 "$case_dir/stdout.log" "$case_dir/stderr.log" "$tb_log" >&2 || true
      return 1
    fi
    printf 'MDPI_EXTENT_EXPECTED_FAILURE CASE=%s\n' "$case_id" >"$case_dir/scenario-marker.txt"
    return 0
  fi
  if (( status != 0 )); then
    tail -n 80 "$case_dir/stderr.log" >&2 || true
    tail -n 80 "$case_dir/stdout.log" >&2 || true
    echo "$case_name failed; see $case_dir" >&2
    return 1
  fi
  [[ -f $tb_log && ! -L $tb_log && -s $tb_log ]] || {
    echo "$case_name missing nonempty regular testbench log: $tb_log" >&2
    return 1
  }
  if ! rg -F -x -q "$marker" "$tb_log"; then
    echo "$case_name missing exact testbench marker: $marker" >&2
    tail -n 80 "$tb_log" >&2 || true
    return 1
  fi
  printf '%s\n' "$marker" >"$case_dir/scenario-marker.txt"
}

jobs=(pointer_cli_size_4:1:pass int_array_one:2:pass byte_array_one:3:pass
  byte_pointer_runtime_size:4:pass pointer_missing_runtime_size:5:failure int_array_cli_override:6:pass
  pointer_cli_size_zero:7:failure int_reference_array_four:8:pass
  pointer_to_array_missing_extent:9:failure pointer_to_array_cli_size:10:pass)
if ! run_bounded_jobs "$requested_jobs" run_case "${jobs[@]}"; then
  echo "MDPI_EXTENT_TESTS_FAILED output=$out" >&2
  exit 1
fi
printf 'MDPI_EXTENT_TESTS_PASS cases=%s output=%s\n' "${#jobs[@]}" "$out"
