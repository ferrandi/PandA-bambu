#!/usr/bin/env bash
# Generate the real kernel with Bambu, then adversarially simulate its whole
# generated top and AXI resource using Verilator (no controller/datapath stub).
# Usage: run_generated_kernel_tests.sh BAMBU FRESH_OUTPUT_DIR
set -euo pipefail

TESTS=$(cd "$(dirname "$0")" && pwd -P)
ROOT=$(realpath "$TESTS/../../../../")
BAMBU=${1:-$ROOT/install/bin/bambu}
if [[ "$BAMBU" == */* ]]; then
  BAMBU=$(realpath "$BAMBU")
else
  BAMBU=$(command -v "$BAMBU" || true)
  [[ -n "$BAMBU" ]] && BAMBU=$(realpath "$BAMBU")
fi
[[ -n "$BAMBU" && -x "$BAMBU" ]] || { echo "Bambu executable not found: ${1:-$BAMBU}" >&2; exit 2; }
command -v verilator >/dev/null || { echo "verilator is required" >&2; exit 2; }

PREFIX=$(dirname "$(dirname "$BAMBU")")
SETTINGS="$PREFIX/settings.sh"
[[ -r "$SETTINGS" ]] || { echo "verified sibling settings.sh is required: $SETTINGS" >&2; exit 2; }
# shellcheck disable=SC1090
source "$SETTINGS"
[[ $(realpath -m "${BAMBU_HLS:-}") == $(realpath "$PREFIX") ]] || {
  echo "settings.sh selected BAMBU_HLS='${BAMBU_HLS:-<unset>}' instead of $PREFIX" >&2; exit 2;
}
PLUGIN="$PREFIX/lib/panda/clang-16/ASTAnalyzer.so"
[[ -r "$PLUGIN" ]] || { echo "selected ASTAnalyzer plugin is missing: $PLUGIN" >&2; exit 2; }

if (($# != 2)); then
  echo "usage: $0 BAMBU FRESH_OUTPUT_DIR" >&2
  exit 2
fi
OUT=$2
if [[ -e "$OUT" || -L "$OUT" ]]; then
  echo "output directory must not already exist: $OUT" >&2; exit 2
fi
OUT=$(realpath -m "$OUT")
mkdir -p "$OUT"
mkdir -p "$OUT/tmp" "$OUT/generated"
export TMPDIR="$OUT/tmp" TMP="$OUT/tmp" TEMP="$OUT/tmp"

SOURCE="$TESTS/../bambu_axi_burst_sum_o1.cpp"
TB="$TESTS/tb_generated_kernel.sv"
GEN="$OUT/generated"
ARGS=("$SOURCE" --top-fname=kernel --compiler=I386_CLANG16 --generate-interface=INFER
      --device-name=xcu250,-2L,figd2104 --AXI-burst-type=INCREMENTAL --no-clean
      --bambu-parameter=experimental-m-axi-burst=1
      --mem-delay-read=64 --mem-delay-write=64
      --memory-allocation-policy=NO_BRAM --distram-threshold=0
      --extra-cc-options=-DNDEBUG --extra-cc-options=-DBAMBU --extra-cc-options=-DNOCACHE)
{
  printf 'root=%s\noutput=%s\nbambu=%s\nprefix=%s\nsettings=%s\nplugin=%s\n' \
    "$ROOT" "$OUT" "$BAMBU" "$PREFIX" "$(realpath "$SETTINGS")" "$PLUGIN"
  printf 'TMPDIR=%s\nTMP=%s\nTEMP=%s\n' "$TMPDIR" "$TMP" "$TEMP"
  printf 'argv='; printf '%q ' "$BAMBU" "${ARGS[@]}"; printf '\n'
  "$BAMBU" --version 2>&1 | head -n 2
  verilator --version
  sha256sum "$BAMBU" "$PLUGIN" "$SETTINGS" "$SOURCE" "$TB"
} >"$OUT/provenance.txt"

if ! (cd "$GEN" && "$BAMBU" "${ARGS[@]}" >bambu.stdout.log 2>bambu.stderr.log); then
  tail -n 100 "$GEN/bambu.stderr.log" >&2 || true
  tail -n 100 "$GEN/bambu.stdout.log" >&2 || true
  echo "Bambu kernel RTL generation failed; see $GEN/bambu.*.log" >&2
  exit 1
fi
RTL="$GEN/kernel.v"
LIB="$GEN/panda_libtech.v"
for artifact in "$RTL" "$LIB"; do
  [[ -s "$artifact" ]] || { echo "missing generated artifact: $artifact" >&2; exit 2; }
done
rg -q 'Configured read burst regions for bundle src \(B=16\)' "$GEN/bambu.stderr.log" || {
  echo "Bambu did not report the configured B=16 region; refusing legacy-path simulation" >&2; exit 2;
}
rg -q '^module kernel\(' "$RTL" || { echo "generated kernel top is absent" >&2; exit 2; }
python3 - "$RTL" <<'PY'
from pathlib import Path
import re
import sys

rtl = Path(sys.argv[1]).read_text()
normalized = re.sub(r"\s+", "", rtl)
expected = ("MinimalAXI4MasterPipelined#(.B_MAX(16),"
            ".MAX_OUTSTANDING(1),.FIFO_DEPTH(256)")
if expected not in normalized:
    raise SystemExit("generated burst resource parameters do not match fixture B=16/O=1/D=256")
# The widths must be wired to the real port widths, not left at the XML
# defaults: a one-bit count width makes the engine fetch a single element.
for width_param in (".BITSIZE_m_axi_araddr(32)", ".BITSIZE_m_axi_rdata(32)", ".BITSIZE_in5(32)"):
    if width_param not in normalized:
        raise SystemExit(f"generated burst resource lacks width parameter {width_param}")
PY
rg -q 'module MinimalAXI4MasterPipelined\(' "$LIB" || {
  echo "generated library lacks MinimalAXI4MasterPipelined" >&2; exit 2;
}
# These checks intentionally bind the test to the generated resource's real
# FIFO consume path. A hierarchy refactor must update/review this qualification.
rg -q 'fifo_load_pop' "$LIB" && rg -q 'fifo \[0:FIFO_DEPTH-1\]' "$LIB" || {
  echo "generated burst engine no longer exposes the expected FIFO consume path" >&2; exit 2;
}
rg -q 'src_bambu_artificial_ParmMgr_modgen_16_i0 \(' "$RTL" &&
  rg -q 'MinimalAXI4MasterPipelined' "$RTL" || {
  echo "expected generated resource hierarchy is absent" >&2; exit 2;
}
sha256sum "$RTL" "$LIB" >>"$OUT/provenance.txt"

OBJ="$OUT/verilator_obj"
if ! verilator --binary -j "${SIM_JOBS:-4}" --timing --public-flat-rw --top-module tb_generated_kernel -Wno-fatal \
    --Mdir "$OBJ" "$RTL" "$LIB" "$TB" >"$OUT/verilator-build.log" 2>&1; then
  tail -n 120 "$OUT/verilator-build.log" >&2
  echo "Verilator compile failed; see $OUT/verilator-build.log" >&2
  exit 1
fi
EXE="$OBJ/Vtb_generated_kernel"
"$EXE" | tee "$OUT/positive.log"
if "$EXE" +NEGATIVE_SENSITIVITY >"$OUT/negative-sensitivity.log" 2>&1; then
  echo "data-order negative-sensitivity run unexpectedly passed" >&2
  exit 1
fi
rg -q 'AXI response data order/value mismatch|generated AXI engine FIFO consume/pop data order/value mismatch' \
  "$OUT/negative-sensitivity.log" || {
    cat "$OUT/negative-sensitivity.log" >&2
    echo "negative-sensitivity failed for an unexpected reason" >&2
    exit 1
  }
echo "GENERATED_KERNEL_NEGATIVE_SENSITIVITY PASS (corrupted AXI data rejected by data-order scoreboard)"
echo "GENERATED_KERNEL_TESTS PASS: $OUT"
