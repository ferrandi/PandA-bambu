#!/usr/bin/env bash
# Manual end-to-end G2 test: compile and simulate a generated write top.
# Usage (outside the sandbox):
#   BURST_G2_SIM_OUTSIDE_SANDBOX=1 run_generated_write_kernel_tests.sh BAMBU \
#     [FRESH_OUTPUT_DIR] [TOP]
#   FRESH_OUTPUT_DIR defaults to ./out_generated_write_kernel, created in the
#   directory the script is launched from.
set -euo pipefail

TESTS=$(cd "$(dirname "$0")" && pwd -P)
ROOT=$(realpath "$TESTS/../../../../")
if (($# < 1 || $# > 3)); then
    echo "usage: BURST_G2_SIM_OUTSIDE_SANDBOX=1 $0 BAMBU [FRESH_OUTPUT_DIR] [TOP]" >&2
    echo "the full command must run outside the sandbox; default output: ./out_generated_write_kernel" >&2
    exit 2
fi
if [[ ${BURST_G2_SIM_OUTSIDE_SANDBOX:-} != 1 ]]; then
    echo "refusing compiler/build/simulation unless BURST_G2_SIM_OUTSIDE_SANDBOX=1 is set" >&2
    exit 2
fi
# Parallelism is a launcher decision, not a script policy: any positive value is
# accepted. The default follows the machine, capped at 16.
DEFAULT_SIM_JOBS=$(nproc 2>/dev/null || echo 4)
if ((DEFAULT_SIM_JOBS > 16)); then DEFAULT_SIM_JOBS=16; fi
SIM_JOBS=${SIM_JOBS:-$DEFAULT_SIM_JOBS}
[[ "$SIM_JOBS" =~ ^[1-9][0-9]*$ ]] || { echo "SIM_JOBS must be a positive integer" >&2; exit 2; }

BAMBU_ARG=$1
if [[ "$BAMBU_ARG" == */* ]]; then BAMBU=$(realpath "$BAMBU_ARG"); else BAMBU=$(command -v "$BAMBU_ARG" || true); [[ -n "$BAMBU" ]] && BAMBU=$(realpath "$BAMBU"); fi
[[ -n ${BAMBU:-} && -x "$BAMBU" ]] || { echo "Bambu executable not found: $BAMBU_ARG" >&2; exit 2; }
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

OUT_ARG=${2:-out_generated_write_kernel}
TASK_OUT=$(realpath -m -- "$OUT_ARG")
if [[ -e "$TASK_OUT" || -e "$OUT_ARG" || -L "$OUT_ARG" ]]; then
    echo "output directory must be fresh and not a symlink: $OUT_ARG" >&2; exit 2
fi
mkdir -p "$(dirname "$TASK_OUT")"
mkdir "$TASK_OUT" "$TASK_OUT/tmp" "$TASK_OUT/generated"
export TMPDIR="$TASK_OUT/tmp" TMP="$TASK_OUT/tmp" TEMP="$TASK_OUT/tmp"

SOURCE="$ROOT/panda_regressions/hls/bambu_axi_burst/write_compiler_cases/write_cases.cpp"
TB="$TESTS/tb_generated_write_kernel.sv"
GEN="$TASK_OUT/generated"
TOP_ARG=${3:-write_count_seed}
case "$TOP_ARG" in
    write_count_seed|write_with_unused_same_bundle|write_with_unused_same_bundle_reversed) ;;
    *) echo "unsupported G2 top: $TOP_ARG" >&2; exit 2 ;;
esac
ARGS=("--top-fname=$TOP_ARG" --compiler=I386_CLANG16 --generate-interface=INFER
      --device-name=xcu250,-2L,figd2104 --AXI-burst-type=INCREMENTAL --no-clean
      --bambu-parameter=experimental-m-axi-burst=1
      --extra-cc-options=-DNDEBUG --extra-cc-options=-DBAMBU)
{
    printf 'root=%s\noutput=%s\nbambu=%s\nprefix=%s\nsettings=%s\nplugin=%s\n' \
        "$ROOT" "$TASK_OUT" "$BAMBU" "$PREFIX" "$(realpath "$SETTINGS")" "$PLUGIN"
    printf 'TMPDIR=%s\nTMP=%s\nTEMP=%s\n' "$TMPDIR" "$TMP" "$TEMP"
    printf 'source=%s\ntestbench=%s\nargv=' "$SOURCE" "$TB"
    printf '%q ' "$BAMBU" "$SOURCE" "${ARGS[@]}"; printf '\n'
    "$BAMBU" --version 2>&1 | head -n 2
    verilator --version
    sha256sum "$BAMBU" "$PLUGIN" "$SETTINGS" "$SOURCE" "$TB"
} >"$TASK_OUT/provenance.txt"

if ! (cd "$GEN" && "$BAMBU" "$SOURCE" "${ARGS[@]}" >bambu.stdout.log 2>bambu.stderr.log); then
    tail -n 100 "$GEN/bambu.stderr.log" >&2 || true
    tail -n 100 "$GEN/bambu.stdout.log" >&2 || true
    echo "write_count kernel generation failed; see $GEN/bambu.*.log" >&2
    exit 1
fi
RTL="$GEN/$TOP_ARG.v"
LIB="$GEN/panda_libtech.v"
[[ -s "$RTL" ]] || RTL="$GEN/kernel.v"
for artifact in "$RTL" "$LIB"; do
    [[ -s "$artifact" ]] || { echo "missing generated artifact: $artifact" >&2; exit 2; }
done
python3 - "$RTL" "$LIB" "$TOP_ARG" <<'PY'
from pathlib import Path
import re
import sys

rtl = Path(sys.argv[1]).read_text(errors="replace")
lib = Path(sys.argv[2]).read_text(errors="replace")
requested_top = sys.argv[3]
top_matches = list(re.finditer(rf"(?m)^module\s+({re.escape(requested_top)}|kernel)\s*\(", rtl))
if not top_matches:
    raise SystemExit(f"expected generated {requested_top} top ({requested_top} or kernel naming)")
top = top_matches[-1]
end = rtl.find("endmodule", top.start())
if end < 0:
    raise SystemExit("generated top module has no endmodule")
top_body = rtl[top.start():end]
inputs = ["clock", "reset", "start_port", "dst", "n"]
if requested_top == "write_count_seed":
    inputs.append("seed")
inputs += [
          "m_axi_output_awready", "m_axi_output_wready", "m_axi_output_bid",
          "m_axi_output_bresp", "m_axi_output_buser", "m_axi_output_bvalid",
          "m_axi_output_arready", "m_axi_output_rid", "m_axi_output_rdata",
          "m_axi_output_rresp", "m_axi_output_rlast", "m_axi_output_ruser",
          "m_axi_output_rvalid"]
outputs = ["done_port", "idle_port", "m_axi_output_awid", "m_axi_output_awaddr",
           "m_axi_output_awlen", "m_axi_output_awsize", "m_axi_output_awburst",
           "m_axi_output_awvalid", "m_axi_output_wdata", "m_axi_output_wstrb",
           "m_axi_output_wlast", "m_axi_output_wvalid", "m_axi_output_bready",
           "m_axi_output_arvalid", "m_axi_output_rready"]
missing = [name for direction, names in (("input", inputs), ("output", outputs))
           for name in names
           if not re.search(rf"(?m)^\s*{direction}\s+(?:\[[^]]+\]\s*)?{re.escape(name)}\s*;", top_body)]
if missing:
    raise SystemExit("generated top port directions/names differ from G2 assumptions: " + ", ".join(missing))
unused_port = re.search(r"(?m)^\s*input\s+(?:\[[^]]+\]\s*)?unused\s*;", top_body)
if requested_top != "write_count_seed" and unused_port is None:
    raise SystemExit(f"generated {requested_top} top does not expose the unused pointer sentinel input")
if "MinimalAXI4MasterPipelinedWrite" not in rtl or "module MinimalAXI4MasterPipelinedWrite" not in lib:
    raise SystemExit("generated RTL/library lacks the selected write-burst component")
normalized = re.sub(r"\s+", "", rtl)
if not all(value in normalized for value in (".B_MAX(16)", ".MAX_OUTSTANDING(1)")):
    raise SystemExit("generated writer parameters are not B_MAX=16, MAX_OUTSTANDING=1")
PY
sha256sum "$RTL" "$LIB" >>"$TASK_OUT/provenance.txt"
printf 'generated_top=%s\n' "$(basename "$RTL" .v)" >>"$TASK_OUT/provenance.txt"

if ! rg -q 'MinimalAXI4MasterPipelinedWrite' "$RTL"; then
    echo "writer hierarchy missing; refusing to simulate fallback/legacy path" >&2; exit 2
fi
TOP=$(sed -nE "s/^module[[:space:]]+($TOP_ARG|kernel)[[:space:]]*\\(.*/\\1/p" "$RTL" | tail -n 1)
[[ -n "$TOP" ]] || { echo "could not identify generated write_count_seed top" >&2; exit 2; }
TB_TOP=tb_generated_write_kernel
TB_DEFINES=("+define+GENERATED_TOP=$TOP")
if [[ "$TOP_ARG" != write_count_seed ]]; then
    TB_DEFINES+=(+define+UNUSED_BUNDLE_CASE)
    if rg -q '^\s*input\s+(\[[^]]+\]\s*)?unused\s*;' "$RTL"; then
        TB_DEFINES+=(+define+GENERATED_UNUSED_PORT)
    fi
fi
if ! verilator --binary -j "$SIM_JOBS" --timing --assert --public-flat-rw -Wno-fatal \
    --top-module "$TB_TOP" --Mdir "$TASK_OUT/verilator_obj" \
    "$RTL" "$LIB" "$TB" "${TB_DEFINES[@]}" >"$TASK_OUT/verilator-build.log" 2>&1; then
    tail -n 120 "$TASK_OUT/verilator-build.log" >&2
    echo "Verilator build failed; see $TASK_OUT/verilator-build.log" >&2
    exit 1
fi
timeout 180s "$TASK_OUT/verilator_obj/V$TB_TOP" | tee "$TASK_OUT/simulation.log"
rg -q '^GENERATED_WRITE_KERNEL_G2_PASS ' "$TASK_OUT/simulation.log" || {
    echo "missing G2 pass marker" >&2; exit 1;
}
echo "GENERATED_WRITE_KERNEL_G2_PASS: $TASK_OUT"
