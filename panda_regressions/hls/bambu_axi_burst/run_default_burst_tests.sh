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
  if (( $# >= 6 )); then shift 6; else shift 5; fi
  local case_dir="$OUT/$tag" rc=0
  mkdir -p "$case_dir"
  local args=("--top-fname=$top" --compiler=I386_CLANG16 --generate-interface=INFER
    --device-name=xcu250,-2L,figd2104 --no-clean
    --extra-cc-options=-DNDEBUG --extra-cc-options=-DBAMBU "--extra-cc-options=-D$macro")
  args+=("$@")
  printf '%q ' "$BAMBU" "$SOURCE" "${args[@]}" >"$case_dir/argv.txt"
  printf '\n' >>"$case_dir/argv.txt"
  if (cd "$case_dir" && "$BAMBU" "$SOURCE" "${args[@]}" >stdout.log 2>stderr.log); then
    :
  else
    rc=$?
  fi
  if [[ "$expected" == reject ]]; then
    [[ $rc -ne 0 ]] || { echo "$tag was accepted; expected rejection" >&2; return 1; }
    grep -Fq "Parameter 'experimental-m-axi-burst' has been renamed to 'm-axi-burst'." \
      "$case_dir/stdout.log" "$case_dir/stderr.log" || {
      echo "$tag failed without the expected parameter rename diagnostic; see $case_dir" >&2; return 1;
    }
    echo "DEFAULT BURST RENAME PASS: $tag"
    return
  fi
  [[ $rc -eq 0 ]] || { echo "$tag failed with Bambu status $rc; see $case_dir" >&2; return 1; }
  if [[ "$expected" == default-read || "$expected" == default-write || "$expected" == default-duplex ]]; then
    local kind=${expected#default-}
    python3 "$CHECKER" "$kind" "$case_dir" --bundle "$bundle"
  elif [[ "$expected" == override-read ]]; then
    python3 "$CHECKER" read "$case_dir" --bundle "$bundle" --read-burst 8 \
      --read-outstanding 3 --read-fifo 32
  elif [[ "$expected" == override-write ]]; then
    python3 "$CHECKER" write "$case_dir" --bundle "$bundle" --write-burst 8 \
      --write-outstanding 3 --write-fifo 16
  elif [[ "$expected" == fixed ]]; then
    python3 "$CHECKER" fixed "$case_dir" --bundle "$bundle"
  elif [[ "$expected" == optout ]]; then
    python3 "$CHECKER" optout "$case_dir" --bundle "$bundle"
  elif [[ "$expected" == no-access ]]; then
    python3 "$CHECKER" no-access "$case_dir" --bundle "$bundle"
  elif [[ "$expected" == fallback ]]; then
    python3 "$CHECKER" fallback "$case_dir" --bundle "$bundle" --reason "$reason"
  else
    echo "invalid expectation $expected" >&2; return 2
  fi
}

# The three positive cases intentionally omit both --bambu-parameter=m-axi-burst
# and --AXI-burst-type. The device's INCREMENTAL profile supplies burst type.
run_case default-read read_only AXI_DEFAULT_CASE_READ default-read data
run_case default-write write_only AXI_DEFAULT_CASE_WRITE default-write data
run_case default-duplex duplex_copy AXI_DEFAULT_CASE_DUPLEX default-duplex shared
run_case explicit-optout read_only AXI_DEFAULT_CASE_READ optout data \
  '' --bambu-parameter=m-axi-burst=0
run_case read-override read_only AXI_DEFAULT_CASE_READ override-read data \
  '' --extra-cc-options=-DAXI_DEFAULT_READ_OVERRIDE
run_case write-override write_only AXI_DEFAULT_CASE_WRITE override-write data \
  '' --extra-cc-options=-DAXI_DEFAULT_WRITE_OVERRIDE
run_case fixed-device-default read_only AXI_DEFAULT_CASE_READ fixed data \
  '' -b "$FIXED_DEVICE"
run_case fallback-stride stride_read AXI_DEFAULT_CASE_STRIDE fallback data \
  'read-burst CFG/SSA recognition failed or bundle is not exclusive'
run_case fallback-cache cached_read AXI_DEFAULT_CASE_CACHE fallback data \
  'cache-enabled bundle'
run_case fallback-nested nested_read AXI_DEFAULT_CASE_NESTED fallback data \
  'read-burst CFG/SSA recognition failed or bundle is not exclusive'
run_case unused-m-axi-pointer no_access AXI_DEFAULT_CASE_NO_ACCESS no-access data
run_case old-parameter-name read_only AXI_DEFAULT_CASE_READ reject data \
  '' --bambu-parameter=experimental-m-axi-burst=1

echo "DEFAULT AXI BURST PROFILE PASS: default read/write/duplex, opt-out, explicit profiles, FIXED and fallback diagnostics, no-access and renamed parameter"
