#!/usr/bin/env bash
set -euo pipefail
ulimit -c 0

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd -P)
ROOT=$(realpath "$SCRIPT_DIR/../../../../")
EXTRACTOR="$SCRIPT_DIR/extract_write_engine.py"
TB="$SCRIPT_DIR/tb_write_burst_engine.sv"
XML="$ROOT/etc/libtech/NC_AXI_IPs.xml"

if (($# != 1)); then
    echo "usage: $0 fresh-output-directory-under-documentation/tmp" >&2
    exit 2
fi
if [[ ${BURST_WRITER_SIM_OUTSIDE_SANDBOX:-} != 1 ]]; then
    echo "refusing RTL simulation unless BURST_WRITER_SIM_OUTSIDE_SANDBOX=1 is set" >&2
    exit 2
fi
if ! command -v verilator >/dev/null 2>&1; then
    echo "verilator is required" >&2
    exit 2
fi
OUT_PARENT=$(realpath -m "$(dirname "$1")")
TASK_OUT=$(realpath -m "$1")
TMP_ROOT=$(realpath -m "$ROOT/documentation/tmp")
case "$TASK_OUT/" in
    "$TMP_ROOT"/*/) ;;
    *) echo "output directory must be below $TMP_ROOT" >&2; exit 2 ;;
esac
if [[ "$TASK_OUT" == "$TMP_ROOT" || -e "$TASK_OUT" || -e "$1" || -L "$1" ]]; then
    echo "output directory must be fresh and must not be a symlink: $1" >&2
    exit 2
fi
mkdir -p "$OUT_PARENT"
mkdir "$TASK_OUT"
mkdir "$TASK_OUT/tmp"
export TMPDIR="$TASK_OUT/tmp" TMP="$TASK_OUT/tmp" TEMP="$TASK_OUT/tmp"
cleanup() { rm -rf -- "$TASK_OUT/tmp"; }
trap cleanup EXIT

python3 -B "$EXTRACTOR" --xml "$XML" --out "$TASK_OUT/MinimalAXI4MasterPipelinedWrite.sv" \
    | tee "$TASK_OUT/extract.log"
{
    printf 'xml='; sha256sum "$XML"
    printf 'extractor='; sha256sum "$EXTRACTOR"
    printf 'testbench='; sha256sum "$TB"
    printf 'verilator='; verilator --version
    printf 'git_head='; git -C "$ROOT" rev-parse HEAD
    printf 'outside_sandbox_flag=%s\n' "$BURST_WRITER_SIM_OUTSIDE_SANDBOX"
} > "$TASK_OUT/provenance.txt"

SIM_JOBS=${SIM_JOBS:-4}
if [[ ! $SIM_JOBS =~ ^[1-9][0-9]*$ ]]; then
    echo "SIM_JOBS must be a positive integer" >&2
    exit 2
fi

build_run() {
    local label=$1 burst=$2 depth=$3 addr=$4 test_case=$5
    shift 5
    local build="$TASK_OUT/build-$label"
    mkdir -p "$build"
    timeout 180s verilator --binary -j "$SIM_JOBS" --timing --assert -Wall -Wno-fatal \
        -Wno-UNUSEDSIGNAL -Wno-BLKSEQ --top-module tb_write_burst_engine \
        -GB_MAX="$burst" -GMAX_OUTSTANDING=1 -GFIFO_DEPTH="$depth" -GADDR_BITS="$addr" \
        --Mdir "$build/obj" "$TASK_OUT/MinimalAXI4MasterPipelinedWrite.sv" "$TB" \
        > "$build/compile.log" 2>&1 || {
            tail -n 100 "$build/compile.log" >&2
            echo "RTL compile failed for $label" >&2
            return 1
        }
    local binary="$build/obj/Vtb_write_burst_engine"
    local case_name log
    for case_name in "$test_case" "$@"; do
        log="$build/$case_name.log"
        timeout 60s "$binary" "+CASE=$case_name" > "$log" 2>&1 || {
            cat "$log" >&2
            echo "RTL scenario failed: $label/$case_name" >&2
            return 1
        }
        if ! rg -q "^WRITE_BURST_ENGINE PASS CASE=$case_name " "$log" &&
           ! rg -q "^WRITE_BURST_ENGINE PARAM PASS " "$log"; then
            cat "$log" >&2
            echo "missing exact PASS marker for $label/$case_name" >&2
            return 1
        fi
        cat "$log" >> "$TASK_OUT/results.log"
    done
}

build_run_o() {
    local label=$1 burst=$2 outstanding=$3 depth=$4 addr=$5 test_case=$6
    shift 6
    local build="$TASK_OUT/build-$label"
    mkdir -p "$build"
    timeout 180s verilator --binary -j "$SIM_JOBS" --timing --assert -Wall -Wno-fatal \
        -Wno-UNUSEDSIGNAL -Wno-BLKSEQ --top-module tb_write_burst_engine \
        -GB_MAX="$burst" -GMAX_OUTSTANDING="$outstanding" -GFIFO_DEPTH="$depth" -GADDR_BITS="$addr" \
        --Mdir "$build/obj" "$TASK_OUT/MinimalAXI4MasterPipelinedWrite.sv" "$TB" \
        > "$build/compile.log" 2>&1 || {
            tail -n 100 "$build/compile.log" >&2
            echo "RTL compile failed for $label" >&2
            return 1
        }
    local binary="$build/obj/Vtb_write_burst_engine"
    local case_name log
    for case_name in "$test_case" "$@"; do
        log="$build/$case_name.log"
        timeout 60s "$binary" "+CASE=$case_name" > "$log" 2>&1 || {
            cat "$log" >&2
            echo "RTL scenario failed: $label/$case_name" >&2
            return 1
        }
        if ! rg -q "^WRITE_BURST_ENGINE PASS CASE=$case_name " "$log"; then
            cat "$log" >&2
            echo "missing exact PASS marker for $label/$case_name" >&2
            return 1
        fi
        cat "$log" >> "$TASK_OUT/results.log"
    done
}

build_async_reset_run() {
    local polarity=$1 active_low=$2
    local label="ASYNC_RESET_${polarity}"
    local source="$TASK_OUT/MinimalAXI4MasterPipelinedWrite-$label.sv"
    local build="$TASK_OUT/build-$label"
    mkdir -p "$build"
    python3 -B "$EXTRACTOR" --xml "$XML" --out "$source" \
        --reset-mode async --reset-polarity "$polarity"
    {
        printf 'variant=%s reset_mode=async reset_polarity=%s\n' "$label" "$polarity"
        printf 'generated_hdl='; sha256sum "$source"
        printf 'xml='; sha256sum "$XML"
        printf 'extractor='; sha256sum "$EXTRACTOR"
        printf 'testbench='; sha256sum "$TB"
    } > "$build/provenance.txt"
    timeout 180s verilator --binary -j "$SIM_JOBS" --timing --assert -Wall -Wno-fatal \
        -Wno-UNUSEDSIGNAL -Wno-BLKSEQ --top-module tb_write_burst_engine \
        -GB_MAX=16 -GMAX_OUTSTANDING=2 -GFIFO_DEPTH=32 -GADDR_BITS=32 \
        -GASYNC_RESET=1 -GRESET_ACTIVE_LOW="$active_low" \
        --Mdir "$build/obj" "$source" "$TB" > "$build/compile.log" 2>&1 || {
            tail -n 100 "$build/compile.log" >&2
            echo "RTL compile failed for $label" >&2
            return 1
        }
    local log="$build/ASYNC_RESET.log"
    timeout 60s "$build/obj/Vtb_write_burst_engine" +CASE=ASYNC_RESET > "$log" 2>&1 || {
        cat "$log" >&2
        echo "RTL scenario failed: $label/ASYNC_RESET" >&2
        return 1
    }
    if ! rg -q '^WRITE_BURST_ENGINE PASS CASE=ASYNC_RESET ' "$log"; then
        cat "$log" >&2
        echo "missing exact PASS marker for $label/ASYNC_RESET" >&2
        return 1
    fi
    cat "$log" >> "$TASK_OUT/results.log"
    printf 'ASYNC_RESET_VARIANT_PASS %s polarity=%s\n' "$label" "$polarity" >> "$TASK_OUT/results.log"
}

expect_parameter_fatal() {
    local label=$1 burst=$2 outstanding=$3 depth=$4 expected=$5
    local build="$TASK_OUT/negative-$label"
    mkdir -p "$build"
    timeout 180s verilator --binary -j "$SIM_JOBS" --timing --assert -Wno-fatal \
        --top-module tb_write_burst_engine -GB_MAX="$burst" -GMAX_OUTSTANDING="$outstanding" \
        -GFIFO_DEPTH="$depth" --Mdir "$build/obj" \
        "$TASK_OUT/MinimalAXI4MasterPipelinedWrite.sv" "$TB" > "$build/compile.log" 2>&1 || {
            tail -n 80 "$build/compile.log" >&2
            echo "negative parameter fixture compile failed: $label" >&2
            return 1
        }
    local status=0
    timeout 20s "$build/obj/Vtb_write_burst_engine" +CASE=PARAM > "$build/run.log" 2>&1 || status=$?
    if (( status == 0 )) || ! rg -Fq "$expected" "$build/run.log"; then
        cat "$build/run.log" >&2
        echo "parameter rejection witness failed: $label" >&2
        return 1
    fi
    echo "EXPECTED_FATAL $label: $expected" >> "$TASK_OUT/results.log"
}

expect_mutation_failure() {
    local label=$1 source_pattern=$2 replacement=$3 burst=$4 outstanding=$5 depth=$6
    local addr=$7 test_case=$8 expected=$9
    local mutant="$TASK_OUT/mutant-$label.sv"
    local build="$TASK_OUT/mutation-$label"
    python3 - "$TASK_OUT/MinimalAXI4MasterPipelinedWrite.sv" "$mutant" \
        "$source_pattern" "$replacement" <<'PY'
from pathlib import Path
import sys

source, target, needle, replacement = sys.argv[1:]
text = Path(source).read_text()
if text.count(needle) != 1:
    raise SystemExit(f"expected exactly one mutation target, found {text.count(needle)}")
Path(target).write_text(text.replace(needle, replacement))
PY
    mkdir -p "$build"
    timeout 180s verilator --binary -j "$SIM_JOBS" --timing --assert -Wno-fatal \
        --top-module tb_write_burst_engine -GB_MAX="$burst" -GMAX_OUTSTANDING="$outstanding" \
        -GFIFO_DEPTH="$depth" -GADDR_BITS="$addr" \
        --Mdir "$build/obj" "$mutant" "$TB" > "$build/compile.log" 2>&1 || {
            tail -n 80 "$build/compile.log" >&2
            echo "mutation fixture compile failed: $label" >&2
            return 1
        }
    local status=0
    timeout 60s "$build/obj/Vtb_write_burst_engine" "+CASE=$test_case" > "$build/run.log" 2>&1 || status=$?
    if (( status == 0 || status == 124 || status == 137 )) || ! rg -Fq "$expected" "$build/run.log"; then
        cat "$build/run.log" >&2
        echo "mutation witness failed: $label" >&2
        return 1
    fi
    echo "MUTATION_REJECTED $label: $expected" >> "$TASK_OUT/results.log"
}

: > "$TASK_OUT/results.log"
build_run B16-D16 16 16 32 CORE W_BEFORE_AW AW_BEFORE_W STALLS \
    CFG_SIZE_CHANGE CFG_COUNT_CHANGE \
    BAD_CONFIG_SIZE BAD_CONFIG_ALIGN COUNT_MSB OVERFLOW32 BAD_STORE_SIZE \
    BAD_STORE_SIZE8 BAD_STORE_ADDRESS EARLY_DRAIN BID BRESP MAX_COUNT \
    PENDING_STORE PENDING_OPERAND_CHANGE PENDING_OPERAND_CHANGE_ON_FREE \
    TRANSIENT_DRAIN_RESTORE TRANSIENT_CONFIGURE_PENDING \
    DRAIN_PENDING_FULL_COUNT DRAIN_PENDING_SHORT_COUNT STORE_ACK_PULSE DUPLICATE_START \
    DRAIN_BEFORE_B DRAIN_ONE_SHOT PREMATURE_CONFIGURE INIT_DRAIN
build_run B16-D32 16 32 32 CORE BRESP_NO_NEXT_DESCRIPTOR
build_run B1-D1 1 1 32 CORE
build_run B3-D4 3 4 32 CORE
build_run B16-D4096 16 4096 32 CORE
build_run_o O2-D16 16 2 16 32 MULTI_OUTSTANDING
build_run_o O2-D32 16 2 32 32 INDEPENDENT_CURSORS BRESP_MULTI
build_run_o O3-D16 16 3 16 32 MULTI_OUTSTANDING
build_run_o O16-D16 16 16 16 32 MULTI_OUTSTANDING
build_run B16-A64 16 32 64 OVERFLOW64 MAX_COUNT CFG_BASE_CHANGE COUNT_MAX_64
build_run B16-A64-MSB 16 32 64 COUNT_MSB
build_run B16-A32-F3 16 32 32 MULTIPAGE PREMATURE_CONFIGURE_B_PENDING
build_run_o O2-D32-F3 16 2 32 32 BRESP_AW_W_HELD BRESP_W_BEFORE_AW
build_run B256-D256 256 256 32 PARAM
build_async_reset_run active-low 1
build_async_reset_run active-high 0
expect_parameter_fatal B0 0 1 1 "B_MAX must be in [1,256]"
expect_parameter_fatal B257 257 1 512 "B_MAX must be in [1,256]"
expect_parameter_fatal O0 16 0 16 "MAX_OUTSTANDING must be in [1,16]"
expect_parameter_fatal O17 16 17 32 "MAX_OUTSTANDING must be in [1,16]"
expect_parameter_fatal FIFO_BELOW_B 16 1 8 "FIFO_DEPTH must be a power of two >= B_MAX and <= 4096"
expect_parameter_fatal FIFO_NONPOWER 3 1 3 "FIFO_DEPTH must be a power of two >= B_MAX and <= 4096"
expect_mutation_failure data-order "assign m_axi_wdata = fifo_head;" \
    "assign m_axi_wdata = ~fifo_head;" 16 1 32 32 CORE "WRITE_DATA_MISMATCH"
expect_mutation_failure last-order "assign m_axi_wlast = w_valid && (descriptor_w_remaining[descriptor_w_ptr] == 1);" \
    "assign m_axi_wlast = w_valid && (descriptor_w_remaining[descriptor_w_ptr] != 1);" \
    16 1 32 32 CORE "WLAST_MISMATCH"
expect_mutation_failure serialize-until-b "(descriptor_count >= MAX_OUTSTANDING) || fault_reg" \
    "((descriptor_count >= MAX_OUTSTANDING) || (descriptor_count != 0)) || fault_reg" \
    16 2 16 32 TWO_AW_BEFORE_B "F2_TWO_AW_BEFORE_FIRST_B_REQUIRED"
expect_mutation_failure count-msb-guard "(in5[COUNT_BITS-1] == 1'b0) &&" \
    "" 16 1 32 64 COUNT_MSB "F3_COUNT_MSB_GUARD_INSTALLED_CONTEXT_OR_TRAFFIC"
expect_mutation_failure configure-size "in2 != cfg_size" \
    "1'b0" 16 1 32 32 CFG_SIZE_CHANGE "CONFIGURE_CHANGE_NOT_REJECTED operand=0"
expect_mutation_failure configure-base-low32 "in4 != cfg_base" \
    "in4[31:0] != cfg_base[31:0]" 16 1 32 64 CFG_BASE_CHANGE "CONFIGURE_CHANGE_NOT_REJECTED operand=1"
expect_mutation_failure configure-count-width "in5 != cfg_count" \
    "in5[COUNT_W-1:0] != cfg_count[COUNT_W-1:0]" \
    16 1 32 32 CFG_COUNT_CHANGE "CONFIGURE_CHANGE_NOT_REJECTED operand=2"

echo "WRITE_BURST_ENGINE_QUALIFY_PASS: $TASK_OUT"
