#!/usr/bin/env bash
set -euo pipefail
ulimit -c 0

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd -P)
ROOT=$(realpath "$SCRIPT_DIR/../../../../")
source "$ROOT/panda_regressions/hls/bambu_axi_burst/parallel_helpers.sh"
EXTRACTOR="$SCRIPT_DIR/extract_write_engine.py"
TB="$SCRIPT_DIR/tb_write_burst_engine.sv"
XML="$ROOT/etc/libtech/NC_AXI_IPs.xml"
source "$SCRIPT_DIR/modelsim_helpers.sh"

modelsim_writer_compile() {
    local build=$1 top=$2 rtl=$3 tb=$4 plan_fifo=$5
    if [[ $plan_fifo == 1 ]]; then
        modelsim_compile "$build" "$top" "$rtl" "$tb" -- +define+PLAN_FIFO_PRESENT
    else
        modelsim_compile "$build" "$top" "$rtl" "$tb"
    fi
}

modelsim_writer_expect_fatal() {
    local log=$1 expected=$2 label=$3 status=$4 allow_elaboration=${5:-0} normalized="$1.normalized"
    modelsim_normalize_log "$log" "$normalized" || return $?
    # Runtime fatals use effective status 1; Questa reports constant parameter
    # checks during vopt elaboration with status 12. Only parameter tests opt in.
    if { (( status != 1 )) && { (( ! allow_elaboration || status != 12 )); }; } || \
       ! grep -Fq "** Fatal: $expected" "$normalized" || \
       grep -Eq '^WRITE_BURST_ENGINE PASS CASE=|^WRITE_BURST_ENGINE PARAM PASS ' "$normalized" || \
       [[ $(grep -Ec '^\*\* Fatal:' "$normalized" || true) != 1 ]] || \
       grep -Eq '^\*\* Error:|Segmentation fault|stack smashing|AddressSanitizer|Aborted' "$normalized"; then
        cat "$log" >&2
        echo "expected one named ModelSim fatal and no PASS marker for $label (status=$status): $expected" >&2
        return 1
    fi
}

