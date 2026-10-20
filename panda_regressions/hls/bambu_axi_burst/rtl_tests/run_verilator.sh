#!/usr/bin/env bash
set -euo pipefail
ulimit -c 0

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd -P)
ROOT=$(realpath "$SCRIPT_DIR/../../../../")
TESTS="$SCRIPT_DIR"
XML="$ROOT/etc/libtech/NC_AXI_IPs.xml"
if (($# < 1 || $# > 2)); then echo "usage: $0 fresh-output-directory [transactions|credits|bounds|configure|all]" >&2; exit 2; fi
SUITE=${2:-all}
case "$SUITE" in
    transactions|credits|bounds|configure|all) ;;
    *) echo "unknown Verilator suite: $SUITE" >&2; exit 2 ;;
esac
if [[ -e "$1" || -L "$1" ]]; then
    echo "output directory must not already exist: $1" >&2; exit 2
fi
# Parallelism is a launcher decision, not a script policy: any positive value is
# accepted. The default follows the machine, capped at 16.
DEFAULT_SIM_JOBS=$(nproc 2>/dev/null || echo 4)
if ((DEFAULT_SIM_JOBS > 16)); then DEFAULT_SIM_JOBS=16; fi
SIM_JOBS=${SIM_JOBS:-$DEFAULT_SIM_JOBS}
if [[ ! $SIM_JOBS =~ ^[1-9][0-9]*$ ]]; then
    echo "SIM_JOBS must be a positive integer, got: $SIM_JOBS" >&2; exit 2
fi
for tool in python3 verilator rg timeout; do
    command -v "$tool" >/dev/null 2>&1 || { echo "missing required tool: $tool" >&2; exit 2; }
done
TASK_OUT=$(realpath -m "$1")
mkdir -p "$TASK_OUT"
mkdir -p "$TASK_OUT/tmp"
export TMPDIR="$TASK_OUT/tmp"
export TMP="$TASK_OUT/tmp"
export TEMP="$TASK_OUT/tmp"

python3 -B "$TESTS/extract_write_engine.py" --xml "$XML" \
    --target MinimalAXI4MasterPipelined --out "$TASK_OUT/MinimalAXI4MasterPipelined.sv"

echo "Verilator: $(verilator --version)"
echo "Stimulus seed: deterministic (no random stimulus)"
echo "All generated RTL, logs, and build products: $TASK_OUT"
echo "Verilator suite: $SUITE (build and scenario parallelism: $SIM_JOBS)"

# Verilator can build and run simulations concurrently. SIM_JOBS bounds both:
# -j for the model build, and the number of scenario processes in flight.

wait_scenario_batch() {
    local -n pids_ref=$1
    local -n names_ref=$2
    local build_dir=$3
    local i status name log
    for i in "${!pids_ref[@]}"; do
        status=0
        wait "${pids_ref[$i]}" || status=$?
        name="${names_ref[$i]}"
        log="$build_dir/scenario-${name}.log"
        if (( status != 0 )); then
            echo "FAIL scenario $name (status $status); log: $log" >&2
            cat "$log" >&2 || true
            SCENARIO_FAILURES=$((SCENARIO_FAILURES + 1))
        elif ! rg -q "^DUPLEX_READ PASS CASE=${name}$" "$log"; then
            echo "FAIL scenario $name: missing exact PASS marker; log: $log" >&2
            cat "$log" >&2 || true
            SCENARIO_FAILURES=$((SCENARIO_FAILURES + 1))
        elif [[ "$name" == READY_CREDIT_PROOF ]] && {
            ! rg -q '^PASS READY_CREDIT_PROOF FULL O=' "$log" ||
            ! rg -q '^PASS READY_CREDIT_PROOF O=.* full-LOAD-credit-AR-response-drain N=' "$log";
        }; then
            echo "FAIL scenario $name: missing positive full-FIFO/credit-release witness; log: $log" >&2
            cat "$log" >&2 || true
            SCENARIO_FAILURES=$((SCENARIO_FAILURES + 1))
        fi
    done
    pids_ref=()
    names_ref=()
}

