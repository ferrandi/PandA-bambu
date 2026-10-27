#!/usr/bin/env bash
set -Eeuo pipefail

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd -P)
ROOT=$(realpath "$SCRIPT_DIR/../../../../")
T4B_TESTS="$ROOT/panda_regressions/verilog/axi_burst/t4b_tests"

usage() {
    echo "usage: $0 characterize|qualify [fresh-output-directory]" >&2
    echo "  default: ./out_memaxi_model_<mode>, created in the directory the script is launched from" >&2
    echo "  explicit MODELSIM_ROOT selects MODELSIM_ROOT/questasim/bin; otherwise Bambu PATH/backend lookup is used" >&2
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

MODELSIM_SOURCE=MODELSIM_ROOT
if [[ -n ${MODELSIM_ROOT:-} ]]; then
    MODELSIM_BIN="$MODELSIM_ROOT/questasim/bin"
    MODELSIM_MENTOR_ROOT=$MODELSIM_ROOT
    MODELSIM_VSIM="$MODELSIM_BIN/vsim"
else
    # A complete simulator already on PATH is sufficient for this RTL-only
    # runner; only load Bambu settings when the resolver needs backend roots.
    if ! command -v vsim >/dev/null 2>&1 && [[ -z ${BAMBU_HLS_BACKEND_PATH:-} ]]; then
        if [[ -n ${BAMBU_HLS:-} && -r "$BAMBU_HLS/settings.sh" ]]; then
            # shellcheck disable=SC1090
            source "$BAMBU_HLS/settings.sh"
        elif [[ -r "$ROOT/install/settings.sh" ]]; then
            # Standalone RTL characterization follows Bambu's installed defaults.
            # shellcheck disable=SC1091
            source "$ROOT/install/settings.sh"
        else
            echo "BAMBU_HLS_BACKEND_PATH is unset and no Bambu settings.sh is available" >&2
            exit 2
        fi
    fi
    # shellcheck disable=SC1091
    source "$ROOT/etc/libtech/backend/simulation/modelsim/tool_lookup.sh"
    bambu_modelsim_resolve || { echo "ModelSim vsim could not be resolved using Bambu's search path" >&2; exit 2; }
    MODELSIM_SOURCE=$BAMBU_MODELSIM_SOURCE
    MODELSIM_BIN=$BAMBU_MODELSIM_BIN
    MODELSIM_MENTOR_ROOT=$BAMBU_MODELSIM_ROOT
    MODELSIM_VSIM=$BAMBU_MODELSIM_PATH
fi
VLOG="$MODELSIM_BIN/vlog"
VLIB="$MODELSIM_BIN/vlib"
VMAP="$MODELSIM_BIN/vmap"
for tool in "$MODELSIM_VSIM" "$VLOG" "$VLIB" "$VMAP"; do
    [[ -x $tool ]] || { echo "ModelSim tool unavailable in selected bin $MODELSIM_BIN: $tool" >&2; exit 2; }
done
export PATH="$MODELSIM_BIN:$PATH"

mkdir "$OUT"
mkdir -p "$OUT/tmp" "$OUT/hdl" "$OUT/logs"
export TMPDIR="$OUT/tmp" TMP="$OUT/tmp" TEMP="$OUT/tmp"
python3 "$SCRIPT_DIR/extract_testbench_memaxi.py" \
    --xml "$ROOT/etc/libtech/NC_TESTBENCH_IPs.xml" \
    --out "$OUT/hdl/TestbenchMEMAXI.sv" \
    --t4b-extractor "$T4B_TESTS/extract_t4b_cells.py" >"$OUT/logs/extract.log"
cp "$SCRIPT_DIR/tb_testbench_memaxi_backend.sv" "$OUT/hdl/tb_testbench_memaxi_backend.sv"
cp "$SCRIPT_DIR/tb_testbench_memaxi_model.sv" "$OUT/hdl/tb_testbench_memaxi_model.sv"

VSIM=$MODELSIM_VSIM

{
    printf 'mode=%s\nsimulator_source=%s\nsimulator_root=%s\nsimulator_bin=%s\nvsim=%s\nvsim_resolved=%s\nvlog=%s\nvlog_resolved=%s\n' \
        "$MODE" "$MODELSIM_SOURCE" "$MODELSIM_MENTOR_ROOT" "$MODELSIM_BIN" "$VSIM" "$(realpath "$VSIM")" "$VLOG" "$(realpath "$VLOG")"
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
                grep -Fq "F0_NEGATIVE_START case=$test_case delay=$delay" "$LOG" || {
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
                grep -Eq "^#? *\\*\\* Fatal: $diagnostic$" "$LOG" || {
                    echo "negative case missed exact Fatal diagnostic: case=$test_case delay=$delay expected='$diagnostic' (vsim exit=$status)" >&2
                    tail -n 80 "$LOG" >&2
                    exit 1
                }
                fatal_count=$(grep -Ec '^#? *\*\* Fatal:' "$LOG" || true)
                if [[ $fatal_count != 1 ]] || grep -Eq '^#? *\*\* Error:' "$LOG"; then
                    echo "negative case emitted additional simulator errors: case=$test_case delay=$delay fatals=$fatal_count (vsim exit=$status)" >&2
                    tail -n 80 "$LOG" >&2
                    exit 1
                fi
                grep -Eq 'memory(/|\.)independent_axi_model' "$LOG" &&
                    grep -Eq 'File: .*/TestbenchMEMAXI\.sv' "$LOG" || {
                    echo "negative case Fatal did not identify the generated model process/file: case=$test_case delay=$delay (vsim exit=$status)" >&2
                    tail -n 80 "$LOG" >&2
                    exit 1
                }
                if grep -Fq 'F0 fixture watchdog expired' "$LOG" ||
                    grep -Fq 'F0_NEGATIVE_DID_NOT_ABORT' "$LOG" ||
                    grep -Fq 'F0_QUALIFY_PASS' "$LOG"; then
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
        grep -Eq "F0_EXPECTED_VIOLATION $marker" "$OUT/logs/$MODE.log" || {
            echo "missing required characterization witness '$marker'" >&2
            tail -n 120 "$OUT/logs/$MODE.log" >&2
            exit 1
        }
    done
    for marker in 'F0_OBSERVED AR_NEXT_ACCEPTED_DURING_FINAL_STALL' 'F0_OBSERVED AW_W_DISTINCT_HANDSHAKES' 'F0_CHARACTERIZATION_EXPECTED_DEFECTS'; do
        grep -Eq "$marker" "$OUT/logs/$MODE.log" || {
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
        grep -Eq "$marker" "$OUT/logs/$MODE.log" || {
            echo "missing required stall witness '$marker'" >&2
            tail -n 120 "$OUT/logs/$MODE.log" >&2
            exit 1
        }
    done
else
    for delay in 1 63; do
        LOG="$OUT/logs/$MODE-delay-$delay.log"
        grep -Eq 'F0_QUALIFY_PASS' "$LOG" || {
            echo "qualify mode did not emit a pass marker for WRITE_DELAY=$delay" >&2
            tail -n 120 "$LOG" >&2
            exit 1
        }
        if grep -Eq 'F0_QUALIFY_FAILURE|F0_EXPECTED_VIOLATION' "$LOG"; then
            echo "qualify mode reported a violation for WRITE_DELAY=$delay" >&2
            tail -n 120 "$LOG" >&2
            exit 1
        fi
        grep -Eq 'F0_WITNESS_PASS R_MULTI_BEAT_DRAIN' "$LOG" || {
            echo "qualify mode did not prove the multi-beat read drain for WRITE_DELAY=$delay" >&2
            tail -n 120 "$LOG" >&2
            exit 1
        }
        for order in 0 1 2; do
            grep -Eq "F0_WRITE_TIMING_PASS order=$order delay=$delay " "$LOG" || {
                echo "qualify mode missed write timing order=$order delay=$delay" >&2
                tail -n 120 "$LOG" >&2
                exit 1
            }
        done
    done
fi
echo "PASS TestbenchMEMAXI $MODE; artifacts: $OUT"