if (($# > 1)); then
    echo "usage: $0 [fresh-output-directory]" >&2
    echo "  default: ./out_write_burst_engine, created in the directory the script is launched from" >&2
    exit 2
fi
if [[ ${BURST_WRITER_SIM_OUTSIDE_SANDBOX:-} != 1 ]]; then
    echo "refusing RTL simulation unless BURST_WRITER_SIM_OUTSIDE_SANDBOX=1 is set" >&2
    exit 2
fi
for tool in python3 sha256sum timeout grep sed tail tr; do
    if ! command -v "$tool" >/dev/null 2>&1; then
        echo "$tool is required" >&2
        exit 2
    fi
done
modelsim_resolve
MODELSIM_VERSION=$(modelsim_version) || exit 2
WRITER_GROUP_JOBS=${BURST_WRITER_GROUP_JOBS:-12}
if [[ ! $WRITER_GROUP_JOBS =~ ^[1-9][0-9]*$ ]]; then
    echo "BURST_WRITER_GROUP_JOBS must be a positive integer" >&2
    exit 2
fi
OUT_ARG=${1:-out_write_burst_engine}
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

python3 -B "$EXTRACTOR" --xml "$XML" --out "$TASK_OUT/MinimalAXI4MasterPipelinedWrite.sv" \
    | tee "$TASK_OUT/extract.log"
{
    printf 'xml='; sha256sum "$XML"
    printf 'extractor='; sha256sum "$EXTRACTOR"
    printf 'testbench='; sha256sum "$TB"
    printf 'modelsim_lookup_source=%s\nmodelsim_bin=%s\nvsim=%s\nvlib=%s\nvlog=%s\nvmap=%s\n' \
        "$MODELSIM_SOURCE" "$MODELSIM_BIN" "$MODELSIM_VSIM" \
        "$MODELSIM_VLIB" "$MODELSIM_VLOG" "$MODELSIM_VMAP"
    printf 'modelsim_version=%s\n' "$(printf '%s' "$MODELSIM_VERSION" | tr '\n' ' ')"
    printf 'timeout=%s\nwriter_group_jobs=%s\n' "$(command -v timeout)" "$WRITER_GROUP_JOBS"
    printf 'git_head='; git -C "$ROOT" rev-parse HEAD
    printf 'outside_sandbox_flag=%s\n' "$BURST_WRITER_SIM_OUTSIDE_SANDBOX"
} > "$TASK_OUT/provenance.txt"

build_run() {
    local label=$1 burst=$2 depth=$3 addr=$4 test_case=$5
    shift 5
    local fast_store_ack=0
    if [[ ${1:-} == --fast-store-ack=* ]]; then
        fast_store_ack=${1#*=}
        shift
    fi
    local build="$TASK_OUT/build-$label"
    modelsim_writer_compile "$build" tb_write_burst_engine \
        "$TASK_OUT/MinimalAXI4MasterPipelinedWrite.sv" "$TB" 1 || return $?
    local case_name log normalized status
    for case_name in "$test_case" "$@"; do
        log="$build/$case_name.log"
        status=0
        modelsim_run "$build" tb_write_burst_engine "$log" 60 \
            -gB_MAX="$burst" -gMAX_OUTSTANDING=1 -gFIFO_DEPTH="$depth" \
            -gADDR_BITS="$addr" -gFAST_STORE_ACK="$fast_store_ack" -- "+CASE=$case_name" || status=$?
        modelsim_normalize_log "$log" "$log.normalized" || return $?
        if (( status != 0 )); then
            cat "$log" >&2
            echo "RTL scenario failed: $label/$case_name" >&2
            return 1
        fi
        if ! grep -Eq "^WRITE_BURST_ENGINE PASS CASE=$case_name " "$log.normalized" &&
           ! grep -Eq "^WRITE_BURST_ENGINE PARAM PASS " "$log.normalized"; then
            cat "$log" >&2
            echo "missing exact PASS marker for $label/$case_name" >&2
            return 1
        fi
        if [[ $case_name == FAST_STORE_ACK_STREAM ]] &&
           ! grep -Eq '^WRITE_FAST_STORE_ACK_STREAM beats=64 interval=1$' "$log.normalized"; then
            cat "$log" >&2
            echo "missing fast STORE ACK throughput marker for $label/$case_name" >&2
            return 1
        fi
        if [[ $case_name == FAST_STORE_ACK_BACKPRESSURE ]] &&
           ! grep -Eq '^WRITE_FAST_STORE_ACK_BACKPRESSURE_PASS ' "$log.normalized"; then
            cat "$log" >&2
            echo "missing fast STORE pending-capture marker for $label/$case_name" >&2
            return 1
        fi
        cat "$log.normalized" >> "$RESULTS_LOG" || return $?
    done
}

build_run_o() {
    local label=$1 burst=$2 outstanding=$3 depth=$4 addr=$5 test_case=$6
    shift 6
    local fast_store_ack=0
    if [[ ${1:-} == --fast-store-ack=* ]]; then
        fast_store_ack=${1#*=}
        shift
    fi
    local build="$TASK_OUT/build-$label"
    modelsim_writer_compile "$build" tb_write_burst_engine \
        "$TASK_OUT/MinimalAXI4MasterPipelinedWrite.sv" "$TB" 1 || return $?
    local case_name log normalized status
    for case_name in "$test_case" "$@"; do
        log="$build/$case_name.log"
        status=0
        modelsim_run "$build" tb_write_burst_engine "$log" 60 \
            -gB_MAX="$burst" -gMAX_OUTSTANDING="$outstanding" -gFIFO_DEPTH="$depth" \
            -gADDR_BITS="$addr" -gFAST_STORE_ACK="$fast_store_ack" -- "+CASE=$case_name" || status=$?
        modelsim_normalize_log "$log" "$log.normalized" || return $?
        if (( status != 0 )); then
            cat "$log" >&2
            echo "RTL scenario failed: $label/$case_name" >&2
            return 1
        fi
        if ! grep -Eq "^WRITE_BURST_ENGINE PASS CASE=$case_name " "$log.normalized"; then
            cat "$log" >&2
            echo "missing exact PASS marker for $label/$case_name" >&2
            return 1
        fi
        if [[ $case_name == SUSTAINED_B1 ]] &&
           ! grep -Eq '^WRITE_SUSTAINED_PASS profile=B1-O16-F1 ' "$log.normalized"; then
            cat "$log" >&2
            echo "missing sustained-throughput marker for $label/$case_name" >&2
            return 1
        fi
        if [[ $case_name == SUSTAINED_B16 ]] &&
           ! grep -Eq '^WRITE_SUSTAINED_PASS profile=B16-O2-D32 ' "$log.normalized"; then
            cat "$log" >&2
            echo "missing sustained-throughput marker for $label/$case_name" >&2
            return 1
        fi
        if [[ $case_name == FAST_STORE_ACK_STREAM ]] &&
           ! grep -Eq '^WRITE_FAST_STORE_ACK_STREAM beats=64 interval=1$' "$log.normalized"; then
            cat "$log" >&2
            echo "missing fast STORE ACK throughput marker for $label/$case_name" >&2
            return 1
        fi
        if [[ $case_name == FAST_STORE_ACK_BACKPRESSURE ]] &&
           ! grep -Eq '^WRITE_FAST_STORE_ACK_BACKPRESSURE_PASS ' "$log.normalized"; then
            cat "$log" >&2
            echo "missing fast STORE pending-capture marker for $label/$case_name" >&2
            return 1
        fi
        cat "$log.normalized" >> "$RESULTS_LOG" || return $?
    done
}

build_async_reset_run() {
    local polarity=$1 active_low=$2 fast_store_ack=${3:-0}
    local label="ASYNC_RESET_${polarity}_FAST${fast_store_ack}"
    local source="$TASK_OUT/MinimalAXI4MasterPipelinedWrite-$label.sv"
    local build="$TASK_OUT/build-$label"
    mkdir -p "$build" || return $?
    python3 -B "$EXTRACTOR" --xml "$XML" --out "$source" \
        --reset-mode async --reset-polarity "$polarity" || return $?
    {
        printf 'variant=%s reset_mode=async reset_polarity=%s fast_store_ack=%s\n' \
            "$label" "$polarity" "$fast_store_ack"
        printf 'generated_hdl='; sha256sum "$source"
        printf 'xml='; sha256sum "$XML"
        printf 'extractor='; sha256sum "$EXTRACTOR"
        printf 'testbench='; sha256sum "$TB"
    } > "$build/provenance.txt" || return $?
    modelsim_writer_compile "$build" tb_write_burst_engine "$source" "$TB" 1 || return $?
    local log="$build/ASYNC_RESET.log"
    local normalized="$log.normalized" status=0
    modelsim_run "$build" tb_write_burst_engine "$log" 60 \
        -gB_MAX=16 -gMAX_OUTSTANDING=2 -gFIFO_DEPTH=32 -gADDR_BITS=32 \
        -gFAST_STORE_ACK="$fast_store_ack" -gASYNC_RESET=1 \
        -gRESET_ACTIVE_LOW="$active_low" -- +CASE=ASYNC_RESET || status=$?
    modelsim_normalize_log "$log" "$normalized" || return $?
    if (( status != 0 )); then
        cat "$log" >&2
        echo "RTL scenario failed: $label/ASYNC_RESET" >&2
        return 1
    fi
    if ! grep -Eq '^WRITE_BURST_ENGINE PASS CASE=ASYNC_RESET ' "$normalized"; then
        cat "$log" >&2
        echo "missing exact PASS marker for $label/ASYNC_RESET" >&2
        return 1
    fi
    cat "$normalized" >> "$RESULTS_LOG" || return $?
    printf 'ASYNC_RESET_VARIANT_PASS %s polarity=%s\n' "$label" "$polarity" >> "$RESULTS_LOG"
}

# Constant parameter checks can be rejected by Questa's vopt elaboration or
# at simulation time. Both require the unique named fatal diagnostic.
expect_parameter_fatal() {
    local label=$1 burst=$2 outstanding=$3 depth=$4 expected=$5
    local build="$TASK_OUT/negative-$label"
    modelsim_writer_compile "$build" tb_write_burst_engine \
        "$TASK_OUT/MinimalAXI4MasterPipelinedWrite.sv" "$TB" 0 || return $?
    local status=0
    modelsim_run "$build" tb_write_burst_engine "$build/run.log" 20 \
        -gB_MAX="$burst" -gMAX_OUTSTANDING="$outstanding" -gFIFO_DEPTH="$depth" \
        -gADDR_BITS=32 -- +CASE=PARAM || status=$?
    modelsim_writer_expect_fatal "$build/run.log" "$expected" "$label" "$status" 1 || return $?
    printf 'EXPECTED_PARAMETER_FATAL %s status=%s: %s\n' "$label" "$status" "$expected" >> "$RESULTS_LOG"
}

expect_mutation_failure() {
    local label=$1 source_pattern=$2 replacement=$3 burst=$4 outstanding=$5 depth=$6
    local addr=$7 test_case=$8 expected=$9 fast_store_ack=${10:-0}
    local mutant="$TASK_OUT/mutant-$label.sv"
    local build="$TASK_OUT/mutation-$label"
    python3 - "$TASK_OUT/MinimalAXI4MasterPipelinedWrite.sv" "$mutant" \
        "$source_pattern" "$replacement" <<'PY' || return $?
from pathlib import Path
import sys

source, target, needle, replacement = sys.argv[1:]
text = Path(source).read_text()
if text.count(needle) != 1:
    raise SystemExit(f"expected exactly one mutation target, found {text.count(needle)}")
Path(target).write_text(text.replace(needle, replacement))
PY
    mkdir -p "$build" || return $?
    modelsim_writer_compile "$build" tb_write_burst_engine "$mutant" "$TB" 0 || return $?
    local status=0
    modelsim_run "$build" tb_write_burst_engine "$build/run.log" 60 \
        -gB_MAX="$burst" -gMAX_OUTSTANDING="$outstanding" -gFIFO_DEPTH="$depth" \
        -gADDR_BITS="$addr" -gFAST_STORE_ACK="$fast_store_ack" -- "+CASE=$test_case" || status=$?
    modelsim_writer_expect_fatal "$build/run.log" "$expected" "$label" "$status" || return $?
    printf 'MUTATION_REJECTED %s status=%s: %s\n' "$label" "$status" "$expected" >> "$RESULTS_LOG"
}

run_writer_group() {
    local group=$1
    local RESULTS_LOG="$TASK_OUT/group-results/$group.log"
    : > "$RESULTS_LOG" || return $?
    case "$group" in
        core-B16-D16)
            build_run B16-D16 16 16 32 CORE BOUNDARY W_BEFORE_AW AW_BEFORE_W STALLS \
                CFG_SIZE_CHANGE CFG_COUNT_CHANGE CONFIG_PIPELINE_VALID CONFIG_PIPELINE_INVALID \
                BAD_CONFIG_SIZE BAD_CONFIG_ALIGN COUNT_MSB OVERFLOW32 BAD_STORE_SIZE \
                BAD_STORE_SIZE8 BAD_STORE_ADDRESS EARLY_DRAIN BID BRESP MAX_COUNT \
                PENDING_STORE PENDING_OPERAND_CHANGE PENDING_OPERAND_CHANGE_ON_FREE \
                PENDING_OPERAND_CHANGE_WITH_ROOM PENDING_OPERAND_CHANGE_AT_COMMIT \
                TRANSIENT_DRAIN_RESTORE TRANSIENT_CONFIGURE_PENDING \
                DRAIN_PENDING_FULL_COUNT DRAIN_PENDING_SHORT_COUNT STORE_ACK_PULSE DUPLICATE_START \
                DRAIN_BEFORE_B DRAIN_ONE_SHOT PREMATURE_CONFIGURE INIT_DRAIN
            ;;
        core-B16-D32)
            # CFG_SIZE_CHANGE and CFG_COUNT_CHANGE also run here: the configure-size and
            # configure-count-width mutations run at FIFO_DEPTH=32, so the clean comparison has
            # to be at the same parameters.
            build_run B16-D32 16 32 32 CORE BRESP_NO_NEXT_DESCRIPTOR CFG_SIZE_CHANGE CFG_COUNT_CHANGE
            ;;
        core-B1-D1)
            build_run B1-D1 1 1 32 CORE CONFIG_PIPELINE_B1
            ;;
        core-B1-O16)
            build_run_o B1-O16 1 16 1 32 PLAN_FIFO_WITNESS SUSTAINED_B1
            ;;
        core-B3-D4)
            build_run B3-D4 3 4 32 CORE
            ;;
        core-B16-D4096)
            build_run B16-D4096 16 4096 32 CORE
            ;;
        core-B16-A32-F3)
            build_run B16-A32-F3 16 32 32 MULTIPAGE PREMATURE_CONFIGURE_B_PENDING \
                PREMATURE_CONFIGURE_B_DRAINS_E0
            ;;
        multi-O2-D16)
            build_run_o O2-D16 16 2 16 32 MULTI_OUTSTANDING TWO_AW_BEFORE_B
            ;;
        multi-O2-D32)
            build_run_o O2-D32 16 2 32 32 INDEPENDENT_CURSORS BRESP_MULTI PEER_FAULT_SPECULATIVE SUSTAINED_B16
            ;;
        multi-O3-D16)
            build_run_o O3-D16 16 3 16 32 MULTI_OUTSTANDING
            ;;
        multi-O16-D16)
            build_run_o O16-D16 16 16 16 32 MULTI_OUTSTANDING
            ;;
        multi-O2-D32-F3)
            build_run_o O2-D32-F3 16 2 32 32 BRESP_AW_W_HELD BRESP_W_BEFORE_AW
            ;;
        address-B16-A64)
            build_run B16-A64 16 32 64 OVERFLOW64 MAX_COUNT CFG_BASE_CHANGE COUNT_MAX_64 \
                CONFIGURE64_ZERO_FFFC CONFIGURE64_END_2P64 CONFIGURE64_CARRY_NONMAX \
                CONFIGURE64_HIGH_MAX_NO_CARRY CONFIGURE64_BAD_ALIGN_ZERO COUNT_MSB
            ;;
        limit-B255-D256)
            build_run B255-D256 255 256 32 CORE
            ;;
        limit-B256-O16)
            build_run_o B256-O16 256 16 256 32 CORE
            ;;
        limit-B256-D256)
            build_run B256-D256 256 256 32 PARAM
            ;;
        maxlen-B256-O2)
            build_run_o B256-O2-MAXLEN 256 2 512 32 MAX_BURST_LENGTH
            ;;
        async-reset-active-low)
            build_async_reset_run active-low 1
            ;;
        async-reset-active-high)
            build_async_reset_run active-high 0
            ;;
        invalid-B0)
            expect_parameter_fatal B0 0 1 1 "B_MAX must be in [1,256]"
            ;;
        invalid-B257)
            expect_parameter_fatal B257 257 1 512 "B_MAX must be in [1,256]"
            ;;
        invalid-O0)
            expect_parameter_fatal O0 16 0 16 "MAX_OUTSTANDING must be in [1,16]"
            ;;
        invalid-O17)
            expect_parameter_fatal O17 16 17 32 "MAX_OUTSTANDING must be in [1,16]"
            ;;
        invalid-FIFO-BELOW-B)
            expect_parameter_fatal FIFO_BELOW_B 16 1 8 "FIFO_DEPTH must be a power of two >= B_MAX and <= 4096"
            ;;
        invalid-FIFO-NONPOWER)
            expect_parameter_fatal FIFO_NONPOWER 3 1 3 "FIFO_DEPTH must be a power of two >= B_MAX and <= 4096"
            ;;
        mutation-data-order)
            expect_mutation_failure data-order "assign m_axi_wdata = fifo_head;" \
                "assign m_axi_wdata = ~fifo_head;" 16 1 32 32 CORE "WRITE_DATA_MISMATCH"
            ;;
        mutation-last-order)
            expect_mutation_failure last-order "assign m_axi_wlast = w_valid && (descriptor_w_remaining_reg[descriptor_w_ptr_reg] == 1);" \
                "assign m_axi_wlast = w_valid && (descriptor_w_remaining_reg[descriptor_w_ptr_reg] != 1);" \
                16 1 32 32 CORE "WLAST_MISMATCH"
            ;;
        mutation-serialize-until-b)
            expect_mutation_failure serialize-until-b "(descriptor_count_reg >= MAX_OUTSTANDING)" \
                "((descriptor_count_reg >= MAX_OUTSTANDING) || (descriptor_count_reg != 0))" \
                16 2 16 32 TWO_AW_BEFORE_B "F2_TWO_AW_BEFORE_FIRST_B_REQUIRED"
            ;;
        mutation-count-msb-guard)
            expect_mutation_failure count-msb-guard "(in5[COUNT_BITS-1] == 1'b0) &&" \
                "" 16 1 32 64 COUNT_MSB "F3_COUNT_MSB_GUARD_INSTALLED_CONTEXT_OR_TRAFFIC"
            ;;
        mutation-configure-size)
            expect_mutation_failure configure-size "in2 != cfg_size_reg" \
                "1'b0" 16 1 32 32 CFG_SIZE_CHANGE "CONFIGURE_CHANGE_NOT_REJECTED operand=0"
            ;;
        mutation-configure-base-low32)
            expect_mutation_failure configure-base-low32 "in4 != cfg_base_reg" \
                "in4[31:0] != cfg_base_reg[31:0]" 16 1 32 64 CFG_BASE_CHANGE "CONFIGURE_CHANGE_NOT_REJECTED operand=1"
            ;;
        mutation-configure-count-width)
            expect_mutation_failure configure-count-width "in5 != cfg_count_reg" \
                "in5[COUNT_W-1:0] != cfg_count_reg[COUNT_W-1:0]" \
                16 1 32 32 CFG_COUNT_CHANGE "CONFIGURE_CHANGE_NOT_REJECTED operand=2"
            ;;
        fast-store-ack)
            build_run FAST-B16-D16 16 16 32 FAST_STORE_ACK_INVALID --fast-store-ack=1 \
                FAST_STORE_ACK_BFAULT FAST_STORE_ACK_ZERO_REGION STORE_ACK_PULSE \
                PENDING_STORE PENDING_OPERAND_CHANGE PENDING_OPERAND_CHANGE_ON_FREE \
                PENDING_OPERAND_CHANGE_WITH_ROOM PENDING_OPERAND_CHANGE_AT_COMMIT \
                TRANSIENT_DRAIN_RESTORE DRAIN_PENDING_FULL_COUNT DRAIN_PENDING_SHORT_COUNT \
                DRAIN_BEFORE_B DRAIN_ONE_SHOT FAST_STORE_ACK_BACKPRESSURE
            ;;
        fast-store-ack-address64)
            build_run FAST-A64 16 32 64 FAST_STORE_ACK_STREAM --fast-store-ack=1 \
                CONFIGURE64_ZERO_FFFC CONFIGURE64_END_2P64 CONFIGURE64_CARRY_NONMAX \
                CONFIGURE64_HIGH_MAX_NO_CARRY CONFIGURE64_BAD_ALIGN_ZERO \
                FAST_STORE_ACK_CARRY FAST_STORE_ACK_WRAP \
                COUNT_MSB OVERFLOW64
            ;;
        fast-store-ack-stream)
            build_run_o FAST-STREAM-O2-D32 16 2 32 32 FAST_STORE_ACK_STREAM --fast-store-ack=1
            ;;
        fast-peer-fault)
            build_run_o FAST-O2-D32 16 2 32 32 PEER_FAULT_SPECULATIVE --fast-store-ack=1
            ;;
        fast-async-reset)
            build_async_reset_run active-low 1 1
            ;;
        mutation-fast-store-ack)
            expect_mutation_failure fast-store-ack-disabled \
                "assign fast_store_accept = (FAST_STORE_ACK != 0) && store_input && !store_pending_reg &&" \
                "assign fast_store_accept = 1'b0 && store_input && !store_pending_reg &&" \
                16 2 32 32 FAST_STORE_ACK_STREAM "FAST_STORE_ACK_DIRECT_ACK_MISSING" 1
            ;;
        mutation-fast-store-ack-pending)
            expect_mutation_failure fast-store-ack-pending-capture \
                "wire capture_store_pending = (FAST_STORE_ACK == 0) ? (fifo_count_reg == FIFO_DEPTH) : !fast_store_accept;" \
                "wire capture_store_pending = fifo_count_reg == FIFO_DEPTH;" \
                16 2 32 32 FAST_STORE_ACK_BACKPRESSURE "FAST_STORE_ACK_BACKPRESSURE" 1
            ;;
        mutation-fast-store-ack-credit)
            expect_mutation_failure fast-store-ack-credit-pending \
                "(fast_pipe_valid | store_pending_reg)}) < FIFO_SLOT_LIMIT;" \
                "(fast_pipe_valid)}) < FIFO_SLOT_LIMIT;" \
                16 2 32 32 FAST_STORE_ACK_BACKPRESSURE "FAST_STORE_ACK_BACKPRESSURE" 1
            ;;
        configure-pipeline)
            "$SCRIPT_DIR/run_configure_writer_pipeline.sh" "$TASK_OUT/configure-writer-pipeline" \
                | tee -a "$RESULTS_LOG"
            ;;
        *)
            echo "internal error: unknown writer test group '$group'" >&2
            return 2
            ;;
    esac
}

