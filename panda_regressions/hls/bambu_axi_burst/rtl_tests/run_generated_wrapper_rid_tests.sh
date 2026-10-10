#!/usr/bin/env bash
# Build a real Bambu-generated resource wrapper and exercise its 6-bit RID map.
# Usage: run_generated_wrapper_rid_tests.sh BAMBU GENERATED_DIR FRESH_OUTPUT_DIR
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
if (($# != 3)); then
  echo "usage: $0 BAMBU GENERATED_DIR FRESH_OUTPUT_DIR" >&2
  exit 2
fi
if [[ -e "$3" || -L "$3" ]]; then
  echo "output directory must not already exist: $3" >&2; exit 2
fi
TASK_OUT=$(realpath -m "$3")
mkdir -p "$TASK_OUT"
mkdir -p "$TASK_OUT/tmp"
export TMPDIR="$TASK_OUT/tmp"
export TMP="$TASK_OUT/tmp"
export TEMP="$TASK_OUT/tmp"

SOURCE="$TESTS/../bambu_axi_burst_sum_o1.cpp"
GEN=${2:-}
if [[ -n "$GEN" ]]; then
  GEN=$(realpath "$GEN")
  [[ -s "$GEN/kernel.v" && -s "$GEN/panda_libtech.v" ]] || {
    echo "existing generated RTL directory lacks kernel.v or panda_libtech.v: $GEN" >&2; exit 2;
  }
else
  GEN="$TASK_OUT/compiler"
  mkdir -p "$GEN"
  if ! (cd "$GEN" && "$BAMBU" "$SOURCE" --top-fname=kernel --compiler=I386_CLANG16 \
      --generate-interface=INFER --device-name=xcu250,-2L,figd2104 \
      --AXI-burst-type=INCREMENTAL --bambu-parameter=experimental-m-axi-burst=1 \
      --mem-delay-read=64 --mem-delay-write=64 \
      --memory-allocation-policy=NO_BRAM --distram-threshold=0 --no-clean \
      --extra-cc-options=-DNDEBUG --extra-cc-options=-DBAMBU --extra-cc-options=-DNOCACHE \
      >bambu.stdout.log 2>bambu.stderr.log); then
    tail -n 100 "$GEN/bambu.stderr.log" >&2 || true
    tail -n 100 "$GEN/bambu.stdout.log" >&2 || true
    echo "Bambu generation for wrapper RID test failed; see $GEN/bambu.*.log" >&2
    exit 1
  fi
fi
RTL="$GEN/kernel.v"
LIB="$GEN/panda_libtech.v"
TB="$TESTS/tb_generated_wrapper_rid.sv"
for artifact in "$RTL" "$LIB"; do
  [[ -s "$artifact" ]] || { echo "missing generated artifact: $artifact" >&2; exit 2; }
done
if [[ -s "$GEN/bambu.stderr.log" ]]; then
  rg -q 'Configured read burst regions for bundle src \(B=16\)' "$GEN/bambu.stderr.log" || {
    echo "Bambu did not configure the src burst region; refusing a legacy-path wrapper test" >&2; exit 2;
  }
fi
python3 - "$RTL" <<'PY'
from pathlib import Path
import re
import sys

normalized = re.sub(r"\s+", "", Path(sys.argv[1]).read_text())
expected = ("MinimalAXI4MasterPipelined#(.B_MAX(16),"
            ".MAX_OUTSTANDING(1),.FIFO_DEPTH(256)")
if expected not in normalized:
    raise SystemExit("generated wrapper burst parameters do not match fixture B=16/O=1/D=256")
for width_param in (".BITSIZE_m_axi_araddr(32)", ".BITSIZE_m_axi_rdata(32)", ".BITSIZE_in5(32)"):
    if width_param not in normalized:
        raise SystemExit(f"generated wrapper lacks width parameter {width_param}")
PY
rg -q '^module src_bambu_artificial_ParmMgr_modgen\(' "$RTL" || {
  echo "expected generated m_axi resource wrapper is absent" >&2; exit 2;
}
rg -q 'wire burst_rid = \|p_m_axi_src_rid;' "$RTL" || {
  echo "generated wrapper does not contain the expected full-width RID reduction" >&2; exit 2;
}

compile_and_run() {
  local name=$1 source_rtl=$2 source_lib=$3 args=${4:-}
  local obj="$TASK_OUT/$name/obj_dir"
  mkdir -p "$(dirname "$obj")"
  verilator --binary -j "${SIM_JOBS:-4}" --timing --top-module tb_generated_wrapper_rid -Wno-fatal \
    --Mdir "$obj" "$source_rtl" "$source_lib" "$TB" \
    >"$TASK_OUT/$name-build.log" 2>&1 || {
      tail -n 80 "$TASK_OUT/$name-build.log" >&2
      return 1
    }
  # shellcheck disable=SC2086
  "$obj/Vtb_generated_wrapper_rid" $args | tee "$TASK_OUT/$name-run.log"
}

compile_and_run generated "$RTL" "$LIB"

# A deliberately broken bit-0-only reduction is made only in a scratch copy
# of the generated file. This must make RID 2 and 32 incorrectly complete.
mkdir -p "$TASK_OUT/negative"
cp "$RTL" "$TASK_OUT/negative/renamed_sum.v"
python3 - "$TASK_OUT/negative/renamed_sum.v" <<'PY'
from pathlib import Path
import sys

path = Path(sys.argv[1])
text = path.read_text()
correct = "wire burst_rid = |p_m_axi_src_rid;"
broken = "wire burst_rid = p_m_axi_src_rid[0];"
if text.count(correct) != 1:
    raise SystemExit("expected exactly one full-width generated RID reduction")
path.write_text(text.replace(correct, broken))
PY
compile_and_run negative "$TASK_OUT/negative/renamed_sum.v" "$LIB" +NEGATIVE_SENSITIVITY

echo "GENERATED WRAPPER RID TESTS PASS: $TASK_OUT"
