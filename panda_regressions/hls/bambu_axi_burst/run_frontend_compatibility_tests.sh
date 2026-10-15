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
rg -q '<axi_burst_type value="0"/>' "$FIXED_DEVICE" || {
  echo "could not create FIXED device override from $DEVICE" >&2; exit 2;
}

COMMON=(--top-fname=copy_read_first --compiler=I386_CLANG16 --generate-interface=INFER
  --device-name=xcu250,-2L,figd2104 --no-clean --bambu-parameter=experimental-m-axi-burst=1
  --extra-cc-options=-DNDEBUG --extra-cc-options=-DBAMBU)
SOURCE="$TESTS/compiler_cases/early_write_burst.cpp"
PARAMETER_LINE='^[[:space:]]*Parameter @'

INCR="$OUT/incr"
mkdir -p "$INCR"
if (cd "$INCR" && "$BAMBU" "$SOURCE" "${COMMON[@]}" --AXI-burst-type=INCREMENTAL >stdout.log 2>stderr.log); then
  :
else
  echo "INCR write-burst positive case failed; see $INCR" >&2; exit 1
fi
rg -q "Configured read burst regions for bundle z_input" "$INCR/stdout.log" "$INCR/stderr.log" || {
  echo "INCR case did not configure the read region; see $INCR" >&2; exit 1;
}
rg -q "Configured write burst region for bundle a_output" "$INCR/stdout.log" "$INCR/stderr.log" || {
  echo "INCR case did not configure the write region; see $INCR" >&2; exit 1;
}
rg -q "$PARAMETER_LINE" "$INCR/stdout.log" "$INCR/stderr.log" || {
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
bambu_ir = (output / "panda-temp/early_write_burst.cpp.bambuir").read_text()

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
rg -q "Configured read burst regions for bundle data" "$ORDINARY_GLOBAL/stdout.log" "$ORDINARY_GLOBAL/stderr.log" || {
  echo "ordinary global read disabled the unrelated pointer read burst; see $ORDINARY_GLOBAL" >&2; exit 1;
}
if rg -q '^[[:space:]]*Parameter @[^[:cntrl:]]*ordinary_global' "$ORDINARY_GLOBAL/stdout.log" "$ORDINARY_GLOBAL/stderr.log"; then
  echo "ordinary global was incorrectly attributed as a top-level interface parameter; see $ORDINARY_GLOBAL" >&2; exit 1
fi
rg -q 'MinimalAXI4MasterPipelined.*\.B_MAX\(16\)' "$ORDINARY_GLOBAL/read_with_ordinary_global.v" || {
  echo "ordinary global case did not generate the expected read burst resource; see $ORDINARY_GLOBAL" >&2; exit 1;
}
rg -q 'output p_m_axi_data_arvalid' "$ORDINARY_GLOBAL/read_with_ordinary_global.v" || {
  echo "ordinary global case did not generate the data-bundle AXI read interface; see $ORDINARY_GLOBAL" >&2; exit 1;
}

for top in read_with_volatile_global; do
  out=$(run_global_read_case "$top")
  if rg -q "Configured read burst regions for bundle data" "$out/stdout.log" "$out/stderr.log"; then
    echo "$top unexpectedly enabled read bursting across a non-ordinary global access; see $out" >&2; exit 1
  fi
  rg -q "Keeping legacy m_axi accesses for bundle data: memory writes or opaque effects" \
    "$out/stdout.log" "$out/stderr.log" || {
    echo "$top fell back without the expected conservative reason; see $out" >&2; exit 1;
  }
done

FIXED="$OUT/fixed-device"
mkdir -p "$FIXED"
if (cd "$FIXED" && "$BAMBU" "$SOURCE" "${COMMON[@]}" --AXI-burst-type=INCREMENTAL -b "$FIXED_DEVICE" \
    >stdout.log 2>stderr.log); then
  echo "FIXED device override unexpectedly accepted; see $FIXED" >&2; exit 1
fi
rg -q "Configured write bursts for bundle 'a_output' require AXI INCREMENTAL burst type; the effective AXI burst type is FIXED\\." \
  "$FIXED/stdout.log" "$FIXED/stderr.log" || {
  echo "FIXED override failed without the expected early usage diagnostic; see $FIXED" >&2; exit 1;
}
if rg -qi 'THROW_ASSERT|assertion failed|Configured write burst region|Configured read burst regions' \
  "$FIXED/stdout.log" "$FIXED/stderr.log"; then
  echo "FIXED override reached interface rewriting/configuration or an internal assertion; see $FIXED" >&2; exit 1
fi
if rg -q "$PARAMETER_LINE" "$FIXED/stdout.log" "$FIXED/stderr.log"; then
  echo "FIXED override reached parameter processing before rejecting the burst; see $FIXED" >&2; exit 1
fi

WRITE_ONLY_COMMON=(--top-fname=write_only --compiler=I386_CLANG16 --generate-interface=INFER
  --device-name=xcu250,-2L,figd2104 --no-clean --extra-cc-options=-DNDEBUG --extra-cc-options=-DBAMBU)
WRITE_ONLY_FIXED="$OUT/fixed-device-write-only"
mkdir -p "$WRITE_ONLY_FIXED"
if (cd "$WRITE_ONLY_FIXED" && "$BAMBU" "$SOURCE" "${WRITE_ONLY_COMMON[@]}" \
    --bambu-parameter=experimental-m-axi-burst=1 --AXI-burst-type=INCREMENTAL -b "$FIXED_DEVICE" \
    >stdout.log 2>stderr.log); then
  echo "FIXED device override unexpectedly accepted write-only burst; see $WRITE_ONLY_FIXED" >&2; exit 1
fi
rg -q "Configured write bursts for bundle 'output' require AXI INCREMENTAL burst type; the effective AXI burst type is FIXED\\." \
  "$WRITE_ONLY_FIXED/stdout.log" "$WRITE_ONLY_FIXED/stderr.log" || {
  echo "write-only FIXED override failed without the expected usage diagnostic; see $WRITE_ONLY_FIXED" >&2; exit 1;
}
if rg -qi 'THROW_ASSERT|assertion failed|Configured write burst region|Configured read burst regions' \
  "$WRITE_ONLY_FIXED/stdout.log" "$WRITE_ONLY_FIXED/stderr.log"; then
  echo "write-only FIXED override reached configuration/interface rewriting or an internal assertion; see $WRITE_ONLY_FIXED" >&2; exit 1
fi
if rg -q "$PARAMETER_LINE" "$WRITE_ONLY_FIXED/stdout.log" "$WRITE_ONLY_FIXED/stderr.log"; then
  echo "write-only FIXED override reached parameter processing before rejecting the burst; see $WRITE_ONLY_FIXED" >&2; exit 1
fi

LEGACY_FIXED="$OUT/fixed-feature-off-write-only"
mkdir -p "$LEGACY_FIXED"
if ! (cd "$LEGACY_FIXED" && "$BAMBU" "$SOURCE" "${WRITE_ONLY_COMMON[@]}" \
    --bambu-parameter=experimental-m-axi-burst=0 --AXI-burst-type=FIXED -b "$FIXED_DEVICE" \
    >stdout.log 2>stderr.log); then
  echo "feature-off FIXED legacy case failed; see $LEGACY_FIXED" >&2; exit 1
fi
if rg -qi 'Configured write burst region|Configured read burst regions|THROW_ASSERT|assertion failed' \
  "$LEGACY_FIXED/stdout.log" "$LEGACY_FIXED/stderr.log"; then
  echo "feature-off FIXED case unexpectedly configured a burst or hit an internal assertion; see $LEGACY_FIXED" >&2; exit 1
fi
echo "FRONTEND COMPATIBILITY PASS: copy/global-read compatibility, volatile fallback, early write preflight, and feature-off legacy"
