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
python3 -B "$SCRIPT_DIR/extract_write_engine.py" --xml "$ROOT/etc/libtech/NC_MEM_IPs.xml" \
    --target STD_FIFO --out "$OUT/storage.sv"
modelsim_compile "$OUT/build" tb_std_fifo \
    "$OUT/storage.sv" "$SCRIPT_DIR/tb_std_fifo.sv"
for depth in 1 2 3 16 64 512 4096; do
    for width in 1 32 80; do
        log="$OUT/depth-$depth-width-$width.log"
        modelsim_run "$OUT/build" tb_std_fifo "$log" 120 -gDEPTH="$depth" -gDATA_BITS="$width"
        grep -q 'COMPLETE_FIFO_PASS' "$log"
    done
done
echo "STD_FIFO_SIM_PASS profiles=21: $OUT"
