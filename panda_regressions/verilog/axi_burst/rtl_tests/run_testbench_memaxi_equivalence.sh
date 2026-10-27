#!/usr/bin/env bash
# Differential qualification against the XML in an explicit baseline revision.
set -euo pipefail
SCRIPT_DIR=$(cd -- "$(dirname -- "$0")" && pwd -P)
ROOT=$(realpath "$SCRIPT_DIR/../../../../")
source "$SCRIPT_DIR/modelsim_helpers.sh"
if (($# != 2)); then
    echo "usage: $0 BASELINE_GIT_REF FRESH_OUTPUT_DIRECTORY" >&2
    exit 2
fi
BASELINE_REF=$1
OUT=$(realpath -m -- "$2")
[[ ! -e $OUT && ! -L $OUT ]] || { echo "output must be fresh: $OUT" >&2; exit 2; }
modelsim_resolve
mkdir -p "$OUT/hdl"
git -C "$ROOT" show "$BASELINE_REF:etc/libtech/NC_TESTBENCH_IPs.xml" > "$OUT/baseline.xml"
cp "$ROOT/etc/libtech/NC_TESTBENCH_IPs.xml" "$OUT/candidate.xml"
for variant in baseline candidate; do
    python3 -B "$SCRIPT_DIR/extract_testbench_memaxi.py" --xml "$OUT/$variant.xml" \
        --out "$OUT/hdl/$variant.sv" \
        --t4b-extractor "$SCRIPT_DIR/../t4b_tests/extract_t4b_cells.py"
done
python3 - "$OUT" <<'PY'
from pathlib import Path
import sys
out = Path(sys.argv[1])
for version, module in [('baseline', 'BaselineTestbenchMEMAXI'), ('candidate', 'CandidateTestbenchMEMAXI')]:
    path = out / 'hdl' / f'{version}.sv'
    source = path.read_text()
    target = 'module TestbenchMEMAXI #('
    if source.count(target) != 1:
        raise SystemExit(f'expected exactly one module header in {path}')
    path.write_text(source.replace(target, f'module {module} #('))
PY
{
    printf 'baseline_ref=%s\ngit_head=%s\n' "$BASELINE_REF" "$(git -C "$ROOT" rev-parse HEAD)"
    modelsim_version
    sha256sum "$OUT/baseline.xml" "$OUT/candidate.xml" "$OUT/hdl/"*.sv \
        "$SCRIPT_DIR/tb_testbench_memaxi_equivalence.sv" "$SCRIPT_DIR/tb_testbench_memaxi_backend.sv"
} > "$OUT/provenance.txt"
modelsim_compile "$OUT/build" tb_testbench_memaxi_equivalence \
    "$OUT/hdl/baseline.sv" "$OUT/hdl/candidate.sv" \
    "$SCRIPT_DIR/tb_testbench_memaxi_backend.sv" "$SCRIPT_DIR/tb_testbench_memaxi_equivalence.sv"
for delay in 1 63; do
    log="$OUT/delay-$delay.log"
    modelsim_run "$OUT/build" tb_testbench_memaxi_equivalence "$log" 120 -gWRITE_DELAY="$delay"
    grep -Eq "^#? *MEM_DIFFERENTIAL_PASS delay=$delay cycles=3000 " "$log" || {
        cat "$log" >&2
        exit 1
    }
done
echo "MEMAXI_EQUIVALENCE_PASS: $OUT"
