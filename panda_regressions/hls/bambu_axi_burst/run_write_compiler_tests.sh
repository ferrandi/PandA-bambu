#!/usr/bin/env bash
# Manual F1b gate; deliberately not wired into CTest or CI.
# Usage: run_write_compiler_tests.sh {write|read-compat|all|float-sim} BAMBU OUTPUT_DIR
# OUTPUT_DIR must not exist. BAMBU is the exact executable to test; the script
# pins settings.sh/plugin provenance to its prefix and records the full argv.
set -euo pipefail
TESTS=$(cd "$(dirname "$0")" && pwd)
ROOT=$(realpath "$TESTS/../../..")
if [[ ${1:-} == -h || ${1:-} == --help ]]; then sed -n '2,5p' "$0"; exit 0; fi
MODE=${1:?mode required}; BAMBU_ARG=${2:?Bambu executable required}; OUT_ARG=${3:?fresh output directory required}
case "$MODE" in write|read-compat|all|float-sim) ;; *) echo "invalid mode: $MODE" >&2; exit 2 ;; esac
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
BURST_PARAMETER=${BURST_PARAMETER:-m-axi-burst}
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
  run_accepted copy_float_payload copy-float-payload '(%?[A-Za-z0-9_.]*n|region\.bound|copy_float_payload)' 1
  python3 "$checker" accepted --output "$OUT/write/copy-float-payload" \
    --count-pattern '(%?[A-Za-z0-9_.]*n|region\.bound|copy_float_payload)' --copy \
    --expected-read-burst 16 --expected-write-burst 16 \
    --expected-read-bundle input --expected-write-bundle output
  run_case copy_distinct_with_unused_write_owner distinct-buffers-unused-write-owner accepted
  python3 "$checker" accepted --output "$OUT/write/distinct-buffers-unused-write-owner" \
    --count-pattern '(%?[A-Za-z0-9_.]*n|region\.bound)' --copy --single-write-bundle output
  # Cross-bundle coalescing needs the restrict contract on both owners; without it the write
  # burst is refused and the scalar interface is kept.
  run_fallback copy_unrestricted_buffers unrestricted-buffers-copy \
    'reads through another m_axi bundle without a restrict contract on both owners; cross-bundle write ordering is not proven' \
    1 output writes
  run_fallback copy_unrestricted_buffers unrestricted-buffers-read \
    'writes through another m_axi bundle without a restrict contract on both owners; cross-bundle prefetch ordering is not proven' \
    1 input accesses
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
  run_case read_write_same_bundle same-bundle-pointwise-duplex accepted
  python3 "$checker" accepted --output "$OUT/write/same-bundle-pointwise-duplex"
  # Reads and writes on one bundle are a duplex case: the guarded distinct-base duplex mode is
  # the accepted outcome, and the bundle must not fall back to legacy scalar transfers.
  run_case read_write_distinct_buffers_same_bundle same-bundle-distinct-buffers-mixed-mode accepted
  python3 "$checker" duplex --output "$OUT/write/same-bundle-distinct-buffers-mixed-mode" --bundle shared
  run_fallback cache_write cache-fallback 'cache-enabled bundle'
  run_case write_count feature-off fallback 'explicit write-burst request ignored because m-axi-burst=0' 0
  python3 "$checker" fallback --output "$OUT/write/feature-off" --bundle output --feature-off \
    --reason 'explicit write-burst request ignored because m-axi-burst=0'
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
        --expect-engine-parameter B_MAX=3 --expect-engine-parameter MAX_OUTSTANDING=16 \
        --expect-engine-parameter FIFO_DEPTH=8
    fi
  done
  run_case write_outstanding_one profile-outstanding-1 accepted '' 1 "$profile_fixture"
  python3 "$checker" accepted --output "$OUT/write/profile-outstanding-1" --expected-write-burst 16
  # F2: more than one write burst may be in flight. The requested outstanding must survive
  # into the emitted engine generic; the FIFO default stays next_pow2(2*B_MAX).
  run_case write_outstanding_sixteen profile-outstanding-16 accepted '' 1 "$profile_fixture"
  python3 "$checker" accepted --output "$OUT/write/profile-outstanding-16" --expected-write-burst 16
  python3 "$checker" profile --output "$OUT/write/profile-outstanding-16" \
    --expect-profile-attribute write_burst_outstanding=16 \
    --expect-engine-parameter B_MAX=16 --expect-engine-parameter MAX_OUTSTANDING=16 \
    --expect-engine-parameter FIFO_DEPTH=64
  run_invalid_profile write_outstanding_zero invalid-outstanding-zero \
    'write_burst_outstanding must be an integer in \[1,16\]' "$TESTS/write_compiler_cases/invalid_profiles/write_outstanding_zero.cpp"
  run_case write_duplicate_equal_profile duplicate-equal-profile accepted '' 1 "$profile_fixture"
  python3 "$checker" accepted --output "$OUT/write/duplicate-equal-profile" --expected-write-burst 16
  run_invalid_profile write_burst_257 invalid-burst-257 \
    'Maximum AXI burst length must be an integer in \[1,256\]' "$TESTS/write_compiler_cases/invalid_profiles/write_burst_257.cpp"
  run_invalid_profile write_fifo_out_of_range invalid-fifo-4097 \
    'write_fifo_depth must be a power-of-two integer in \[1,4096\] beats' "$TESTS/write_compiler_cases/invalid_profiles/write_fifo_out_of_range.cpp"
  run_case write_outstanding_with_default_burst outstanding-default-burst accepted '' 1 \
    "$TESTS/write_compiler_cases/write_outstanding_with_default_burst.cpp"
  python3 "$checker" accepted --output "$OUT/write/outstanding-default-burst" --expected-write-burst 16
  python3 "$checker" profile --output "$OUT/write/outstanding-default-burst" \
    --expect-profile-attribute write_burst_outstanding=1 \
    --expect-engine-parameter B_MAX=16 --expect-engine-parameter MAX_OUTSTANDING=1 \
    --expect-engine-parameter FIFO_DEPTH=32
  run_invalid_profile write_attributes_non_m_axi non-m-axi-profile \
    "AXI write profile attributes num_write_outstanding, write_burst_outstanding and write_fifo_depth are only valid for mode=m_axi" \
    "$TESTS/write_compiler_cases/invalid_profiles/write_attributes_non_m_axi.cpp"
  run_case scalar_m_axi_without_burst no-burst-feature-off legacy '' 0 \
    "$TESTS/write_compiler_cases/legacy_m_axi_without_write_burst.cpp"
  run_invalid_profile invalid_write_profile invalid-zero \
    'Maximum AXI burst length must be an integer in \[1,256\]' "$TESTS/write_compiler_cases/invalid_profiles/invalid_write_profile.cpp"
  run_case outstanding_two_profile outstanding-two accepted '' 1 \
    "$TESTS/write_compiler_cases/outstanding_two_profile.cpp"
  python3 "$checker" accepted --output "$OUT/write/outstanding-two" --expected-write-burst 16
  python3 "$checker" profile --output "$OUT/write/outstanding-two" \
    --expect-profile-attribute write_burst_outstanding=2 \
    --expect-engine-parameter B_MAX=16 --expect-engine-parameter MAX_OUTSTANDING=2 \
    --expect-engine-parameter FIFO_DEPTH=32
  run_case write_outstanding_three profile-outstanding-3 accepted '' 1 "$profile_fixture"
  python3 "$checker" accepted --output "$OUT/write/profile-outstanding-3" --expected-write-burst 16
  python3 "$checker" profile --output "$OUT/write/profile-outstanding-3" \
    --expect-profile-attribute write_burst_outstanding=3 \
    --expect-engine-parameter B_MAX=16 --expect-engine-parameter MAX_OUTSTANDING=3 \
    --expect-engine-parameter FIFO_DEPTH=32
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
  run_case write_fifo_with_default_burst fifo-default-burst accepted '' 1 \
    "$TESTS/write_compiler_cases/write_fifo_with_default_burst.cpp"
  python3 "$checker" accepted --output "$OUT/write/fifo-default-burst" --expected-write-burst 16
  python3 "$checker" profile --output "$OUT/write/fifo-default-burst" \
    --expect-profile-attribute write_fifo_depth=64 \
    --expect-engine-parameter B_MAX=16 --expect-engine-parameter MAX_OUTSTANDING=16 \
    --expect-engine-parameter FIFO_DEPTH=64
  grep -Fq 'write_max=16(default) write_outstanding=16(default) write_fifo=64(explicit)' \
    "$OUT/write/fifo-default-burst/stderr.log" || {
      echo "FIFO-only profile lost its implicit B16/O16 or explicit D64 provenance" >&2; return 1;
    }
  run_invalid_profile conflicting_write_burst_profile conflicting-write-profile \
    "Conflicting maximum AXI burst length values in interface bundle 'data'" \
    "$TESTS/write_compiler_cases/invalid_profiles/conflicting_write_burst_profile.cpp"
  run_invalid_profile conflicting_write_fifo_profile conflicting-write-fifo-profile \
    "Conflicting write_fifo_depth values in interface bundle 'data'" \
    "$TESTS/write_compiler_cases/invalid_profiles/conflicting_write_fifo_profile.cpp"

  local alias_fixture="$TESTS/compiler_cases/axi_write_outstanding_alias_cases.cpp"
  local alias_dir alias_top alias_macro alias_expect alias_reason rc
  run_alias_case() {
    alias_top=$1 alias_macro=$2 alias_expect=$3 alias_reason=${4:-}
    alias_dir="$OUT/write/alias-$alias_top"
    mkdir -p "$alias_dir"
    local args=("--top-fname=$alias_top" --compiler=I386_CLANG16 --generate-interface=INFER
      --device-name=xcu250,-2L,figd2104 --AXI-burst-type=INCREMENTAL --no-clean
      --extra-cc-options=-DNDEBUG --extra-cc-options=-DBAMBU "--extra-cc-options=-D$alias_macro")
    printf '%q ' "$BAMBU" "$alias_fixture" "${args[@]}" >"$alias_dir/argv.txt"; printf '\n' >>"$alias_dir/argv.txt"
    if (cd "$alias_dir" && "$BAMBU" "$alias_fixture" "${args[@]}" >stdout.log 2>stderr.log); then rc=0; else rc=$?; fi
    if [[ $alias_expect == accepted ]]; then
      [[ $rc -eq 0 ]] || { echo "alias fixture $alias_top failed, rc=$rc; see $alias_dir" >&2; return 1; }
      python3 "$checker" accepted --output "$alias_dir" --expected-write-burst 16
      python3 "$checker" profile --output "$alias_dir" \
        --expect-profile-attribute write_burst_outstanding=3 \
        --expect-engine-parameter B_MAX=16 --expect-engine-parameter MAX_OUTSTANDING=3 \
        --expect-engine-parameter FIFO_DEPTH=32
    elif [[ $alias_expect == fallback ]]; then
      [[ $rc -eq 0 ]] || { echo "alias fixture $alias_top failed, rc=$rc; see $alias_dir" >&2; return 1; }
      python3 "$checker" fallback --output "$alias_dir" --bundle data --direction writes \
        --reason "$alias_reason"
      python3 "$checker" profile --output "$alias_dir" --expect-profile-attribute write_burst_outstanding=3
    else
      [[ $rc -ne 0 ]] || { echo "invalid alias fixture $alias_top compiled successfully; see $alias_dir" >&2; return 1; }
      python3 "$checker" invalid --output "$alias_dir" --reason "$alias_reason"
    fi
  }
  run_alias_case write_outstanding_vitis_alias AXI_WRITE_OUTSTANDING_ALIAS accepted
  run_alias_case write_outstanding_equal_aliases AXI_WRITE_OUTSTANDING_EQUAL accepted
  run_alias_case write_outstanding_equal_ports AXI_WRITE_OUTSTANDING_EQUAL_PORTS fallback \
    'opaque, volatile, atomic, or unclassified memory effects'
  run_alias_case write_outstanding_conflict AXI_WRITE_OUTSTANDING_CONFLICT invalid \
    "Conflicting write_burst_outstanding values in interface bundle 'data'"
  run_alias_case write_outstanding_port_conflict AXI_WRITE_OUTSTANDING_PORT_CONFLICT invalid \
    "Conflicting write_burst_outstanding values in interface bundle 'data'"
  run_alias_case write_outstanding_zero AXI_WRITE_OUTSTANDING_ZERO invalid \
    "num_write_outstanding must be an integer in \\[1,16\\]"
  run_alias_case write_outstanding_17 AXI_WRITE_OUTSTANDING_17 invalid \
    "num_write_outstanding must be an integer in \\[1,16\\]"
  run_alias_case write_outstanding_overflow AXI_WRITE_OUTSTANDING_OVERFLOW invalid \
    "num_write_outstanding must be an integer in \\[1,16\\]"
  run_alias_case write_outstanding_missing AXI_WRITE_OUTSTANDING_MISSING invalid \
    "num_write_outstanding must be a decimal integer in its supported range"
  run_alias_case write_outstanding_nondecmial AXI_WRITE_OUTSTANDING_NONDECIMAL invalid \
    "num_write_outstanding must be a decimal integer in its supported range"
}