WRITER_GROUPS=(core-B16-D16 core-B16-D32 core-B1-D1 core-B1-O16 core-B3-D4 \
    core-B16-D4096 core-B16-A32-F3 multi-O2-D16 multi-O2-D32 multi-O3-D16 \
    multi-O16-D16 multi-O2-D32-F3 address-B16-A64 \
    limit-B255-D256 limit-B256-O16 limit-B256-D256 maxlen-B256-O2 async-reset-active-low \
    async-reset-active-high invalid-B0 invalid-B257 invalid-O0 invalid-O17 \
    invalid-FIFO-BELOW-B invalid-FIFO-NONPOWER mutation-data-order \
    mutation-last-order mutation-serialize-until-b mutation-count-msb-guard \
    mutation-configure-size mutation-configure-base-low32 mutation-configure-count-width \
    configure-pipeline fast-store-ack fast-store-ack-stream fast-peer-fault fast-async-reset \
    fast-store-ack-address64 mutation-fast-store-ack mutation-fast-store-ack-pending \
    mutation-fast-store-ack-credit)
if [[ -n ${BURST_WRITER_GROUP_FILTER:-} ]]; then
    IFS=, read -r -a WRITER_GROUPS <<< "$BURST_WRITER_GROUP_FILTER"
    for group in "${WRITER_GROUPS[@]}"; do
        if [[ ! $group =~ ^[a-zA-Z0-9-]+$ ]]; then
            echo "invalid writer group filter entry: $group" >&2
            exit 2
        fi
        case "$group" in
            core-B16-D16|core-B16-D32|core-B1-D1|core-B1-O16|core-B3-D4|core-B16-D4096|\
            core-B16-A32-F3|multi-O2-D16|multi-O2-D32|multi-O3-D16|multi-O16-D16|\
            multi-O2-D32-F3|address-B16-A64|limit-B255-D256|\
            limit-B256-O16|limit-B256-D256|maxlen-B256-O2|async-reset-active-low|async-reset-active-high|\
            invalid-B0|invalid-B257|invalid-O0|invalid-O17|invalid-FIFO-BELOW-B|\
            invalid-FIFO-NONPOWER|mutation-data-order|mutation-last-order|\
            mutation-serialize-until-b|mutation-count-msb-guard|mutation-configure-size|\
            mutation-configure-base-low32|mutation-configure-count-width|configure-pipeline|\
            fast-store-ack|fast-store-ack-address64|fast-store-ack-stream|fast-peer-fault|fast-async-reset|\
            mutation-fast-store-ack|mutation-fast-store-ack-pending|\
            mutation-fast-store-ack-credit) ;;
            *) echo "unknown writer group filter entry: $group" >&2; exit 2 ;;
        esac
    done
