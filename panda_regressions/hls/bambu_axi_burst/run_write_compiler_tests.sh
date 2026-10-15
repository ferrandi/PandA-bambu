#!/usr/bin/env bash
# Manual, compiler-only F1b gate; deliberately not wired into CTest or CI.
# Usage: run_write_compiler_tests.sh {write|read-compat|all} BAMBU OUTPUT_DIR
# OUTPUT_DIR must not exist. BAMBU is the exact executable to test; the script
# pins settings.sh/plugin provenance to its prefix and records the full argv.
set -euo pipefail
TESTS=$(cd "$(dirname "$0")" && pwd)
ROOT=$(realpath "$TESTS/../../..")
if [[ ${1:-} == -h || ${1:-} == --help ]]; then sed -n '2,5p' "$0"; exit 0; fi
MODE=${1:?mode required}; BAMBU_ARG=${2:?Bambu executable required}; OUT_ARG=${3:?fresh output directory required}
case "$MODE" in write|read-compat|all) ;; *) echo "invalid mode: $MODE" >&2; exit 2 ;; esac
if [[ "$BAMBU_ARG" == */* ]]; then BAMBU=$(realpath "$BAMBU_ARG"); else BAMBU=$(command -v "$BAMBU_ARG" || true); [[ -n "$BAMBU" ]] && BAMBU=$(realpath "$BAMBU"); fi
if [[ -z ${BAMBU:-} || ! -x "$BAMBU" ]]; then echo "Bambu executable not found: $BAMBU_ARG" >&2; exit 2; fi
PREFIX=$(dirname "$(dirname "$BAMBU")")
SETTINGS="$PREFIX/settings.sh"
if [[ -f "$SETTINGS" ]]; then
  # shellcheck disable=SC1090
  source "$SETTINGS"
  if [[ $(realpath -m "${BAMBU_HLS:-}") != $(realpath "$PREFIX") ]]; then
    echo "settings.sh selected BAMBU_HLS='${BAMBU_HLS:-<unset>}' instead of $PREFIX" >&2; exit 2
  fi
fi
if [[ -e "$OUT_ARG" || -L "$OUT_ARG" ]]; then echo "output directory must not already exist: $OUT_ARG" >&2; exit 2; fi
OUT=$(realpath -m "$OUT_ARG")
mkdir -p "$OUT/tmp"
export TMPDIR="$OUT/tmp" TMP="$OUT/tmp" TEMP="$OUT/tmp"
BURST_PARAMETER=${BURST_PARAMETER:-experimental-m-axi-burst}
if [[ -z ${BAMBU_HLS:-} ]]; then echo "BAMBU_HLS is unset; refusing to guess ASTAnalyzer plugin" >&2; exit 2; fi
PLUGIN="$(realpath "$BAMBU_HLS")/lib/panda/clang-16/ASTAnalyzer.so"
if [[ ! -r "$PLUGIN" ]]; then echo "selected ASTAnalyzer plugin is missing: $PLUGIN" >&2; exit 2; fi
{
  printf 'bambu_executable=%s\nbambu_prefix=%s\nBAMBU_HLS=%s\nplugin=%s\n' "$BAMBU" "$PREFIX" "$(realpath "$BAMBU_HLS")" "$PLUGIN"
  printf 'TMPDIR=%s\n' "$TMPDIR"
  sha256sum "$BAMBU" "$PLUGIN"
} >"$OUT/tool_provenance.txt"

run_write_suite() {
  if ! "$BAMBU" --list-bambu-parameters >"$OUT/bambu_parameters.txt" 2>&1; then echo "cannot query Bambu parameters" >&2; return 2; fi
  if ! awk -v name="$BURST_PARAMETER" '$1 == name { found=1 } END { exit !found }' "$OUT/bambu_parameters.txt"; then
    echo "Bambu does not register '$BURST_PARAMETER'; use an F1b-enabled build" >&2; return 2
  fi
  local fixture="$TESTS/write_compiler_cases/write_cases.cpp"
  local checker="$TESTS/check_write_compiler_artifacts.py"
  local feature=1
  run_case() {
    local top=$1 tag=$2 expectation=$3 reason=${4:-} feature_value=${5:-$feature}
    local source_file=${6:-$fixture}
    local dir="$OUT/write/$tag" rc=0
    mkdir -p "$dir"
    local args=("--top-fname=$top" --compiler=I386_CLANG16 --generate-interface=INFER
      --device-name=xcu250,-2L,figd2104 --AXI-burst-type=INCREMENTAL --no-clean
      "--bambu-parameter=${BURST_PARAMETER}=${feature_value}"
      --extra-cc-options=-DNDEBUG --extra-cc-options=-DBAMBU)
    printf '%q ' "$BAMBU" "$source_file" "${args[@]}" >"$dir/argv.txt"; printf '\n' >>"$dir/argv.txt"
    if (cd "$dir" && "$BAMBU" "$source_file" "${args[@]}" >stdout.log 2>stderr.log); then rc=0; else rc=$?; fi
    if [[ "$expectation" == accepted && $rc -ne 0 ]]; then echo "expected $top accepted, rc=$rc; see $dir" >&2; return 1; fi
    if [[ "$expectation" == invalid && $rc -eq 0 ]]; then echo "expected $top rejected, but it compiled; see $dir" >&2; return 1; fi
    if [[ "$expectation" == fallback && $rc -ne 0 ]]; then echo "expected $top to compile with legacy fallback, rc=$rc; see $dir" >&2; return 1; fi
    case "$expectation" in
      accepted) ;;
      fallback) python3 "$checker" fallback --output "$dir" --reason "$reason" ;;
      invalid) python3 "$checker" invalid --output "$dir" --reason "$reason" ;;
    esac
  }
  # Positive artifact contract checks. The runtime bound is checked against the
  # retained IR, not inferred from successful compilation alone.
  run_accepted() {
    local top=$1 tag=$2 count_regex=$3 copy=${4:-0}
    run_case "$top" "$tag" accepted
    local copy_arg=()
    [[ "$copy" == 1 ]] && copy_arg+=(--copy)
    python3 "$checker" accepted --output "$OUT/write/$tag" --count-pattern "$count_regex" "${copy_arg[@]}"
  }
  run_fallback() { run_case "$1" "$2" fallback "$3" "${4:-1}"; }
  run_invalid_profile() { run_case "$1" "$2" invalid "$3" 1 "$4"; }
  run_accepted write_count runtime-count '(%?[A-Za-z0-9_.]*n|region\.bound|write_count)'
  run_accepted write_count_seed seeded-runtime-count '(%?[A-Za-z0-9_.]*n|region\.bound|write_count_seed)'
  for top in write_with_unused_same_bundle write_with_unused_same_bundle_reversed; do
    tag=${top//_/-}
    run_case "$top" "$tag" accepted
    python3 "$checker" accepted --output "$OUT/write/$tag" --single-write-bundle output
  done
  run_accepted write_nonpositive_bound nonpositive-signed-bound '(%?[A-Za-z0-9_.]*n|region\.bound|write_nonpositive_bound)'
  run_accepted copy_distinct_buffers distinct-buffers-copy '(%?[A-Za-z0-9_.]*n|region\.bound)' 1
  run_accepted copy_distinct_buffers_reversed distinct-buffers-copy-reversed '(%?[A-Za-z0-9_.]*n|region\.bound)' 1
  run_fallback has_break break 'write-burst CFG/SSA/effect recognition failed'
  run_fallback early_return early-return 'write-burst CFG/SSA/effect recognition failed'
  run_fallback conditional_store conditional-store 'write-burst CFG/SSA/effect recognition failed'
  run_fallback strided_store stride 'write-burst CFG/SSA/effect recognition failed'
  run_fallback byte_store byte-width '32.bit|width|size'
  run_fallback halfword_store halfword-width '32.bit|width|size'
  run_fallback volatile_effect volatile-effect 'volatile|effect|ordering'
  run_fallback call_effect_before_region opaque-effect 'opaque|effect|ordering|call'
  run_fallback mixed_recognized_and_unknown whole-bundle-fallback \
    'Keeping legacy m_axi writes for bundle data: opaque, volatile, atomic, or unclassified memory effects'
  run_fallback read_write_same_bundle same-bundle-mixed-mode 'mixed.mode|duplex|read/write|read and write|same.bundle'
  run_fallback read_write_distinct_buffers_same_bundle same-bundle-distinct-buffers-mixed-mode 'mixed.mode|duplex|read/write|read and write|same.bundle'
  run_fallback cache_write cache-fallback 'cache|unsupported'
  run_fallback write_count feature-off 'disabled|opt.out|feature' 0
  run_invalid_profile invalid_write_profile invalid-zero \
    'max_write_burst_length|range|zero|positive' "$TESTS/write_compiler_cases/invalid_profiles/invalid_write_profile.cpp"
  run_invalid_profile outstanding_two_profile f1-o1-limitation \
    'write_burst_outstanding|outstanding.*1|O=1' "$TESTS/write_compiler_cases/invalid_profiles/outstanding_two_profile.cpp"
  run_case valid_write_profile valid-write-profile accepted
  python3 "$checker" accepted --output "$OUT/write/valid-write-profile"
  python3 "$checker" profile --output "$OUT/write/valid-write-profile" \
    --expect-profile-attribute write_burst_outstanding=1 --expect-profile-attribute write_fifo_depth=64
  run_invalid_profile invalid_write_outstanding invalid-outstanding \
    'write_burst_outstanding.*(range|between|1.*16|\[1,16\])' \
    "$TESTS/write_compiler_cases/invalid_profiles/invalid_write_outstanding.cpp"
  run_invalid_profile invalid_write_fifo_non_power_two invalid-fifo-not-power-two \
    'write_fifo_depth.*(power.of.two|power of two)' \
    "$TESTS/write_compiler_cases/invalid_profiles/invalid_write_fifo_non_power_two.cpp"
  run_invalid_profile invalid_write_fifo_too_shallow invalid-fifo-below-bmax \
    'write_fifo_depth.*(at least|greater than or equal|B_MAX|burst length)' \
    "$TESTS/write_compiler_cases/invalid_profiles/invalid_write_fifo_too_shallow.cpp"
  run_invalid_profile invalid_write_fifo_without_burst invalid-fifo-without-burst \
    'max_write_burst_length.*attribute is required' \
    "$TESTS/write_compiler_cases/invalid_profiles/invalid_write_fifo_without_burst.cpp"
  run_invalid_profile conflicting_write_burst_profile conflicting-write-profile \
    'Conflicting max_write_burst_length|conflicting.*write.*burst' \
    "$TESTS/write_compiler_cases/invalid_profiles/conflicting_write_burst_profile.cpp"
  run_invalid_profile conflicting_write_fifo_profile conflicting-write-fifo-profile \
    'Conflicting write_fifo_depth values in interface bundle' \
    "$TESTS/write_compiler_cases/invalid_profiles/conflicting_write_fifo_profile.cpp"
}

if [[ "$MODE" == write || "$MODE" == all ]]; then run_write_suite; fi
if [[ "$MODE" == read-compat || "$MODE" == all ]]; then
  # Compatibility-only invocation; never runs by default as part of `write`.
  "$TESTS/run_compiler_tests.sh" burst-profile "$BAMBU" "$OUT/read-compat"
fi
echo "Manual compiler checks complete; no simulation was run. Output: $OUT"
