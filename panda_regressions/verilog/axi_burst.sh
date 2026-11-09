#!/usr/bin/env bash
# AXI master/component RTL tests. This suite does not invoke Bambu.
set -euo pipefail
SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)
ROOT=$(realpath "$SCRIPT_DIR/../..")
TESTS="$SCRIPT_DIR/axi_burst/rtl_tests"
if (($# != 1)); then
    echo "usage: $0 FRESH_OUTPUT_DIRECTORY" >&2
    exit 2
fi
OUT=$(realpath -m -- "$1")
[[ ! -e $OUT && ! -L $OUT ]] || { echo "output must be fresh: $OUT" >&2; exit 2; }
source "$TESTS/modelsim_helpers.sh"
modelsim_resolve
mkdir -p "$OUT/tmp"
export TMPDIR="$OUT/tmp" TMP="$OUT/tmp" TEMP="$OUT/tmp"
export BURST_TEST_TMPDIR="$OUT"
export PYTHONDONTWRITEBYTECODE=1
failed=0
run_case() {
    local name=$1 start=$SECONDS status
    shift
    if "$@" > "$OUT/$name.log" 2>&1; then
        status=0
        echo "PASS: $name"
    else
        status=$?
        failed=1
        echo "FAIL: $name (exit $status)" >&2
        tail -n 60 "$OUT/$name.log" >&2
    fi
    printf '%s\t%s\t%s\n' "$name" "$status" "$((SECONDS - start))" >> "$OUT/results.tsv"
}
run_case fifo bash "$TESTS/run_std_fifo.sh" "$OUT/fifo"
run_case memory bash "$TESTS/run_std_sdp_bramfw.sh" "$OUT/memory"
run_case openmp bash "$TESTS/run_openmp_fifo.sh" "$OUT/openmp"
run_case offline-rtl python3 -B -m unittest discover -s "$TESTS"
run_case offline-runners python3 -B -m unittest discover -s "$SCRIPT_DIR/axi_burst" -p 'test_*.py'
run_case read-master bash "$TESTS/run_read_burst_modelsim.sh" "$OUT/read-master" all
run_case write-master env BURST_WRITER_SIM_OUTSIDE_SANDBOX=1 \
    bash "$TESTS/run_write_burst_engine.sh" "$OUT/write-master"
run_case vlog bash "$TESTS/run_vlog_portability.sh" "$OUT/vlog"
run_case diagnostics env BURST_WRITER_SIM_OUTSIDE_SANDBOX=1 SIM_JOBS=1 \
    bash "$TESTS/run_burst_fault_diagnostics.sh" "$OUT/diagnostics"
run_case widths env SIM_JOBS=1 \
    bash "$TESTS/run_axi_width_contract.sh" "$OUT/widths"
run_case duplex-core env BURST_DUPLEX_SIM_OUTSIDE_SANDBOX=1 SIM_JOBS=1 \
    bash "$TESTS/run_duplex_core_tests.sh" "$OUT/duplex-core"
run_case adapter-size env BURST_ADAPTER_SIM_OUTSIDE_SANDBOX=1 SIM_JOBS=1 \
    bash "$TESTS/run_adapter_size_guard.sh" "$OUT/adapter-size"
run_case memory-model env BURST_MODELSIM_OUTSIDE_SANDBOX=1 SIM_JOBS=1 \
    bash "$TESTS/run_testbench_memaxi_model.sh" qualify "$OUT/memory-model"
python3 - "$OUT" <<'PY'
from pathlib import Path
import sys
import xml.etree.ElementTree as ET
out = Path(sys.argv[1])
rows = [line.split('\t') for line in (out / 'results.tsv').read_text().splitlines()]
suite = ET.Element('testsuite', name='verilog-axi-burst', tests=str(len(rows)),
                   failures=str(sum(int(status) != 0 for _, status, _ in rows)),
                   time=str(sum(int(seconds) for _, _, seconds in rows)))
for name, status, seconds in rows:
    case = ET.SubElement(suite, 'testcase', classname='verilog.axi_burst', name=name, time=seconds)
    if int(status):
        failure = ET.SubElement(case, 'failure', message=f'exit {status}')
        failure.text = '\n'.join((out / f'{name}.log').read_text(errors='replace').splitlines()[-60:])
ET.ElementTree(suite).write(out / 'junit.xml', encoding='utf-8', xml_declaration=True)
PY
printf 'Verilog component logs and JUnit report: %s\n' "$OUT"
exit "$failed"
