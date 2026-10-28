#!/usr/bin/env bash
set -Eeuo pipefail
ulimit -c 0

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)
ROOT=$(realpath "$SCRIPT_DIR/../../../../")
XML="$ROOT/etc/libtech/NC_AXI_IPs.xml"
source "$SCRIPT_DIR/modelsim_helpers.sh"

if (($# != 1)) || [[ -e $1 || -L $1 ]]; then
    echo "usage: $0 fresh-output-directory" >&2
    exit 2
fi
TASK_OUT=$(realpath -m "$1")
mkdir -p "$TASK_OUT/tmp"
export TMPDIR="$TASK_OUT/tmp" TMP="$TASK_OUT/tmp" TEMP="$TASK_OUT/tmp"
modelsim_resolve

python3 -B "$SCRIPT_DIR/extract_write_engine.py" --xml "$XML" \
    --target MinimalAXI4MasterPipelined --out "$TASK_OUT/reader.sv"
python3 -B "$SCRIPT_DIR/extract_write_engine.py" --xml "$XML" \
    --target MinimalAXI4MasterPipelinedWrite --out "$TASK_OUT/writer.sv"

compile_and_run() {
    local build=$1 top=$2 cell=$3 tb=$4 width=$5 expected=$6 name=$7 expected_size=${8:-}
    local raw="$TASK_OUT/$name.raw.log" normalized="$TASK_OUT/$name.log" status=0 marker expected_marker
    case "$top" in
        tb_reader_width_contract) marker=AXI_READER_WIDTH_PASS ;;
        tb_writer_width_contract) marker=AXI_WRITER_WIDTH_PASS ;;
        *) echo "unsupported width contract top: $top" >&2; return 2 ;;
    esac
    if [[ ! -f $build/compile.status ]]; then
        modelsim_compile "$build" "$top" "$cell" "$tb"
    fi
    if modelsim_run "$build" "$top" "$raw" 60 "-gDATA_BITS=$width" --; then
        status=0
    else
        status=$?
    fi
    modelsim_normalize_log "$raw" "$normalized"
    local raw_status effective_status diagnostic_count
    if [[ ! -r $raw.raw-status || ! -r $raw.status ]]; then
        cat "$normalized" >&2
        echo "$name is missing ModelSim raw/effective status" >&2
        return 1
    fi
    raw_status=$(<"$raw.raw-status")
    effective_status=$(<"$raw.status")
    if ! [[ $raw_status =~ ^[0-9]+$ && $effective_status =~ ^[0-9]+$ ]]; then
        cat "$normalized" >&2
        echo "$name has invalid ModelSim raw/effective status" >&2
        return 1
    fi
    if grep -Eiq 'segmentation fault|stack smashing|addresssanitizer|core dumped|aborted|killed|watchdog|timed out|timeout' "$normalized"; then
        cat "$normalized" >&2
        echo "$name encountered a crash, watchdog, or timeout (raw=$raw_status effective=$effective_status)" >&2
        return 1
    fi
    if [[ $top == tb_reader_width_contract && $expected == pass ]]; then
        if [[ ! $expected_size =~ ^[0-7]$ ]]; then
            echo "$name is missing a valid expected ARSIZE" >&2
            return 2
        fi
        expected_marker="$marker DATA_BITS=$width ARSIZE=$expected_size"
    else
        expected_marker="$marker DATA_BITS=$width"
    fi
    if [[ $expected == pass ]]; then
        if ((raw_status != 0 || effective_status != 0 || status != 0)) ||
            grep -Eq '^\*\* (Fatal|Error):' "$normalized" ||
            [[ $(grep -Ec 'AXI_(READER|WRITER)_WIDTH_PASS' "$normalized" || true) != 1 ]] ||
            [[ $(grep -Fxc "$expected_marker" "$normalized" || true) != 1 ]]; then
            cat "$normalized" >&2
            echo "$name did not complete as expected (status=$status)" >&2
            return 1
        fi
        echo "$marker width=$width" | tee -a "$TASK_OUT/results.log"
    else
        diagnostic_count=$(grep -Fxc "** Fatal: $expected" "$normalized" || true)
        local accepted_failure_status=0
        if [[ $raw_status == 0 || $raw_status == 1 ]]; then
            ((effective_status == 1 && status == 1)) && accepted_failure_status=1
        elif [[ $top == tb_writer_width_contract && $raw_status == 12 &&
                $effective_status == 12 && $status == 12 ]]; then
            accepted_failure_status=1
        fi
        if (( ! accepted_failure_status )) ||
            [[ $(grep -Ec '^\*\* Fatal:' "$normalized" || true) != 1 ]] ||
            [[ $diagnostic_count != 1 ]] || grep -Eq '^\*\* Error:' "$normalized" ||
            grep -Eq 'AXI_(READER|WRITER)_WIDTH_PASS' "$normalized"; then
            cat "$normalized" >&2
            echo "$name did not fail for the expected parameter diagnostic (status=$status)" >&2
            return 1
        fi
        echo "AXI_WIDTH_EXPECTED_FAILURE PASS name=$name width=$width" | tee -a "$TASK_OUT/results.log"
    fi
}

reader_build="$TASK_OUT/build-reader"
reader="$TASK_OUT/reader.sv"
reader_tb="$SCRIPT_DIR/tb_reader_width_contract.sv"
modelsim_compile "$reader_build" tb_reader_width_contract "$reader" "$reader_tb"
for width_size in 8:0 16:1 32:2 64:3 128:4 256:5 512:6 1024:7; do
    IFS=: read -r width expected_size <<<"$width_size"
    name="reader-$width"
    compile_and_run "$reader_build" tb_reader_width_contract "$reader" "$reader_tb" \
        "$width" pass "$name" "$expected_size"
done
compile_and_run "$reader_build" tb_reader_width_contract "$reader" "$reader_tb" 2048 \
    'BITSIZE_m_axi_rdata must be a power-of-two width in [8,1024]' reader-2048
compile_and_run "$reader_build" tb_reader_width_contract "$reader" "$reader_tb" 4096 \
    'BITSIZE_m_axi_rdata must be a power-of-two width in [8,1024]' reader-4096

writer_build="$TASK_OUT/build-writer"
writer="$TASK_OUT/writer.sv"
writer_tb="$SCRIPT_DIR/tb_writer_width_contract.sv"
modelsim_compile "$writer_build" tb_writer_width_contract "$writer" "$writer_tb"
compile_and_run "$writer_build" tb_writer_width_contract "$writer" "$writer_tb" 32 pass writer-32
compile_and_run "$writer_build" tb_writer_width_contract "$writer" "$writer_tb" 8 \
    'F1 writer supports 32-bit data beats only' writer-8
compile_and_run "$writer_build" tb_writer_width_contract "$writer" "$writer_tb" 64 \
    'F1 writer supports 32-bit data beats only' writer-64

echo "AXI_WIDTH_CONTRACT_PASS output=$TASK_OUT" | tee -a "$TASK_OUT/results.log"
