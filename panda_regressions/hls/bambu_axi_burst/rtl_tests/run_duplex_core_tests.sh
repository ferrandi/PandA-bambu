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
    printf 'peer_fault_scope=peer-fault-core-prerequisite-not-production-wrapper-G1\n'
    printf 'profiles=PEER_FAULT_TEST:0(base scenarios),1(peer-fault scenarios); '
    printf 'FAST_STORE_ACK:0(legacy acknowledgement),1(always-on acknowledgement)\n'
} > "$TASK_OUT/provenance.txt"

compile_profile() {
    local peer_fault_test=$1
    local fast_store_ack=$2
    local profile="pf${peer_fault_test}-fast${fast_store_ack}"
    local build="$TASK_OUT/build/profile-$profile/obj"
    mkdir -p "$(dirname -- "$build")"
    timeout 240s verilator --binary -j "$SIM_JOBS" --timing --assert -Wall -Wno-fatal \
        -Wno-BLKSEQ --top-module tb_duplex_cores \
        -GREAD_B_MAX=16 -GREAD_OUTSTANDING=3 -GREAD_FIFO_DEPTH=64 \
        -GWRITE_B_MAX=8 -GWRITE_OUTSTANDING=2 -GWRITE_FIFO_DEPTH=32 \
        -GPEER_FAULT_TEST="$peer_fault_test" -GFAST_STORE_ACK="$fast_store_ack" \
        --Mdir "$build" "$TASK_OUT/MinimalAXI4MasterPipelined.sv" \
        "$TASK_OUT/MinimalAXI4MasterPipelinedWrite.sv" "$TB" \
        > "$TASK_OUT/build/profile-$profile/compile.log" 2>&1 || {
            tail -n 120 "$TASK_OUT/build/profile-$profile/compile.log" >&2
            echo "duplex-core RTL compile failed for $profile" >&2
            exit 1
        }
    printf '%s\n' "profile_$profile=$build/Vtb_duplex_cores" >> "$TASK_OUT/provenance.txt"
}

run_profile_cases() {
    local peer_fault_test=$1
    local fast_store_ack=$2
    local profile="pf${peer_fault_test}-fast${fast_store_ack}"
    shift 2
    local build="$TASK_OUT/build/profile-$profile/obj"
    local case_name log
    for case_name in "$@"; do
        log="$TASK_OUT/${case_name}-${profile}.log"
        timeout 90s "$build/Vtb_duplex_cores" "+CASE=$case_name" > "$log" 2>&1 || {
            cat "$log" >&2
            echo "duplex-core scenario failed: $case_name (profile=$profile)" >&2
            exit 1
        }
        if ! rg -q "^DUPLEX_CORES PASS CASE=$case_name " "$log"; then
            cat "$log" >&2
            echo "missing exact PASS marker for $case_name" >&2
            exit 1
        fi
        cat "$log" >> "$TASK_OUT/results.log"
    done
}

BASE_CASES=(LOAD_WAIT_STORE LOAD_DURING_WRITE_BLOCK AW_W_B_BLOCK_AR_PROGRESS \
            AR_R_BLOCK_WRITE_PROGRESS SIMULTANEOUS PROFILE_AND_CONFIG_ISOLATION \
            ZERO_READ_NONZERO_WRITE ZERO_WRITE_NONZERO_READ INPLACE_POINTWISE \
            RRESP_FAULT_LOCAL BRESP_FAULT_LOCAL)
PEER_FAULT_CASES=(PEER_BRESP_DRAIN PEER_RRESP_RESERVED_WRITE_DRAIN \
                  PEER_RRESP_FINAL_BEAT PEER_FAULT_BLOCKS_RESERVATION \
                  PEER_FAULT_RESET_RESTART)
: > "$TASK_OUT/results.log"
compile_profile 0 1
run_profile_cases 0 1 "${BASE_CASES[@]}"
compile_profile 1 1
run_profile_cases 1 1 "${PEER_FAULT_CASES[@]}"
compile_profile 0 0
run_profile_cases 0 0 "${BASE_CASES[@]}"
compile_profile 1 0
run_profile_cases 1 0 "${PEER_FAULT_CASES[@]}"
printf 'DUPLEX_CORE_PROFILES_QUALIFIED: FAST_STORE_ACK=0 and 1, PEER_FAULT_TEST=0 and 1\n' >> "$TASK_OUT/results.log"
echo "DUPLEX_CORE_PRELIMINARY_PASS: $TASK_OUT"
