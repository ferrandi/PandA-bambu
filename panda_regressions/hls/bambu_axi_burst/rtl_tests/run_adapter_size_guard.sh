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
#      SIZE values from both branches (2 for a 32-bit port, 3 for a 64-bit port).
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

if (($# != 1)); then
    echo "usage: $0 <fresh-output-directory>" >&2
    exit 2
fi
if [[ ${BURST_ADAPTER_SIM_OUTSIDE_SANDBOX:-} != 1 ]]; then
    echo "refusing RTL simulation unless BURST_ADAPTER_SIM_OUTSIDE_SANDBOX=1 is set" >&2
    exit 2
fi
if ! command -v verilator >/dev/null 2>&1; then
    echo "verilator is required" >&2
    exit 2
fi
SIM_JOBS=${SIM_JOBS:-2}

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
    printf 'verilator='; verilator --version
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
    mkdir -p "$build/obj"
    extra=()
    [[ $mode == clog2 ]] && extra+=(-D_SIM_HAVE_CLOG2)
    if ! timeout 300s verilator --binary -j "$SIM_JOBS" --timing --assert -Wall -Wno-fatal \
        -Wno-UNUSEDSIGNAL -Wno-UNDRIVEN -Wno-PINMISSING -CFLAGS -O0 \
        --top-module tb_adapter_size "${extra[@]}" --Mdir "$build/obj" \
        "$TASK_OUT/MinimalAXI4Adapter.sv" "$TASK_OUT/MinimalAXI4AdapterSingleBeat.sv" "$TB" \
        > "$build/compile.log" 2>&1; then
        tail -n 60 "$build/compile.log" >&2
        echo "adapter size bench compile failed for mode=$mode" >&2
        exit 1
    fi
    log="$build/run.log"
    if ! timeout 60s "$build/obj/Vtb_adapter_size" > "$log" 2>&1; then
        cat "$log" >&2
        echo "adapter size bench failed for mode=$mode" >&2
        exit 1
    fi
    if ! grep -q "^ADAPTER_SIZE_PASS .*path=$mode\$" "$log"; then
        cat "$log" >&2
        echo "missing adapter SIZE marker for mode=$mode" >&2
        exit 1
    fi
    if grep -q '^ADAPTER_SIZE_MISMATCH' "$log"; then
        cat "$log" >&2
        echo "adapter SIZE mismatch for mode=$mode" >&2
        exit 1
    fi
    grep -E '^ADAPTER_SIZE' "$log" >> "$TASK_OUT/results.log"
    printf 'ADAPTER_SIZE_MODE_PASS mode=%s %s\n' "$mode" \
        "$(grep -m1 '^ADAPTER_SIZE_PASS' "$log" | cut -d' ' -f2-)" >> "$TASK_OUT/results.log"
done

grep -E "ADAPTER_SIZE_BUS_REG" "$TASK_OUT/results.log" || true
echo "ADAPTER_SIZE_GUARD_PASS modes=log2_function,clog2 size32=2 size64=3" | tee -a "$TASK_OUT/results.log"