run_scenarios() {
    local binary=$1 build_dir=$2
    shift 2
    local batch_pids=() batch_names=() scenario
    SCENARIO_FAILURES=0
    for scenario in "$@"; do
        timeout 120s "$binary" "+$scenario" "+CASE=$scenario" >"$build_dir/scenario-${scenario}.log" 2>&1 &
        batch_pids+=("$!")
        batch_names+=("$scenario")
        if (( ${#batch_pids[@]} >= SIM_JOBS )); then
            wait_scenario_batch batch_pids batch_names "$build_dir"
        fi
    done
    wait_scenario_batch batch_pids batch_names "$build_dir"
    if (( SCENARIO_FAILURES != 0 )); then
        echo "ERROR: $SCENARIO_FAILURES scenario(s) failed for $binary" >&2
        return 1
    fi
}

run_one_scenario() {
    local binary=$1 build_dir=$2 scenario=$3
    local log="$build_dir/scenario-${scenario}.log" status=0
    timeout 120s "$binary" "+$scenario" "+CASE=$scenario" >"$log" 2>&1 || status=$?
    if (( status != 0 )); then
        echo "FAIL scenario $scenario (status $status); log: $log" >&2
        cat "$log" >&2 || true
        return 1
    fi
    if ! rg -q "^DUPLEX_READ PASS CASE=${scenario}$" "$log"; then
        echo "FAIL scenario $scenario: missing exact PASS marker; log: $log" >&2
        cat "$log" >&2 || true
        return 1
    fi
}

run_build() {
    local top=$1 burst_max=$2 tb=$3 snapshot=${4:-} outstanding=${5:-1} depth=${6:-256} launch=${7:-yes}
    local build="$TASK_OUT/build-${top}-B${burst_max}-O${outstanding}-D${depth}"
    mkdir -p "$build"
    local sources=("$TASK_OUT/MinimalAXI4MasterPipelined.sv")
    if [[ -n "$snapshot" ]]; then
        sources+=("$snapshot")
    fi
    sources+=("$tb")
    local parameter_args=(-GB_MAX="$burst_max")
    if [[ "$top" == tb_burst_engine ]]; then
        parameter_args+=(-GMAX_OUTSTANDING="$outstanding" -GFIFO_DEPTH="$depth")
    fi
    timeout 120s verilator --binary -j "$SIM_JOBS" --timing --assert -Wall -Wno-fatal -CFLAGS -O0 \
        -Wno-UNUSEDSIGNAL -Wno-BLKSEQ --top-module "$top" \
        "${parameter_args[@]}" --Mdir "$build" "${sources[@]}" \
        >"$build/compile.log" 2>&1 || {
            cat "$build/compile.log"
            return 1
        }
    if [[ "$launch" == yes ]]; then
        run_one_scenario "$build/V$top" "$build" SMOKE
    fi
}

if [[ $SUITE == all || $SUITE == transactions ]]; then
    for burst_max in 1 2 16 256; do
        run_build tb_burst_engine "$burst_max" "$TESTS/tb_burst_engine.sv"
        if [[ "$burst_max" == 16 ]]; then
            run_scenarios "$TASK_OUT/build-tb_burst_engine-B16-O1-D256/Vtb_burst_engine" \
                "$TASK_OUT/build-tb_burst_engine-B16-O1-D256" \
                ERROR BADALIGN OVERFLOW BADSIZE_CONFIG STORE \
                RANGE_ZERO RANGE_LIMIT_BASE0 RANGE_LIMIT_BASE4 \
                RANGE_OVER_BASE4 RANGE_OVER_PLUS1 RANGE_OVER_HALF \
                RANGE_OVER_MAX RANGE_TOP_VALID RANGE_TOP_OVER RANGE_ORACLE \
                BADADDR_EMPTY BADSIZE_EMPTY BADADDR_FULL BADSIZE_FULL \
                DRAIN_CONFIG_RACE CONTINUOUS_CFG ADJACENT_CFG BADRID EARLY_RLAST \
                CONFIG_PIPELINE_VALID CONFIG_PIPELINE_INVALID \
                RRESP_FIRST RRESP_INTERMEDIATE RRESP_FINAL \
                ADJACENT_CFG_TRUNC ADJACENT_CFG_SIZE \
                MISSING_RLAST RESET_TRAFFIC FIFO_WRAP
        fi
        if [[ "$burst_max" == 256 ]]; then
            run_scenarios "$TASK_OUT/build-tb_burst_engine-B256-O1-D256/Vtb_burst_engine" \
                "$TASK_OUT/build-tb_burst_engine-B256-O1-D256" FIFO_BACKPRESSURE
        fi
    done

    # Keep responses held until the requested descriptor queue is full. This proves
    # actual AR overlap and exercises explicit descriptor-pointer wrap for O=3/15.
    for outstanding in 3 15 16; do
        depth=256
        [[ "$outstanding" == 3 ]] && depth=64
        run_build tb_burst_engine 16 "$TESTS/tb_burst_engine.sv" "" "$outstanding" "$depth" no
        run_one_scenario "$TASK_OUT/build-tb_burst_engine-B16-O${outstanding}-D${depth}/Vtb_burst_engine" \
            "$TASK_OUT/build-tb_burst_engine-B16-O${outstanding}-D${depth}" MULTI
    done

    # Required-hit A+Q+C plus independent bus-derived descriptor accounting,
    # malformed R during a held later AR, coordinated multi-outstanding reset,
    # boundary/tail with a busy configure, and rejection of a second pending config.
    run_build tb_burst_engine 16 "$TESTS/tb_burst_engine.sv" "" 2 64 no
    run_scenarios "$TASK_OUT/build-tb_burst_engine-B16-O2-D64/Vtb_burst_engine" \
        "$TASK_OUT/build-tb_burst_engine-B16-O2-D64" AQC FAULT_STALLED_AR RRESP_STALLED_AR RESET_MULTI PENDING_SECOND
    run_build tb_burst_engine 16 "$TESTS/tb_burst_engine.sv" "" 3 64 no
    run_scenarios "$TASK_OUT/build-tb_burst_engine-B16-O3-D64/Vtb_burst_engine" \
        "$TASK_OUT/build-tb_burst_engine-B16-O3-D64" \
        BOUNDARY_PENDING RRESP_RESERVATION_RACE RREADY_SIDEBAND
fi

# Distinguish descriptor-count limits from beat-credit limits. D=1 also checks
# minimum-width data FIFO pointers/counters and stop/restart behavior.
if [[ $SUITE == all || $SUITE == credits ]]; then
    run_build tb_burst_engine 16 "$TESTS/tb_burst_engine.sv" "" 16 32 no
    run_scenarios "$TASK_OUT/build-tb_burst_engine-B16-O16-D32/Vtb_burst_engine" \
        "$TASK_OUT/build-tb_burst_engine-B16-O16-D32" CREDIT_LIMIT CREDIT_POP READY_CREDIT_PROOF
    run_build tb_burst_engine 256 "$TESTS/tb_burst_engine.sv" "" 2 8 no
    run_one_scenario "$TASK_OUT/build-tb_burst_engine-B256-O2-D8/Vtb_burst_engine" \
        "$TASK_OUT/build-tb_burst_engine-B256-O2-D8" CREDIT_LIMIT
    run_build tb_burst_engine 16 "$TESTS/tb_burst_engine.sv" "" 1 1 no
    run_scenarios "$TASK_OUT/build-tb_burst_engine-B16-O1-D1/Vtb_burst_engine" \
        "$TASK_OUT/build-tb_burst_engine-B16-O1-D1" CREDIT_LIMIT READY_CREDIT_PROOF

    # Isolate configuration latency from the B=1, FIFO=1 read steady state.
    run_build tb_burst_engine 1 "$TESTS/tb_burst_engine.sv" "" 1 1 no
    run_one_scenario "$TASK_OUT/build-tb_burst_engine-B1-O1-D1/Vtb_burst_engine" \
        "$TASK_OUT/build-tb_burst_engine-B1-O1-D1" CONFIG_PIPELINE_B1
fi

# Exercise the largest supported data FIFO and fail closed at every parameter
# boundary with a checked diagnostic (not merely a compile/runtime failure).
if [[ $SUITE == all || $SUITE == bounds ]]; then
    run_build tb_burst_engine 16 "$TESTS/tb_burst_engine.sv" "" 1 4096 no
    run_one_scenario "$TASK_OUT/build-tb_burst_engine-B16-O1-D4096/Vtb_burst_engine" \
        "$TASK_OUT/build-tb_burst_engine-B16-O1-D4096" DEPTH4096

expect_parameter_failure() {
    local name=$1 outstanding=$2 depth=$3 diagnostic=$4
    local binary="$TASK_OUT/build-tb_burst_engine-B16-O${outstanding}-D${depth}/Vtb_burst_engine"
    local log="$TASK_OUT/invalid-${name}.log"
    run_build tb_burst_engine 16 "$TESTS/tb_burst_engine.sv" "" "$outstanding" "$depth" no
    python3 - "$binary" "$log" "$name" <<'PY'
import resource
import subprocess
import sys

binary, log_path, case_name = sys.argv[1:]

def disable_core_dumps():
    resource.setrlimit(resource.RLIMIT_CORE, (0, 0))

try:
    with open(log_path, "wb") as log:
        result = subprocess.run([binary, "+CASE=SMOKE", "+SMOKE"],
                                stdout=log, stderr=subprocess.STDOUT,
                                timeout=10, preexec_fn=disable_core_dumps,
                                check=False)
except subprocess.TimeoutExpired:
    with open(log_path, "ab") as log:
        log.write(b"\nparameter-validation simulator timed out\n")
    sys.exit(f"ERROR: invalid parameters {case_name} timed out")

with open(log_path, "ab") as log:
    log.write(f"\nparameter-validation simulator exit status: {result.returncode}\n".encode())
if result.returncode == 0:
    sys.exit(f"ERROR: invalid parameters {case_name} unexpectedly passed")
PY
    if ! rg -q "$diagnostic" "$log"; then
        cat "$log" >&2
        echo "ERROR: invalid parameters $name did not report '$diagnostic'" >&2
        return 1
    fi
    echo "PASS invalid parameters $name rejected with diagnostic '$diagnostic'"
}
expect_parameter_failure O0 0 256 'MAX_OUTSTANDING must be in \[1,16\]'
expect_parameter_failure O17 17 256 'MAX_OUTSTANDING must be in \[1,16\]'
expect_parameter_failure D0 1 0 'FIFO_DEPTH must be a power of two in \[1,4096\]'
expect_parameter_failure D3 1 3 'FIFO_DEPTH must be a power of two in \[1,4096\]'
expect_parameter_failure D8192 1 8192 'FIFO_DEPTH must be a power of two in \[1,4096\]'

# Element-count limit. The region counters are COUNT_W = BITSIZE_in5 - 1 bits
# wide, so a 64-bit address must not turn the address check into permission to
# configure a count the counters cannot hold.
ct_build="$TASK_OUT/build-tb_count_trunc-addr64"
mkdir -p "$ct_build"
timeout 120s verilator --binary -j "$SIM_JOBS" --timing --assert -Wall -Wno-fatal -CFLAGS -O0 \
    -Wno-UNUSEDSIGNAL -Wno-BLKSEQ --top-module tb_count_trunc \
    -GB_MAX=16 -GMAX_OUTSTANDING=1 -GFIFO_DEPTH=256 -GADDR_BITS=64 \
    --Mdir "$ct_build" "$TASK_OUT/MinimalAXI4MasterPipelined.sv" "$TESTS/tb_count_trunc.sv" \
    >"$ct_build/compile.log" 2>&1 || {
        cat "$ct_build/compile.log" >&2
        exit 1
    }
for scenario in COUNT_TRUNC_64 COUNT_MAX_64 \
    RANGE_ZERO_COUNT RANGE_MAX_COUNT RANGE_TOP_COUNT \
    RANGE_LAST_BEAT_1 RANGE_LAST_BEAT_2 RANGE_LOW_CARRY_NONMAX \
    RANGE_HIGH_MAX_CARRY_ZERO RANGE_HIGH_MAX_NO_CARRY \
    RANGE_HIGH_MAX_CARRY_NONZERO \
    RANGE_ZERO_MISALIGNED; do
    log="$ct_build/scenario-${scenario}.log"
    timeout 120s "$ct_build/Vtb_count_trunc" "+$scenario" >"$log" 2>&1 || {
        cat "$log" >&2
        exit 1
    }
    rg -q "^DUPLEX_READ PASS CASE=${scenario}$" "$log" || {
        cat "$log" >&2
        echo "ERROR: missing exact PASS marker for $scenario" >&2
        exit 1
    }
done
echo "PASS element-count limit enforced at the counter width, not the address limit"
echo "PASS 64-bit CONFIGURE range boundaries exercised"

run_range_fallback_case() {
    local name=$1 input_bits=$2 scenario=$3
    local build_dir="$TASK_OUT/build-tb_count_trunc-$name"
    mkdir -p "$build_dir"
    timeout 120s verilator --binary -j "$SIM_JOBS" --timing --assert -Wall -Wno-fatal -CFLAGS -O0 \
        -Wno-UNUSEDSIGNAL -Wno-BLKSEQ --top-module tb_count_trunc \
        -GB_MAX=16 -GMAX_OUTSTANDING=1 -GFIFO_DEPTH=256 -GADDR_BITS=64 \
        -GIN_BITS="$input_bits" \
        --Mdir "$build_dir" "$TASK_OUT/MinimalAXI4MasterPipelined.sv" \
        "$TESTS/tb_count_trunc.sv" >"$build_dir/compile.log" 2>&1 || {
            cat "$build_dir/compile.log" >&2
            exit 1
        }
    local log="$build_dir/scenario-$scenario.log"
    timeout 120s "$build_dir/Vtb_count_trunc" "+$scenario" "+CASE=$scenario" >"$log" 2>&1 || {
        cat "$log" >&2
        exit 1
    }
    rg -q "^DUPLEX_READ PASS CASE=$scenario$" "$log" || {
        cat "$log" >&2
        echo "ERROR: missing exact PASS marker for $scenario" >&2
        exit 1
    }
}

run_range_fallback_case narrow-in4 32 FALLBACK_NARROW_IN4
run_range_fallback_case wide-in4 65 FALLBACK_WIDE_IN4
run_range_fallback_case wide-zero-in4 65 FALLBACK_WIDE_ZERO
echo "PASS CONFIGURE range fallback widths exercised"
fi

if [[ $SUITE == all || $SUITE == configure ]]; then
    "$TESTS/run_configure_read_pipeline.sh" "$TASK_OUT/configure-read-pipeline"
fi

echo "PASS burst engine RTL regression group=$SUITE. Artifacts retained at $TASK_OUT"
