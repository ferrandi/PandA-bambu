#!/usr/bin/env bash
set -Eeuo pipefail

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd -P)
ROOT=$(realpath "$SCRIPT_DIR/../../../../")
T4B_TESTS="$ROOT/panda_regressions/hls/bambu_axi_burst/t4b_tests"

usage() {
    echo "usage: $0 characterize|qualify [fresh-output-directory]" >&2
    echo "  default: ./out_memaxi_model_<mode>, created in the directory the script is launched from" >&2
    exit 2
}

[[ $# == 1 || $# == 2 ]] || usage
MODE=$1
[[ $MODE == characterize || $MODE == qualify ]] || usage
OUT_ARG=${2:-out_memaxi_model_$MODE}
[[ ! -L $OUT_ARG ]] || { echo "refusing symlink output path: $OUT_ARG" >&2; exit 2; }
OUT=$(realpath -m -- "$OUT_ARG")
[[ ! -e $OUT ]] || { echo "refusing to reuse output directory: $OUT" >&2; exit 2; }
[[ ${BURST_MODELSIM_OUTSIDE_SANDBOX:-0} == 1 ]] || {
    echo "refusing ModelSim execution without BURST_MODELSIM_OUTSIDE_SANDBOX=1; invoke this command outside the sandbox" >&2
    exit 2
}
mkdir "$OUT"
mkdir -p "$OUT/tmp" "$OUT/hdl" "$OUT/logs"
export TMPDIR="$OUT/tmp" TMP="$OUT/tmp" TEMP="$OUT/tmp"
python3 "$SCRIPT_DIR/extract_testbench_memaxi.py" \
    --xml "$ROOT/etc/libtech/NC_TESTBENCH_IPs.xml" \
    --out "$OUT/hdl/TestbenchMEMAXI.sv" \
    --t4b-extractor "$T4B_TESTS/extract_t4b_cells.py" >"$OUT/logs/extract.log"
cp "$SCRIPT_DIR/tb_testbench_memaxi_backend.sv" "$OUT/hdl/tb_testbench_memaxi_backend.sv"
cp "$SCRIPT_DIR/tb_testbench_memaxi_model.sv" "$OUT/hdl/tb_testbench_memaxi_model.sv"

MODELSIM_ROOT=${MODELSIM_ROOT:-/opt/mentor}
VSIM="$MODELSIM_ROOT/questasim/bin/vsim"
VLOG="$MODELSIM_ROOT/questasim/bin/vlog"
VLIB="$MODELSIM_ROOT/questasim/bin/vlib"
VMAP="$MODELSIM_ROOT/questasim/bin/vmap"
for tool in "$VSIM" "$VLOG" "$VLIB" "$VMAP"; do
    [[ -x $tool ]] || { echo "ModelSim tool unavailable: $tool" >&2; exit 2; }
done

{
    printf 'mode=%s\nvsim=%s\nvlog=%s\n' "$MODE" "$VSIM" "$VLOG"
    "$VSIM" -version 2>&1 | head -n 4
    sha256sum "$VSIM" "$VLOG" "$VLIB" "$VMAP" \
        "$ROOT/etc/libtech/NC_TESTBENCH_IPs.xml" \
        "$T4B_TESTS/extract_t4b_cells.py" \
        "$SCRIPT_DIR/extract_testbench_memaxi.py" \
        "$SCRIPT_DIR/tb_testbench_memaxi_backend.sv" \
        "$SCRIPT_DIR/tb_testbench_memaxi_model.sv"
} >"$OUT/modelsim_provenance.txt"

cd "$OUT"
"$VLIB" work >"$OUT/logs/vlib.log" 2>&1
"$VMAP" work "$OUT/work" >"$OUT/logs/vmap.log" 2>&1
"$VLOG" -sv \
    "$OUT/hdl/TestbenchMEMAXI.sv" \
    "$OUT/hdl/tb_testbench_memaxi_backend.sv" \
    "$OUT/hdl/tb_testbench_memaxi_model.sv" >"$OUT/logs/vlog.log" 2>&1 || {
        tail -n 120 "$OUT/logs/vlog.log" >&2
        exit 1
    }
if [[ $MODE == characterize ]]; then
    "$VSIM" -c work.tb_testbench_memaxi_model "+MODE=$MODE" -do 'run -all; quit -f' \
        >"$OUT/logs/$MODE.log" 2>&1 || {
            tail -n 120 "$OUT/logs/$MODE.log" >&2
            exit 1
        }
else
    for delay in 1 63; do
        "$VSIM" -c work.tb_testbench_memaxi_model -gWRITE_DELAY="$delay" "+MODE=$MODE" \
            -do 'run -all; quit -f' >"$OUT/logs/$MODE-delay-$delay.log" 2>&1 || {
                tail -n 120 "$OUT/logs/$MODE-delay-$delay.log" >&2
                exit 1
            }
    done
    if [[ $MODE == qualify ]]; then
        for delay in 1 63; do
            for test_case in early_wlast missing_final_wlast read_wrap read_reserved write_wrap write_reserved; do
                LOG="$OUT/logs/negative-$test_case-delay-$delay.log"
                status=0
                "$VSIM" -c work.tb_testbench_memaxi_model -gWRITE_DELAY="$delay" "+MODE=$MODE" "+NEGATIVE=$test_case" \
                    -do 'run -all; quit -f' >"$LOG" 2>&1 || status=$?
                printf '%s\n' "$status" >"$LOG.status"
                rg -Fq "F0_NEGATIVE_START case=$test_case delay=$delay" "$LOG" || {
                    echo "negative case did not start: case=$test_case delay=$delay (vsim exit=$status)" >&2
                    tail -n 80 "$LOG" >&2
                    exit 1
                }
                case $test_case in
                    early_wlast) diagnostic='AXI WLAST mismatch at beat 0 of 2' ;;
                    missing_final_wlast) diagnostic='AXI WLAST mismatch at beat 1 of 2' ;;
                    read_wrap) diagnostic='Unsupported AXI read burst type: 2' ;;
                    read_reserved) diagnostic='Unsupported AXI read burst type: 3' ;;
                    write_wrap) diagnostic='Unsupported AXI write burst type: 2' ;;
                    write_reserved) diagnostic='Unsupported AXI write burst type: 3' ;;
                esac
                rg -q "^#? *\\*\\* Fatal: $diagnostic$" "$LOG" || {
                    echo "negative case missed exact Fatal diagnostic: case=$test_case delay=$delay expected='$diagnostic' (vsim exit=$status)" >&2
                    tail -n 80 "$LOG" >&2
                    exit 1
                }
                fatal_count=$(rg -c '^#? *\*\* Fatal:' "$LOG" || true)
                if [[ $fatal_count != 1 ]] || rg -q '^#? *\*\* Error:' "$LOG"; then
                    echo "negative case emitted additional simulator errors: case=$test_case delay=$delay fatals=$fatal_count (vsim exit=$status)" >&2
                    tail -n 80 "$LOG" >&2
                    exit 1
                fi
                rg -q 'memory(/|\.)independent_axi_model' "$LOG" &&
                    rg -q 'File: .*/TestbenchMEMAXI\.sv' "$LOG" || {
                    echo "negative case Fatal did not identify the generated model process/file: case=$test_case delay=$delay (vsim exit=$status)" >&2
                    tail -n 80 "$LOG" >&2
                    exit 1
                }
                if rg -Fq 'F0 fixture watchdog expired' "$LOG" ||
                    rg -Fq 'F0_NEGATIVE_DID_NOT_ABORT' "$LOG" ||
                    rg -Fq 'F0_QUALIFY_PASS' "$LOG"; then
                    echo "negative case ended through an invalid path: case=$test_case delay=$delay (vsim exit=$status)" >&2
                    tail -n 80 "$LOG" >&2
                    exit 1
                fi
                echo "PASS negative case=$test_case delay=$delay diagnostic='$diagnostic' vsim_exit=$status"
            done
        done
    fi
