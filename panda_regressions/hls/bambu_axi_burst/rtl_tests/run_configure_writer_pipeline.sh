#!/usr/bin/env bash
set -euo pipefail
ulimit -c 0

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd -P)
ROOT=$(realpath "$SCRIPT_DIR/../../../../")
EXTRACTOR="$SCRIPT_DIR/extract_write_engine.py"
TB="$SCRIPT_DIR/tb_configure_writer_pipeline.sv"
XML="$ROOT/etc/libtech/NC_AXI_IPs.xml"
WRITER_ROOT=$ROOT
source "$SCRIPT_DIR/modelsim_writer_helpers.sh"

if (($# > 1)); then
    echo "usage: $0 [fresh-output-directory]" >&2
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
modelsim_writer_resolve
MODELSIM_WRITER_VERSION=$(modelsim_writer_version) || exit 2

OUT_ARG=${1:-"$PWD/out_configure_writer_pipeline"}
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

RTL="$TASK_OUT/MinimalAXI4MasterPipelinedWrite.sv"
python3 -B "$EXTRACTOR" --xml "$XML" --out "$RTL" > "$TASK_OUT/extract.log"
{
    printf 'git_head='; git -C "$ROOT" rev-parse HEAD
    printf 'xml='; sha256sum "$XML"
    printf 'core_extract='; sha256sum "$RTL"
    printf 'testbench='; sha256sum "$TB"
    printf 'runner='; sha256sum "$SCRIPT_DIR/run_configure_writer_pipeline.sh"
    printf 'extractor='; sha256sum "$EXTRACTOR"
    printf 'modelsim_lookup_source=%s\nmodelsim_bin=%s\nvsim=%s\nvlib=%s\nvlog=%s\nvmap=%s\n' \
        "$MODELSIM_WRITER_SOURCE" "$MODELSIM_WRITER_BIN" "$MODELSIM_WRITER_VSIM" \
        "$MODELSIM_WRITER_VLIB" "$MODELSIM_WRITER_VLOG" "$MODELSIM_WRITER_VMAP"
    printf 'modelsim_version=%s\n' "$(printf '%s' "$MODELSIM_WRITER_VERSION" | tr '\n' ' ')"
} > "$TASK_OUT/provenance.txt"

: > "$TASK_OUT/aggregate.log"
BUILD="$TASK_OUT/build"
modelsim_writer_compile "$BUILD" tb_configure_writer_pipeline "$RTL" "$TB" 0 || exit $?

for scenario in RESET_E1 PEER_FAULT_E1; do
    log="$BUILD/$scenario.log"
    status=0
    modelsim_writer_run "$BUILD" tb_configure_writer_pipeline "$log" 30 -- "+CASE=$scenario" || status=$?
    modelsim_writer_normalize_log "$log" "$log.normalized" || exit $?
    if (( status != 0 )); then
        cat "$log" >&2
        echo "RTL scenario failed: $scenario" >&2
        exit 1
    fi
    if ! grep -Eq "^CONFIGURE_WRITER_PIPELINE PASS CASE=$scenario$" "$log.normalized"; then
        cat "$log" >&2
        echo "missing exact PASS marker for $scenario" >&2
        exit 1
    fi
    cat "$log.normalized" >> "$TASK_OUT/aggregate.log"
    cat "$log.normalized"
done

printf 'CONFIGURE_WRITER_PIPELINE_SUITE_PASS cases=2\n' | tee -a "$TASK_OUT/aggregate.log"
