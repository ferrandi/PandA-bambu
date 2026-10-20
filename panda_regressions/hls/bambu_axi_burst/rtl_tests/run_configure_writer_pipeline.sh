#!/usr/bin/env bash
set -euo pipefail
ulimit -c 0

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd -P)
ROOT=$(realpath "$SCRIPT_DIR/../../../../")
EXTRACTOR="$SCRIPT_DIR/extract_write_engine.py"
TB="$SCRIPT_DIR/tb_configure_writer_pipeline.sv"
XML="$ROOT/etc/libtech/NC_AXI_IPs.xml"

if (($# > 1)); then
    echo "usage: $0 [fresh-output-directory]" >&2
    exit 2
fi
if [[ ${BURST_WRITER_SIM_OUTSIDE_SANDBOX:-} != 1 ]]; then
    echo "refusing RTL simulation unless BURST_WRITER_SIM_OUTSIDE_SANDBOX=1 is set" >&2
    exit 2
fi
for tool in verilator python3 rg sha256sum; do
    if ! command -v "$tool" >/dev/null 2>&1; then
        echo "$tool is required" >&2
        exit 2
    fi
done
SIM_JOBS=${SIM_JOBS:-4}
if [[ ! $SIM_JOBS =~ ^[1-9][0-9]*$ ]]; then
    echo "SIM_JOBS must be a positive integer" >&2
    exit 2
fi

OUT_ARG=${1:-"$PWD/out_configure_writer_pipeline"}
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
cleanup() { rm -rf -- "$TASK_OUT/tmp"; }
trap cleanup EXIT

RTL="$TASK_OUT/MinimalAXI4MasterPipelinedWrite.sv"
python3 -B "$EXTRACTOR" --xml "$XML" --out "$RTL" > "$TASK_OUT/extract.log"
{
    printf 'git_head='; git -C "$ROOT" rev-parse HEAD
    printf 'xml='; sha256sum "$XML"
    printf 'core_extract='; sha256sum "$RTL"
    printf 'testbench='; sha256sum "$TB"
    printf 'runner='; sha256sum "$SCRIPT_DIR/run_configure_writer_pipeline.sh"
    printf 'extractor='; sha256sum "$EXTRACTOR"
    printf 'verilator='; verilator --version
    printf 'sim_jobs=%s\n' "$SIM_JOBS"
} > "$TASK_OUT/provenance.txt"

: > "$TASK_OUT/aggregate.log"
BUILD="$TASK_OUT/build"
mkdir -p "$BUILD"
timeout 180s verilator --binary -j "$SIM_JOBS" --timing --assert -Wall -Wno-fatal \
    -CFLAGS -O0 -Wno-UNUSEDSIGNAL -Wno-BLKSEQ --top-module tb_configure_writer_pipeline \
    --Mdir "$BUILD/obj" "$RTL" "$TB" > "$BUILD/compile.log" 2>&1 || {
        tail -n 100 "$BUILD/compile.log" >&2
        echo "RTL compile failed for writer configure pipeline" >&2
        exit 1
    }

for scenario in RESET_E1 PEER_FAULT_E1; do
    log="$BUILD/$scenario.log"
    timeout 30s "$BUILD/obj/Vtb_configure_writer_pipeline" "+CASE=$scenario" > "$log" 2>&1 || {
        cat "$log" >&2
        echo "RTL scenario failed: $scenario" >&2
        exit 1
    }
    if ! rg -q "^CONFIGURE_WRITER_PIPELINE PASS CASE=$scenario$" "$log"; then
        cat "$log" >&2
        echo "missing exact PASS marker for $scenario" >&2
        exit 1
    fi
    cat "$log" >> "$TASK_OUT/aggregate.log"
    cat "$log"
done

printf 'CONFIGURE_WRITER_PIPELINE_SUITE_PASS cases=2\n' | tee -a "$TASK_OUT/aggregate.log"
