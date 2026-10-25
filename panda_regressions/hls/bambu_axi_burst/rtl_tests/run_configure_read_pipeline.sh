#!/usr/bin/env bash
set -euo pipefail
ulimit -c 0

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd -P)
ROOT=$(realpath "$SCRIPT_DIR/../../../../")
XML="$ROOT/etc/libtech/NC_AXI_IPs.xml"
EXTRACTOR="$SCRIPT_DIR/extract_write_engine.py"
TB="$SCRIPT_DIR/tb_configure_read_pipeline.sv"
source "$SCRIPT_DIR/modelsim_helpers.sh"
if (($# > 1)); then echo "usage: $0 [output-directory]" >&2; exit 2; fi
OUT=${1:-"$PWD/out_configure_read_pipeline"}
for tool in python3 timeout sha256sum git sed grep; do
    command -v "$tool" >/dev/null 2>&1 || { echo "missing required tool: $tool" >&2; exit 2; }
done
modelsim_resolve
OUT=$(realpath -m -- "$OUT")
if [[ -e $OUT || -L $OUT ]]; then echo "output directory must not already exist: $OUT" >&2; exit 2; fi
mkdir -p "$OUT/tmp"
export TMPDIR="$OUT/tmp" TMP="$OUT/tmp" TEMP="$OUT/tmp"

READ_RTL="$OUT/MinimalAXI4MasterPipelined.sv"
python3 -B "$EXTRACTOR" --xml "$XML" --target MinimalAXI4MasterPipelined --out "$READ_RTL"
BUILD="$OUT/build"
modelsim_compile "$BUILD" tb_configure_read_pipeline "$READ_RTL" "$TB"
{
    printf 'git_head=%s\n' "$(git -C "$ROOT" rev-parse HEAD)"
    printf 'xml='; sha256sum "$XML"
    printf 'extractor='; sha256sum "$EXTRACTOR"
    printf 'generated_hdl='; sha256sum "$READ_RTL"
    printf 'testbench='; sha256sum "$TB"
    printf 'runner='; sha256sum "$0"
} > "$OUT/provenance.txt"
cat "$OUT/provenance.txt" > "$OUT/aggregate.log"
echo "ModelSim: $(modelsim_version | sed -n '1p')"
for scenario in READ_RESET READ_PEER_FAULT; do
    raw="$BUILD/$scenario.raw.log"
    log="$BUILD/$scenario.log"
    status=0
    modelsim_run "$BUILD" tb_configure_read_pipeline "$raw" 30 -- "+CASE=$scenario" || status=$?
    modelsim_normalize_log "$raw" "$log"
    if ((status != 0)) || ! grep -Eq "^READ_CONFIG_PIPELINE PASS CASE=$scenario$" "$log"; then
        cat "$log" >&2 || true
        echo "reader configure pipeline scenario failed or lacks exact marker: $scenario (status=$status)" >&2
        exit 1
    fi
    cat "$log"
    cat "$log" >> "$OUT/aggregate.log"
done
echo "READ_CONFIG_PIPELINE PASS cases=2" | tee -a "$OUT/aggregate.log"
