#!/usr/bin/env bash
# Guard for the AXI SIZE field of the minimal adapters (F-review P1).
#
# ARSIZE/AWSIZE of MinimalAXI4Adapter and MinimalAXI4AdapterSingleBeat is computed in two branches:
# $clog2 when _SIM_HAVE_CLOG2 is defined, the Verilog log2 function otherwise. PandA emits the
# macro prologue only in generated designs, and synthesis never sees it, so the second branch is
# the synthesis path: a backtick in front of log2 there invokes a nonexistent macro and silently
# turns SIZE into the byte count (4 instead of 2 for a 32-bit port). This guard therefore
#   1. statically rejects any backtick-log2 invocation in the component XML, and
#   2. simulates the extracted adapters with and without _SIM_HAVE_CLOG2 and requires the same
#      SIZE values from both branches (0/2/3 for 8/32/64-bit ports), on accepted
#      and stalled AXI address transactions for both adapter implementations.
#   3. disconnects the AXI SIZE outputs in a negative-control bench and requires
#      the active-transaction checks to fail while size_next remains unchanged.
#
# Run outside the file sandbox:
#   BURST_ADAPTER_SIM_OUTSIDE_SANDBOX=1 ./run_adapter_size_guard.sh <fresh-output-directory>
set -euo pipefail
ulimit -c 0

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd -P)
ROOT=$(realpath "$SCRIPT_DIR/../../../../")
EXTRACTOR="$SCRIPT_DIR/extract_write_engine.py"
TB="$SCRIPT_DIR/tb_adapter_size.sv"
XML="$ROOT/etc/libtech/NC_AXI_IPs.xml"
source "$SCRIPT_DIR/modelsim_helpers.sh"

if (($# != 1)); then
    echo "usage: $0 <fresh-output-directory>" >&2
    exit 2
fi
if [[ ${BURST_ADAPTER_SIM_OUTSIDE_SANDBOX:-} != 1 ]]; then
    echo "refusing RTL simulation unless BURST_ADAPTER_SIM_OUTSIDE_SANDBOX=1 is set" >&2
    exit 2
fi
modelsim_resolve

OUT_ARG=$1
OUT_PARENT=$(realpath -m -- "$(dirname "$OUT_ARG")")
TASK_OUT=$(realpath -m -- "$OUT_ARG")
if [[ -e "$TASK_OUT" || -e "$OUT_ARG" || -L "$OUT_ARG" ]]; then
    echo "output directory must be fresh and must not be a symlink: $OUT_ARG" >&2
    exit 2
fi
mkdir -p "$OUT_PARENT"
mkdir "$TASK_OUT"
mkdir "$TASK_OUT/tmp"
export TMPDIR="$TASK_OUT/tmp" TMP="$TASK_OUT/tmp" TEMP="$TASK_OUT/tmp"
trap 'rm -rf -- "$TASK_OUT/tmp"' EXIT

if LC_ALL=C grep -q '`log2(' "$XML"; then
    echo "ADAPTER_SIZE_STATIC_FAIL: the component XML invokes log2 as a macro:" >&2
    LC_ALL=C grep -n '`log2(' "$XML" >&2
    exit 1
fi
echo "ADAPTER_SIZE_STATIC_PASS no backtick log2 invocation in the component XML" > "$TASK_OUT/results.log"

{
    printf 'xml='; sha256sum "$XML"
    printf 'extractor='; sha256sum "$EXTRACTOR"
    printf 'testbench='; sha256sum "$TB"
    printf 'modelsim=%s\n' "$MODELSIM_VSIM"
    printf '%s\n' "$(modelsim_version)"
    printf 'git_head=%s\n' "$(git -C "$ROOT" rev-parse HEAD)"
} > "$TASK_OUT/provenance.txt"

for target in MinimalAXI4Adapter MinimalAXI4AdapterSingleBeat; do
    python3 -B "$EXTRACTOR" --xml "$XML" --target "$target" \
        --out "$TASK_OUT/$target.sv" >> "$TASK_OUT/extract.log" 2>&1 || {
            cat "$TASK_OUT/extract.log" >&2
            echo "extraction failed for $target" >&2
            exit 1
        }
done

for mode in log2_function clog2; do
    build="$TASK_OUT/build-$mode"
    extra=()
    [[ $mode == clog2 ]] && extra+=(+define+_SIM_HAVE_CLOG2)
    if ! modelsim_compile "$build" tb_adapter_size \
        "$TASK_OUT/MinimalAXI4Adapter.sv" "$TASK_OUT/MinimalAXI4AdapterSingleBeat.sv" "$TB" \
        -- "${extra[@]}"; then
        echo "adapter size bench compile failed for mode=$mode" >&2
        exit 1
    fi
    raw="$build/run.raw.log"
    log="$build/run.log"
    if ! modelsim_run "$build" tb_adapter_size "$raw" 60; then
        cat "$raw" >&2
        echo "adapter size bench failed for mode=$mode" >&2
        exit 1
    fi
    modelsim_normalize_log "$raw" "$log"
    if ! grep -q "^ADAPTER_SIZE_PASS .*path=$mode\$" "$log"; then
        cat "$log" >&2
        echo "missing adapter SIZE marker for mode=$mode" >&2
        exit 1
    fi
    grep -E '^ADAPTER_SIZE' "$log" >> "$TASK_OUT/results.log"
    printf 'ADAPTER_SIZE_MODE_PASS mode=%s %s\n' "$mode" \
        "$(grep -m1 '^ADAPTER_SIZE_PASS' "$log" | cut -d' ' -f2-)" >> "$TASK_OUT/results.log"
done

mutant_tb="$TASK_OUT/tb_adapter_size_disconnected.sv"
sed -e 's/\.m_axi_arsize(arsize)/.m_axi_arsize()/g' \
    -e 's/\.m_axi_awsize(awsize)/.m_axi_awsize()/g' "$TB" > "$mutant_tb"
if [[ $(grep -c '\.m_axi_arsize()' "$mutant_tb") -ne 2 ||
      $(grep -c '\.m_axi_awsize()' "$mutant_tb") -ne 2 ]]; then
    echo "failed to create disconnected SIZE negative control" >&2
    exit 1
fi
negative="$TASK_OUT/build-disconnected"
if ! modelsim_compile "$negative" tb_adapter_size \
    "$TASK_OUT/MinimalAXI4Adapter.sv" "$TASK_OUT/MinimalAXI4AdapterSingleBeat.sv" "$mutant_tb"; then
    tail -n 60 "$negative/compile.log" >&2
    echo "disconnected SIZE negative-control compile failed" >&2
    exit 1
fi
raw="$negative/run.raw.log"
log="$negative/run.log"
if modelsim_run "$negative" tb_adapter_size "$raw" 60; then
    modelsim_normalize_log "$raw" "$log"
    cat "$log" >&2
    echo "disconnected SIZE negative control unexpectedly passed" >&2
    exit 1
fi
modelsim_normalize_log "$raw" "$log"
if ! grep -q 'ADAPTER_SIZE_MISMATCH phase=read-valid ARSIZE' "$log" ||
   grep -q 'ADAPTER_SIZE_PASS' "$log"; then
    cat "$log" >&2
    echo "disconnected SIZE negative control failed for an unexpected reason" >&2
    exit 1
fi
echo "ADAPTER_SIZE_NEGATIVE_CONTROL_PASS mutation=disconnect_AXSIZE size_next=unchanged" | tee -a "$TASK_OUT/results.log"
echo "ADAPTER_SIZE_GUARD_PASS modes=log2_function,clog2 widths=8,32,64 adapters=both" | tee -a "$TASK_OUT/results.log"
