#!/usr/bin/env bash
# Compile real C++ fixtures with Bambu and check its emitted artifacts.
# Usage: run_compiler_tests.sh {pragma|burst-profile|xml-profile|latency-metadata|ir|rtl|fallback|fallback-matrix|effect-fallback|root-reinvocation|burst-type|shared-resource|all} BAMBU OUTPUT_DIR
set -euo pipefail
TESTS=$(cd "$(dirname "$0")" && pwd)
ROOT=$(realpath "$TESTS/../../..")
if [[ ${1:-} == -h || ${1:-} == --help ]]; then
  sed -n '2,3p' "$0"; echo "Output must be a fresh directory."; exit 0
fi
MODE=${1:?mode required}; BAMBU=${2:?Bambu executable required}; OUT=${3:?fresh output directory required}
case "$MODE" in pragma|burst-profile|xml-profile|latency-metadata|ir|rtl|fallback|fallback-matrix|effect-fallback|root-reinvocation|burst-type|shared-resource|all) ;; *) echo "invalid mode: $MODE" >&2; exit 2;; esac
if [[ ${BURST_SIMULATE:-0} == 1 && "$MODE" != all && "$MODE" != root-reinvocation ]]; then
  echo "BURST_SIMULATE=1 is supported only with all or root-reinvocation mode" >&2; exit 2
