#!/usr/bin/env bash
set -euo pipefail
ulimit -c 0

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd -P)
ROOT=$(realpath "$SCRIPT_DIR/../../../../")
TESTS=$SCRIPT_DIR
XML="$ROOT/etc/libtech/NC_AXI_IPs.xml"
source "$SCRIPT_DIR/modelsim_helpers.sh"

if (($# < 1 || $# > 2)); then
    echo "usage: $0 fresh-output-directory [transactions|credits|bounds|configure|all]" >&2
    exit 2
fi
SUITE=${2:-all}
case "$SUITE" in
    transactions|credits|bounds|configure|all) ;;
    *) echo "unknown ModelSim suite: $SUITE" >&2; exit 2 ;;
esac
if [[ -e $1 || -L $1 ]]; then
    echo "output directory must not already exist: $1" >&2
    exit 2
fi
for tool in python3 timeout sha256sum git sed grep; do
    command -v "$tool" >/dev/null 2>&1 || { echo "missing required tool: $tool" >&2; exit 2; }
done
modelsim_resolve
TASK_OUT=$(realpath -m "$1")
mkdir -p "$TASK_OUT/tmp"
export TMPDIR="$TASK_OUT/tmp" TMP="$TASK_OUT/tmp" TEMP="$TASK_OUT/tmp"

python3 -B "$TESTS/extract_write_engine.py" --xml "$XML" \
    --target MinimalAXI4MasterPipelined --out "$TASK_OUT/MinimalAXI4MasterPipelined.sv"
READ_RTL="$TASK_OUT/MinimalAXI4MasterPipelined.sv"
TB="$TESTS/tb_burst_engine.sv"
printf 'git_head=%s\n' "$(git -C "$ROOT" rev-parse HEAD)" > "$TASK_OUT/provenance.txt"
printf 'xml=' >> "$TASK_OUT/provenance.txt"; sha256sum "$XML" >> "$TASK_OUT/provenance.txt"
printf 'extractor=' >> "$TASK_OUT/provenance.txt"; sha256sum "$TESTS/extract_write_engine.py" >> "$TASK_OUT/provenance.txt"
printf 'generated_hdl=' >> "$TASK_OUT/provenance.txt"; sha256sum "$READ_RTL" >> "$TASK_OUT/provenance.txt"
printf 'testbench=' >> "$TASK_OUT/provenance.txt"; sha256sum "$TB" >> "$TASK_OUT/provenance.txt"
printf 'runner=' >> "$TASK_OUT/provenance.txt"; sha256sum "$0" >> "$TASK_OUT/provenance.txt"
cat "$TASK_OUT/provenance.txt" > "$TASK_OUT/aggregate.log"
echo "ModelSim: $(modelsim_version | sed -n '1p')"
echo "Stimulus seed: deterministic (no random stimulus)"
echo "All generated RTL, logs, and build products: $TASK_OUT"
echo "ModelSim suite: $SUITE (serial within each isolated work library)"

record_rtl_hash() {
    printf '%s  %s\n' "$(sha256sum "$1")" "$1" >> "$TASK_OUT/extracted_hdl_sha256.txt"
}

build_design() {
    local build=$1 top=$2 source=$3
    shift 3
    mkdir -p "$build"
    modelsim_compile "$build" "$top" "$source" "$@" || {
        echo "ModelSim compile failed for $top in $build" >&2
        return 1
    }
    printf '%s\n' "$@" > "$build/elaboration.args"
}

read_elaboration_args() {
    local build=$1
    mapfile -t ELAB_ARGS < "$build/elaboration.args"
}

