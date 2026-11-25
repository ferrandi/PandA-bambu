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
TIMEOUT=$(command -v timeout) || { echo "timeout is required" >&2; exit 2; }
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
VSIM_TIMEOUT=120s
version_status=0
if "$TIMEOUT" 30s "$VSIM" -version >"$OUT/logs/vsim-version.log" 2>&1; then
    version_status=0
else
    version_status=$?
fi
printf '%s\n' "$version_status" >"$OUT/logs/vsim-version.log.raw-status"
printf '%s\n' "$version_status" >"$OUT/logs/vsim-version.log.effective-status"
if (( version_status != 0 )); then
    echo "ModelSim version query failed or timed out" >&2
    tail -n 40 "$OUT/logs/vsim-version.log" >&2
    exit 1
fi

run_vsim() {
    local log=$1 raw_status=0 effective_status
    shift
    if "$TIMEOUT" "$VSIM_TIMEOUT" "$VSIM" "$@" >"$log" 2>&1; then
        raw_status=0
    else
        raw_status=$?
    fi
    effective_status=$raw_status
    if (( raw_status == 0 )) && grep -Eq '^#? *\*\* (Fatal|Error):' "$log"; then
        effective_status=1
    fi
    printf '%s\n' "$raw_status" >"$log.raw-status"
    printf '%s\n' "$raw_status" >"$log.status"
    printf '%s\n' "$effective_status" >"$log.effective-status"
    return "$effective_status"
}

check_positive_run() {
    local log=$1 label=$2 mode=$3 raw_status effective_status
    raw_status=$(<"$log.raw-status")
    effective_status=$(<"$log.effective-status")
    if [[ $raw_status != 0 || $effective_status != 0 ]] ||
       grep -Eqi '^#? *\*\* (Fatal|Error):|(^#? *)Aborted([[:space:]]|$)|segmentation fault|stack smashing|core dumped|watchdog|timed out' "$log" ||
       { [[ $mode != characterize ]] && grep -Fq 'F0_EXPECTED_VIOLATION' "$log"; }; then
        echo "$label failed despite any transcript markers (raw=$raw_status effective=$effective_status): $log" >&2
        tail -n 80 "$log" >&2
        return 1
    fi
}

check_negative_run() {
    local log=$1 test_case=$2 delay=$3 diagnostic=$4 raw_status effective_status normalized aborted_count
    raw_status=$(<"$log.raw-status")
    effective_status=$(<"$log.effective-status")
    normalized="$log.normalized"
    sed -E 's/^# //' "$log" >"$normalized"
    if [[ $raw_status != 0 && $raw_status != 1 && $raw_status != 134 ]] ||
       { [[ $raw_status == 134 ]] && [[ $effective_status != 134 ]]; } ||
       { [[ $raw_status != 134 ]] && [[ $effective_status != 1 ]]; } ||
       ! grep -Fqx "F0_NEGATIVE_START case=$test_case delay=$delay" "$normalized" ||
       [[ $(grep -Ec '^\*\* Fatal:' "$normalized" || true) != 1 ]] ||
       [[ $(grep -Fxc "** Fatal: $diagnostic" "$normalized" || true) != 1 ]] ||
       grep -Eq '^\*\* Error:' "$normalized" ||
       grep -Eqi 'segmentation fault|stack smashing|watchdog|timed out|F0_NEGATIVE_DID_NOT_ABORT|F0_QUALIFY_PASS' "$normalized" ||
       { [[ $raw_status != 134 ]] && grep -Eqi 'core dumped' "$normalized"; }; then
        echo "negative case failed its exact-fatal contract: case=$test_case delay=$delay raw=$raw_status effective=$effective_status" >&2
        tail -n 80 "$log" >&2
        return 1
    fi
    aborted_count=$(grep -Ec '^Aborted([[:space:]]|$)' "$normalized" || true)
    if { [[ $raw_status == 134 ]] && [[ $aborted_count != 1 ]]; } ||
       { [[ $raw_status != 134 ]] && [[ $aborted_count != 0 ]]; }; then
        echo "negative case has an unexpected Aborted transcript: case=$test_case delay=$delay raw=$raw_status" >&2
        tail -n 80 "$log" >&2
        return 1
    fi
    grep -Eq 'memory(/|\.)independent_axi_model' "$normalized" &&
        grep -Eq 'File: .*/TestbenchMEMAXI\.sv' "$normalized" || {
            echo "negative case Fatal did not identify the generated model process/file: case=$test_case delay=$delay" >&2
            tail -n 80 "$log" >&2
            return 1
        }
}

{
    printf 'mode=%s\nsimulator_source=%s\nsimulator_root=%s\nsimulator_bin=%s\nvsim=%s\nvsim_resolved=%s\nvlog=%s\nvlog_resolved=%s\n' \
        "$MODE" "$MODELSIM_SOURCE" "$MODELSIM_MENTOR_ROOT" "$MODELSIM_BIN" "$VSIM" "$(realpath "$VSIM")" "$VLOG" "$(realpath "$VLOG")"
    printf 'vsim_timeout=%s\n' "$VSIM_TIMEOUT"
    sed -n '1,4p' "$OUT/logs/vsim-version.log"
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
    LOG="$OUT/logs/$MODE.log"
    run_vsim "$LOG" -c work.tb_testbench_memaxi_model "+MODE=$MODE" -do 'run -all; quit -f' || true
    check_positive_run "$LOG" "$MODE" characterize
else
    for delay in 1 63; do
        LOG="$OUT/logs/$MODE-delay-$delay.log"
        run_vsim "$LOG" -c work.tb_testbench_memaxi_model -gWRITE_DELAY="$delay" "+MODE=$MODE" \
            -do 'run -all; quit -f' || true
        check_positive_run "$LOG" "$MODE WRITE_DELAY=$delay" qualify
    done
    if [[ $MODE == qualify ]]; then
        for delay in 1 63; do
            for test_case in early_wlast missing_final_wlast read_wrap read_reserved write_wrap write_reserved; do
                LOG="$OUT/logs/negative-$test_case-delay-$delay.log"
                run_vsim "$LOG" -c work.tb_testbench_memaxi_model -gWRITE_DELAY="$delay" "+MODE=$MODE" "+NEGATIVE=$test_case" \
                    -do 'run -all; quit -f' || true
                case $test_case in
                    early_wlast) diagnostic='AXI WLAST mismatch at beat 0 of 2' ;;
                    missing_final_wlast) diagnostic='AXI WLAST mismatch at beat 1 of 2' ;;
                    read_wrap) diagnostic='Unsupported AXI read burst type: 2' ;;
                    read_reserved) diagnostic='Unsupported AXI read burst type: 3' ;;
                    write_wrap) diagnostic='Unsupported AXI write burst type: 2' ;;
                    write_reserved) diagnostic='Unsupported AXI write burst type: 3' ;;
                esac
                check_negative_run "$LOG" "$test_case" "$delay" "$diagnostic"
                status=$(<"$LOG.raw-status")
                echo "PASS negative case=$test_case delay=$delay diagnostic='$diagnostic' vsim_raw_exit=$status"
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
        normalized="$LOG.normalized"
        sed -E 's/^# //' "$LOG" >"$normalized"
        grep -Fqx 'F0_RESPONSE_INJECTION_PASS read=SLVERR,DECERR,OKAY write=SLVERR,DECERR,OKAY reset=cleared stall=stable' "$normalized" || {
            echo "qualify mode did not prove TestbenchMEMAXI response injection for WRITE_DELAY=$delay" >&2
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
