#!/usr/bin/env bash
# Manual compiler-only checks for early write-burst validation.
# Usage: run_frontend_compatibility_tests.sh BAMBU OUTPUT_DIR
set -euo pipefail
TESTS=$(cd "$(dirname "$0")" && pwd)
ROOT=$(realpath "$TESTS/../../..")
BAMBU_ARG=${1:?Bambu executable required}
OUT_ARG=${2:?fresh output directory required}
if [[ "$BAMBU_ARG" == */* ]]; then BAMBU=$(realpath "$BAMBU_ARG"); else BAMBU=$(command -v "$BAMBU_ARG" || true); [[ -n "$BAMBU" ]] && BAMBU=$(realpath "$BAMBU"); fi
if [[ -z ${BAMBU:-} || ! -x "$BAMBU" ]]; then echo "Bambu executable not found: $BAMBU_ARG" >&2; exit 2; fi
if [[ -e "$OUT_ARG" || -L "$OUT_ARG" ]]; then echo "output directory must not already exist: $OUT_ARG" >&2; exit 2; fi
OUT=$(realpath -m "$OUT_ARG")
mkdir -p "$OUT/tmp"
export TMPDIR="$OUT/tmp" TMP="$OUT/tmp" TEMP="$OUT/tmp"

DEVICE="$ROOT/etc/libtech/xilinx/xcu250-2Lfigd2104.xml"
FIXED_DEVICE="$OUT/xcu250-fixed.xml"
sed 's/<axi_burst_type value="1"\/>/<axi_burst_type value="0"\/>/' "$DEVICE" >"$FIXED_DEVICE"
grep -Eq '<axi_burst_type value="0"/>' "$FIXED_DEVICE" || {
  echo "could not create FIXED device override from $DEVICE" >&2; exit 2;
}

COMMON=(--top-fname=copy_read_first --compiler=I386_CLANG16 --generate-interface=INFER
  --device-name=xcu250,-2L,figd2104 --no-clean --bambu-parameter=m-axi-burst=1
  --extra-cc-options=-DNDEBUG --extra-cc-options=-DBAMBU
  --extra-cc-options=-DAXI_ANALYSIS_CASE_01)
SOURCE="$TESTS/compiler_cases/axi_analysis_cases.cpp"
PARAMETER_LINE='^[[:space:]]*Parameter @'

INCR="$OUT/incr"
mkdir -p "$INCR"
if (cd "$INCR" && "$BAMBU" "$SOURCE" "${COMMON[@]}" --AXI-burst-type=INCREMENTAL >stdout.log 2>stderr.log); then
  :
else
  echo "INCR write-burst positive case failed; see $INCR" >&2; exit 1
fi
grep -Eq "Configured read burst regions for bundle z_input" "$INCR/stdout.log" "$INCR/stderr.log" || {
  echo "INCR case did not configure the read region; see $INCR" >&2; exit 1;
}
grep -Eq "Configured write burst region for bundle a_output" "$INCR/stdout.log" "$INCR/stderr.log" || {
  echo "INCR case did not configure the write region; see $INCR" >&2; exit 1;
}
grep -Eq "$PARAMETER_LINE" "$INCR/stdout.log" "$INCR/stderr.log" || {
  echo "INCR case did not reach parameter processing; see $INCR" >&2; exit 1;
}

run_global_read_case() {
  local top=$1
  local out="$OUT/$top"
  mkdir -p "$out"
  (cd "$out" && "$BAMBU" "$SOURCE" "${COMMON[@]/copy_read_first/$top}" \
      --AXI-burst-type=INCREMENTAL >stdout.log 2>stderr.log) || {
    echo "$top compilation failed; see $out" >&2; exit 1;
  }
  printf '%s\n' "$out"
}

ORDINARY_GLOBAL=$(run_global_read_case read_with_ordinary_global)
python3 - "$ORDINARY_GLOBAL" <<'PY'
import pathlib
import re
import sys

output = pathlib.Path(sys.argv[1])
llvm_ir = (output / "panda-temp/after_CSROA.ll").read_text()
bambu_ir = (output / "panda-temp/axi_analysis_cases.cpp.bambuir").read_text()

if not re.search(r'^@ordinary_global = external .*global \[256 x i32\]', llvm_ir, re.M):
    raise SystemExit("ordinary global is not an external mutable LLVM global")
global_gep = re.search(
    r'^\s*(%[\w.]+) = getelementptr \[256 x i32\], \[256 x i32\]\* @ordinary_global,',
    llvm_ir,
    re.M,
)
if not global_gep or not re.search(
    rf'^\s*%[\w.]+ = load i32, i32\* {re.escape(global_gep.group(1))}\b',
    llvm_ir,
    re.M,
):
    raise SystemExit("LLVM IR does not load through the ordinary_global address")

global_variable = re.search(r'^(@\d+)\s+variable_val_node name:\s+(@\d+)', bambu_ir, re.M)
if not global_variable:
    raise SystemExit("Bambu IR does not import ordinary_global as a variable")
variable_id, name_id = global_variable.groups()
if not re.search(rf'^{re.escape(name_id)}\s+identifier_node\s+strg: "ordinary_global"', bambu_ir, re.M):
    raise SystemExit("Bambu IR global variable does not retain the ordinary_global name")
if not re.search(rf'^{re.escape(variable_id)}\s+variable_val_node[^\n]*\n(?:[ \t].*\n){{0,3}}[ \t]*extern\b', bambu_ir, re.M):
    raise SystemExit("Bambu IR ordinary_global is not marked external")
global_use = re.search(rf'^(@\d+)\s+ssa_node[^\n]*use_vars:\s+{re.escape(variable_id)}\b', bambu_ir, re.M)
if not global_use:
    raise SystemExit("Bambu IR has no SSA address use of ordinary_global")
if not re.search(
    rf'^@\d+\s+mem_access_node[^\n]*op\s*:\s*{re.escape(global_use.group(1))}\b[^\n]*ordinary_memory_access',
    bambu_ir,
    re.M,
):
    raise SystemExit("Bambu IR has no ordinary memory load through ordinary_global")
PY
grep -Eq "Configured read burst regions for bundle data" "$ORDINARY_GLOBAL/stdout.log" "$ORDINARY_GLOBAL/stderr.log" || {
  echo "ordinary global read disabled the unrelated pointer read burst; see $ORDINARY_GLOBAL" >&2; exit 1;
}
if grep -Eq '^[[:space:]]*Parameter @[^[:cntrl:]]*ordinary_global' "$ORDINARY_GLOBAL/stdout.log" "$ORDINARY_GLOBAL/stderr.log"; then
  echo "ordinary global was incorrectly attributed as a top-level interface parameter; see $ORDINARY_GLOBAL" >&2; exit 1
fi
grep -Eq 'MinimalAXI4MasterPipelined.*\.B_MAX\(16\)' "$ORDINARY_GLOBAL/read_with_ordinary_global.v" || {
  echo "ordinary global case did not generate the expected read burst resource; see $ORDINARY_GLOBAL" >&2; exit 1;
}
grep -Eq 'output p_m_axi_data_arvalid' "$ORDINARY_GLOBAL/read_with_ordinary_global.v" || {
  echo "ordinary global case did not generate the data-bundle AXI read interface; see $ORDINARY_GLOBAL" >&2; exit 1;
}

for top in read_with_volatile_global; do
  out=$(run_global_read_case "$top")
  if grep -Eq "Configured read burst regions for bundle data" "$out/stdout.log" "$out/stderr.log"; then
    echo "$top unexpectedly enabled read bursting across a non-ordinary global access; see $out" >&2; exit 1
  fi
  grep -Eq "Keeping legacy m_axi accesses for bundle data: memory writes or opaque effects" \
    "$out/stdout.log" "$out/stderr.log" || {
    echo "$top fell back without the expected conservative reason; see $out" >&2; exit 1;
  }
done

FIXED="$OUT/fixed-device"
mkdir -p "$FIXED"
if ! (cd "$FIXED" && "$BAMBU" "$SOURCE" "${COMMON[@]}" --AXI-burst-type=INCREMENTAL -b "$FIXED_DEVICE" \
    >stdout.log 2>stderr.log); then
  echo "FIXED device override failed instead of falling back; see $FIXED" >&2; exit 1
fi
grep -Eqi 'Keeping legacy m_axi .*AXI burst type is FIXED; incremental prefetch is unsupported|Keeping legacy m_axi .*AXI burst type is FIXED; write coalescing is unsupported' "$FIXED/stdout.log" "$FIXED/stderr.log" || {
  echo "FIXED override did not report a conservative fallback reason; see $FIXED" >&2; exit 1;
}
if grep -Eqi 'THROW_ASSERT|assertion failed|Configured write burst region|Configured read burst regions|MinimalAXI4MasterPipelined' \
  "$FIXED/stdout.log" "$FIXED/stderr.log"; then
  echo "FIXED override emitted burst configuration/RTL or hit an internal assertion; see $FIXED" >&2; exit 1
fi

WRITE_ONLY_COMMON=(--top-fname=write_only --compiler=I386_CLANG16 --generate-interface=INFER
  --device-name=xcu250,-2L,figd2104 --no-clean --extra-cc-options=-DNDEBUG --extra-cc-options=-DBAMBU
  --extra-cc-options=-DAXI_ANALYSIS_CASE_01)
WRITE_ONLY_FIXED="$OUT/fixed-device-write-only"
mkdir -p "$WRITE_ONLY_FIXED"
if ! (cd "$WRITE_ONLY_FIXED" && "$BAMBU" "$SOURCE" "${WRITE_ONLY_COMMON[@]}" \
    --bambu-parameter=m-axi-burst=1 --AXI-burst-type=INCREMENTAL -b "$FIXED_DEVICE" \
    >stdout.log 2>stderr.log); then
  echo "FIXED device override failed instead of falling back; see $WRITE_ONLY_FIXED" >&2; exit 1
fi
grep -Eqi 'Keeping legacy m_axi writes for bundle output: AXI burst type is FIXED; write coalescing is unsupported' "$WRITE_ONLY_FIXED/stdout.log" "$WRITE_ONLY_FIXED/stderr.log" || {
  echo "write-only FIXED override did not report a conservative fallback reason; see $WRITE_ONLY_FIXED" >&2; exit 1;
}
if grep -Eqi 'THROW_ASSERT|assertion failed|Configured write burst region|Configured read burst regions|MinimalAXI4MasterPipelined' \
  "$WRITE_ONLY_FIXED/stdout.log" "$WRITE_ONLY_FIXED/stderr.log"; then
  echo "write-only FIXED override emitted burst configuration/RTL or hit an internal assertion; see $WRITE_ONLY_FIXED" >&2; exit 1
fi

LEGACY_FIXED="$OUT/fixed-feature-off-write-only"
mkdir -p "$LEGACY_FIXED"
if ! (cd "$LEGACY_FIXED" && "$BAMBU" "$SOURCE" "${WRITE_ONLY_COMMON[@]}" \
    --bambu-parameter=m-axi-burst=0 --AXI-burst-type=FIXED -b "$FIXED_DEVICE" \
    >stdout.log 2>stderr.log); then
  echo "feature-off FIXED legacy case failed; see $LEGACY_FIXED" >&2; exit 1
fi
if grep -Eqi 'Configured write burst region|Configured read burst regions|THROW_ASSERT|assertion failed' \
  "$LEGACY_FIXED/stdout.log" "$LEGACY_FIXED/stderr.log"; then
  echo "feature-off FIXED case unexpectedly configured a burst or hit an internal assertion; see $LEGACY_FIXED" >&2; exit 1
fi

run_feature_off_case() {
  local top=$1 source=$2 expected_reason=${3:-} case_macro=${4:-}
  local out="$OUT/feature-off-$top"
  mkdir -p "$out"
  local macro_args=()
  if [[ -n $case_macro ]]; then macro_args+=("--extra-cc-options=-D$case_macro"); fi
  if ! (cd "$out" && "$BAMBU" "$source" --top-fname="$top" --compiler=I386_CLANG16 \
      --generate-interface=INFER --device-name=xcu250,-2L,figd2104 --AXI-burst-type=FIXED \
      --bambu-parameter=m-axi-burst=0 --no-clean \
      --extra-cc-options=-DNDEBUG --extra-cc-options=-DBAMBU "${macro_args[@]}" >stdout.log 2>stderr.log); then
    echo "$top feature-off legacy case failed; see $out" >&2; exit 1
  fi
  if [[ -n "$expected_reason" ]]; then
    grep -Fq "$expected_reason" "$out/stdout.log" "$out/stderr.log" || {
      echo "$top lacks its direction-specific feature-off diagnostic; see $out" >&2; exit 1;
    }
  elif grep -Eq 'explicit (read|write)-burst request ignored because m-axi-burst=0' \
      "$out/stdout.log" "$out/stderr.log"; then
    echo "$top emitted a feature-off burst diagnostic without a request; see $out" >&2; exit 1
  fi
  if grep -rIEq 'Configured (read|write) burst regions?|MinimalAXI4MasterPipelined|THROW_ASSERT|assertion failed' "$out"; then
    echo "$top feature-off case configured burst RTL or hit an internal assertion; see $out" >&2; exit 1
  fi
}

run_feature_off_case read_with_ordinary_global "$SOURCE" \
  "Keeping legacy m_axi reads for bundle data: explicit read-burst request ignored because m-axi-burst=0" \
  AXI_ANALYSIS_CASE_01
run_feature_off_case write_only "$SOURCE" \
  "Keeping legacy m_axi writes for bundle output: explicit write-burst request ignored because m-axi-burst=0" \
  AXI_ANALYSIS_CASE_01
run_feature_off_case scalar_m_axi_without_burst \
  "$TESTS/write_compiler_cases/legacy_m_axi_without_write_burst.cpp"

DEFAULT_FIXED="$OUT/default-fixed-hints"
mkdir -p "$DEFAULT_FIXED"
if ! (cd "$DEFAULT_FIXED" && "$BAMBU" "$SOURCE" --top-fname=copy_read_first \
    --compiler=I386_CLANG16 --generate-interface=INFER --device-name=xcu250,-2L,figd2104 \
    --AXI-burst-type=FIXED -b "$FIXED_DEVICE" --bambu-parameter=m-axi-burst=0 --no-clean \
    --extra-cc-options=-DNDEBUG --extra-cc-options=-DBAMBU \
    --extra-cc-options=-DAXI_ANALYSIS_CASE_01 >stdout.log 2>stderr.log); then
  echo "explicit opt-out FIXED burst-hint case failed; see $DEFAULT_FIXED" >&2; exit 1
fi
grep -Fq 'explicit read-burst request ignored because m-axi-burst=0' \
  "$DEFAULT_FIXED/stdout.log" "$DEFAULT_FIXED/stderr.log" || {
  echo "explicit opt-out FIXED case did not ignore the read hint; see $DEFAULT_FIXED" >&2; exit 1;
}
grep -Fq 'explicit write-burst request ignored because m-axi-burst=0' \
  "$DEFAULT_FIXED/stdout.log" "$DEFAULT_FIXED/stderr.log" || {
  echo "explicit opt-out FIXED case did not ignore the write hint; see $DEFAULT_FIXED" >&2; exit 1;
}
if grep -rIEq 'Configured (read|write) burst regions?|MinimalAXI4MasterPipelined|THROW_ASSERT|assertion failed' \
    "$DEFAULT_FIXED"; then
  echo "opt-out FIXED case configured a burst engine or hit an internal assertion; see $DEFAULT_FIXED" >&2; exit 1
fi

LEGACY_READ_HINT_SOURCE="$ROOT/panda_regressions/hls/bambu_interface_test/axi_legacy_burst_hint.cpp"
run_default_legacy_read_hint() {
  local name=$1
  shift
  local out="$OUT/$name"
  mkdir -p "$out"
  if ! (cd "$out" && "$BAMBU" "$LEGACY_READ_HINT_SOURCE" --top-fname=copy_bytes \
      --compiler=I386_CLANG16 --generate-interface=INFER --device-name=xcu250,-2L,figd2104 \
      --no-clean --bambu-parameter=m-axi-burst=0 \
      --extra-cc-options=-DNDEBUG --extra-cc-options=-DBAMBU "$@" >stdout.log 2>stderr.log); then
    echo "$name default legacy read-hint case failed; see $out" >&2; exit 1
  fi
  grep -Fq 'explicit read-burst request ignored because m-axi-burst=0' \
    "$out/stdout.log" "$out/stderr.log" || {
    echo "$name did not ignore the read hint under the default feature setting; see $out" >&2; exit 1;
  }
  if grep -rIEq 'Configured (read|write) burst regions?|MinimalAXI4MasterPipelined|THROW_ASSERT|assertion failed' "$out"; then
    echo "$name configured burst RTL or hit an internal assertion; see $out" >&2; exit 1
  fi
}

# These runs check explicit opt-out with and without an AXI burst-type option.
run_default_legacy_read_hint default-legacy-read-hint
run_default_legacy_read_hint fixed-device-default-legacy-read-hint -b "$FIXED_DEVICE"

DEFAULT_LEGACY="$OUT/default-no-hints"
mkdir -p "$DEFAULT_LEGACY"
if ! (cd "$DEFAULT_LEGACY" && "$BAMBU" "$TESTS/write_compiler_cases/legacy_m_axi_without_write_burst.cpp" \
    --top-fname=scalar_m_axi_without_burst --compiler=I386_CLANG16 --generate-interface=INFER \
    --device-name=xcu250,-2L,figd2104 --AXI-burst-type=INCREMENTAL --bambu-parameter=m-axi-burst=0 --no-clean \
    --extra-cc-options=-DNDEBUG --extra-cc-options=-DBAMBU >stdout.log 2>stderr.log); then
  echo "default no-hint legacy m_axi case failed; see $DEFAULT_LEGACY" >&2; exit 1
fi
if grep -rIEq 'explicit (read|write)-burst request ignored|Configured (read|write) burst regions?|MinimalAXI4MasterPipelined|THROW_ASSERT|assertion failed' \
    "$DEFAULT_LEGACY"; then
  echo "default no-hint legacy case emitted burst diagnostics/RTL or hit an assertion; see $DEFAULT_LEGACY" >&2; exit 1
fi

run_non_axi_burst_hint_case() {
  local feature=$1 out="$OUT/nonaxi-hints-feature-$1"
  mkdir -p "$out"
  if ! (cd "$out" && "$BAMBU" "$TESTS/compiler_cases/axi_invalid_attribute_cases.cpp" \
      --top-fname=legacy_non_axi_burst_hints --compiler=I386_CLANG16 --generate-interface=INFER \
      --device-name=xcu250,-2L,figd2104 --no-clean \
      "--bambu-parameter=m-axi-burst=$feature" \
      --extra-cc-options=-DNDEBUG --extra-cc-options=-DBAMBU \
      --extra-cc-options=-DAXI_CASE_LEGACY_NON_AXI_BURST_HINTS >stdout.log 2>stderr.log); then
    echo "non-m_axi legacy burst hints failed with feature=$feature; see $out" >&2; exit 1
  fi
  local outputs=("$out/stdout.log" "$out/stderr.log")
  while IFS= read -r cc_output; do outputs+=("$cc_output"); done \
    < <(find "$out/panda-temp" -maxdepth 1 -type f -name '__cc_output*' -print 2>/dev/null || true)
  for attr in max_read_burst_length max_write_burst_length; do
    grep -Fq "Ignoring $attr for non-m_axi interface mode 'ap_memory'" "${outputs[@]}" || {
      echo "non-m_axi $attr hint lacked its compatibility diagnostic with feature=$feature; see $out" >&2; exit 1;
    }
  done
  local architecture
  architecture=$(find "$out" -type f -name architecture.xml -print -quit)
  [[ -n $architecture ]] || { echo "non-m_axi hint case emitted no architecture.xml; see $out" >&2; exit 1; }
  python3 - "$architecture" <<'PY'
import sys
import xml.etree.ElementTree as ET

root = ET.parse(sys.argv[1]).getroot()
bundles = root.findall(".//function/bundles/bundle")
if not bundles or not any(bundle.get("mode") == "array" for bundle in bundles):
    raise SystemExit("non-m_axi hints did not preserve the ap_memory bundles")
for bundle in bundles:
    for attr in ("max_read_burst_length", "max_write_burst_length"):
        if attr in bundle.attrib:
            raise SystemExit(f"ignored non-m_axi hint was serialized: {bundle.attrib}")
PY
}

run_non_axi_burst_hint_case 0
run_non_axi_burst_hint_case 1
echo "FRONTEND COMPATIBILITY PASS: explicit opt-out, default burst opt-in, FIXED fallback, legacy no-hint, globals, and compatibility"
