#!/usr/bin/env bash
# Manually simulate disjoint, overlapping, and zero-trip duplex ranges on the default device.
# Usage: BURST_DUPLEX_ALIAS_SIM_OUTSIDE_SANDBOX=1 run_duplex_alias_tests.sh BAMBU [FRESH_OUTPUT_DIR]
set -Eeuo pipefail

script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)
repo_root=$(git -C "$script_dir/../../.." rev-parse --show-toplevel)
runner_file=$(realpath "${BASH_SOURCE[0]}")
source_file="$script_dir/bambu_axi_burst_duplex_alias.cpp"
testbench="$script_dir/bambu_axi_burst_duplex_alias_tb.cpp"
bundle_name=shared
activation_witness="Activated guarded distinct-base duplex for bundle $bundle_name (overlap uses serialized scalar transfers)"
checker="$script_dir/check_duplex_alias_results.py"
helpers="$script_dir/parallel_helpers.sh"
range_probe_runner="$script_dir/rtl_tests/run_duplex_alias_range_guard.sh"
range_probe_testbench="$script_dir/rtl_tests/tb_duplex_alias_range_guard.sv"

usage() { echo "usage: BURST_DUPLEX_ALIAS_SIM_OUTSIDE_SANDBOX=1 $0 BAMBU [FRESH_OUTPUT_DIR]" >&2; }
if (($# < 1 || $# > 2)); then usage; exit 2; fi
if [[ ${BURST_DUPLEX_ALIAS_SIM_OUTSIDE_SANDBOX:-} != 1 ]]; then
  echo "refusing compiler/simulation unless BURST_DUPLEX_ALIAS_SIM_OUTSIDE_SANDBOX=1 is set; run outside the sandbox" >&2
  exit 2
fi
address_bits=${DUPLEX_ALIAS_ADDRESS_BITS:-32}
if [[ $address_bits != 32 && $address_bits != 64 ]]; then
  echo "DUPLEX_ALIAS_ADDRESS_BITS must be 32 or 64 (got '$address_bits')" >&2; exit 2
fi
export PYTHONDONTWRITEBYTECODE=1

bambu_arg=$1
if [[ $bambu_arg == */* ]]; then
  bambu=$(realpath "$bambu_arg" 2>/dev/null || true)
else
  bambu=$(command -v "$bambu_arg" || true)
  [[ -z $bambu ]] || bambu=$(realpath "$bambu")
fi
[[ -n ${bambu:-} && -x $bambu ]] || { echo "Bambu executable not found: $bambu_arg" >&2; exit 2; }
for tool in verilator grep python3 sha256sum; do
  command -v "$tool" >/dev/null || { echo "$tool is required" >&2; exit 2; }
done
[[ -r $source_file && -r $testbench && -r $checker && -r $helpers && -x $range_probe_runner && -r $range_probe_testbench ]] || {
  echo "duplex alias sources, checker, parallel helper, or range-guard probe are missing" >&2; exit 2;
}

prefix=$(dirname "$(dirname "$bambu")")
settings="$prefix/settings.sh"
plugin="$prefix/lib/panda/clang-16/ASTAnalyzer.so"
[[ -r $settings && -s $plugin ]] || { echo "installed settings/plugin missing below $prefix" >&2; exit 2; }
# shellcheck disable=SC1090
source "$settings"
[[ $(realpath -m "${BAMBU_HLS:-}") == "$(realpath "$prefix")" ]] || {
  echo "settings.sh selected an unexpected BAMBU_HLS=${BAMBU_HLS:-<unset>}" >&2; exit 2;
}
simulator=$(realpath "$(command -v verilator)")

requested_jobs=${DUPLEX_ALIAS_JOBS:-2}
if [[ ! $requested_jobs =~ ^[0-9]+$ ]]; then
  echo "DUPLEX_ALIAS_JOBS must be a positive integer (got '$requested_jobs')" >&2; exit 2
fi
while [[ $requested_jobs == 0* && ${#requested_jobs} -gt 1 ]]; do requested_jobs=${requested_jobs#0}; done
if [[ $requested_jobs == 0 || ${#requested_jobs} -gt 10 ]] || \
  { ((${#requested_jobs} == 10)) && [[ $requested_jobs > 2147483647 ]]; }; then
  echo "DUPLEX_ALIAS_JOBS must be between 1 and 2147483647 (got '${DUPLEX_ALIAS_JOBS:-}')" >&2; exit 2
fi
requested_jobs=$((10#$requested_jobs))

out_arg=${2:-"$PWD/out_duplex_alias"}
if [[ -e $out_arg || -L $out_arg ]]; then
  echo "output directory must be fresh and not a symlink: $out_arg" >&2; exit 2
fi
out=$(realpath -m -- "$out_arg")

mkdir "$out"
mkdir "$out/jobs"
out=$(realpath "$out")
mkdir "$out/tmp"
export TMPDIR="$out/tmp" TMP="$out/tmp" TEMP="$out/tmp"

digest() { sha256sum -- "$1" | awk '{print $1}'; }
git_head=$(git -C "$repo_root" rev-parse HEAD)
{
  printf 'git_head=%s\nrepo_root=%s\noutput=%s\nbambu=%s\nsettings=%s\nplugin=%s\nsimulator=%s\njobs=%s\naddr_bits=%s\n' \
    "$git_head" "$repo_root" "$out" "$bambu" "$(realpath "$settings")" "$plugin" "$simulator" "$requested_jobs" "$address_bits"
  printf 'range_probe_runner=%s\nrange_probe_testbench=%s\n' "$range_probe_runner" "$range_probe_testbench"
  for entry in "source:$source_file" "testbench:$testbench" "checker:$checker" "runner:$runner_file" \
               "range_probe_runner:$range_probe_runner" "range_probe_testbench:$range_probe_testbench" \
               "bambu:$bambu" "settings:$settings" "plugin:$plugin" "simulator:$simulator"; do
    name=${entry%%:*}; path=${entry#*:}
    printf '%s=%s\n%s_sha256=%s\n' "$name" "$path" "$name" "$(digest "$path")"
  done
  printf 'simulator_version='; "$simulator" --version 2>&1 | sed -n '1p'
  printf 'bambu_version_begin\n'; "$bambu" --version 2>&1 | sed -n '1,3p'; printf 'bambu_version_end\n'
} >"$out/provenance.txt"

source "$helpers"
BURST_JOB_LOG_DIR="$out/jobs"
BURST_QUIET_JOB_STATUS=0
profiles=(symmetric asymmetric)
scenarios=(disjoint forward-overlap reverse-overlap zero-trip)
run_case() {
  local job=$1 profile=${1%%__*} scenario=${1#*__} profile_id case_id case_dir marker count
  case "$profile" in symmetric) profile_id=1 ;; asymmetric) profile_id=2 ;; *) return 2 ;; esac
  case "$scenario" in disjoint) case_id=1 ;; forward-overlap) case_id=2 ;; reverse-overlap) case_id=3 ;; zero-trip) case_id=4 ;; *) return 2 ;; esac
  case_dir="$out/$profile/$scenario"
  mkdir -p "$case_dir"
  mkdir "$case_dir/tmp"
  export TMPDIR="$case_dir/tmp" TMP="$case_dir/tmp" TEMP="$case_dir/tmp"
  count=64
  if [[ $scenario == zero-trip ]]; then count=0; fi
  marker="DUPLEX_ALIAS_GOLDEN_PASS CASE=$scenario profile=$profile_id N=$count"
  if [[ $scenario == zero-trip ]]; then marker+=" NO_DATA_CHANGE=1"; fi
  local -a args=(--top-fname=kernel --compiler=I386_CLANG16 --generate-interface=INFER "--addr-bus-bitsize=$address_bits"
    --tb-memory-mapping=SHARED
    --memory-allocation-policy=NO_BRAM
    --AXI-burst-type=INCREMENTAL --no-clean --simulate --simulator=VERILATOR
    --debug-classes=InterfaceInfer,BuildVirtualPhi
    # Unbounded pointer parameters: size the co-simulation image of the streamed memory.
    --tb-param-size=src:1024 --tb-param-size=dst:1024
    "--generate-tb=$testbench" --bambu-parameter=experimental-m-axi-burst=1
    --extra-cc-options=-DNDEBUG --extra-cc-options=-DBAMBU
    "--extra-cc-options=-DDUPLEX_ALIAS_PROFILE=$profile_id"
    "--extra-cc-options=-DDUPLEX_ALIAS_CASE=$case_id")
  if [[ $scenario == zero-trip || $scenario == forward-overlap || $scenario == reverse-overlap ]]; then
    args+=(--generate-vcd)
  fi
  if [[ $scenario == forward-overlap || $scenario == reverse-overlap ]]; then
    args+=(--mem-delay-write=64)
  fi
  {
    printf 'git_head=%s\nprofile=%s\nprofile_id=%s\nscenario=%s\nscenario_id=%s\nfeature=1\naddr_bits=%s\noutput=%s\n' \
      "$git_head" "$profile" "$profile_id" "$scenario" "$case_id" "$address_bits" "$case_dir"
    printf 'source=%s\ntestbench=%s\nchecker=%s\nrunner=%s\nbambu=%s\nsettings=%s\nplugin=%s\nsimulator=%s\n' \
      "$source_file" "$testbench" "$checker" "$runner_file" "$bambu" "$(realpath "$settings")" "$plugin" "$simulator"
    for entry in "source:$source_file" "testbench:$testbench" "checker:$checker" "runner:$runner_file" \
                 "bambu:$bambu" "settings:$settings" "plugin:$plugin" "simulator:$simulator"; do
      name=${entry%%:*}; path=${entry#*:}
      printf '%s_sha256=%s\n' "$name" "$(digest "$path")"
    done
    printf 'marker=%s\nTMPDIR=%s\nargv=' "$marker" "$TMPDIR"
    printf '%q ' "$bambu" "$source_file" "${args[@]}"; printf '\n'
  } >"$case_dir/provenance.txt"
  if ! (cd "$case_dir" && "$bambu" "$source_file" "${args[@]}" >stdout.log 2>stderr.log); then
    tail -n 80 "$case_dir/stderr.log" >&2 || true
    tail -n 80 "$case_dir/stdout.log" >&2 || true
    echo "$profile/$scenario failed; see $case_dir" >&2
    return 1
  fi
  local activation_count
  activation_count=$( (grep -F -o -- "$activation_witness" "$case_dir/stdout.log" "$case_dir/stderr.log" || true) | wc -l)
  [[ $activation_count == 1 ]] || {
    echo "$profile/$scenario requires exactly one runtime-guard activation witness; found $activation_count" >&2
    tail -n 80 "$case_dir/stdout.log" "$case_dir/stderr.log" >&2 || true
    return 1
  }
  local testbench_log="$case_dir/HLS_output/simulation/testbench.log"
  [[ -s $testbench_log ]] || { echo "$profile/$scenario missing nonempty testbench log" >&2; return 1; }
  grep -F -x -q "$marker" "$testbench_log" || {
    echo "$profile/$scenario missing exact marker: $marker" >&2
    tail -n 80 "$testbench_log" >&2 || true
    return 1
  }
  if [[ $scenario == zero-trip || $scenario == forward-overlap || $scenario == reverse-overlap ]]; then
    [[ -s $case_dir/HLS_output/simulation/test.vcd ]] || {
      echo "$profile/$scenario missing nonempty VCD for the AXI witness" >&2; return 1;
    }
  fi
  find "$case_dir" -type f \( -name '*.v' -o -name '*.sv' -o -name '*.xml' -o -name '*.log' -o -name '*.vcd' \) -print0 | sort -z |
    while IFS= read -r -d '' artifact; do
      printf '%s  %s\n' "$(digest "$artifact")" "${artifact#"$case_dir/"}"
    done >"$case_dir/artifact-sha256.txt"
  printf 'DUPLEX_ALIAS_SCENARIO PASS PROFILE=%s CASE=%s\n' "$profile" "$scenario"
}

jobs=()
for profile in "${profiles[@]}"; do
  for scenario in "${scenarios[@]}"; do jobs+=("${profile}__${scenario}"); done
done
if ! run_bounded_jobs "$requested_jobs" run_case "${jobs[@]}"; then
  printf 'DUPLEX_ALIAS_FAILED output=%s\n' "$out" | tee -a "$out/aggregate.log" >&2
  exit 1
fi

range_probe_output="$out/rtl_range_guard"
if ! "$range_probe_runner" "$out/symmetric/disjoint/kernel.v" "$range_probe_output" "$address_bits" \
    >"$out/rtl_range_guard_runner.log" 2>&1; then
  cat "$out/rtl_range_guard_runner.log" >&2 || true
  printf 'DUPLEX_ALIAS_FAILED output=%s\n' "$out" | tee -a "$out/aggregate.log" >&2
  exit 1
fi
cat "$out/rtl_range_guard_runner.log"

python3 -B "$checker" "$out" --source "$source_file" --testbench "$testbench" \
  --bambu "$bambu" --settings "$settings" --plugin "$plugin" --simulator "$simulator" \
  --runner "$runner_file" --range-probe-runner "$range_probe_runner" \
  --range-probe-testbench "$range_probe_testbench" --addr-bits "$address_bits" >"$out/checker.log" 2>&1 || {
  cat "$out/checker.log" >&2
  printf 'DUPLEX_ALIAS_FAILED output=%s\n' "$out" | tee -a "$out/aggregate.log" >&2
  exit 1
}
cat "$out/checker.log"
printf 'DUPLEX_ALIAS_PASS profiles=symmetric,asymmetric scenarios=8 output=%s\n' "$out" | tee "$out/aggregate.log"
