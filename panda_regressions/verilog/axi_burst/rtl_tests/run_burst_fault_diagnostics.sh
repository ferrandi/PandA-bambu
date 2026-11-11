#!/usr/bin/env bash
set -euo pipefail
ulimit -c 0
SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd -P)
ROOT=$(realpath "$SCRIPT_DIR/../../../../")
source "$SCRIPT_DIR/modelsim_helpers.sh"
EXTRACTOR="$SCRIPT_DIR/extract_write_engine.py"
TB="$SCRIPT_DIR/tb_burst_fault_diagnostics.sv"
XML="$ROOT/etc/libtech/NC_AXI_IPs.xml"
if (($# != 1)); then echo "usage: $0 fresh-output-directory" >&2; exit 2; fi
if [[ ${BURST_WRITER_SIM_OUTSIDE_SANDBOX:-} != 1 ]]; then echo "set BURST_WRITER_SIM_OUTSIDE_SANDBOX=1 for authorized ModelSim execution" >&2; exit 2; fi
OUT=$(realpath -m -- "$1")
[[ ! -e $OUT && ! -L $1 ]] || { echo "output must be fresh: $OUT" >&2; exit 2; }
mkdir -p "$OUT/tmp"
export TMPDIR="$OUT/tmp" TMP="$OUT/tmp" TEMP="$OUT/tmp"
cleanup() { rm -rf -- "$OUT/tmp"; }
trap cleanup EXIT
modelsim_resolve
VERSION=$(modelsim_version)
python3 -B "$EXTRACTOR" --xml "$XML" --target MinimalAXI4MasterPipelined --out "$OUT/reader.sv"
python3 -B "$EXTRACTOR" --xml "$XML" --target MinimalAXI4MasterPipelinedWrite --out "$OUT/writer.sv"
{
    printf 'xml='; sha256sum "$XML"
    printf 'extractor='; sha256sum "$EXTRACTOR"
    printf 'testbench='; sha256sum "$TB"
    printf 'modelsim_source=%s\nmodelsim_bin=%s\nmodelsim_version=%s\n' "$MODELSIM_SOURCE" "$MODELSIM_BIN" "${VERSION//$'\n'/ }"
    printf 'git_head='; git -C "$ROOT" rev-parse HEAD
} > "$OUT/provenance.txt"
BUILD="$OUT/build"
modelsim_compile "$BUILD" tb_burst_fault_diagnostics "$OUT/reader.sv" "$OUT/writer.sv" "$TB"

run_case() {
    local name=$1 mode=$2 expected=$3; shift 3
    local log="$BUILD/$name.log" status=0 test_read=1
    [[ $name == WRITE_* ]] && test_read=0
    modelsim_run "$BUILD" tb_burst_fault_diagnostics "$log" 30 -gTEST_READ="$test_read" -- "+CASE=$name" "$@" || status=$?
    if [[ $name == READ_REARM ]]; then
        modelsim_expect_fault_log "$log" "$mode" "$name" "$expected" "$expected"
    elif [[ $name == READ_IGNORED_THEN_FAULT ]]; then
        modelsim_expect_fault_log "$log" "$mode" "$name" \
            'AXI_BURST_COMMAND_IGNORED engine=read reason=UNSUPPORTED_OPCODE instance=tb_burst_fault_diagnostics.reader_dut.report_axi_read_diagnostic opcode=3 size=32 address=00001000 count=1 resp=00 id=0 last=0 descriptors=0 remaining=0' \
            'AXI_BURST_FAULT engine=read reason=CONFIGURE_INVALID instance=tb_burst_fault_diagnostics.reader_dut.report_axi_read_diagnostic opcode=2 size=8 address=00001000 count=1 resp=00 id=0 last=0 descriptors=0 remaining=0'
    else
        modelsim_expect_fault_log "$log" "$mode" "$name" "$expected"
    fi
    if [[ $mode == continue ]] && ! grep -Fq "BURST_DIAGNOSTICS_PASS case=$name" "${log}.fault-normalized"; then
        cat "$log" >&2; echo "missing continuation marker: $name" >&2; return 1
    fi
    printf '%s raw=%s effective=%s\n' "$name" "$(<"$log.raw-status")" "$(<"$log.status")" >> "$OUT/statuses.txt"
}

# ModelSim fatal text is checked exactly, including the engine, cause and stable
# accepted/configuration payload. The instance path is a stable bench hierarchy.
run_case READ_CONFIG_FATAL fatal 'AXI_BURST_FAULT engine=read reason=CONFIGURE_INVALID instance=tb_burst_fault_diagnostics.reader_dut.report_axi_read_diagnostic opcode=2 size=8 address=00001000 count=1 resp=00 id=0 last=0 descriptors=0 remaining=0'
run_case READ_CONFIG_CONTINUE continue 'AXI_BURST_FAULT engine=read reason=CONFIGURE_INVALID instance=tb_burst_fault_diagnostics.reader_dut.report_axi_read_diagnostic opcode=2 size=8 address=00001000 count=1 resp=00 id=0 last=0 descriptors=0 remaining=0' +AXI_BURST_EXPECT_FAULT
run_case READ_REARM continue 'AXI_BURST_FAULT engine=read reason=CONFIGURE_INVALID instance=tb_burst_fault_diagnostics.reader_dut.report_axi_read_diagnostic opcode=2 size=8 address=00001000 count=1 resp=00 id=0 last=0 descriptors=0 remaining=0' +AXI_BURST_EXPECT_FAULT
run_case WRITE_CONFIG_FATAL fatal 'AXI_BURST_FAULT engine=write reason=CONFIGURE_INVALID instance=tb_burst_fault_diagnostics.writer_dut.report_axi_write_diagnostic opcode=2 size=8 address=00001000 count=1 payload=00000000 resp=00 id=0 descriptors=0 remaining=0'
run_case WRITE_CONFIG_CONTINUE continue 'AXI_BURST_FAULT engine=write reason=CONFIGURE_INVALID instance=tb_burst_fault_diagnostics.writer_dut.report_axi_write_diagnostic opcode=2 size=8 address=00001000 count=1 payload=00000000 resp=00 id=0 descriptors=0 remaining=0' +AXI_BURST_EXPECT_FAULT
run_case READ_IGNORED continue 'AXI_BURST_COMMAND_IGNORED engine=read reason=UNSUPPORTED_OPCODE instance=tb_burst_fault_diagnostics.reader_dut.report_axi_read_diagnostic opcode=3 size=32 address=00001000 count=1 resp=00 id=0 last=0 descriptors=0 remaining=0' +AXI_BURST_EXPECT_FAULT
run_case WRITE_IGNORED continue 'AXI_BURST_COMMAND_IGNORED engine=write reason=UNSUPPORTED_OPCODE instance=tb_burst_fault_diagnostics.writer_dut.report_axi_write_diagnostic opcode=0 size=32 address=00001000 count=1 payload=00000000 resp=00 id=0 descriptors=0 remaining=0' +AXI_BURST_EXPECT_FAULT
run_case READ_IGNORED_THEN_FAULT continue 'unused' +AXI_BURST_EXPECT_FAULT
run_case READ_PEER continue 'AXI_BURST_PEER_FAULT engine=read reason=peer instance=tb_burst_fault_diagnostics.reader_dut.report_axi_read_diagnostic opcode=-1 size=0 address=00000000 count=0 resp=00 id=0 last=0 descriptors=0 remaining=0' +AXI_BURST_EXPECT_FAULT
run_case WRITE_PEER continue 'AXI_BURST_PEER_FAULT engine=write reason=peer instance=tb_burst_fault_diagnostics.writer_dut.report_axi_write_diagnostic opcode=-1 size=0 address=00000000 count=0 payload=00000000 resp=00 id=0 descriptors=0 remaining=0' +AXI_BURST_EXPECT_FAULT
run_case READ_RRESP_IGNORED_FATAL fatal 'AXI_BURST_FAULT engine=read reason=RRESP instance=tb_burst_fault_diagnostics.reader_dut.report_axi_read_diagnostic opcode=-1 size=0 address=00001000 count=1 resp=10 id=0 last=1 descriptors=1 remaining=1'
run_case READ_RRESP_IGNORED_CONTINUE continue 'AXI_BURST_FAULT engine=read reason=RRESP instance=tb_burst_fault_diagnostics.reader_dut.report_axi_read_diagnostic opcode=-1 size=0 address=00001000 count=1 resp=10 id=0 last=1 descriptors=1 remaining=1' +AXI_BURST_EXPECT_FAULT
run_case WRITE_BRESP_IGNORED_FATAL fatal 'AXI_BURST_FAULT engine=write reason=BRESP instance=tb_burst_fault_diagnostics.writer_dut.report_axi_write_diagnostic opcode=-1 size=0 address=00001000 count=1 payload=00000000 resp=10 id=0 descriptors=1 remaining=0'
run_case WRITE_BRESP_IGNORED_CONTINUE continue 'AXI_BURST_FAULT engine=write reason=BRESP instance=tb_burst_fault_diagnostics.writer_dut.report_axi_write_diagnostic opcode=-1 size=0 address=00001000 count=1 payload=00000000 resp=10 id=0 descriptors=1 remaining=0' +AXI_BURST_EXPECT_FAULT
echo "AXI_BURST_DIAGNOSTICS_RUN_PASS output=$OUT"