run_one_scenario() {
    local build=$1 top=$2 scenario=$3 log="$build/scenario-${3}.raw.log"
    local normalized="$build/scenario-${scenario}.log"
    read_elaboration_args "$build"
    local status=0
    modelsim_run "$build" "$top" "$log" 120 "${ELAB_ARGS[@]}" -- "+$scenario" "+CASE=$scenario" || status=$?
    modelsim_normalize_log "$log" "$normalized"
    if ((status != 0)); then
        cat "$normalized" >&2 || true
        echo "FAIL scenario $scenario (ModelSim status $status); log: $normalized" >&2
        return 1
    fi
    if ! grep -Eq "^DUPLEX_READ PASS CASE=${scenario}$" "$normalized"; then
        cat "$normalized" >&2 || true
        echo "FAIL scenario $scenario: missing exact PASS marker; log: $normalized" >&2
        return 1
    fi
    if [[ $scenario == READY_CREDIT_PROOF ]] && {
        ! grep -Eq '^PASS READY_CREDIT_PROOF FULL O=' "$normalized" ||
        ! grep -Eq '^PASS READY_CREDIT_PROOF O=.* full-LOAD-credit-AR-response-drain N=' "$normalized";
    }; then
        cat "$normalized" >&2 || true
        echo "FAIL scenario $scenario: missing positive full-FIFO/credit-release witness" >&2
        return 1
    fi
    cat "$normalized" >> "$TASK_OUT/aggregate.log"
}

run_scenarios() {
    local build=$1 top=$2 scenario
    shift 2
    # One run.do and one work library per build: scenarios are deliberately serial.
    for scenario in "$@"; do run_one_scenario "$build" "$top" "$scenario"; done
}

run_build() {
    local top=$1 burst_max=$2 tb=$3 snapshot=${4:-} outstanding=${5:-1} depth=${6:-256} launch=${7:-yes}
    local build="$TASK_OUT/build-${top}-B${burst_max}-O${outstanding}-D${depth}"
    local -a sources=() args=("-gB_MAX=$burst_max")
    [[ -z $snapshot ]] || sources+=("$snapshot")
    sources+=("$tb")
    if [[ $top == tb_burst_engine ]]; then
        args+=("-gMAX_OUTSTANDING=$outstanding" "-gFIFO_DEPTH=$depth")
    fi
    build_design "$build" "$top" "$READ_RTL" "${sources[@]}" --
    # Parameters belong to vsim elaboration, while sources belong to vlog.
    printf '%s\n' "${args[@]}" > "$build/elaboration.args"
    [[ $launch != yes ]] || run_one_scenario "$build" "$top" SMOKE
}

run_async_reset_build() {
    local polarity=$1 active_low=1 label source build
    [[ $polarity != active-high ]] || active_low=0
    label="ASYNC_RESET_${polarity}"
    source="$TASK_OUT/MinimalAXI4MasterPipelined-${label}.sv"
    build="$TASK_OUT/build-${label}"
    python3 -B "$TESTS/extract_write_engine.py" --xml "$XML" \
        --target MinimalAXI4MasterPipelined --reset-mode async --reset-polarity "$polarity" \
        --out "$source" > "$build.extract.log" 2>&1 || { cat "$build.extract.log" >&2; return 1; }
    record_rtl_hash "$source"
    build_design "$build" tb_burst_engine "$source" "$TB" --
    printf '%s\n' -gB_MAX=16 -gMAX_OUTSTANDING=1 -gFIFO_DEPTH=256 -gASYNC_RESET=1 "-gRESET_ACTIVE_LOW=$active_low" > "$build/elaboration.args"
    run_one_scenario "$build" tb_burst_engine ASYNC_RESET
    echo "ASYNC_RESET_VARIANT_PASS polarity=$polarity reset_mode=async" | tee -a "$TASK_OUT/aggregate.log"
}