fi

if [[ $MODE == characterize ]]; then
    for marker in R_STALL_PAYLOAD_CHANGED R_BACKEND_READ_DURING_STALL R_BEAT_HANDSHAKE_COUNT W_COMMIT_DUPLICATION_WHILE_B_STALLED W_COMMIT_COUNT W_BEFORE_AW_ASSOCIATION; do
        rg -q "F0_EXPECTED_VIOLATION $marker" "$OUT/logs/$MODE.log" || {
            echo "missing required characterization witness '$marker'" >&2
            tail -n 120 "$OUT/logs/$MODE.log" >&2
            exit 1
        }
    done
    for marker in 'F0_OBSERVED AR_NEXT_ACCEPTED_DURING_FINAL_STALL' 'F0_OBSERVED AW_W_DISTINCT_HANDSHAKES' 'F0_CHARACTERIZATION_EXPECTED_DEFECTS'; do
        rg -q "$marker" "$OUT/logs/$MODE.log" || {
            echo "missing required characterization observation '$marker'" >&2
            tail -n 120 "$OUT/logs/$MODE.log" >&2
            exit 1
        }
    done
    for marker in \
        'F0_OBSERVED R_MULTI_BEAT_DRAIN' \
        'F0_OBSERVED R_STALL_CASE beat=0 beats=4 same_address_write=1' \
        'F0_OBSERVED R_STALL_CASE beat=1 beats=4 same_address_write=1' \
        'F0_OBSERVED R_STALL_CASE beat=3 beats=4 same_address_write=1' \
        'F0_OBSERVED R_STALL_CASE beat=0 beats=1 same_address_write=1'; do
        rg -q "$marker" "$OUT/logs/$MODE.log" || {
            echo "missing required stall witness '$marker'" >&2
            tail -n 120 "$OUT/logs/$MODE.log" >&2
            exit 1
        }
    done
else
    for delay in 1 63; do
        LOG="$OUT/logs/$MODE-delay-$delay.log"
        rg -q 'F0_QUALIFY_PASS' "$LOG" || {
            echo "qualify mode did not emit a pass marker for WRITE_DELAY=$delay" >&2
            tail -n 120 "$LOG" >&2
            exit 1
        }
        if rg -q 'F0_QUALIFY_FAILURE|F0_EXPECTED_VIOLATION' "$LOG"; then
            echo "qualify mode reported a violation for WRITE_DELAY=$delay" >&2
            tail -n 120 "$LOG" >&2
            exit 1
        fi
        rg -q 'F0_WITNESS_PASS R_MULTI_BEAT_DRAIN' "$LOG" || {
            echo "qualify mode did not prove the multi-beat read drain for WRITE_DELAY=$delay" >&2
            tail -n 120 "$LOG" >&2
            exit 1
        }
        for order in 0 1 2; do
            rg -q "F0_WRITE_TIMING_PASS order=$order delay=$delay " "$LOG" || {
                echo "qualify mode missed write timing order=$order delay=$delay" >&2
                tail -n 120 "$LOG" >&2
                exit 1
            }
        done
    done
fi
echo "PASS TestbenchMEMAXI $MODE; artifacts: $OUT"
