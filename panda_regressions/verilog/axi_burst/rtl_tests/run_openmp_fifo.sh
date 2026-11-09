#!/usr/bin/env bash
set -euo pipefail
SCRIPT_DIR=$(cd -- "$(dirname -- "$0")" && pwd -P)
ROOT=$(realpath "$SCRIPT_DIR/../../../../")
source "$SCRIPT_DIR/modelsim_helpers.sh"
if (($# != 1)); then
    echo "usage: $0 FRESH_OUTPUT_DIRECTORY" >&2
    exit 2
fi
OUT=$(realpath -m -- "$1")
[[ ! -e $OUT && ! -L $OUT ]] || { echo "output must be fresh: $OUT" >&2; exit 2; }
modelsim_resolve
mkdir -p "$OUT"
python3 -B - "$SCRIPT_DIR" "$ROOT" "$OUT" <<'PY'
from pathlib import Path
import sys
sys.path.insert(0, sys.argv[1])
from extract_write_engine import extract
root, out = Path(sys.argv[2]), Path(sys.argv[3])
xml = root / 'etc/libtech/NC_OPENMP_IPs.xml'
rtl = extract(xml, 'kmp_bambu_general_memory_interface')
rtl = rtl.replace('module kmp_bambu_general_memory_interface #(', 'module GMICandidate #(', 1)
(out / 'candidate.sv').write_text(rtl)
PY
modelsim_compile "$OUT/build" tb_openmp_fifo "$OUT/candidate.sv" \
    "$SCRIPT_DIR/openmp_fifo_reference.sv" "$SCRIPT_DIR/std_sdp_bramfw_reference.sv" \
    "$SCRIPT_DIR/tb_openmp_fifo.sv"
for bits in 1 2 4 6; do
    for filter in 0 1; do
        log="$OUT/c$bits-f$filter.log"
        modelsim_run "$OUT/build" tb_openmp_fifo "$log" 90 -gCONTEXT_BITS="$bits" -gFILTER="$filter"
        grep -q GMI_EQUIVALENCE_PASS "$log"
    done
done
for filter in 0 1; do
    log="$OUT/no-cs-f$filter.log"
    modelsim_run "$OUT/build" tb_openmp_fifo "$log" 90 -gCS=0 -gFILTER="$filter"
    grep -q GMI_EQUIVALENCE_PASS "$log"
done
echo "STD_FIFO_OPENMP_PASS profiles=10: $OUT"
