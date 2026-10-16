#!/usr/bin/env bash
set -Eeuo pipefail
ulimit -c 0

SCRIPT_DIR=$(cd -- "$(dirname -- "$0")" && pwd -P)
ROOT=$(realpath "$SCRIPT_DIR/../../../../")
EXTRACTOR="$SCRIPT_DIR/extract_write_engine.py"
TB="$SCRIPT_DIR/tb_duplex_cores.sv"
XML="$ROOT/etc/libtech/NC_AXI_IPs.xml"

if (($# != 1)); then
    echo "usage: $0 fresh-output-directory" >&2
    exit 2
fi
if [[ ${BURST_DUPLEX_SIM_OUTSIDE_SANDBOX:-} != 1 ]]; then
    echo "refusing RTL simulation unless BURST_DUPLEX_SIM_OUTSIDE_SANDBOX=1 is set" >&2
    exit 2
fi
command -v verilator >/dev/null 2>&1 || { echo "verilator is required" >&2; exit 2; }
command -v rg >/dev/null 2>&1 || { echo "rg is required" >&2; exit 2; }
DEFAULT_SIM_JOBS=$(nproc 2>/dev/null || echo 4)
((DEFAULT_SIM_JOBS > 16)) && DEFAULT_SIM_JOBS=16
SIM_JOBS=${SIM_JOBS:-$DEFAULT_SIM_JOBS}
[[ $SIM_JOBS =~ ^[1-9][0-9]*$ ]] || { echo "SIM_JOBS must be a positive integer" >&2; exit 2; }

OUT_ARG=$1
if [[ -e "$OUT_ARG" || -L "$OUT_ARG" ]]; then
    echo "output directory must be fresh and must not be a symlink: $OUT_ARG" >&2
    exit 2
fi
OUT_PARENT=$(realpath -m -- "$(dirname -- "$OUT_ARG")")
TASK_OUT="$OUT_PARENT/$(basename -- "$OUT_ARG")"
[[ ! -e "$TASK_OUT" && ! -L "$TASK_OUT" ]] || { echo "output directory resolves to an existing path" >&2; exit 2; }
mkdir -p "$OUT_PARENT"
mkdir "$TASK_OUT"
mkdir "$TASK_OUT/build"

python3 -B "$EXTRACTOR" --xml "$XML" --target MinimalAXI4MasterPipelined \
    --out "$TASK_OUT/MinimalAXI4MasterPipelined.sv" | tee "$TASK_OUT/extract-reader.log"
python3 -B "$EXTRACTOR" --xml "$XML" --target MinimalAXI4MasterPipelinedWrite \
    --out "$TASK_OUT/MinimalAXI4MasterPipelinedWrite.sv" | tee "$TASK_OUT/extract-writer.log"
{
    printf 'git_head='; git -C "$ROOT" rev-parse HEAD
    printf 'xml='; sha256sum "$XML"
    printf 'extractor='; sha256sum "$EXTRACTOR"
    printf 'testbench='; sha256sum "$TB"
    printf 'runner='; sha256sum "$SCRIPT_DIR/run_duplex_core_tests.sh"
    printf 'verilator='; verilator --version
    printf 'sim_jobs=%s\n' "$SIM_JOBS"
    printf 'scope=preliminary-two-core-shared-memory-not-production-wrapper\n'
} > "$TASK_OUT/provenance.txt"

BUILD="$TASK_OUT/build/obj"
timeout 240s verilator --binary -j "$SIM_JOBS" --timing --assert -Wall -Wno-fatal \
    -Wno-BLKSEQ --top-module tb_duplex_cores \
    -GREAD_B_MAX=16 -GREAD_OUTSTANDING=3 -GREAD_FIFO_DEPTH=64 \
    -GWRITE_B_MAX=8 -GWRITE_OUTSTANDING=2 -GWRITE_FIFO_DEPTH=32 \
    --Mdir "$BUILD" "$TASK_OUT/MinimalAXI4MasterPipelined.sv" \
    "$TASK_OUT/MinimalAXI4MasterPipelinedWrite.sv" "$TB" > "$TASK_OUT/build/compile.log" 2>&1 || {
        tail -n 120 "$TASK_OUT/build/compile.log" >&2
        echo "duplex-core RTL compile failed" >&2
        exit 1
    }

CASES=(LOAD_WAIT_STORE LOAD_DURING_WRITE_BLOCK AW_W_B_BLOCK_AR_PROGRESS \
       AR_R_BLOCK_WRITE_PROGRESS SIMULTANEOUS PROFILE_AND_CONFIG_ISOLATION \
       ZERO_READ_NONZERO_WRITE ZERO_WRITE_NONZERO_READ INPLACE_POINTWISE \
       RRESP_FAULT_LOCAL BRESP_FAULT_LOCAL)
: > "$TASK_OUT/results.log"
for case_name in "${CASES[@]}"; do
    log="$TASK_OUT/$case_name.log"
    timeout 90s "$BUILD/Vtb_duplex_cores" "+CASE=$case_name" > "$log" 2>&1 || {
        cat "$log" >&2
        echo "duplex-core scenario failed: $case_name" >&2
        exit 1
    }
    if ! rg -q "^DUPLEX_CORES PASS CASE=$case_name " "$log"; then
        cat "$log" >&2
        echo "missing exact PASS marker for $case_name" >&2
        exit 1
    fi
    cat "$log" >> "$TASK_OUT/results.log"
done
echo "DUPLEX_CORE_PRELIMINARY_PASS: $TASK_OUT"
