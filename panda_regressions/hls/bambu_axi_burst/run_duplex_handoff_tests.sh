#!/usr/bin/env bash
# Compiler-only witness for same-bundle duplex handoff into scheduling.
set -Eeuo pipefail

script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)
repo_root=$(git -C "$script_dir/../../.." rev-parse --show-toplevel)
runner_file=$(realpath "${BASH_SOURCE[0]}")
checker="$script_dir/check_duplex_handoff.py"
effect_checker="$script_dir/check_duplex_effect_analysis_artifacts.py"
burst_checker="$script_dir/check_burst_results.py"
helpers="$script_dir/parallel_helpers.sh"
copy_source="$script_dir/bambu_axi_burst_duplex_copy.cpp"
independent_source="$script_dir/bambu_axi_burst_duplex_independent.cpp"
tmp_root="$repo_root/documentation/tmp"

usage() { echo "usage: $0 BAMBU [FRESH_OUTPUT_DIR]" >&2; }
if (($# < 1 || $# > 2)); then usage; exit 2; fi
bambu_arg=$1
if [[ $bambu_arg == */* ]]; then bambu=$(realpath "$bambu_arg" 2>/dev/null || true)
else bambu=$(command -v "$bambu_arg" || true); [[ -z $bambu ]] || bambu=$(realpath "$bambu"); fi
[[ -n ${bambu:-} && -x $bambu ]] || { echo "Bambu executable not found: $bambu_arg" >&2; exit 2; }
for tool in python3 sha256sum rg; do command -v "$tool" >/dev/null || { echo "required tool not found: $tool" >&2; exit 2; }; done
[[ -r $checker && -r $effect_checker && -r $burst_checker && -r $helpers && -r $copy_source && -r $independent_source ]] || {
  echo "duplex handoff checker, sources, or parallel helper missing" >&2; exit 2;
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

requested_jobs=${DUPLEX_HANDOFF_JOBS:-4}
if [[ ! $requested_jobs =~ ^[0-9]+$ ]]; then
  echo "DUPLEX_HANDOFF_JOBS must be a positive integer (got '$requested_jobs')" >&2; exit 2
fi
while [[ $requested_jobs == 0* && ${#requested_jobs} -gt 1 ]]; do requested_jobs=${requested_jobs#0}; done
if [[ $requested_jobs == 0 || ${#requested_jobs} -gt 10 ]] || \
  { ((${#requested_jobs} == 10)) && [[ $requested_jobs > 2147483647 ]]; }; then
  echo "DUPLEX_HANDOFF_JOBS must be between 1 and 2147483647 (got '${DUPLEX_HANDOFF_JOBS:-}')" >&2; exit 2
fi
requested_jobs=$((10#$requested_jobs))
(( requested_jobs <= 4 )) || requested_jobs=4

out_arg=${2:-"$tmp_root/out_duplex_handoff"}
if [[ -e $out_arg || -L $out_arg ]]; then
  echo "output directory must be fresh and not a symlink: $out_arg" >&2; exit 2
fi
out=$(realpath -m -- "$out_arg")
tmp_root_real=$(realpath -m -- "$tmp_root")
case "$out/" in
  "$tmp_root_real/"*) ;;
  *) echo "output directory must be under $tmp_root_real: $out" >&2; exit 2 ;;
esac

mkdir -p "$tmp_root"
mkdir "$out"
out=$(realpath "$out")
mkdir "$out/job_logs" "$out/tmp"
export TMPDIR="$out/tmp" TMP="$out/tmp" TEMP="$out/tmp" PYTHONDONTWRITEBYTECODE=1

digest() { sha256sum -- "$1" | awk '{print $1}'; }
git_head=$(git -C "$repo_root" rev-parse HEAD)
profiles=(copy-symmetric copy-asymmetric independent-symmetric independent-asymmetric)
profile_marker() { printf 'DUPLEX_HANDOFF_PASS CASE=%s\n' "$1"; }
{
  printf 'git_head=%s\noutput=%s\nbambu=%s\nbambu_sha256=%s\nsettings=%s\nsettings_sha256=%s\nplugin=%s\nplugin_sha256=%s\njobs=%s\n' \
    "$git_head" "$out" "$bambu" "$(digest "$bambu")" "$settings" "$(digest "$settings")" \
    "$plugin" "$(digest "$plugin")" "$requested_jobs"
  for file in "$copy_source" "$independent_source" "$checker" "$effect_checker" "$burst_checker" "$runner_file" "$helpers"; do
    printf 'input=%s\ninput_sha256=%s\n' "$file" "$(digest "$file")"
  done
  printf 'bambu_version_begin\n'; "$bambu" --version 2>&1; printf 'bambu_version_end\n'
} >"$out/provenance.txt"

source "$helpers"
BURST_JOB_LOG_DIR="$out/job_logs"
BURST_QUIET_JOB_STATUS=1

run_profile() {
  local case_name=$1 kind profile macro source top bits directory marker addr_arg
  case "$case_name" in
    copy-symmetric) kind=copy; profile=symmetric; macro=1; source=$copy_source; top=kernel; bits=32 ;;
    copy-asymmetric) kind=copy; profile=asymmetric; macro=2; source=$copy_source; top=kernel; bits=32 ;;
    independent-symmetric) kind=independent; profile=symmetric; macro=1; source=$independent_source; top=bambu_axi_burst_duplex_independent; bits=64 ;;
    independent-asymmetric) kind=independent; profile=asymmetric; macro=2; source=$independent_source; top=bambu_axi_burst_duplex_independent; bits=64 ;;
    *) echo "unknown handoff case: $case_name" >&2; return 2 ;;
  esac
  directory="$out/$case_name"
  mkdir "$directory" "$directory/tmp"
  export TMPDIR="$directory/tmp" TMP="$directory/tmp" TEMP="$directory/tmp"
  local -a args=("--top-fname=$top" --compiler=I386_CLANG16 --generate-interface=INFER
    --device-name=xcu250,-2L,figd2104 "--addr-bus-bitsize=$bits" --clock-period=3.33
    --AXI-burst-type=INCREMENTAL --bambu-parameter=experimental-m-axi-burst=1
    --bambu-parameter=print-ir-manager=1 --debug-classes=InterfaceInfer,BuildVirtualPhi,parametric_list_based
    --print-dot --no-clean --extra-cc-options=-DNDEBUG --extra-cc-options=-DBAMBU
    "--extra-cc-options=-D$([[ $kind == copy ]] && echo DUPLEX_PROFILE || echo DUPLEX_INDEPENDENT_PROFILE)=$macro")
  {
    printf 'git_head=%s\ncase=%s\nkind=%s\nprofile=%s\ntop=%s\nsource=%s\nsource_sha256=%s\n' \
      "$git_head" "$case_name" "$kind" "$profile" "$top" "$source" "$(digest "$source")"
    printf 'bambu=%s\nbambu_sha256=%s\nchecker=%s\nchecker_sha256=%s\nrunner=%s\nrunner_sha256=%s\n' \
      "$bambu" "$(digest "$bambu")" "$checker" "$(digest "$checker")" "$runner_file" "$(digest "$runner_file")"
    printf 'argv='; printf '%q ' "$bambu" "$source" "${args[@]}"; printf '\n'
  } >"$directory/provenance.txt"
  if ! (cd "$directory" && "$bambu" "$source" "${args[@]}" >stdout.log 2>stderr.log); then
    tail -n 80 "$directory/stderr.log" >&2 || true
    echo "$case_name compiler invocation failed; see $directory" >&2
    return 1
  fi
  local virtual
  virtual=$(find "$directory/panda-temp" -maxdepth 1 -type f -name 'after_Frontend::BuildVirtualPhi::*.raw' -print -quit 2>/dev/null || true)
  for required in "$directory/panda-temp/before_Frontend::InterfaceInfer.raw" \
      "$directory/panda-temp/after_Frontend::InterfaceInfer.raw" "$directory/$top"_scheduling.xml \
      "$directory/HLS_output/dot/$top/HLS_scheduling.dot" "$directory/HLS_output/dot/$top/OP_DFG.dot" \
      "$directory/HLS_output/dot/$top/OP_ADG.dot" "$directory/$top.v" "$directory/panda-temp/architecture.xml" \
      "$directory/stderr.log"; do
    [[ -f $required && ! -L $required && -s $required ]] || { echo "$case_name missing nonempty artifact: $required" >&2; return 1; }
  done
  [[ -n $virtual && -f $virtual && ! -L $virtual && -s $virtual ]] || {
    echo "$case_name missing nonempty post-BuildVirtualPhi raw" >&2; return 1;
  }
  if ! python3 -B "$checker" "$directory" --top "$top" --profile "$profile" --kind "$kind" \
      >"$directory/checker.log" 2>&1; then
    cat "$directory/checker.log" >&2
    echo "$case_name handoff checker failed" >&2
    return 1
  fi
  local marker; marker=$(profile_marker "$case_name")
  printf '%s\n' "$marker" | tee "$directory/aggregate.log"
  (
    cd "$directory"
    find . -type f ! -name artifact-sha256.txt -print0 | sort -z |
      while IFS= read -r -d '' artifact; do
        artifact=${artifact#./}
        printf '%s  %s\n' "$(digest "$directory/$artifact")" "$artifact"
      done
  ) >"$directory/artifact-sha256.txt"
}

if ! run_bounded_jobs "$requested_jobs" run_profile "${profiles[@]}"; then
  printf 'DUPLEX_HANDOFF_FAILED output=%s\n' "$out" | tee -a "$out/aggregate.log" >&2
  exit 1
fi
for case_name in "${profiles[@]}"; do
  rg -F -x -q "$(profile_marker "$case_name")" "$out/$case_name/aggregate.log" || {
    echo "missing exact PASS marker for $case_name" >&2; exit 1;
  }
done
for case_name in "${profiles[@]}"; do cat "$out/$case_name/aggregate.log"; done >"$out/aggregate.log"
printf 'DUPLEX_HANDOFF_PASS profiles=%s output=%s\n' "${profiles[*]}" "$out" | tee -a "$out/aggregate.log"