run_float_copy_simulations() {
  if [[ ${BURST_WRITE_FLOAT_SIM_OUTSIDE_SANDBOX:-} != 1 ]]; then
    echo "float-sim requires BURST_WRITE_FLOAT_SIM_OUTSIDE_SANDBOX=1 and must run outside the sandbox" >&2
    return 2
  fi
  for tool in vsim vlib vmap vlog; do
    command -v "$tool" >/dev/null || { echo "ModelSim tool $tool is required in PATH" >&2; return 2; }
  done
  local fixture="$TESTS/write_compiler_cases/write_cases.cpp"
  local testbench="$TESTS/bambu_axi_burst_write_tb.cpp"
  local checker="$TESTS/check_write_compiler_artifacts.py"
  local simulator_invocation simulator_resolved settings
  simulator_invocation=$(command -v vsim)
  simulator_resolved=$(realpath "$simulator_invocation")
  settings="$PREFIX/settings.sh"
  local float_out="$OUT/float-copy"
  mkdir -p "$float_out"
  {
    printf 'bambu=%s\nsettings=%s\nplugin=%s\nsource=%s\ntestbench=%s\nchecker=%s\n' \
      "$BAMBU" "$settings" "$PLUGIN" "$fixture" "$testbench" "$checker"
    printf 'simulator=MODELSIM\nsimulator_invocation=%s\nsimulator_resolved=%s\n' \
      "$simulator_invocation" "$simulator_resolved"
    printf 'simulator_version='; "$simulator_invocation" -version 2>&1 | sed -n '1p' || true
    sha256sum "$BAMBU" "$settings" "$PLUGIN" "$fixture" "$testbench" "$checker" "$simulator_resolved"
  } >"$float_out/provenance.txt"
  for feature in 1 0; do
    local label=on dir status=0
    [[ $feature == 0 ]] && label=off
    dir="$float_out/feature-$label"
    mkdir -p "$dir/tmp"
    local -a args=("--top-fname=copy_float_payload" --compiler=I386_CLANG16 --generate-interface=INFER
      --device-name=xcu250,-2L,figd2104 --AXI-burst-type=INCREMENTAL --clock-period=3.33 --no-clean
      --simulate --simulator=MODELSIM "--generate-tb=$testbench"
      --tb-param-size=src:16384 --tb-param-size=dst:16384
      "--bambu-parameter=${BURST_PARAMETER}=$feature"
      --extra-cc-options=-DNDEBUG --extra-cc-options=-DBAMBU --extra-cc-options=-DBURST_TEST_FLOAT_COPY)
    {
      printf 'feature=%s\noutput=%s\nTMPDIR=%s\nargv=' "$feature" "$dir" "$dir/tmp"
      printf '%q ' "$BAMBU" "$fixture" "${args[@]}"; printf '\n'
      sha256sum "$BAMBU" "$fixture" "$testbench" "$checker" "$simulator_resolved"
    } >"$dir/provenance.txt"
    if (cd "$dir" && TMPDIR="$dir/tmp" TMP="$dir/tmp" TEMP="$dir/tmp" \
        "$BAMBU" "$fixture" "${args[@]}" >stdout.log 2>stderr.log); then
      status=0
    else
      status=$?
    fi
    if (( status != 0 )); then
      tail -n 80 "$dir/stderr.log" "$dir/stdout.log" >&2 || true
      echo "float copy feature-$label ModelSim run failed (rc=$status); see $dir" >&2
      return "$status"
    fi
    local tb_log="$dir/HLS_output/simulation/testbench.log"
    if [[ ! -s $tb_log ]] || ! grep -Fqx 'AXI_BURST_FLOAT_COPY_GOLDEN_PASS N=257' "$tb_log"; then
      echo "float copy feature-$label run omitted its exact golden marker; see $tb_log" >&2
      return 1
    fi
    if [[ $feature == 1 ]]; then
      python3 "$checker" accepted --output "$dir" --copy \
        --count-pattern '(%?[A-Za-z0-9_.]*n|region\.bound|copy_float_payload)' \
        --expected-read-burst 16 --expected-write-burst 16 \
        --expected-read-bundle input --expected-write-bundle output
    else
      python3 "$checker" fallback --output "$dir" --bundle output --direction writes \
        --reason 'explicit write-burst request ignored because m-axi-burst=0'
      python3 "$checker" fallback --output "$dir" --bundle input --direction reads \
        --reason 'explicit read-burst request ignored because m-axi-burst=0'
    fi
    printf 'AXI_BURST_FLOAT_COPY_MODELSIM_PASS FEATURE=%s\n' "$label"
  done
}

if [[ "$MODE" == write || "$MODE" == all ]]; then run_write_suite; fi
if [[ "$MODE" == read-compat || "$MODE" == all ]]; then
  # Compatibility-only invocation; never runs by default as part of `write`.
  "$TESTS/run_compiler_tests.sh" burst-profile "$BAMBU" "$OUT/read-compat"
fi
if [[ "$MODE" == float-sim ]]; then
  run_float_copy_simulations
  echo "Manual float copy ModelSim checks complete. Output: $OUT"
else
  echo "Manual compiler checks complete; no simulation was run. Output: $OUT"
fi
