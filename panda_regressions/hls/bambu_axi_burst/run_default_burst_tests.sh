#!/usr/bin/env bash
# Compiler-only qualification of default-on AXI burst profiles.
# Usage: run_default_burst_tests.sh BAMBU OUTPUT_DIR
set -euo pipefail

TESTS=$(cd "$(dirname "$0")" && pwd)
ROOT=$(realpath "$TESTS/../../..")
BAMBU_ARG=${1:?Bambu executable required}
OUT_ARG=${2:?fresh output directory required}
CHECKER="$TESTS/check_default_burst_artifacts.py"
SOURCE="$TESTS/compiler_cases/axi_default_profile_cases.cpp"
BURST_PARAMETER=${BURST_PARAMETER:-m-axi-burst}

if [[ "$BAMBU_ARG" == */* ]]; then BAMBU=$(realpath "$BAMBU_ARG"); else
  BAMBU=$(command -v "$BAMBU_ARG" || true)
  [[ -n "$BAMBU" ]] && BAMBU=$(realpath "$BAMBU")
fi
if [[ -z ${BAMBU:-} || ! -x "$BAMBU" ]]; then
  echo "Bambu executable not found: $BAMBU_ARG" >&2; exit 2
fi
if [[ -e "$OUT_ARG" || -L "$OUT_ARG" ]]; then
  echo "output directory must not already exist: $OUT_ARG" >&2; exit 2
fi
OUT=$(realpath -m "$OUT_ARG")
mkdir -p "$OUT/tmp"
export TMPDIR="$OUT/tmp" TMP="$OUT/tmp" TEMP="$OUT/tmp"

PREFIX=$(dirname "$(dirname "$BAMBU")")
if [[ -f "$PREFIX/settings.sh" ]]; then
  # shellcheck disable=SC1090
  source "$PREFIX/settings.sh"
  if [[ $(realpath -m "${BAMBU_HLS:-}") != $(realpath "$PREFIX") ]]; then
    echo "settings.sh selected BAMBU_HLS='${BAMBU_HLS:-<unset>}' instead of $PREFIX" >&2
    exit 2
  fi
fi
if [[ -z ${BAMBU_HLS:-} ]]; then
  echo "BAMBU_HLS is unset; refusing to guess ASTAnalyzer plugin" >&2; exit 2
fi
PLUGIN="$(realpath "$BAMBU_HLS")/lib/panda/clang-16/ASTAnalyzer.so"
[[ -r "$PLUGIN" ]] || { echo "selected ASTAnalyzer plugin is missing: $PLUGIN" >&2; exit 2; }
DEVICE="$ROOT/etc/libtech/xilinx/xcu250-2Lfigd2104.xml"
FIXED_DEVICE="$OUT/xcu250-fixed.xml"
sed 's/<axi_burst_type value="1"\/>/<axi_burst_type value="0"\/>/' "$DEVICE" >"$FIXED_DEVICE"
grep -Fq '<axi_burst_type value="0"/>' "$FIXED_DEVICE" || {
  echo "could not create FIXED device override from $DEVICE" >&2; exit 2;
}

run_case() {
  local tag=$1 top=$2 macro=$3 expected=$4 bundle=${5:-data} reason=${6:-}
  local read_burst=${PROFILE_READ_BURST:-16} read_outstanding=${PROFILE_READ_OUTSTANDING:-16}
  local read_fifo=${PROFILE_READ_FIFO:-256} write_burst=${PROFILE_WRITE_BURST:-16}
  local write_outstanding=${PROFILE_WRITE_OUTSTANDING:-16} write_fifo=${PROFILE_WRITE_FIFO:-32}
  local read_burst_origin=${PROFILE_READ_BURST_ORIGIN:-default}
  local write_burst_origin=${PROFILE_WRITE_BURST_ORIGIN:-default}
  if (( $# >= 6 )); then shift 6; else shift 5; fi
  local case_dir="$OUT/$tag" source_file=${CASE_SOURCE_OVERRIDE:-$SOURCE}
  local args=("--top-fname=$top" --compiler=I386_CLANG16 --generate-interface=INFER
    --device-name=xcu250,-2L,figd2104 --no-clean
    --extra-cc-options=-DNDEBUG --extra-cc-options=-DBAMBU)
  [[ -z $macro ]] || args+=("--extra-cc-options=-D$macro")
  args+=("$@")
  if [[ "$expected" == reject ]]; then
    python3 "$TESTS/compiler_case_manifest.py" execute --output "$case_dir" --bambu "$BAMBU" \
      --expected-failure "$reason" -- "$source_file" "${args[@]}"
    local diagnostic_files=("$case_dir/stdout.log" "$case_dir/stderr.log")
    while IFS= read -r output; do diagnostic_files+=("$output"); done \
      < <(find "$case_dir/panda-temp" -maxdepth 1 -type f -name '__cc_output*' -print 2>/dev/null || true)
    grep -Fq "$reason" \
      "${diagnostic_files[@]}" || {
      echo "$tag failed without the expected parameter validation diagnostic; see $case_dir" >&2; return 1;
    }
    echo "DEFAULT BURST INVALID-VALUE PASS: $tag"
    return
  fi
  python3 "$TESTS/compiler_case_manifest.py" execute --output "$case_dir" --bambu "$BAMBU" \
    -- "$source_file" "${args[@]}"
  if [[ "$expected" == default-read || "$expected" == default-write || "$expected" == default-duplex ||
        "$expected" == override-read || "$expected" == override-write ]]; then
    local kind
    case "$expected" in
      default-read|override-read) kind=read ;;
      default-write|override-write) kind=write ;;
      default-duplex) kind=duplex ;;
    esac
    python3 "$CHECKER" "$kind" "$case_dir" --bundle "$bundle" \
      --read-burst "$read_burst" --read-outstanding "$read_outstanding" --read-fifo "$read_fifo" \
      --write-burst "$write_burst" --write-outstanding "$write_outstanding" --write-fifo "$write_fifo" \
      --read-burst-origin "$read_burst_origin" --write-burst-origin "$write_burst_origin"
  elif [[ "$expected" == fixed ]]; then
    python3 "$CHECKER" fixed "$case_dir" --bundle "$bundle"
  elif [[ "$expected" == optout ]]; then
    python3 "$CHECKER" optout "$case_dir" --bundle "$bundle"
  elif [[ "$expected" == rtl-fallback ]]; then
    python3 "$CHECKER" rtl-fallback "$case_dir" --bundle "$bundle"
  elif [[ "$expected" == no-access ]]; then
    python3 "$CHECKER" no-access "$case_dir" --bundle "$bundle"
  elif [[ "$expected" == fallback ]]; then
    python3 "$CHECKER" fallback "$case_dir" --bundle "$bundle" --reason "$reason"
  else
    echo "invalid expectation $expected" >&2; return 2
  fi
}

run_case_with_source() {
  local source_file=$1
  shift
  [[ -s $source_file ]] || { echo "required source missing: $source_file" >&2; return 2; }
  CASE_SOURCE_OVERRIDE=$source_file run_case "$@"
}

run_noop_pair() {
  local tag=$1 top=$2 macro=$3 bundle=$4 on_case=$5 source_file=$6 reason=$7 off_kind=$8
  shift 8
  local -a extra_args=("$@")
  local on_dir
  run_case_with_source "$source_file" "$tag-off-a" "$top" "$macro" "$off_kind" "$bundle" "" \
    "${extra_args[@]}" --bambu-parameter=m-axi-burst=0
  run_case_with_source "$source_file" "$tag-off-b" "$top" "$macro" "$off_kind" "$bundle" "" \
    "${extra_args[@]}" --bambu-parameter=m-axi-burst=0
  if [[ $on_case == - ]]; then
    on_dir="$tag-on"
    run_case_with_source "$source_file" "$on_dir" "$top" "$macro" rtl-fallback "$bundle" "" \
      "${extra_args[@]}"
  else
    on_dir=$on_case
  fi
  python3 "$CHECKER" rtl-equal "$OUT/$tag-off-a" --compare "$OUT/$tag-off-b"
  python3 "$CHECKER" rtl-equal "$OUT/$tag-off-a" --compare "$OUT/$on_dir"
  printf 'DEFAULT BURST NO-OP PASS: %s reason=%s\n' "$tag" "$reason"
  printf 'case=%s reason=%s off_runs=%s,%s on_run=%s\n' \
    "$tag" "$reason" "$tag-off-a" "$tag-off-b" "$on_dir" >>"$OUT/noop-rtl-comparisons.txt"
}

PROFILE_MANIFEST="$TESTS/default_profile_cases.tsv"
while IFS='|' read -r tag top macro expected bundle burst_setting extra_macro reason \
  read_burst read_outstanding read_fifo write_burst write_outstanding write_fifo \
  read_burst_origin write_burst_origin; do
  [[ -z "$tag" || "$tag" == \#* ]] && continue
  local_args=()
  case "$burst_setting" in
    default) ;;
    off) local_args+=("--bambu-parameter=${BURST_PARAMETER}=0") ;;
    invalid) local_args+=("--bambu-parameter=${BURST_PARAMETER}=2") ;;
    *) echo "invalid burst setting in $PROFILE_MANIFEST: $burst_setting" >&2; exit 2 ;;
  esac
  [[ -z "$extra_macro" ]] || local_args+=("--extra-cc-options=-D$extra_macro")
  PROFILE_READ_BURST=$read_burst PROFILE_READ_OUTSTANDING=$read_outstanding PROFILE_READ_FIFO=$read_fifo \
  PROFILE_WRITE_BURST=$write_burst PROFILE_WRITE_OUTSTANDING=$write_outstanding PROFILE_WRITE_FIFO=$write_fifo \
  PROFILE_READ_BURST_ORIGIN=$read_burst_origin PROFILE_WRITE_BURST_ORIGIN=$write_burst_origin \
    run_case "$tag" "$top" "$macro" "$expected" "$bundle" "$reason" "${local_args[@]}"
done < "$PROFILE_MANIFEST"
run_case fixed-device-default read_only AXI_DEFAULT_CASE_READ fixed data \
  '' -b "$FIXED_DEVICE"
run_case fallback-stride stride_read AXI_DEFAULT_CASE_STRIDE fallback data \
  'read-burst CFG/SSA recognition failed or bundle is not exclusive'
run_case fallback-cache cached_read AXI_DEFAULT_CASE_CACHE fallback data \
  'cache-enabled bundle'
run_case fallback-nested nested_read AXI_DEFAULT_CASE_NESTED fallback data \
  'read-burst CFG/SSA recognition failed or bundle is not exclusive'
run_case unused-m-axi-pointer no_access AXI_DEFAULT_CASE_NO_ACCESS no-access data
# Repeated off runs establish deterministic generated RTL before comparing the
# default-on fallback cases. Only the generated provenance banner is ignored.
: >"$OUT/noop-rtl-comparisons.txt"
run_noop_pair fallback-stride stride_read AXI_DEFAULT_CASE_STRIDE data fallback-stride "$SOURCE" \
  'non-unit stride prevents burst recognition' optout
run_noop_pair fallback-cache cached_read AXI_DEFAULT_CASE_CACHE data fallback-cache "$SOURCE" \
  'cache-enabled bundle' optout
run_noop_pair fallback-nested nested_read AXI_DEFAULT_CASE_NESTED data fallback-nested "$SOURCE" \
  'nested loops use the conservative scalar path' optout
run_noop_pair unused-m-axi-pointer no_access AXI_DEFAULT_CASE_NO_ACCESS data unused-m-axi-pointer "$SOURCE" \
  'pointer has no memory accesses' optout
BYTE_SOURCE="$ROOT/panda_regressions/hls/bambu_interface_test/axi_legacy_burst_hint.cpp"
run_noop_pair byte-legacy copy_bytes '' shared - "$BYTE_SOURCE" '8-bit accesses are outside the burst ABI' optout
MM_AXI_SOURCE="$ROOT/panda_regressions/hls/bambu_axi_verification/mm_axi.c"
run_noop_pair mm-axi-matrix mmult '' gmem0 - "$MM_AXI_SOURCE" \
  'nested tiled matrix loops retain the legacy RTL' optout \
  --extra-cc-options=-Drank=8 --extra-cc-options=-Dtile_rank=2

python3 "$CHECKER" rtl-different "$OUT/default-read" --compare "$OUT/explicit-optout"

echo "DEFAULT AXI BURST PROFILE PASS: profile defaults, explicit profiles, fallbacks, RTL no-op comparisons and positive control"
