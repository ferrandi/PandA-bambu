#!/usr/bin/env bash
set -euo pipefail
ulimit -c 0

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd -P)
ROOT=$(realpath "$SCRIPT_DIR/../../../../")
XML="$ROOT/etc/libtech/NC_AXI_IPs.xml"
EXTRACTOR="$SCRIPT_DIR/extract_write_engine.py"
TB="$SCRIPT_DIR/tb_configure_read_pipeline.sv"
if (($# > 1)); then
    echo "usage: $0 [output-directory]" >&2
    exit 2
fi
OUT=${1:-"$PWD/out_configure_read_pipeline"}

DEFAULT_SIM_JOBS=$(nproc 2>/dev/null || echo 4)
SIM_JOBS=${SIM_JOBS:-$DEFAULT_SIM_JOBS}
if [[ ! $SIM_JOBS =~ ^[1-9][0-9]*$ ]]; then
    echo "SIM_JOBS must be a positive integer, got: $SIM_JOBS" >&2
    exit 2
fi
for tool in python3 verilator rg timeout sha256sum git; do
    command -v "$tool" >/dev/null 2>&1 || { echo "missing required tool: $tool" >&2; exit 2; }
done
OUT=$(realpath -m -- "$OUT")
if [[ -e "$OUT" || -L "$OUT" ]]; then
    echo "output directory must not already exist: $OUT" >&2
    exit 2
fi
mkdir -p "$OUT/tmp"
export TMPDIR="$OUT/tmp" TMP="$OUT/tmp" TEMP="$OUT/tmp"

READ_RTL="$OUT/MinimalAXI4MasterPipelined.sv"
python3 -B "$EXTRACTOR" --xml "$XML" --target MinimalAXI4MasterPipelined --out "$READ_RTL"

BUILD="$OUT/build"
mkdir -p "$BUILD"
{
    printf 'git_head=%s\n' "$(git -C "$ROOT" rev-parse HEAD)"
    printf 'xml='; sha256sum "$XML"
    printf 'extractor='; sha256sum "$EXTRACTOR"
    printf 'generated_hdl='; sha256sum "$READ_RTL"
    printf 'testbench='; sha256sum "$TB"
    printf 'runner='; sha256sum "$SCRIPT_DIR/run_configure_read_pipeline.sh"
} > "$OUT/provenance.txt"
cat "$OUT/provenance.txt" > "$OUT/aggregate.log"
timeout 180s verilator --binary -j "$SIM_JOBS" --timing --assert -Wall -Wno-fatal -CFLAGS -O0 \
    -Wno-UNUSEDSIGNAL -Wno-BLKSEQ --top-module tb_configure_read_pipeline \
    --Mdir "$BUILD/obj" "$READ_RTL" "$TB" > "$BUILD/compile.log" 2>&1 || {
        tail -n 100 "$BUILD/compile.log" >&2
        echo "reader configure pipeline testbench compile failed" >&2
        exit 1
    }

for scenario in READ_RESET READ_PEER_FAULT; do
    log="$BUILD/$scenario.log"
    timeout 30s "$BUILD/obj/Vtb_configure_read_pipeline" "+CASE=$scenario" > "$log" 2>&1 || {
        cat "$log" >&2
        echo "reader configure pipeline scenario failed: $scenario" >&2
        exit 1
    }
    rg -q "^READ_CONFIG_PIPELINE PASS CASE=$scenario$" "$log" || {
        cat "$log" >&2
        echo "missing exact PASS marker for $scenario" >&2
        exit 1
    }
    cat "$log"
    cat "$log" >> "$OUT/aggregate.log"
done
echo "READ_CONFIG_PIPELINE PASS cases=2" | tee -a "$OUT/aggregate.log"
