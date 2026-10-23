#!/usr/bin/env bash
# Manually qualify generated RTL for independent read/write duplex traffic.
# Usage: BURST_DUPLEX_INDEPENDENT_SIM_OUTSIDE_SANDBOX=1 run_duplex_independent_tests.sh BAMBU [FRESH_OUTPUT_DIR]
set -Eeuo pipefail

script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)
repo_root=$(git -C "$script_dir/../../.." rev-parse --show-toplevel)
runner_file=$(realpath "${BASH_SOURCE[0]}")
source_file="$script_dir/bambu_axi_burst_duplex_independent.cpp"
testbench="$script_dir/duplex_independent_tb.cpp"
checker="$script_dir/check_duplex_independent_results.py"
helpers="$script_dir/parallel_helpers.sh"

usage() { echo "usage: BURST_DUPLEX_INDEPENDENT_SIM_OUTSIDE_SANDBOX=1 $0 BAMBU [FRESH_OUTPUT_DIR]" >&2; }
if (($# < 1 || $# > 2)); then usage; exit 2; fi
if [[ ${BURST_DUPLEX_INDEPENDENT_SIM_OUTSIDE_SANDBOX:-} != 1 ]]; then
  echo "refusing compiler/simulation unless BURST_DUPLEX_INDEPENDENT_SIM_OUTSIDE_SANDBOX=1 is set; run outside the sandbox" >&2
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
  echo "duplex independent source, testbench, checker, or parallel helper is missing" >&2; exit 2;
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

requested_jobs=${DUPLEX_INDEPENDENT_JOBS:-2}
if [[ ! $requested_jobs =~ ^[0-9]+$ ]]; then
  echo "DUPLEX_INDEPENDENT_JOBS must be a positive integer (got '$requested_jobs')" >&2; exit 2
fi
while [[ $requested_jobs == 0* && ${#requested_jobs} -gt 1 ]]; do requested_jobs=${requested_jobs#0}; done
if [[ $requested_jobs == 0 || ${#requested_jobs} -gt 10 ]] || \
  { ((${#requested_jobs} == 10)) && [[ $requested_jobs > 2147483647 ]]; }; then
  echo "DUPLEX_INDEPENDENT_JOBS must be between 1 and 2147483647 (got '${DUPLEX_INDEPENDENT_JOBS:-}')" >&2; exit 2
fi
requested_jobs=$((10#$requested_jobs))

out_arg=${2:-"$PWD/out_duplex_independent"}
if [[ -e $out_arg || -L $out_arg ]]; then
  echo "output directory must be fresh and not a symlink: $out_arg" >&2; exit 2
fi
out=$(realpath -m -- "$out_arg")

device="$repo_root/etc/libtech/xilinx/xcu250-2Lfigd2104.xml"
[[ -r $device ]] || { echo "device description missing: $device" >&2; exit 2; }
common=(--top-fname=bambu_axi_burst_duplex_independent --compiler=I386_CLANG16
  --generate-interface=INFER --device-name=xcu250,-2L,figd2104 --addr-bus-bitsize=64
  --AXI-burst-type=INCREMENTAL --mem-delay-read=64 --mem-delay-write=2 --tb-queue-size=32
  --debug-classes=InterfaceInfer,BuildVirtualPhi,parametric_list_based
  --memory-allocation-policy=NO_BRAM --distram-threshold=0 --clock-period=3.33
  # Unbounded pointer parameters: size the co-simulation image of the streamed memory.
  --tb-param-size=src:16384 --tb-param-size=dst:16384
  --no-clean --simulate --simulator=VERILATOR "--generate-tb=$testbench"
  --tb-param-size=src:16384 --tb-param-size=dst:16384
  --extra-cc-options=-DNDEBUG --extra-cc-options=-DBAMBU)

mkdir "$out"
out=$(realpath "$out")
mkdir "$out/job_logs" "$out/tmp"
export TMPDIR="$out/tmp" TMP="$out/tmp" TEMP="$out/tmp"
export PYTHONDONTWRITEBYTECODE=1

digest() { sha256sum -- "$1" | awk '{print $1}'; }
git_head=$(git -C "$repo_root" rev-parse HEAD)
{
  printf 'git_head=%s\ninvocation=' "$git_head"
  printf '%q ' "$0" "$bambu_arg" "${2:-$PWD/out_duplex_independent}"
  printf '\nrepo_root=%s\noutput=%s\nbambu=%s\nsettings=%s\nplugin=%s\nsimulator=%s\njobs=%s\n' \
    "$repo_root" "$out" "$bambu" "$(realpath "$settings")" "$plugin" "$simulator" "$requested_jobs"
  printf 'source=%s\nsource_sha256=%s\ntestbench=%s\ntestbench_sha256=%s\nchecker=%s\nchecker_sha256=%s\nrunner=%s\nrunner_sha256=%s\nparallel_helper=%s\nparallel_helper_sha256=%s\n' \
    "$source_file" "$(digest "$source_file")" "$testbench" "$(digest "$testbench")" \
    "$checker" "$(digest "$checker")" "$runner_file" "$(digest "$runner_file")" "$helpers" "$(digest "$helpers")"
  printf 'bambu_sha256=%s\nsettings_sha256=%s\nplugin_sha256=%s\nsimulator_sha256=%s\n' \
    "$(digest "$bambu")" "$(digest "$settings")" "$(digest "$plugin")" "$(digest "$simulator")"
  printf 'simulator_version='; "$simulator" --version 2>&1 | sed -n '1p'
  printf 'bambu_version_begin\n'; "$bambu" --version 2>&1 | sed -n '1,3p'; printf 'bambu_version_end\n'
} >"$out/provenance.txt"

source "$helpers"
BURST_JOB_LOG_DIR="$out"
BURST_QUIET_JOB_STATUS=0
run_profile() {
  local profile=$1 macro marker profile_dir
  case "$profile" in
    symmetric) macro=1 ;;
    asymmetric) macro=2 ;;
    *) echo "unknown duplex independent profile: $profile" >&2; return 2 ;;
  esac
  profile_dir="$out/$profile"
  marker="DUPLEX_INDEPENDENT_GOLDEN_PASS PROFILE=$profile N=38"
  mkdir "$profile_dir" "$profile_dir/tmp"
  export TMPDIR="$profile_dir/tmp" TMP="$profile_dir/tmp" TEMP="$profile_dir/tmp"
  local -a args=("${common[@]}" --bambu-parameter=experimental-m-axi-burst=1
    "--extra-cc-options=-DDUPLEX_INDEPENDENT_PROFILE=$macro")
  {
    printf 'git_head=%s\nprofile=%s\nmacro=%s\nfeature=1\noutput=%s\nread_profile=%s\nwrite_profile=%s\n' \
      "$git_head" "$profile" "$macro" "$profile_dir" \
      "$([[ $profile == symmetric ]] && echo 16/1/64 || echo 16/2/64)" \
      "$([[ $profile == symmetric ]] && echo 16/1/32 || echo 8/1/16)"
    printf 'source=%s\ntestbench=%s\nchecker=%s\nrunner=%s\nparallel_helper=%s\nbambu=%s\nsettings=%s\nplugin=%s\nsimulator=%s\n' \
      "$source_file" "$testbench" "$checker" "$runner_file" "$helpers" "$bambu" "$(realpath "$settings")" "$plugin" "$simulator"
    printf 'source_sha256=%s\ntestbench_sha256=%s\nchecker_sha256=%s\nrunner_sha256=%s\nparallel_helper_sha256=%s\nbambu_sha256=%s\nsettings_sha256=%s\nplugin_sha256=%s\nsimulator_sha256=%s\n' \
      "$(digest "$source_file")" "$(digest "$testbench")" "$(digest "$checker")" "$(digest "$runner_file")" \
      "$(digest "$helpers")" "$(digest "$bambu")" "$(digest "$settings")" "$(digest "$plugin")" "$(digest "$simulator")"
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
  (
    cd "$profile_dir"
    find . -type f \( -name '*.v' -o -name '*.sv' -o -name '*.xml' -o -name '*.log' -o -name '*.raw' \) -print0 | sort -z |
      while IFS= read -r -d '' artifact; do
        artifact=${artifact#./}
        printf '%s  %s\n' "$(digest "$profile_dir/$artifact")" "$artifact"
      done
  ) >"$profile_dir/artifact-sha256.txt"
}

profiles=(symmetric asymmetric)
if ! run_bounded_jobs "$requested_jobs" run_profile "${profiles[@]}"; then
  printf 'DUPLEX_INDEPENDENT_FAILED output=%s\n' "$out" | tee -a "$out/aggregate.log" >&2
  exit 1
fi

if ! python3 -B "$checker" "$out" --source "$source_file" --testbench "$testbench" \
  --bambu "$bambu" --settings "$settings" --plugin "$plugin" --simulator "$simulator" \
  --runner "$runner_file" --report "$out/independent.json" >"$out/checker.log" 2>&1; then
  cat "$out/checker.log" >&2
  printf 'DUPLEX_INDEPENDENT_FAILED output=%s\n' "$out" | tee -a "$out/aggregate.log" >&2
  exit 1
fi
cat "$out/checker.log"
printf 'DUPLEX_INDEPENDENT_PASS profiles=%s,%s output=%s\n' "${profiles[0]}" "${profiles[1]}" "$out" | tee "$out/aggregate.log"
