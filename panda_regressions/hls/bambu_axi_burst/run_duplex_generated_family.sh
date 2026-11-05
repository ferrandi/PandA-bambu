#!/usr/bin/env bash
# Shared execution plumbing for the two generated duplex qualification runners.
set -Eeuo pipefail

family=${1:-}
shift || true
case "$family" in
  fallback)
    title="duplex fallback"
    env_prefix=BURST_DUPLEX_FALLBACK
    jobs_name=DUPLEX_FALLBACK_JOBS
    default_out=out_duplex_fallback
    top=bambu_axi_burst_duplex_fallback
    source_name=bambu_axi_burst_duplex_fallback.cpp
    tb_name=duplex_fallback_tb.cpp
    checker_name=check_duplex_fallback_results.py
    profiles=(phased_erew_reverse_same_bundle overlap_rw_same_bundle
      conditional_read_or_write_same_bundle residual_store_same_bundle)
    ;;
  independent)
    title="duplex independent"
    env_prefix=BURST_DUPLEX_INDEPENDENT
    jobs_name=DUPLEX_INDEPENDENT_JOBS
    default_out=out_duplex_independent
    top=bambu_axi_burst_duplex_independent
    source_name=bambu_axi_burst_duplex_independent.cpp
    tb_name=duplex_independent_tb.cpp
    checker_name=check_duplex_independent_results.py
    profiles=(symmetric asymmetric)
    ;;
  *)
    echo "usage: $0 {fallback|independent} [--development-proof] BAMBU [FRESH_OUTPUT_DIR]" >&2
    exit 2
    ;;
esac
runner_invocation=${1:-}
shift || true
proof_mode=functional
if [[ ${1:-} == --development-proof ]]; then
  proof_mode=developer
  shift
fi

script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)
repo_root=$(git -C "$script_dir/../../.." rev-parse --show-toplevel)
runner_file=$(realpath "$script_dir/run_duplex_${family}_tests.sh")
shared_runner="$script_dir/run_duplex_generated_family.sh"
source_file="$script_dir/$source_name"
testbench="$script_dir/$tb_name"
checker="$script_dir/$checker_name"
helpers="$script_dir/parallel_helpers.sh"
usage() { echo "usage: ${env_prefix}_SIM_OUTSIDE_SANDBOX=1 $runner_file BAMBU [FRESH_OUTPUT_DIR]" >&2; }