fi
if [[ "$BAMBU" == */* ]]; then
  BAMBU=$(realpath "$BAMBU")
else
  BAMBU=$(command -v "$BAMBU" || true)
  [[ -n "$BAMBU" ]] && BAMBU=$(realpath "$BAMBU")
fi
if [[ -z "$BAMBU" || ! -x "$BAMBU" ]]; then
  echo "Bambu executable not found: ${2}" >&2; exit 2
fi
BAMBU_PREFIX=$(dirname "$(dirname "$BAMBU")")
BAMBU_SETTINGS="$BAMBU_PREFIX/settings.sh"
SETTINGS_USED=none
if [[ -f "$BAMBU_SETTINGS" ]]; then
  # Pin the plugin/toolchain search path to the selected installation prefix.
  # shellcheck disable=SC1090
  source "$BAMBU_SETTINGS"
  SETTINGS_USED=$(realpath "$BAMBU_SETTINGS")
  if [[ $(realpath -m "${BAMBU_HLS:-}") != $(realpath "$BAMBU_PREFIX") ]]; then
    echo "settings.sh selected BAMBU_HLS='${BAMBU_HLS:-<unset>}' instead of $BAMBU_PREFIX" >&2
    exit 2
  fi
fi
# settings.sh may itself alter these; re-pin all tool scratch paths afterward.
if [[ -e "$OUT" || -L "$OUT" ]]; then
  echo "output directory must not already exist: $OUT" >&2; exit 2
fi
OUT=$(realpath -m "$OUT")
mkdir -p "$OUT"
mkdir -p "$OUT/tmp"
TMPDIR="$OUT/tmp"; TMP=$TMPDIR; TEMP=$TMPDIR
export TMPDIR TMP TEMP
CHECKER="$TESTS/check_compiler_artifacts.py"
BURST_PARAMETER=${BURST_PARAMETER:-experimental-m-axi-burst}
mkdir -p "$OUT/compiler"
if [[ -z ${BAMBU_HLS:-} ]]; then
  echo "BAMBU_HLS is unset and no sibling settings.sh selected a prefix; refusing to guess the ASTAnalyzer plugin" >&2
  exit 2
fi
SELECTED_BAMBU_HLS=$(realpath "$BAMBU_HLS")
CLANG_PLUGIN="$SELECTED_BAMBU_HLS/lib/panda/clang-16/ASTAnalyzer.so"
OPT_CLANG_PLUGIN="$SELECTED_BAMBU_HLS/lib/panda/clang-16/opt_ASTAnalyzer.so"
if [[ ! -r "$CLANG_PLUGIN" ]]; then
  echo "selected ASTAnalyzer plugin is missing: $CLANG_PLUGIN; refusing compiler tests" >&2; exit 2
fi
CLANG16_COMMAND=$(command -v clang++-16 || command -v clang-16 || true)
if [[ -n "$CLANG16_COMMAND" ]]; then CLANG16_COMMAND=$(realpath "$CLANG16_COMMAND"); fi
if [[ "$SETTINGS_USED" == none ]]; then
  echo "NOTICE: no sibling settings.sh; preserving caller BAMBU_HLS and requiring its clang-16 ASTAnalyzer plugin." >&2
fi
{
  printf 'bambu_executable=%s\n' "$BAMBU"
  printf 'bambu_prefix=%s\n' "$BAMBU_PREFIX"
  printf 'settings_sh=%s\n' "$SETTINGS_USED"
  printf 'BAMBU_HLS=%s\n' "$SELECTED_BAMBU_HLS"
  printf 'BAMBU_HLS_BACKEND_PATH=%s\n' "${BAMBU_HLS_BACKEND_PATH:-<unset>}"
  printf 'clang_frontend=I386_CLANG16\n'
  printf 'clang16_command_on_PATH=%s\n' "${CLANG16_COMMAND:-<unresolved>}"
  printf 'clang_16_ast_analyzer=%s\n' "$CLANG_PLUGIN"
  printf 'clang_16_opt_ast_analyzer=%s\n' "$OPT_CLANG_PLUGIN"
  printf 'TMPDIR=%s\nTMP=%s\nTEMP=%s\n' "$TMPDIR" "$TMP" "$TEMP"
  printf 'PATH=%s\n' "$PATH"
  sha256sum "$BAMBU" "$CLANG_PLUGIN"
  if [[ -r "$OPT_CLANG_PLUGIN" ]]; then sha256sum "$OPT_CLANG_PLUGIN"; fi
  if [[ "$SETTINGS_USED" != none ]]; then sha256sum "$SETTINGS_USED"; fi
} >"$OUT/compiler/tool_provenance.txt"
"$BAMBU" --version >"$OUT/compiler/bambu_version.txt" 2>&1 || true
if ! "$BAMBU" --list-bambu-parameters >"$OUT/compiler/bambu_parameters.txt" 2>&1; then
  echo "could not query Bambu parameters; see $OUT/compiler/bambu_parameters.txt" >&2
  exit 2
fi
HAS_BURST_PARAMETER=0
awk -v name="$BURST_PARAMETER" '$1 == name { found=1 } END { exit !found }' \
    "$OUT/compiler/bambu_parameters.txt" && HAS_BURST_PARAMETER=1 || true
run_pragma_case() {
  local top=$1 tag=$2 source=$3 expect=$4
  source=$(realpath "$source")
  local case_dir="$OUT/compiler/pragma/$tag"
  local args=("--top-fname=$top" --compiler=I386_CLANG16 --generate-interface=INFER
              --AXI-burst-type=INCREMENTAL --no-clean --device-name=xcu250,-2L,figd2104
              --extra-cc-options=-DNDEBUG --extra-cc-options=-DBAMBU)
  if [[ $HAS_BURST_PARAMETER == 1 ]]; then args+=("--bambu-parameter=${BURST_PARAMETER}=0"); fi
  mkdir -p "$case_dir"
  printf '%q ' "$BAMBU" "$source" "${args[@]}" >"$case_dir/argv.txt"; printf '\n' >>"$case_dir/argv.txt"
  if (cd "$case_dir" && "$BAMBU" "$source" "${args[@]}" >stdout.log 2>stderr.log); then
    rc=0
  else
    rc=$?
  fi
  if [[ "$expect" == accept && $rc -ne 0 ]]; then
    echo "expected accepted pragma '$top', Bambu exited $rc; see $case_dir/stderr.log" >&2; return 1
  elif [[ "$expect" == reject && $rc -eq 0 ]]; then
    echo "expected rejected pragma '$top', but Bambu accepted it; see $case_dir" >&2; return 1
  elif [[ "$expect" == reject ]]; then
    local plugin_outputs=("$case_dir/stdout.log" "$case_dir/stderr.log")
    while IFS= read -r output; do plugin_outputs+=("$output"); done \
      < <(find "$case_dir/panda-temp" -maxdepth 1 -type f -name '__cc_output*' -print 2>/dev/null || true)
    if ! rg -qi 'Maximum AXI burst length|Conflicting maximum AXI burst length values' "${plugin_outputs[@]}"; then
      echo "'$top' exited $rc but has no ASTAnalyzer burst-pragma diagnostic in compiler output; see $case_dir" >&2
      return 1
    fi
  fi
  echo "PRAGMA $expect PASS: $top (opt-in disabled/default-off)"
}
run_pragma_suite() {
  if [[ $HAS_BURST_PARAMETER == 0 ]]; then
    echo "NOTICE: '$BURST_PARAMETER' is not registered; pragma parser is checked with the compiler's default-off behavior, not an explicit opt-in=0 setting." >&2
  fi
  run_pragma_case duplicate_attributes duplicate-off "$TESTS/compiler_cases/duplicate_attributes.cpp" accept
  for item in "invalid_zero invalid_zero.cpp" "invalid_negative invalid_negative.cpp" \
              "invalid_malformed invalid_malformed.cpp" \
              "invalid_conflict invalid_conflict.cpp" "invalid_non_axi invalid_non_axi.cpp"; do
    read -r top source <<<"$item"
    run_pragma_case "$top" "$top-off" "$TESTS/compiler_cases/$source" reject
  done
  if [[ $HAS_BURST_PARAMETER == 1 ]]; then
    local case_dir="$OUT/compiler/pragma/duplicate-on"
    local args=(--top-fname=duplicate_attributes --compiler=I386_CLANG16 --generate-interface=INFER
                --AXI-burst-type=INCREMENTAL --no-clean --device-name=xcu250,-2L,figd2104
                "--bambu-parameter=${BURST_PARAMETER}=1" --extra-cc-options=-DNDEBUG --extra-cc-options=-DBAMBU)
    mkdir -p "$case_dir"
    printf '%q ' "$BAMBU" "$TESTS/compiler_cases/duplicate_attributes.cpp" "${args[@]}" >"$case_dir/argv.txt"
    printf '\n' >>"$case_dir/argv.txt"
    (cd "$case_dir" && "$BAMBU" "$TESTS/compiler_cases/duplicate_attributes.cpp" "${args[@]}" >stdout.log 2>stderr.log)
    echo "PRAGMA accept PASS: duplicate attributes (opt-in enabled)"
  fi
}
if [[ "$MODE" == pragma || "$MODE" == all ]]; then run_pragma_suite; fi
if [[ "$MODE" == pragma ]]; then exit 0; fi
run_burst_profile_suite() {
  local source="$TESTS/compiler_cases/read_profile_valid.cpp"
  local case_dir="$OUT/compiler/burst-profile/positive"
  # Legacy tolerance path: num_read_outstanding/read_fifo_depth without
  # max_read_burst_length are accepted only when the burst feature is
  # explicitly off. With the default on, the same diagnostic is checked by
  # the no-burst-optin case below.
  local args=(--top-fname=read_profile_valid --compiler=I386_CLANG16 --generate-interface=INFER
              --device-name=xcu250,-2L,figd2104 --AXI-burst-type=INCREMENTAL --no-clean
              --extra-cc-options=-DNDEBUG --extra-cc-options=-DBAMBU
              "--bambu-parameter=${BURST_PARAMETER}=0")
  mkdir -p "$case_dir"
  (cd "$case_dir" && "$BAMBU" "$source" "${args[@]}" >stdout.log 2>stderr.log)
  local architecture
  architecture=$(find "$case_dir" -type f -name architecture.xml -print -quit)
  if [[ -z "$architecture" ]]; then
    echo "positive read-profile compilation emitted no architecture.xml; see $case_dir" >&2; return 1
  fi
  architecture=$(realpath "$architecture")
  python3 "$TESTS/check_read_profile_architecture.py" "$architecture" positive
  echo "BURST-PROFILE PASS: pragma -> canonical architecture.xml serialization"

  local no_b_dir="$OUT/compiler/burst-profile/no-burst-optin"
  mkdir -p "$no_b_dir"
  if (cd "$no_b_dir" && "$BAMBU" "$TESTS/compiler_cases/read_profile_without_burst.cpp" \
      --top-fname=read_profile_without_burst --compiler=I386_CLANG16 --generate-interface=INFER \
      --device-name=xcu250,-2L,figd2104 --AXI-burst-type=INCREMENTAL --no-clean \
      "--bambu-parameter=${BURST_PARAMETER}=1" --extra-cc-options=-DNDEBUG --extra-cc-options=-DBAMBU \
      >stdout.log 2>stderr.log); then
    echo "O/D profile without max_read_burst_length unexpectedly accepted with opt-in" >&2; return 1
  fi
  if ! rg -qi "AXI read profile attributes num_read_outstanding/read_fifo_depth.*require max_read_burst_length.*cannot be applied to the legacy scalar interface" \
      "$no_b_dir/stdout.log" "$no_b_dir/stderr.log"; then
    echo "O/D without B failed without the explicit unsupported-profile diagnostic; see $no_b_dir" >&2; return 1
  fi
  local legacy_dir="$OUT/compiler/burst-profile/no-burst-optout"
  mkdir -p "$legacy_dir"
  (cd "$legacy_dir" && "$BAMBU" "$TESTS/compiler_cases/read_profile_without_burst.cpp" \
      --top-fname=read_profile_without_burst --compiler=I386_CLANG16 --generate-interface=INFER \
      --device-name=xcu250,-2L,figd2104 --AXI-burst-type=INCREMENTAL --no-clean \
      "--bambu-parameter=${BURST_PARAMETER}=0" --extra-cc-options=-DNDEBUG --extra-cc-options=-DBAMBU \
      >stdout.log 2>stderr.log)
  python3 "$CHECKER" fallback --output "$legacy_dir"
  echo "BURST-PROFILE PASS: O/D without B errors with opt-in and remains legacy with opt-in disabled"

  local profile_case profile_top profile_o profile_d profile_dir
  for profile_case in 'o1_d256|1|256' 'o3_d32|3|32' 'o16_d4096|16|4096'; do
    IFS='|' read -r profile_top profile_o profile_d <<<"$profile_case"
    profile_dir="$OUT/compiler/burst-profile/$profile_top"
    mkdir -p "$profile_dir"
    (cd "$profile_dir" && "$BAMBU" "$TESTS/compiler_cases/read_profile_${profile_top}.cpp" \
        --top-fname="read_profile_${profile_top}" --compiler=I386_CLANG16 --generate-interface=INFER \
        --device-name=xcu250,-2L,figd2104 --AXI-burst-type=INCREMENTAL --no-clean \
        "--bambu-parameter=${BURST_PARAMETER}=1" --extra-cc-options=-DNDEBUG --extra-cc-options=-DBAMBU \
        >stdout.log 2>stderr.log)
    python3 "$CHECKER" rtl --output "$profile_dir" --require='MinimalAXI4MasterPipelined' \
      --configure-log="$profile_dir/stderr.log" --configured-contract --expected-burst-max=16 \
      --expected-outstanding="$profile_o" --expected-fifo-depth="$profile_d"
    echo "BURST-PROFILE PASS: generated RTL carries O=$profile_o/D=$profile_d"
  done

  # The three simple out-of-range values O=17, D=3 and D=8192 (and B=257
  # in pragma mode) are covered by the Mantis list. Keep this matrix focused
  # on malformed, overflow, duplicate/conflict, and applicability behavior.
  local cases=(
    "read_profile_invalid_outstanding_zero.cpp|num_read_outstanding must be an integer in \\[1,16\\]"
    "read_profile_invalid_depth_zero.cpp|read_fifo_depth must be a power-of-two integer in \\[1,4096\\] beats"
    "read_profile_invalid_negative.cpp|num_read_outstanding must be a decimal integer in its supported range"
    "read_profile_invalid_malformed.cpp|read_fifo_depth must be a decimal integer in its supported range"
    "read_profile_invalid_huge.cpp|num_read_outstanding must be an integer in \\[1,16\\]"
    "read_profile_invalid_non_axi.cpp|AXI read profile attributes.*only valid for mode=m_axi"
    "read_profile_invalid_conflict.cpp|Conflicting num_read_outstanding values in interface bundle 'data'"
    "read_profile_invalid_duplicate_outstanding_conflict.cpp|Conflicting num_read_outstanding values in interface bundle 'data'"
    "read_profile_invalid_duplicate_depth_conflict.cpp|Conflicting read_fifo_depth values in interface bundle 'data'"
    "read_profile_invalid_duplicate_outstanding_range.cpp|num_read_outstanding must be an integer in \\[1,16\\]"
    "read_profile_invalid_duplicate_depth_range.cpp|read_fifo_depth must be a power-of-two integer in \\[1,4096\\] beats"
    "read_profile_invalid_duplicate_outstanding_missing.cpp|num_read_outstanding must be a decimal integer in its supported range"
    "read_profile_invalid_duplicate_outstanding_comma.cpp|num_read_outstanding must be a decimal integer in its supported range"
    "read_profile_invalid_duplicate_depth_missing.cpp|read_fifo_depth must be a decimal integer in its supported range"
    "read_profile_invalid_duplicate_depth_comma.cpp|read_fifo_depth must be a decimal integer in its supported range"
  )
  local item filename diagnostic top invalid_dir
  for item in "${cases[@]}"; do
    IFS='|' read -r filename diagnostic <<<"$item"
    top=${filename%.cpp}
    invalid_dir="$OUT/compiler/burst-profile/$top"
    mkdir -p "$invalid_dir"
    if (cd "$invalid_dir" && "$BAMBU" "$TESTS/compiler_cases/$filename" \
        --top-fname="$top" --compiler=I386_CLANG16 --generate-interface=INFER \
        --device-name=xcu250,-2L,figd2104 --AXI-burst-type=INCREMENTAL --no-clean \
        --extra-cc-options=-DNDEBUG --extra-cc-options=-DBAMBU >stdout.log 2>stderr.log); then
      echo "expected read-profile fixture '$top' to fail; see $invalid_dir" >&2; return 1
    fi
    if ! rg -q "$diagnostic" "$invalid_dir/stdout.log" "$invalid_dir/stderr.log" \
        "$invalid_dir"/panda-temp/__cc_output* 2>/dev/null; then
      echo "read-profile fixture '$top' failed without expected diagnostic '$diagnostic'; see $invalid_dir" >&2; return 1
    fi
    echo "BURST-PROFILE reject PASS: $top"
  done
}
if [[ "$MODE" == burst-profile ]]; then
  run_burst_profile_suite
  exit 0
fi
if [[ $HAS_BURST_PARAMETER == 0 ]]; then
  echo "Bambu does not register '$BURST_PARAMETER'; use a compiler build with burst opt-in support" >&2
  exit 2
fi
BURST_VALUE=1
[[ "$MODE" == fallback ]] && BURST_VALUE=0
run_compile() {
  local top=$1 tag=$2 source=${3:-${BURST_CASE_SOURCE:-$TESTS/compiler_cases/recognized_loops.cpp}}
  source=$(realpath "$source")
  local case_dir="$OUT/compiler/$tag"
  local args=("--top-fname=$top" --compiler=I386_CLANG16 --generate-interface=INFER
              --device-name=xcu250,-2L,figd2104 --AXI-burst-type=INCREMENTAL --no-clean
              "--bambu-parameter=${BURST_PARAMETER}=${BURST_VALUE}"
              --extra-cc-options=-DNDEBUG --extra-cc-options=-DBAMBU)
  if [[ "$top" == root_reinvocation && ${BURST_SIMULATE:-0} == 1 ]]; then
    args+=(--simulate --simulator=MODELSIM "--generate-tb=$TESTS/compiler_cases/tb_root_reinvocation.cpp")
  fi
  mkdir -p "$case_dir"
  printf '%q ' "$BAMBU" "$source" "${args[@]}" >"$case_dir/argv.txt"
  printf '\n' >>"$case_dir/argv.txt"
  (cd "$case_dir" && "$BAMBU" "$source" "${args[@]}" >stdout.log 2>stderr.log)
}
if [[ "$MODE" == latency-metadata ]]; then
  python3 "$TESTS/run_latency_metadata_tests.py" "$BAMBU" "$OUT/compiler/latency-metadata" \
    --burst-parameter "$BURST_PARAMETER"
  exit 0
fi
if [[ "$MODE" == xml-profile ]]; then
  # Generate both raw-IR seeds through the normal frontend path. The XML
  # mutation tests below deliberately consume these .bambuir files, not the
  # compiler's debug-only .raw dumps.
  run_compile renamed_sum xml-profile-seed "$TESTS/compiler_cases/recognized_loops.cpp"
  run_compile stride_two xml-profile-stride-seed "$TESTS/compiler_cases/unsupported_stride.cpp"
  find_artifact() {
    local root=$1 pattern=$2 found
    found=$(find "$root" -type f -name "$pattern" -print -quit)
    if [[ -z "$found" ]]; then
      echo "missing $pattern below $root" >&2; return 1
    fi
    realpath "$found"
  }
  seed_ir=$(find_artifact "$OUT/compiler/xml-profile-seed" '*.bambuir')
  seed_xml=$(find_artifact "$OUT/compiler/xml-profile-seed" architecture.xml)
  stride_ir=$(find_artifact "$OUT/compiler/xml-profile-stride-seed" '*.bambuir')
  stride_xml=$(find_artifact "$OUT/compiler/xml-profile-stride-seed" architecture.xml)
  mkdir -p "$OUT/compiler/xml-profile"
  python3 "$TESTS/run_xml_profile_tests.py" "$BAMBU" "$OUT/compiler/xml-profile" \
    "$seed_ir" "$seed_xml" "$stride_ir" "$stride_xml" \
    --burst-parameter "$BURST_PARAMETER"
  exit 0
fi
assert_shared_root_order() {
  local order=$1 expected_first=$2 expected_second=$3 log=$4
  local analyzed=()
  mapfile -t analyzed < <(sed -n 's/^[[:space:]]*Analyzing function \([^[:space:]]*\).*$/\1/p' "$log")
  if [[ ${#analyzed[@]} -lt 2 || ${analyzed[0]} != "$expected_first" || ${analyzed[1]} != "$expected_second" ]]; then
    printf 'shared-resource fixture did not exercise %s order: expected %s then %s, observed:' \
      "$order" "$expected_first" "$expected_second" >&2
    printf ' %s' "${analyzed[@]}" >&2
    printf '\nSee %s\n' "$log" >&2
    return 1
  fi
}
run_shared_resource_case() {
  local order=$1 tops=$2 define=$3 expected_first=$4 expected_second=$5
  local case_dir="$OUT/compiler/shared-resource/$order"
  local args=("--top-fname=$tops" --compiler=I386_CLANG16 --generate-interface=INFER
              --device-name=xcu250,-2L,figd2104 --AXI-burst-type=INCREMENTAL --no-clean
              "--bambu-parameter=${BURST_PARAMETER}=1"
              --extra-cc-options=-DNDEBUG --extra-cc-options=-DBAMBU)
  [[ -z "$define" ]] || args+=("--extra-cc-options=-D$define")
  mkdir -p "$case_dir"
  printf '%q ' "$BAMBU" "$TESTS/compiler_cases/mixed_root_shared_bundle.cpp" "${args[@]}" \
    >"$case_dir/argv.txt"
  printf '\n' >>"$case_dir/argv.txt"
  if (cd "$case_dir" && "$BAMBU" "$TESTS/compiler_cases/mixed_root_shared_bundle.cpp" \
      "${args[@]}" >stdout.log 2>stderr.log); then
    echo "expected mixed shared-bundle roots '$tops' to fail; see $case_dir" >&2
    return 1
  fi
  assert_shared_root_order "$order" "$expected_first" "$expected_second" "$case_dir/stderr.log"
  if ! rg -qi "Incompatible m_axi resource ABI for bundle 'data': configured burst-read and legacy scalar read roots cannot share this resource.*Use distinct bundle names" \
      "$case_dir/stdout.log" "$case_dir/stderr.log"; then
    echo "mixed shared-bundle roots '$tops' failed without the expected ABI diagnostic; see $case_dir" >&2
    return 1
  fi
  if find "$case_dir" -type f \( -name 'panda_libtech.v' -o -name 'kernel.v' \) -print -quit | rg -q .; then
    echo "mixed shared-bundle roots '$tops' emitted resource RTL despite the ABI conflict; see $case_dir" >&2
    return 1
  fi
  echo "SHARED-RESOURCE PASS: $order ($tops rejected before resource RTL generation)"
}
run_shared_profile_case() {
  local order=$1 tops=$2 define=$3 expected_first=$4 expected_second=$5
  local case_dir="$OUT/compiler/shared-profile/$order"
  local args=("--top-fname=$tops" --compiler=I386_CLANG16 --generate-interface=INFER
              --device-name=xcu250,-2L,figd2104 --AXI-burst-type=INCREMENTAL --no-clean
              "--bambu-parameter=${BURST_PARAMETER}=1"
              --extra-cc-options=-DNDEBUG --extra-cc-options=-DBAMBU)
  [[ -z "$define" ]] || args+=("--extra-cc-options=-D$define")
  mkdir -p "$case_dir"
  printf '%q ' "$BAMBU" "$TESTS/compiler_cases/mixed_root_shared_bundle.cpp" "${args[@]}" \
    >"$case_dir/argv.txt"
  printf '\n' >>"$case_dir/argv.txt"
  if (cd "$case_dir" && "$BAMBU" "$TESTS/compiler_cases/mixed_root_shared_bundle.cpp" \
      "${args[@]}" >stdout.log 2>stderr.log); then
    echo "expected configured roots with different B values '$tops' to fail; see $case_dir" >&2
    return 1
  fi
  assert_shared_root_order "$order" "$expected_first" "$expected_second" "$case_dir/stderr.log"
  if ! rg -qi "Incompatible m_axi burst profile for bundle 'data': existing roots use max_read_burst_length=(16|32) while this root requires max_read_burst_length=(16|32).*Use distinct bundle names" \
      "$case_dir/stdout.log" "$case_dir/stderr.log"; then
    echo "configured roots '$tops' failed without the expected burst-profile diagnostic; see $case_dir" >&2
    return 1
  fi
  echo "SHARED-PROFILE PASS: $order ($tops rejected for conflicting B values)"
}
run_shared_od_conflict_case() {
  local tag=$1 tops=$2 define=$3 first=$4 second=$5 field=$6 existing=$7 requested=$8
  local case_dir="$OUT/compiler/shared-profile/$tag"
  local args=("--top-fname=$tops" --compiler=I386_CLANG16 --generate-interface=INFER
              --device-name=xcu250,-2L,figd2104 --AXI-burst-type=INCREMENTAL --no-clean
              "--bambu-parameter=${BURST_PARAMETER}=1"
              --extra-cc-options=-DNDEBUG --extra-cc-options=-DBAMBU "--extra-cc-options=-D$define")
  mkdir -p "$case_dir"
  if (cd "$case_dir" && "$BAMBU" "$TESTS/compiler_cases/mixed_root_shared_bundle.cpp" \
      "${args[@]}" >stdout.log 2>stderr.log); then
    echo "expected shared roots '$tops' to reject conflicting $field; see $case_dir" >&2
    return 1
  fi
  assert_shared_root_order "$tag" "$first" "$second" "$case_dir/stderr.log"
  if ! rg -qi "Incompatible m_axi burst profile for bundle 'data': existing roots use .*${field}=${existing}.*while this root requires .*${field}=${requested}.*Use distinct bundle names" \
      "$case_dir/stdout.log" "$case_dir/stderr.log"; then
    echo "shared roots '$tops' failed without expected $field conflict; see $case_dir" >&2
    return 1
  fi
  echo "SHARED-PROFILE PASS: $tag ($tops rejected for conflicting $field)"
}
run_shared_default_match_case() {
  local tag=$1 tops=$2 define=$3 first=$4 second=$5
  local case_dir="$OUT/compiler/shared-profile/$tag"
  local args=("--top-fname=$tops" --compiler=I386_CLANG16 --generate-interface=INFER
              --device-name=xcu250,-2L,figd2104 --AXI-burst-type=INCREMENTAL --no-clean
              "--bambu-parameter=${BURST_PARAMETER}=1"
              --extra-cc-options=-DNDEBUG --extra-cc-options=-DBAMBU "--extra-cc-options=-D$define")
  mkdir -p "$case_dir"
  if (cd "$case_dir" && "$BAMBU" "$TESTS/compiler_cases/mixed_root_shared_bundle.cpp" \
      "${args[@]}" >stdout.log 2>stderr.log); then
    echo "expected the selected multi-root backend guard after profile compatibility; see $case_dir" >&2
    return 1
  fi
  assert_shared_root_order "$tag" "$first" "$second" "$case_dir/stderr.log"
  if ! rg -q "Expected single top function name" "$case_dir/stdout.log" "$case_dir/stderr.log" || \
     rg -q "Incompatible m_axi burst profile" "$case_dir/stdout.log" "$case_dir/stderr.log"; then
    echo "omitted and explicit O1/D32 roots did not pass profile compatibility; see $case_dir" >&2
    return 1
  fi
  # The omitted depth derives to next_pow2(max(2,1)*16) = 32, so the explicit
  # side has to name 32 for the two roots to be compatible.
  echo "SHARED-PROFILE PASS: $tag omitted defaults equal explicit O1/D32 (reached backend multi-top guard)"
}
run_burst_type_case() {
  local tag=$1 expected=$2 burst_type=$3 burst_value=$4 device_file=${5:-}
  local case_dir="$OUT/compiler/burst-type/$tag"
  local args=(--top-fname=renamed_sum --compiler=I386_CLANG16 --generate-interface=INFER
              --no-clean --device-name=xcu250,-2L,figd2104 "--AXI-burst-type=$burst_type"
              "--bambu-parameter=${BURST_PARAMETER}=$burst_value"
              --extra-cc-options=-DNDEBUG --extra-cc-options=-DBAMBU)
  mkdir -p "$case_dir"
  if [[ "$device_file" == device-fixed ]]; then
    local xilinx_device="$ROOT/etc/libtech/xilinx/xcu250-2Lfigd2104.xml"
    sed 's/<axi_burst_type value="1"\/>/<axi_burst_type value="0"\/>/' "$xilinx_device" \
      >"$case_dir/fixed_device.xml"
    if ! rg -q '<axi_burst_type value="0"/>' "$case_dir/fixed_device.xml"; then
      echo "could not create FIXED device override from $xilinx_device" >&2; return 2
    fi
    args+=(-b "$case_dir/fixed_device.xml")
  elif [[ -n "$device_file" ]]; then
    args+=(-b "$device_file")
  fi
  printf '%q ' "$BAMBU" "$TESTS/compiler_cases/recognized_loops.cpp" "${args[@]}" >"$case_dir/argv.txt"
  printf '\n' >>"$case_dir/argv.txt"
  if (cd "$case_dir" && "$BAMBU" "$TESTS/compiler_cases/recognized_loops.cpp" "${args[@]}" >stdout.log 2>stderr.log); then
    rc=0
  else
    rc=$?
  fi
  if [[ "$expected" == accept && $rc -ne 0 ]]; then
    echo "expected accepted AXI burst case '$tag'; see $case_dir/stderr.log" >&2; return 1
  elif [[ "$expected" == reject && $rc -eq 0 ]]; then
    echo "expected FIXED burst case '$tag' to fail; see $case_dir" >&2; return 1
  elif [[ "$expected" == reject ]] && ! rg -qi 'Configured read bursts.*require AXI INCREMENTAL.*effective AXI burst type is FIXED' \
      "$case_dir/stdout.log" "$case_dir/stderr.log"; then
    echo "FIXED burst case '$tag' failed without the expected diagnostic; see $case_dir" >&2; return 1
  fi
  echo "BURST-TYPE $expected PASS: $tag"
}
if [[ "$MODE" == burst-type ]]; then
  run_burst_type_case incr-accepted accept INCREMENTAL 1
  run_burst_type_case fixed-cli-device-incr-accepted accept FIXED 1
  run_burst_type_case fixed-opt-out-legacy accept FIXED 0
  run_burst_type_case fixed-cli-device-fixed-rejected reject FIXED 1 device-fixed
  run_burst_type_case fixed-device-override-rejected reject INCREMENTAL 1 device-fixed
  exit 0
fi
if [[ "$MODE" == shared-resource ]]; then
  run_shared_resource_case configured-first renamed_sum,conditional_load '' renamed_sum conditional_load
  run_shared_resource_case legacy-first conditional_load,renamed_sum SHARED_RESOURCE_LEGACY_FIRST conditional_load renamed_sum
  run_shared_profile_case B16-first renamed_sum,configured32 '' renamed_sum configured32
  run_shared_profile_case B32-first configured32,renamed_sum SHARED_PROFILE_B32_FIRST configured32 renamed_sum
  run_shared_od_conflict_case O3-first renamed_sum,configured32 SHARED_PROFILE_O_CONFLICT_FIRST configured32 renamed_sum num_read_outstanding 3 1
  run_shared_od_conflict_case O3-second renamed_sum,configured32 SHARED_PROFILE_O_CONFLICT renamed_sum configured32 num_read_outstanding 1 3
  # The root that omits read_fifo_depth gets the derived default: with B=16 and
  # one outstanding burst that is next_pow2(max(2,1)*16) = 32, not 256.
  run_shared_od_conflict_case D8-first renamed_sum,configured32 SHARED_PROFILE_D_CONFLICT_FIRST configured32 renamed_sum read_fifo_depth 8 32
  run_shared_od_conflict_case D8-second renamed_sum,configured32 SHARED_PROFILE_D_CONFLICT renamed_sum configured32 read_fifo_depth 32 8
  run_shared_default_match_case defaults-first renamed_sum,configured32 SHARED_PROFILE_DEFAULT_MATCH_FIRST configured32 renamed_sum
  run_shared_default_match_case defaults-second renamed_sum,configured32 SHARED_PROFILE_DEFAULT_MATCH renamed_sum configured32
  exit 0
fi
if [[ "$MODE" == root-reinvocation ]]; then
  run_compile root_reinvocation root_reinvocation "$TESTS/compiler_cases/root_reinvocation.cpp"
  python3 "$CHECKER" rtl --output "$OUT/compiler/root_reinvocation" \
    --require='MinimalAXI4MasterPipelined' --configure-log="$OUT/compiler/root_reinvocation/stderr.log" \
    --configured-contract --expected-burst-max=16
  if [[ ${BURST_SIMULATE:-0} == 1 ]]; then
    echo "Root re-invocation simulation ran through Bambu/ModelSim."
  fi
  exit 0
fi
if [[ "$MODE" == fallback-matrix ]]; then
  BURST_VALUE=0
  run_compile renamed_sum legacy-default
  python3 "$CHECKER" fallback --output "$OUT/compiler/legacy-default"
  BURST_VALUE=1
  run_compile two_regions legacy-two-regions "$TESTS/compiler_cases/recognized_loops.cpp"
  python3 "$CHECKER" fallback --output "$OUT/compiler/legacy-two-regions" \
    --fallback-log="$OUT/compiler/legacy-two-regions/stderr.log"
  run_compile stride_two legacy-stride "$TESTS/compiler_cases/unsupported_stride.cpp"
  python3 "$CHECKER" fallback --output "$OUT/compiler/legacy-stride"
  run_compile cached_region legacy-cache "$TESTS/compiler_cases/unsupported_cache.cpp"
  python3 "$CHECKER" fallback --output "$OUT/compiler/legacy-cache"
  for top in conditional_load early_exit; do
    run_compile "$top" "unsupported-$top" "$TESTS/compiler_cases/unsupported_loops.cpp"
    python3 "$CHECKER" fallback --output "$OUT/compiler/unsupported-$top"
  done
  for top in store_before_src_first store_before_dst_first volatile_access_before_region \
             store_after_region store_in_intermediate_loop; do
    run_compile "$top" "effect-$top" "$TESTS/compiler_cases/effect_fallback.cpp"
    python3 "$CHECKER" fallback --output "$OUT/compiler/effect-$top" \
      --fallback-log="$OUT/compiler/effect-$top/stderr.log"
  done
  exit 0
fi
if [[ -n ${BURST_CASE_SOURCE:-} ]]; then
  TOP=${BURST_TOP:-renamed_sum}
  run_compile "$TOP" "$TOP"
  case "$MODE" in
    fallback) python3 "$CHECKER" fallback --output "$OUT/compiler/$TOP" ;;
    effect-fallback) python3 "$CHECKER" fallback --output "$OUT/compiler/$TOP" \
                       --fallback-log="$OUT/compiler/$TOP/stderr.log" ;;
    ir) python3 "$CHECKER" rtl --output "$OUT/compiler/$TOP" --require='MinimalAXI4MasterPipelined' \
           --configure-log="$OUT/compiler/$TOP/stderr.log" --configured-contract --expected-burst-max=16 ;;
    rtl) python3 "$CHECKER" rtl --output "$OUT/compiler/$TOP" --require='MinimalAXI4MasterPipelined' \
           --configure-log="$OUT/compiler/$TOP/stderr.log" --configured-contract --expected-burst-max=16 ;;
    all) python3 "$CHECKER" rtl --output "$OUT/compiler/$TOP" --require='MinimalAXI4MasterPipelined' \
           --configure-log="$OUT/compiler/$TOP/stderr.log" --configured-contract --expected-burst-max=16 ;;
  esac
else
  case "$MODE" in
    fallback)
      BURST_VALUE=0; run_compile renamed_sum fallback
      python3 "$CHECKER" fallback --output "$OUT/compiler/fallback" ;;
    effect-fallback)
      TOP=${BURST_TOP:-volatile_access_before_region}
      SOURCE=${BURST_CASE_SOURCE:-$TESTS/compiler_cases/effect_fallback.cpp}
      run_compile "$TOP" "effect-$TOP" "$SOURCE"
      python3 "$CHECKER" fallback --output "$OUT/compiler/effect-$TOP" \
        --fallback-log="$OUT/compiler/effect-$TOP/stderr.log" ;;
    ir)
      run_compile renamed_sum ir
      python3 "$CHECKER" rtl --output "$OUT/compiler/ir" --require='MinimalAXI4MasterPipelined' \
        --configure-log="$OUT/compiler/ir/stderr.log" --configured-contract --expected-burst-max=16 ;;
    rtl)
      run_compile renamed_sum rtl
      python3 "$CHECKER" rtl --output "$OUT/compiler/rtl" --require='MinimalAXI4MasterPipelined' \
        --configure-log="$OUT/compiler/rtl/stderr.log" --configured-contract --expected-burst-max=16 ;;
    all)
      for top in renamed_sum zero_count one_count nonmultiple_count; do
        run_compile "$top" "$top"
        python3 "$CHECKER" rtl --output "$OUT/compiler/$top" --require='MinimalAXI4MasterPipelined' \
          --configure-log="$OUT/compiler/$top/stderr.log" --configured-contract --expected-burst-max=16
      done
      run_compile root_reinvocation root_reinvocation "$TESTS/compiler_cases/root_reinvocation.cpp"
      python3 "$CHECKER" rtl --output "$OUT/compiler/root_reinvocation" --require='MinimalAXI4MasterPipelined' \
        --configure-log="$OUT/compiler/root_reinvocation/stderr.log" --configured-contract --expected-burst-max=16
      run_compile two_regions legacy-two-regions "$TESTS/compiler_cases/recognized_loops.cpp"
      python3 "$CHECKER" fallback --output "$OUT/compiler/legacy-two-regions" \
        --fallback-log="$OUT/compiler/legacy-two-regions/stderr.log"
      run_compile stride_two legacy-stride "$TESTS/compiler_cases/unsupported_stride.cpp"
      python3 "$CHECKER" fallback --output "$OUT/compiler/legacy-stride"
      run_compile cached_region legacy-cache "$TESTS/compiler_cases/unsupported_cache.cpp"
      python3 "$CHECKER" fallback --output "$OUT/compiler/legacy-cache"
      # Opt-in is on for these shapes: they must remain on the legacy path
      # rather than instantiate the configured burst engine.
      for top in conditional_load early_exit; do
        run_compile "$top" "unsupported-$top" "$TESTS/compiler_cases/unsupported_loops.cpp"
        python3 "$CHECKER" fallback --output "$OUT/compiler/unsupported-$top"
      done
      for top in store_before_src_first store_before_dst_first volatile_access_before_region \
                 store_after_region store_in_intermediate_loop; do
        run_compile "$top" "effect-$top" "$TESTS/compiler_cases/effect_fallback.cpp"
        python3 "$CHECKER" fallback --output "$OUT/compiler/effect-$top" \
          --fallback-log="$OUT/compiler/effect-$top/stderr.log"
      done
      if [[ ${BURST_SIMULATE:-0} == 1 ]]; then
        echo "Root re-invocation testbench simulation requested; ModelSim will run through Bambu."
      fi ;;
  esac
fi
if [[ ${BURST_SIMULATE:-0} != 1 ]]; then
  echo "Compiler artifact checks completed; no simulation was run. Set BURST_SIMULATE=1 in all mode for the two-call root-reinvocation ModelSim simulation (run outside the sandbox)."
else
  echo "Bambu-generated ModelSim run requested for the root-reinvocation testbench; this invocation must run outside the sandbox."
fi