fi
if [[ -n ${BURST_WRITER_GROUP_FILTER:-} ]]; then
    printf 'writer_group_run=filtered\nwriter_group_filter=%s\nwriter_groups=' \
        "$BURST_WRITER_GROUP_FILTER" >> "$TASK_OUT/provenance.txt"
    printf '%s\n' "$(IFS=,; echo "${WRITER_GROUPS[*]}")" >> "$TASK_OUT/provenance.txt"
else
    printf 'writer_group_run=full\nwriter_group_filter=<unset>\nwriter_groups=' \
        >> "$TASK_OUT/provenance.txt"
    printf '%s\n' "$(IFS=,; echo "${WRITER_GROUPS[*]}")" >> "$TASK_OUT/provenance.txt"
fi
BURST_JOB_LOG_DIR="$TASK_OUT/job-logs"
mkdir -p "$TASK_OUT/group-results" "$BURST_JOB_LOG_DIR"
if ! run_bounded_jobs "$WRITER_GROUP_JOBS" run_writer_group "${WRITER_GROUPS[@]}"; then
    echo "writer RTL test group failed; diagnostics retained at $TASK_OUT/job-logs" >&2
    exit 1
fi

: > "$TASK_OUT/results.log"
for group in "${WRITER_GROUPS[@]}"; do
    cat "$TASK_OUT/group-results/$group.log" >> "$TASK_OUT/results.log"
done

if [[ -n ${BURST_WRITER_GROUP_FILTER:-} ]]; then
    echo "WRITE_BURST_ENGINE_SUBSET_PASS: filter=$BURST_WRITER_GROUP_FILTER output=$TASK_OUT"
else
    echo "WRITE_BURST_ENGINE_QUALIFY_PASS: $TASK_OUT"
fi