if [[ $SUITE == all || $SUITE == transactions ]]; then
    for burst_max in 1 2 16 256; do
        run_build tb_burst_engine "$burst_max" "$TB"
        if [[ $burst_max == 16 ]]; then
            run_scenarios "$TASK_OUT/build-tb_burst_engine-B16-O1-D256" tb_burst_engine \
                ERROR BADALIGN OVERFLOW BADSIZE_CONFIG STORE \
                RANGE_ZERO RANGE_LIMIT_BASE0 RANGE_LIMIT_BASE4 RANGE_OVER_BASE4 RANGE_OVER_PLUS1 RANGE_OVER_HALF \
                RANGE_OVER_MAX RANGE_TOP_VALID RANGE_TOP_OVER RANGE_ORACLE BADADDR_EMPTY BADSIZE_EMPTY BADADDR_FULL BADSIZE_FULL \
                DRAIN_CONFIG_RACE CONTINUOUS_CFG ADJACENT_CFG BADRID EARLY_RLAST CONFIG_PIPELINE_VALID CONFIG_PIPELINE_INVALID \
                RRESP_FIRST RRESP_INTERMEDIATE RRESP_FINAL ADJACENT_CFG_TRUNC ADJACENT_CFG_SIZE \
                MISSING_RLAST RESET_TRAFFIC FIFO_WRAP
        fi
        if [[ $burst_max == 256 ]]; then
            run_scenarios "$TASK_OUT/build-tb_burst_engine-B256-O1-D256" tb_burst_engine FIFO_BACKPRESSURE
        fi
    done
    for outstanding in 3 15 16; do
        depth=256; [[ $outstanding != 3 ]] || depth=64
        run_build tb_burst_engine 16 "$TB" "" "$outstanding" "$depth" no
        run_scenarios "$TASK_OUT/build-tb_burst_engine-B16-O${outstanding}-D${depth}" tb_burst_engine MULTI
    done
    run_build tb_burst_engine 16 "$TB" "" 2 64 no
    run_scenarios "$TASK_OUT/build-tb_burst_engine-B16-O2-D64" tb_burst_engine AQC FAULT_STALLED_AR RRESP_STALLED_AR RESET_MULTI PENDING_SECOND
    run_build tb_burst_engine 16 "$TB" "" 3 64 no
    run_scenarios "$TASK_OUT/build-tb_burst_engine-B16-O3-D64" tb_burst_engine BOUNDARY_PENDING RRESP_RESERVATION_RACE RREADY_SIDEBAND
    for polarity in active-low active-high; do run_async_reset_build "$polarity"; done
fi

if [[ $SUITE == all || $SUITE == credits ]]; then
    run_build tb_burst_engine 16 "$TB" "" 16 32 no
    run_scenarios "$TASK_OUT/build-tb_burst_engine-B16-O16-D32" tb_burst_engine CREDIT_LIMIT CREDIT_POP READY_CREDIT_PROOF
    run_build tb_burst_engine 256 "$TB" "" 2 8 no
    run_scenarios "$TASK_OUT/build-tb_burst_engine-B256-O2-D8" tb_burst_engine CREDIT_LIMIT
    run_build tb_burst_engine 16 "$TB" "" 1 1 no
    run_scenarios "$TASK_OUT/build-tb_burst_engine-B16-O1-D1" tb_burst_engine CREDIT_LIMIT READY_CREDIT_PROOF
    run_build tb_burst_engine 1 "$TB" "" 1 1 no
    run_scenarios "$TASK_OUT/build-tb_burst_engine-B1-O1-D1" tb_burst_engine CONFIG_PIPELINE_B1
fi

