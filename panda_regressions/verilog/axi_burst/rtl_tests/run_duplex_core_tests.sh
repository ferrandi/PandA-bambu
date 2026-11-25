#!/usr/bin/env bash
set -Eeuo pipefail
ulimit -c 0

SCRIPT_DIR=$(cd -- "$(dirname -- "$0")" && pwd -P)
ROOT=$(realpath "$SCRIPT_DIR/../../../../")
EXTRACTOR="$SCRIPT_DIR/extract_write_engine.py"
TB="$SCRIPT_DIR/tb_duplex_cores.sv"
XML="$ROOT/etc/libtech/NC_AXI_IPs.xml"
source "$SCRIPT_DIR/modelsim_helpers.sh"

if (($# != 1)); then
    echo "usage: $0 fresh-output-directory" >&2
    exit 2
fi
if [[ ${BURST_DUPLEX_SIM_OUTSIDE_SANDBOX:-} != 1 ]]; then
    echo "refusing RTL simulation unless BURST_DUPLEX_SIM_OUTSIDE_SANDBOX=1 is set" >&2
    exit 2
fi
for tool in vsim vlib vlog vmap; do
    command -v "$tool" >/dev/null 2>&1 || { echo "$tool is required on PATH" >&2; exit 2; }
done
modelsim_resolve

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
mkdir "$TASK_OUT/build" "$TASK_OUT/tmp"
export TMPDIR="$TASK_OUT/tmp" TMP="$TASK_OUT/tmp" TEMP="$TASK_OUT/tmp"
trap 'rm -rf -- "$TASK_OUT/tmp"' EXIT

python3 -B "$EXTRACTOR" --xml "$XML" --target MinimalAXI4MasterPipelined \
    --out "$TASK_OUT/MinimalAXI4MasterPipelined.sv" | tee "$TASK_OUT/extract-reader.log"
python3 -B "$EXTRACTOR" --xml "$XML" --target MinimalAXI4MasterPipelinedWrite \
    --without-dependencies --out "$TASK_OUT/MinimalAXI4MasterPipelinedWrite.sv" | tee "$TASK_OUT/extract-writer.log"
{
    printf 'git_head='; git -C "$ROOT" rev-parse HEAD
    printf 'xml='; sha256sum "$XML"
    printf 'extractor='; sha256sum "$EXTRACTOR"
    printf 'testbench='; sha256sum "$TB"
    printf 'runner='; sha256sum "$SCRIPT_DIR/run_duplex_core_tests.sh"
    printf 'modelsim_source=%s\nmodelsim_bin=%s\nvsim=%s\nvlib=%s\nvlog=%s\nvmap=%s\n' \
        "$MODELSIM_SOURCE" "$MODELSIM_BIN" "$MODELSIM_VSIM" "$MODELSIM_VLIB" "$MODELSIM_VLOG" "$MODELSIM_VMAP"
    printf 'modelsim_version=%s\n' "$(modelsim_version | tr '\n' ' ')"
    printf 'peer_fault_scope=peer-fault-core-prerequisite-not-production-wrapper-G1\n'
    printf 'profiles=PEER_FAULT_TEST:0(base scenarios),1(peer-fault scenarios); '
    printf 'FAST_STORE_ACK:0(legacy acknowledgement),1(always-on acknowledgement)\n'
} > "$TASK_OUT/provenance.txt"

compile_profile() {
    local peer_fault_test=$1
    local fast_store_ack=$2
    local profile="pf${peer_fault_test}-fast${fast_store_ack}"
    local build="$TASK_OUT/build/profile-$profile"
    modelsim_compile "$build" tb_duplex_cores \
        "$TASK_OUT/MinimalAXI4MasterPipelined.sv" \
        "$TASK_OUT/MinimalAXI4MasterPipelinedWrite.sv" "$TB" || {
            echo "duplex-core RTL compile failed for $profile" >&2
            exit 1
        }
    printf '%s\n' "profile_$profile=$build" >> "$TASK_OUT/provenance.txt"
}

run_profile_cases() {
    local peer_fault_test=$1
    local fast_store_ack=$2
    local profile="pf${peer_fault_test}-fast${fast_store_ack}"
    shift 2
    local build="$TASK_OUT/build/profile-$profile"
    local case_name log normalized status fault_arg
    local -a overrides=(
        -gREAD_B_MAX=16 -gREAD_OUTSTANDING=3 -gREAD_FIFO_DEPTH=64
        -gWRITE_B_MAX=8 -gWRITE_OUTSTANDING=2 -gWRITE_FIFO_DEPTH=32
        "-gPEER_FAULT_TEST=$peer_fault_test" "-gFAST_STORE_ACK=$fast_store_ack"
    )
    for case_name in "$@"; do
        log="$TASK_OUT/${case_name}-${profile}.log"
        normalized="$log.normalized"
        status=0
        fault_arg=()
        case $case_name in
            RRESP_FAULT_LOCAL|BRESP_FAULT_LOCAL|PEER_BRESP_DRAIN|PEER_RRESP_RESERVED_WRITE_DRAIN|PEER_RRESP_FINAL_BEAT|PEER_FAULT_BLOCKS_RESERVATION|PEER_FAULT_RESET_RESTART)
                fault_arg=(+AXI_BURST_EXPECT_FAULT) ;;
        esac
        modelsim_run "$build" tb_duplex_cores "$log" 90 "${overrides[@]}" -- "+CASE=$case_name" "${fault_arg[@]}" || status=$?
        modelsim_normalize_log "$log" "$normalized"
        if (( status != 0 )); then
            cat "$log" >&2
            echo "duplex-core scenario failed: $case_name (profile=$profile)" >&2
            exit 1
        fi
        if ! grep -Eq "^DUPLEX_CORES PASS CASE=$case_name " "$normalized"; then
            cat "$normalized" >&2
            echo "missing exact PASS marker for $case_name" >&2
            exit 1
        fi
        local -a expected_faults=()
        local base_instance=tb_duplex_cores
        case $case_name in
            RRESP_FAULT_LOCAL) expected_faults=("AXI_BURST_FAULT engine=read reason=RRESP instance=${base_instance}.reader.report_axi_read_diagnostic") ;;
            BRESP_FAULT_LOCAL) expected_faults=("AXI_BURST_FAULT engine=write reason=BRESP instance=${base_instance}.writer.report_axi_write_diagnostic") ;;
            PEER_BRESP_DRAIN)
                expected_faults=("AXI_BURST_FAULT engine=write reason=BRESP instance=${base_instance}.writer.report_axi_write_diagnostic"
                                 "AXI_BURST_PEER_FAULT engine=read reason=peer instance=${base_instance}.reader.report_axi_read_diagnostic"
                                 "AXI_BURST_PEER_FAULT engine=write reason=peer instance=${base_instance}.writer.report_axi_write_diagnostic") ;;
            PEER_RRESP_RESERVED_WRITE_DRAIN|PEER_RRESP_FINAL_BEAT)
                expected_faults=("AXI_BURST_FAULT engine=read reason=RRESP instance=${base_instance}.reader.report_axi_read_diagnostic"
                                 "AXI_BURST_PEER_FAULT engine=write reason=peer instance=${base_instance}.writer.report_axi_write_diagnostic"
                                 "AXI_BURST_PEER_FAULT engine=read reason=peer instance=${base_instance}.reader.report_axi_read_diagnostic") ;;
            PEER_FAULT_BLOCKS_RESERVATION)
                expected_faults=("AXI_BURST_PEER_FAULT engine=read reason=peer instance=${base_instance}.reader.report_axi_read_diagnostic"
                                 "AXI_BURST_PEER_FAULT engine=write reason=peer instance=${base_instance}.writer.report_axi_write_diagnostic") ;;
            PEER_FAULT_RESET_RESTART) expected_faults=("AXI_BURST_PEER_FAULT engine=write reason=peer instance=${base_instance}.writer.report_axi_write_diagnostic" "AXI_BURST_PEER_FAULT engine=read reason=peer instance=${base_instance}.reader.report_axi_read_diagnostic") ;;
        esac
        local -a observed_faults=()
        mapfile -t observed_faults < <(sed -E 's/^\*\* Fatal: //' "$normalized" | grep -E '^AXI_BURST_' || true)
        if ((${#observed_faults[@]} != ${#expected_faults[@]})); then
            echo "wrong AXI diagnostic count for $case_name (profile=$profile): expected=${#expected_faults[@]} observed=${#observed_faults[@]}" >&2
            cat "$normalized" >&2; exit 1
        fi
        local i record
        for i in "${!expected_faults[@]}"; do
            record=${observed_faults[i]}
            [[ $record == "${expected_faults[i]}"* ]] || { echo "wrong AXI diagnostic order/content for $case_name at $i: $record" >&2; exit 1; }
            if [[ ${expected_faults[i]} == *engine=read* ]]; then
                [[ $record =~ opcode=-?[0-9]+\ size=-?[0-9]+\ address=[[:xdigit:]]+\ count=[0-9]+\ resp=[01]{2}\ id=-?[0-9]+\ last=-?[0-9]+\ descriptors=[0-9]+\ remaining=[0-9]+$ ]] || { echo "malformed read fault payload for $case_name" >&2; exit 1; }
            else
                [[ $record =~ opcode=-?[0-9]+\ size=-?[0-9]+\ address=[[:xdigit:]]+\ count=[0-9]+\ payload=[[:xdigit:]]+\ resp=[01]{2}\ id=-?[0-9]+\ descriptors=[0-9]+\ remaining=[0-9]+$ ]] || { echo "malformed write fault payload for $case_name" >&2; exit 1; }
            fi
        done
        cat "$normalized" >> "$TASK_OUT/results.log"
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
