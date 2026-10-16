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
  local profile_fixture="$TESTS/write_compiler_cases/write_profile_matrix.cpp"
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
    if [[ "$expectation" == legacy && $rc -ne 0 ]]; then echo "expected $top to compile on the legacy path, rc=$rc; see $dir" >&2; return 1; fi
    case "$expectation" in
      accepted) ;;
      fallback) ;;
      invalid) python3 "$checker" invalid --output "$dir" --reason "$reason" ;;
      legacy) python3 "$checker" legacy --output "$dir" --bundle data --require-legacy ;;
    esac
  }
  # Retained IR confirms only that a source-level runtime count exists; it does
  # not prove the post-InterfaceInfer count bound or qualify runtime behavior.
  run_accepted() {
    local top=$1 tag=$2 count_regex=$3 copy=${4:-0}
    run_case "$top" "$tag" accepted
    local copy_arg=()
    [[ "$copy" == 1 ]] && copy_arg+=(--copy)
    python3 "$checker" accepted --output "$OUT/write/$tag" --count-pattern "$count_regex" "${copy_arg[@]}"
  }
  run_fallback() {
    local top=$1 tag=$2 reason=$3 feature_value=${4:-1} bundle=${5:-data} direction=${6:-writes}
    run_case "$top" "$tag" fallback "$reason" "$feature_value"
    python3 "$checker" fallback --output "$OUT/write/$tag" --reason "$reason" \
      --bundle "$bundle" --direction "$direction"
  }
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
  run_fallback byte_store byte-width 'data width/alignment is not 32-bit/4-byte'
  run_fallback halfword_store halfword-width 'data width/alignment is not 32-bit/4-byte'
  run_fallback volatile_effect volatile-effect 'opaque, volatile, atomic, or unclassified memory effects'
  run_fallback call_effect_before_region opaque-effect 'opaque, volatile, atomic, or unclassified memory effects'
  run_fallback mixed_recognized_and_unknown whole-bundle-fallback \
    'opaque, volatile, atomic, or unclassified memory effects'
  run_fallback read_write_same_bundle same-bundle-mixed-mode 'bundle contains reads; same-bundle read/write requires duplex support' 1 shared writes
  run_fallback read_write_distinct_buffers_same_bundle same-bundle-distinct-buffers-mixed-mode 'bundle contains reads; same-bundle read/write requires duplex support' 1 shared writes
  run_fallback read_write_same_bundle same-bundle-read-stores 'bundle contains stores' 1 shared accesses
  run_fallback cache_write cache-fallback 'cache-enabled bundle'
  run_case write_count feature-off fallback 'explicit write-burst request ignored because experimental-m-axi-burst=0' 0
  python3 "$checker" fallback --output "$OUT/write/feature-off" --bundle output --feature-off \
    --reason 'explicit write-burst request ignored because experimental-m-axi-burst=0'
  for profile in write_burst_one write_burst_three_default_fifo write_burst_256; do
    case "$profile" in
      write_burst_one) burst=1 ;;
      write_burst_three_default_fifo) burst=3 ;;
      write_burst_256) burst=256 ;;
    esac
    run_case "$profile" "profile-${burst}" accepted '' 1 "$profile_fixture"
    python3 "$checker" accepted --output "$OUT/write/profile-${burst}" --expected-write-burst "$burst"
    if [[ "$profile" == write_burst_three_default_fifo ]]; then
      python3 "$checker" profile --output "$OUT/write/profile-${burst}" \
        --expect-engine-parameter B_MAX=3 --expect-engine-parameter MAX_OUTSTANDING=1 \
        --expect-engine-parameter FIFO_DEPTH=8
    fi
  done
  run_case write_outstanding_one profile-outstanding-1 accepted '' 1 "$profile_fixture"
  python3 "$checker" accepted --output "$OUT/write/profile-outstanding-1" --expected-write-burst 16
  run_invalid_profile write_outstanding_sixteen f1-o1-limit \
    "Configured write bursts for bundle 'data' currently support write_burst_outstanding=1 only" "$profile_fixture"
  run_invalid_profile write_outstanding_zero invalid-outstanding-zero \
    'write_burst_outstanding must be an integer in \[1,16\]' "$TESTS/write_compiler_cases/invalid_profiles/write_outstanding_zero.cpp"
  run_case write_duplicate_equal_profile duplicate-equal-profile accepted '' 1 "$profile_fixture"
  python3 "$checker" accepted --output "$OUT/write/duplicate-equal-profile" --expected-write-burst 16
  run_invalid_profile write_burst_257 invalid-burst-257 \
    'Maximum AXI burst length must be an integer in \[1,256\]' "$TESTS/write_compiler_cases/invalid_profiles/write_burst_257.cpp"
  run_invalid_profile write_fifo_out_of_range invalid-fifo-4097 \
    'write_fifo_depth must be a power-of-two integer in \[1,4096\] beats' "$TESTS/write_compiler_cases/invalid_profiles/write_fifo_out_of_range.cpp"
  run_invalid_profile write_outstanding_without_burst invalid-outstanding-without-burst \
    "Invalid AXI interface attribute 'max_write_burst_length' for bundle 'data': attribute is required" "$TESTS/write_compiler_cases/invalid_profiles/write_outstanding_without_burst.cpp"
  run_invalid_profile write_attributes_non_m_axi non-m-axi-profile \
    "Maximum AXI burst length attributes are only valid for mode=m_axi" \
    "$TESTS/write_compiler_cases/invalid_profiles/write_attributes_non_m_axi.cpp"
  run_case scalar_m_axi_without_burst no-burst-feature-off legacy '' 0 \
    "$TESTS/write_compiler_cases/legacy_m_axi_without_write_burst.cpp"
  run_invalid_profile invalid_write_profile invalid-zero \
    'Maximum AXI burst length must be an integer in \[1,256\]' "$TESTS/write_compiler_cases/invalid_profiles/invalid_write_profile.cpp"
  run_invalid_profile outstanding_two_profile f1-o1-limitation \
    "Configured write bursts for bundle 'data' currently support write_burst_outstanding=1 only" "$TESTS/write_compiler_cases/invalid_profiles/outstanding_two_profile.cpp"
  run_case valid_write_profile valid-write-profile accepted
  python3 "$checker" accepted --output "$OUT/write/valid-write-profile"
  python3 "$checker" profile --output "$OUT/write/valid-write-profile" \
    --expect-profile-attribute write_burst_outstanding=1 --expect-profile-attribute write_fifo_depth=64
  run_invalid_profile invalid_write_outstanding invalid-outstanding \
    'write_burst_outstanding must be an integer in \[1,16\]' \
    "$TESTS/write_compiler_cases/invalid_profiles/invalid_write_outstanding.cpp"
  run_invalid_profile invalid_write_fifo_non_power_two invalid-fifo-not-power-two \
    'write_fifo_depth must be a power-of-two integer in \[1,4096\] beats' \
    "$TESTS/write_compiler_cases/invalid_profiles/invalid_write_fifo_non_power_two.cpp"
  run_invalid_profile invalid_write_fifo_too_shallow invalid-fifo-below-bmax \
    "Invalid AXI interface attribute 'write_fifo_depth' for bundle 'data': must be at least max_write_burst_length" \
    "$TESTS/write_compiler_cases/invalid_profiles/invalid_write_fifo_too_shallow.cpp"
  run_invalid_profile invalid_write_fifo_without_burst invalid-fifo-without-burst \
    "Invalid AXI interface attribute 'max_write_burst_length' for bundle 'data': attribute is required" \
    "$TESTS/write_compiler_cases/invalid_profiles/invalid_write_fifo_without_burst.cpp"
  run_invalid_profile conflicting_write_burst_profile conflicting-write-profile \
    "Conflicting maximum AXI burst length values in interface bundle 'data'" \
    "$TESTS/write_compiler_cases/invalid_profiles/conflicting_write_burst_profile.cpp"
  run_invalid_profile conflicting_write_fifo_profile conflicting-write-fifo-profile \
    "Conflicting write_fifo_depth values in interface bundle 'data'" \
    "$TESTS/write_compiler_cases/invalid_profiles/conflicting_write_fifo_profile.cpp"
}

if [[ "$MODE" == write || "$MODE" == all ]]; then run_write_suite; fi
if [[ "$MODE" == read-compat || "$MODE" == all ]]; then
  # Compatibility-only invocation; never runs by default as part of `write`.
  "$TESTS/run_compiler_tests.sh" burst-profile "$BAMBU" "$OUT/read-compat"
fi
echo "Manual compiler checks complete; no simulation was run. Output: $OUT"