if [[ -z $runner_invocation ]] || (($# < 1 || $# > 2)); then usage; exit 2; fi
opt_in_name="${env_prefix}_SIM_OUTSIDE_SANDBOX"
if [[ ${!opt_in_name:-} != 1 ]]; then
  echo "refusing compiler/simulation unless $opt_in_name=1 is set; run outside the sandbox" >&2
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
for tool in vsim vlib vmap vlog; do
  command -v "$tool" >/dev/null || { echo "ModelSim tool $tool is required in PATH" >&2; exit 2; }
done
command -v grep >/dev/null || { echo "grep is required to validate scenario markers" >&2; exit 2; }
for tool in python3 sha256sum; do command -v "$tool" >/dev/null || { echo "$tool is required" >&2; exit 2; }; done
[[ -r $source_file && -r $testbench && -r $checker && -r $helpers && -r $shared_runner ]] || {
  echo "$title source, testbench, checker, or parallel helper is missing" >&2; exit 2;
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
simulator_invocation=$(command -v vsim)
simulator=$(realpath "$simulator_invocation")

requested_jobs=${!jobs_name:-2}
if [[ ! $requested_jobs =~ ^[0-9]+$ ]]; then
  echo "$jobs_name must be a positive integer (got '$requested_jobs')" >&2; exit 2
fi
while [[ $requested_jobs == 0* && ${#requested_jobs} -gt 1 ]]; do requested_jobs=${requested_jobs#0}; done
if [[ $requested_jobs == 0 || ${#requested_jobs} -gt 10 ]] || \
  { ((${#requested_jobs} == 10)) && [[ $requested_jobs > 2147483647 ]]; }; then
  echo "$jobs_name must be between 1 and 2147483647 (got '${!jobs_name:-}')" >&2; exit 2
fi
requested_jobs=$((10#$requested_jobs))

out_arg=${2:-"$PWD/$default_out"}
if [[ -e $out_arg || -L $out_arg ]]; then
  echo "output directory must be fresh and not a symlink: $out_arg" >&2; exit 2
fi
out=$(realpath -m -- "$out_arg")

device="$repo_root/etc/libtech/xilinx/xcu250-2Lfigd2104.xml"
[[ -r $device ]] || { echo "device description missing: $device" >&2; exit 2; }
common=(--top-fname="$top" --compiler=I386_CLANG16
  --generate-interface=INFER --device-name=xcu250,-2L,figd2104)
if [[ $family == independent ]]; then
  common+=(--addr-bus-bitsize=64)
fi
common+=(--AXI-burst-type=INCREMENTAL --mem-delay-read=64 --mem-delay-write=2 --tb-queue-size=32
  --memory-allocation-policy=NO_BRAM --distram-threshold=0 --clock-period=3.33)
if [[ $proof_mode == developer ]]; then
  common+=(--debug-classes=InterfaceInfer,BuildVirtualPhi,parametric_list_based)
fi
if [[ $family == fallback ]]; then
  # This pointer has no compile-time bound; size its co-simulation image explicitly.
  common+=(--tb-param-size=data:16384)
else
  # Both streamed pointer parameters have no compile-time bound.
  common+=(--tb-param-size=src:16384 --tb-param-size=dst:16384)
fi
common+=(--no-clean --simulate --simulator=MODELSIM "--generate-tb=$testbench")
common+=(--extra-cc-options=-DNDEBUG --extra-cc-options=-DBAMBU)

mkdir "$out"
out=$(realpath "$out")
mkdir "$out/job_logs" "$out/tmp"
export TMPDIR="$out/tmp" TMP="$out/tmp" TEMP="$out/tmp"
export PYTHONDONTWRITEBYTECODE=1

digest() { sha256sum -- "$1" | awk '{print $1}'; }
git_head=$(git -C "$repo_root" rev-parse HEAD)
{
  printf 'git_head=%s\ninvocation=' "$git_head"
  printf '%q ' "$runner_invocation" "$bambu_arg" "${2:-$PWD/$default_out}"
  printf '\nrepo_root=%s\noutput=%s\nbambu=%s\nsettings=%s\nplugin=%s\nsimulator=%s\njobs=%s\n' \
    "$repo_root" "$out" "$bambu" "$(realpath "$settings")" "$plugin" "$simulator" "$requested_jobs"
  printf 'proof_mode=%s\n' "$proof_mode"
  printf 'source=%s\nsource_sha256=%s\ntestbench=%s\ntestbench_sha256=%s\nchecker=%s\nchecker_sha256=%s\nrunner=%s\nrunner_sha256=%s\n' \
    "$source_file" "$(digest "$source_file")" "$testbench" "$(digest "$testbench")" \
    "$checker" "$(digest "$checker")" "$runner_file" "$(digest "$runner_file")"
  printf 'shared_runner=%s\nshared_runner_sha256=%s\n' "$shared_runner" "$(digest "$shared_runner")"
  if [[ $family == independent ]]; then
    printf 'parallel_helper=%s\nparallel_helper_sha256=%s\n' "$helpers" "$(digest "$helpers")"
  fi
  printf 'bambu_sha256=%s\nsettings_sha256=%s\nplugin_sha256=%s\nsimulator_sha256=%s\n' \
    "$(digest "$bambu")" "$(digest "$settings")" "$(digest "$plugin")" "$(digest "$simulator")"
  printf 'simulator_invocation=%s\nsimulator_version=' "$simulator_invocation"
  "$simulator_invocation" -version 2>&1 | sed -n '1p' || true
  printf 'bambu_version_begin\n'; "$bambu" --version 2>&1 | sed -n '1,3p'; printf 'bambu_version_end\n'
} >"$out/provenance.txt"

source "$helpers"
BURST_JOB_LOG_DIR="$out"
BURST_QUIET_JOB_STATUS=0
run_profile() {
  local profile=$1 macro marker profile_dir
  local read_profile= write_profile=
  if [[ $family == fallback ]]; then
    case "$profile" in
      phased_erew_reverse_same_bundle) macro=1 ;;
      overlap_rw_same_bundle) macro=2 ;;
      conditional_read_or_write_same_bundle) macro=3 ;;
      residual_store_same_bundle) macro=4 ;;
      *) echo "unknown duplex fallback profile: $profile" >&2; return 2 ;;
    esac
    marker="DUPLEX_FALLBACK_GOLDEN_PASS PROFILE=$profile N=38"
  else
    case "$profile" in
      symmetric) macro=1; read_profile=16/1/64; write_profile=16/1/32 ;;
      asymmetric) macro=2; read_profile=16/2/64; write_profile=8/1/16 ;;
      *) echo "unknown duplex independent profile: $profile" >&2; return 2 ;;
    esac
    marker="DUPLEX_INDEPENDENT_GOLDEN_PASS PROFILE=$profile N=38"
  fi
  profile_dir="$out/$profile"
  mkdir "$profile_dir" "$profile_dir/tmp"
  export TMPDIR="$profile_dir/tmp" TMP="$profile_dir/tmp" TEMP="$profile_dir/tmp"
  local -a args=("${common[@]}" --bambu-parameter=m-axi-burst=1
    "--extra-cc-options=-DDUPLEX_${family^^}_PROFILE=$macro")
  {
    printf 'git_head=%s\nprofile=%s\nmacro=%s\nfeature=1\nproof_mode=%s\noutput=%s\n' \
      "$git_head" "$profile" "$macro" "$proof_mode" "$profile_dir"
    if [[ $family == independent ]]; then
      printf 'read_profile=%s\nwrite_profile=%s\n' "$read_profile" "$write_profile"
    fi
    printf 'source=%s\ntestbench=%s\nchecker=%s\nrunner=%s\n' \
      "$source_file" "$testbench" "$checker" "$runner_file"
    printf 'shared_runner=%s\n' "$shared_runner"
    if [[ $family == independent ]]; then printf 'parallel_helper=%s\n' "$helpers"; fi
    printf 'bambu=%s\nsettings=%s\nplugin=%s\nsimulator=%s\nsimulator_invocation=%s\n' \
      "$bambu" "$(realpath "$settings")" "$plugin" "$simulator" "$simulator_invocation"
    printf 'source_sha256=%s\ntestbench_sha256=%s\nchecker_sha256=%s\nrunner_sha256=%s\n' \
      "$(digest "$source_file")" "$(digest "$testbench")" "$(digest "$checker")" "$(digest "$runner_file")"
    printf 'shared_runner_sha256=%s\n' "$(digest "$shared_runner")"
    if [[ $family == independent ]]; then printf 'parallel_helper_sha256=%s\n' "$(digest "$helpers")"; fi
    printf 'bambu_sha256=%s\nsettings_sha256=%s\nplugin_sha256=%s\nsimulator_sha256=%s\n' \
      "$(digest "$bambu")" "$(digest "$settings")" "$(digest "$plugin")" "$(digest "$simulator")"
    printf 'simulator_version='; "$simulator_invocation" -version 2>&1 | sed -n '1p' || true
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
  if ! grep -F -x -q "$marker" "$testbench_log"; then
    echo "$profile missing exact testbench marker: $marker" >&2
    tail -n 80 "$testbench_log" >&2 || true
    return 1
  fi
  if [[ $family == fallback ]]; then
    find "$profile_dir" -type f \( -name '*.v' -o -name '*.sv' -o -name '*.xml' -o -name '*.log' \) -print0 | sort -z |
      while IFS= read -r -d '' artifact; do
        printf '%s  %s\n' "$(digest "$artifact")" "$artifact"
      done >"$profile_dir/artifact-sha256.txt"
  else
    (
      cd "$profile_dir"
      find . -type f \( -name '*.v' -o -name '*.sv' -o -name '*.xml' -o -name '*.log' -o -name '*.raw' \) -print0 | sort -z |
        while IFS= read -r -d '' artifact; do
          artifact=${artifact#./}
          printf '%s  %s\n' "$(digest "$profile_dir/$artifact")" "$artifact"
        done
    ) >"$profile_dir/artifact-sha256.txt"
  fi
}

if ! run_bounded_jobs "$requested_jobs" run_profile "${profiles[@]}"; then
  if [[ $family == fallback ]]; then
    echo "DUPLEX_FALLBACK_FAILED output=$out" | tee -a "$out/aggregate.log" >&2
  else
    printf 'DUPLEX_INDEPENDENT_FAILED output=%s\n' "$out" | tee -a "$out/aggregate.log" >&2
  fi
  exit 1
fi

report="$out/fallback.json"
if [[ $family == independent ]]; then report="$out/independent.json"; fi
checker_args=(--bambu "$bambu" --settings "$settings" --plugin "$plugin" --simulator "$simulator"
  --runner "$runner_file" --report "$report")
if [[ $proof_mode == developer ]]; then
  checker_args+=(--developer-proof)
fi
if [[ $family == independent ]]; then
  checker_args+=(--simulator-invocation "$simulator_invocation")
fi
if ! python3 -B "$checker" "$out" --source "$source_file" --testbench "$testbench" \
  "${checker_args[@]}" >"$out/checker.log" 2>&1; then
  cat "$out/checker.log" >&2
  if [[ $family == fallback ]]; then
    printf 'DUPLEX_FALLBACK_FAILED output=%s\n' "$out" | tee -a "$out/aggregate.log" >&2
  else
    printf 'DUPLEX_INDEPENDENT_FAILED output=%s\n' "$out" | tee -a "$out/aggregate.log" >&2
  fi
  exit 1
fi
cat "$out/checker.log"
if [[ $family == fallback ]]; then
  profile_list=$(IFS=,; printf '%s' "${profiles[*]}")
  printf 'DUPLEX_FALLBACK_PASS profiles=%s output=%s\n' "$profile_list" "$out" | tee "$out/aggregate.log"
else
  printf 'DUPLEX_INDEPENDENT_PASS profiles=%s,%s output=%s\n' "${profiles[0]}" "${profiles[1]}" "$out" | tee "$out/aggregate.log"
fi