if [[ $SUITE == all || $SUITE == bounds ]]; then
    run_build tb_burst_engine 16 "$TB" "" 1 4096 no
    run_scenarios "$TASK_OUT/build-tb_burst_engine-B16-O1-D4096" tb_burst_engine DEPTH4096

    expect_parameter_failure() {
        local name=$1 outstanding=$2 depth=$3 diagnostic=$4
        local build="$TASK_OUT/build-invalid-${name}" raw="$TASK_OUT/invalid-${name}.raw.log" log="$TASK_OUT/invalid-${name}.log"
        build_design "$build" tb_burst_engine "$READ_RTL" "$TB" --
        printf '%s\n' -gB_MAX=16 "-gMAX_OUTSTANDING=$outstanding" "-gFIFO_DEPTH=$depth" > "$build/elaboration.args"
        read_elaboration_args "$build"
        local status=0
        modelsim_run "$build" tb_burst_engine "$raw" 30 "${ELAB_ARGS[@]}" -- +CASE=SMOKE +SMOKE || status=$?
        modelsim_normalize_log "$raw" "$log"
        local fatal_count
        fatal_count=$(grep -Ec '^(\*\* )?Fatal:' "$log" || true)
        # Questa may run the final block after the bench initializes selected_case.
        if (( status != 1 || fatal_count != 1 )) || ! grep -Fq "$diagnostic" "$log" || \
            grep -Eq '^\*\* Error:|Internal fault|segmentation fault|SIG(SEGV|ABRT)|timed out|Killed' "$log"; then
            cat "$log" >&2
            echo "ERROR: invalid parameters $name did not produce exactly one runtime fatal '$diagnostic' (status=$status fatals=$fatal_count)" >&2
            return 1
        fi
        echo "PASS invalid parameters $name rejected with diagnostic '$diagnostic'" | tee -a "$TASK_OUT/aggregate.log"
    }
    expect_parameter_failure O0 0 256 'MAX_OUTSTANDING must be in [1,16]'
    expect_parameter_failure O17 17 256 'MAX_OUTSTANDING must be in [1,16]'
    expect_parameter_failure D0 1 0 'FIFO_DEPTH must be a power of two in [1,4096]'
    expect_parameter_failure D3 1 3 'FIFO_DEPTH must be a power of two in [1,4096]'
    expect_parameter_failure D8192 1 8192 'FIFO_DEPTH must be a power of two in [1,4096]'

    ct_build="$TASK_OUT/build-tb_count_trunc-addr64"
    build_design "$ct_build" tb_count_trunc "$READ_RTL" "$TESTS/tb_count_trunc.sv" --
    printf '%s\n' -gB_MAX=16 -gMAX_OUTSTANDING=1 -gFIFO_DEPTH=256 -gADDR_BITS=64 > "$ct_build/elaboration.args"
    run_scenarios "$ct_build" tb_count_trunc COUNT_TRUNC_64 COUNT_MAX_64 RANGE_ZERO_COUNT RANGE_MAX_COUNT RANGE_TOP_COUNT \
        RANGE_LAST_BEAT_1 RANGE_LAST_BEAT_2 RANGE_LOW_CARRY_NONMAX RANGE_HIGH_MAX_CARRY_ZERO RANGE_HIGH_MAX_NO_CARRY \
        RANGE_HIGH_MAX_CARRY_NONZERO RANGE_ZERO_MISALIGNED
    echo "PASS element-count limit enforced at the counter width, not the address limit" | tee -a "$TASK_OUT/aggregate.log"
    echo "PASS 64-bit CONFIGURE range boundaries exercised" | tee -a "$TASK_OUT/aggregate.log"

    run_range_fallback_case() {
        local name=$1 input_bits=$2 scenario=$3 build="$TASK_OUT/build-tb_count_trunc-$1"
        build_design "$build" tb_count_trunc "$READ_RTL" "$TESTS/tb_count_trunc.sv" --
        printf '%s\n' -gB_MAX=16 -gMAX_OUTSTANDING=1 -gFIFO_DEPTH=256 -gADDR_BITS=64 "-gIN_BITS=$input_bits" > "$build/elaboration.args"
        run_scenarios "$build" tb_count_trunc "$scenario"
    }
    run_range_fallback_case narrow-in4 32 FALLBACK_NARROW_IN4
    run_range_fallback_case wide-in4 65 FALLBACK_WIDE_IN4
    run_range_fallback_case wide-zero-in4 65 FALLBACK_WIDE_ZERO
    echo "PASS CONFIGURE range fallback widths exercised" | tee -a "$TASK_OUT/aggregate.log"
fi

if [[ $SUITE == all || $SUITE == configure ]]; then
    "$TESTS/run_configure_read_pipeline.sh" "$TASK_OUT/configure-read-pipeline"
fi

echo "PASS burst engine RTL regression group=$SUITE. Artifacts retained at $TASK_OUT" | tee -a "$TASK_OUT/aggregate.log"
