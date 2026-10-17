#!/usr/bin/env bash
# Manual application-wide same-bundle duplex activation matrix.
set -euo pipefail

TESTS=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)
ROOT=$(realpath "$TESTS/../../..")
SOURCE="$TESTS/compiler_cases/duplex_effects/multi_root_activation.cpp"
CHECKER="$TESTS/check_duplex_activation_matrix.py"
CORE_SOURCE="$ROOT/src/frontend_analysis/IR_analysis/InterfaceInfer.cpp"
PARALLEL_HELPERS="$TESTS/parallel_helpers.sh"
CASES=(compatible compatible_reversed unproved unproved_reversed single_direction single_direction_reversed
  unused unused_reversed dataflow dataflow_reversed reserved feature_off incompatible incompatible_reversed
  unrelated_dataflow called architectureless)
CASE_IDS=(1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17)

if [[ $# -lt 1 || $# -gt 2 ]]; then
  echo "usage: $0 <bambu> [<fresh-output-directory>]" >&2
  exit 2
fi
BAMBU_ARG=$1
OUT_ARG=${2:-"$ROOT/documentation/tmp/out_duplex_activation_matrix"}
if [[ "$BAMBU_ARG" == */* ]]; then BAMBU=$(realpath "$BAMBU_ARG"); else BAMBU=$(command -v "$BAMBU_ARG" || true); [[ -n "$BAMBU" ]] && BAMBU=$(realpath "$BAMBU"); fi
if [[ -z ${BAMBU:-} || ! -x "$BAMBU" ]]; then echo "Bambu executable not found: $BAMBU_ARG" >&2; exit 2; fi
PREFIX=$(dirname "$(dirname "$BAMBU")")
SETTINGS="$PREFIX/settings.sh"
if [[ -f "$SETTINGS" ]]; then
  # shellcheck disable=SC1090
  source "$SETTINGS"
fi
if [[ -z ${BAMBU_HLS:-} ]]; then echo "BAMBU_HLS is unset; refusing to guess the ASTAnalyzer plugin" >&2; exit 2; fi
PLUGIN="$(realpath "$BAMBU_HLS")/lib/panda/clang-16/ASTAnalyzer.so"
[[ -r "$PLUGIN" ]] || { echo "selected ASTAnalyzer plugin is missing: $PLUGIN" >&2; exit 2; }
for tool in rg sha256sum; do command -v "$tool" >/dev/null || { echo "required tool missing: $tool" >&2; exit 2; }; done
[[ -f "$SOURCE" && -f "$CHECKER" && -f "$CORE_SOURCE" && -f "$PARALLEL_HELPERS" ]] || {
  echo "matrix source/checker/helper missing" >&2; exit 2;
}
[[ ! -e "$OUT_ARG" && ! -L "$OUT_ARG" ]] || { echo "output path must be fresh: $OUT_ARG" >&2; exit 2; }
DEFAULT_JOBS=$(nproc 2>/dev/null || printf '4')
if [[ ! $DEFAULT_JOBS =~ ^[1-9][0-9]*$ ]]; then DEFAULT_JOBS=4; fi
MATRIX_JOBS=${DUPLEX_MATRIX_JOBS:-$DEFAULT_JOBS}
[[ $MATRIX_JOBS =~ ^[1-9][0-9]*$ ]] || { echo "DUPLEX_MATRIX_JOBS must be positive" >&2; exit 2; }
MATRIX_JOBS=$((10#$MATRIX_JOBS))
(( MATRIX_JOBS <= ${#CASES[@]} )) || MATRIX_JOBS=${#CASES[@]}

OUT=$(realpath -m "$OUT_ARG")
mkdir -p "$OUT/tmp" "$OUT/job_logs"
OUT=$(realpath "$OUT")
export PYTHONDONTWRITEBYTECODE=1

digest() { sha256sum "$1" | awk '{print $1}'; }
GIT_HEAD=$(git -C "$ROOT" rev-parse HEAD)
COMMON=(--AXI-burst-type=INCREMENTAL --compiler=I386_CLANG16 --generate-interface=INFER
  --device-name=xcu250,-2L,figd2104 --clock-period=3.33 --bambu-parameter=print-ir-manager=1
  --debug-classes=InterfaceInfer --print-dot --no-clean --extra-cc-options=-DNDEBUG
  --extra-cc-options=-DBAMBU)

run_case() {
  local name=$1 case_id=0 index=0 out="$OUT/$1" rc=0 tops=matrix_candidate,matrix_peer
  for index in "${!CASES[@]}"; do
    if [[ ${CASES[$index]} == "$name" ]]; then case_id=${CASE_IDS[$index]}; break; fi
  done
  [[ $case_id -gt 0 ]] || { echo "unknown matrix case $name" >&2; return 2; }
  case "$case_id" in
    11|12) tops=matrix_candidate ;;
    16) tops=matrix_candidate,matrix_helper,matrix_peer ;;
    17) tops=matrix_candidate,unrelated_architectureless ;;
  esac
  mkdir "$out" "$out/tmp"
  local -a args=("--top-fname=$tops" "${COMMON[@]}"
    "--extra-cc-options=-DDUPLEX_MATRIX_CASE=$case_id")
  if [[ $name == *_reversed ]]; then args+=(--extra-cc-options=-DDUPLEX_MATRIX_REVERSED); fi
  if [[ $case_id == 12 ]]; then
    args+=(--bambu-parameter=experimental-m-axi-burst=0)
  else
    args+=(--bambu-parameter=experimental-m-axi-burst=1)
  fi
  {
    printf 'git_head=%s\ncase=%s\ncase_id=%s\nbambu=%s\nbambu_sha256=%s\nplugin=%s\nplugin_sha256=%s\nsource=%s\nsource_sha256=%s\ncore_source=%s\ncore_source_sha256=%s\nchecker=%s\nchecker_sha256=%s\nrunner=%s\nrunner_sha256=%s\nparallel_helpers=%s\nparallel_helpers_sha256=%s\n' \
      "$GIT_HEAD" \
      "$name" "$case_id" "$BAMBU" "$(digest "$BAMBU")" "$PLUGIN" "$(digest "$PLUGIN")" \
      "$SOURCE" "$(digest "$SOURCE")" "$CORE_SOURCE" "$(digest "$CORE_SOURCE")" \
      "$CHECKER" "$(digest "$CHECKER")" "$TESTS/run_duplex_activation_matrix.sh" \
      "$(digest "$TESTS/run_duplex_activation_matrix.sh")" "$PARALLEL_HELPERS" "$(digest "$PARALLEL_HELPERS")"
  } >"$out/provenance.txt"
  printf '%q ' "$BAMBU" "$SOURCE" "${args[@]}" >"$out/argv.txt"; printf '\n' >>"$out/argv.txt"
  if (cd "$out" && TMPDIR="$out/tmp" TMP="$out/tmp" TEMP="$out/tmp" \
      "$BAMBU" "$SOURCE" "${args[@]}" >stdout.log 2>stderr.log); then rc=0; else rc=$?; fi
  python3 "$CHECKER" "$case_id" --output "$out" --return-code "$rc"
  printf 'DUPLEX_ACTIVATION_MATRIX PASS CASE=%s\n' "$name"
}

BURST_JOB_LOG_DIR="$OUT/job_logs"
BURST_QUIET_JOB_STATUS=1
source "$PARALLEL_HELPERS"
printf 'Starting %d activation matrix cases, bounded to %s jobs: %s\n' "${#CASES[@]}" "$MATRIX_JOBS" "$OUT"
run_bounded_jobs "$MATRIX_JOBS" run_case "${CASES[@]}"
for name in "${CASES[@]}"; do
  rg -q "^DUPLEX_ACTIVATION_MATRIX PASS CASE=$name$" "$BURST_JOB_LOG_DIR/$name.log" || {
    echo "missing exact matrix pass marker for $name" >&2; exit 1;
  }
done
printf 'DUPLEX_ACTIVATION_MATRIX PASS: %d isolated compiler cases\n' "${#CASES[@]}"
