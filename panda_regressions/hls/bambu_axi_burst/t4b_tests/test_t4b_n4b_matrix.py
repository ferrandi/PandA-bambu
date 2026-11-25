"""Offline tests for the fail-closed T4B N4b matrix wrapper."""
from __future__ import annotations

import csv
import json
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[3]
if not (ROOT / "panda_regressions").is_dir():
    raise RuntimeError(f"unexpected repository root for T4B tests: {ROOT}")
TMP_ROOT = Path(os.environ.get("BURST_TEST_TMPDIR", Path.cwd())) / f"out_{Path(__file__).stem}"
WRAPPER = HERE / "run_t4b_n4b_matrix.sh"


class N4bMatrixTests(unittest.TestCase):
    def setUp(self):
        TMP_ROOT.mkdir(parents=True, exist_ok=True)
        self.temp = tempfile.TemporaryDirectory(prefix="test-t4b-n4b-", dir=TMP_ROOT)
        self.base = Path(self.temp.name)
        self.child = self.base / "fake_case_runner.sh"
        self.log = self.base / "invocations.tsv"
        self.child.write_text("""#!/usr/bin/env bash
set -euo pipefail
B=$1 O=$2 D=$3 L=$4 N=$5 Q=$6 OUT=$7
shift 7
FIXTURE= MODE=
while (($#)); do
  case $1 in
    --fixture) FIXTURE=$2; shift 2 ;;
    --mode) MODE=$2; shift 2 ;;
    --axi-burst-type) AXI=$2; shift 2 ;;
    *) echo "unexpected arg: $1" >&2; exit 19 ;;
  esac
done
printf '%s\\t%s\\t%s\\t%s\\t%s\\t%s\\n' "$FIXTURE" "$N" "$MODE" "$O" "$B" "$D" >> "$T4B_FAKE_LOG"
if [[ ${T4B_FAKE_FAIL_MODE:-} == "$MODE" && ${T4B_FAKE_FAIL_FIXTURE:-} == "$FIXTURE" && ${T4B_FAKE_FAIL_N:-} == "$N" ]]; then
  mkdir -p "$OUT"
  echo retained-child-failure >&2
  exit 7
fi
mkdir -p "$OUT/run/HLS_output"
case $MODE in
  burst)
    python3 - "$OUT/manifest.json" "$MODE" "$FIXTURE" "$B" "$O" "$D" "$L" "$N" "$Q" <<'PY'
import json,sys
path,mode,fixture,b,o,d,l,n,q=sys.argv[1:]
b,o,d,l,n,q=map(int,(b,o,d,l,n,q))
metrics={"L_axi":63,"first_wait_cycles":65,"exposed_wait_cycles":64,"state_stall_cycles":n-1,"outstanding_peak":o,"n_desc":(n+b-1)//b}
if __import__("os").environ.get("T4B_FAKE_MISSING_METRIC"):
    metrics.pop(__import__("os").environ["T4B_FAKE_MISSING_METRIC"], None)
if __import__("os").environ.get("T4B_FAKE_NULL_STATE_STALL"):
    metrics["state_stall_cycles"] = None
if __import__("os").environ.get("T4B_FAKE_WRONG_FIXTURE"):
    fixture = "mulxor" if fixture == "independent" else "independent"
params={"B":b,"O":o,"D":d,"L":l,"N":n,"Q":q,"AXI_burst_type":"INCR",
 "clock_period_ns":3.33,"mem_delay_read":64,"mem_delay_write":2,
 "memory_allocation_policy":"NO_BRAM","distram_threshold":0,"experimental_m_axi_burst":1}
doc={"status":"PASS","mode":mode,"fixture":fixture,"parameters":params,
 "C_total":n+68,"C_stream":n+62,"checker_results":{"status":"PASS","metrics":metrics}}
open(path,"w").write(json.dumps(doc))
PY
    ;;
  legacy)
    python3 - "$OUT/manifest.json" "$MODE" "$FIXTURE" "$B" "$O" "$D" "$L" "$N" "$Q" <<'PY'
import json,sys
path,mode,fixture,b,o,d,l,n,q=sys.argv[1:]
b,o,d,l,n,q=map(int,(b,o,d,l,n,q))
params={"B":b,"O":o,"D":d,"L":l,"N":n,"Q":q,"AXI_burst_type":"INCR",
 "clock_period_ns":3.33,"mem_delay_read":64,"mem_delay_write":2,
 "memory_allocation_policy":"NO_BRAM","distram_threshold":0,"experimental_m_axi_burst":0}
doc={"status":"PASS","mode":mode,"fixture":fixture,"parameters":params,
 "C_total":n+112,"C_stream":n+106,"checker_results":{"status":"PASS","metrics":{"L_axi":63}}}
open(path,"w").write(json.dumps(doc))
PY
    ;;
  scheduler-off)
    mkdir -p "$OUT/run/HLS_output/ir" "$OUT/run/HLS_output/dot" "$OUT/run/HLS_output/xml"
    : > "$OUT/run/HLS_output/ir/kernel.bambuir"
    : > "$OUT/run/HLS_output/dot/kernel.dot"
    : > "$OUT/run/HLS_output/xml/architecture.xml"
    python3 - "$OUT/manifest.json" "$MODE" "$FIXTURE" "$B" "$O" "$D" "$L" "$N" "$Q" <<'PY'
import json,sys
path,mode,fixture,b,o,d,l,n,q=sys.argv[1:]
p={"B":int(b),"O":int(o),"D":int(d),"L":int(l),"N":int(n),"Q":int(q),"AXI_burst_type":"INCR",
 "clock_period_ns":3.33,"mem_delay_read":64,"mem_delay_write":2,"memory_allocation_policy":"NO_BRAM",
 "distram_threshold":0,"experimental_m_axi_burst":1}
if __import__("os").environ.get("T4B_FAKE_WRONG_FIXTURE"):
    fixture = "mulxor" if fixture == "independent" else "independent"
open(path,"w").write(json.dumps({"status":"PASS","mode":mode,"fixture":fixture,"parameters":p}))
PY
    ;;
  *) exit 18 ;;
esac
""", encoding="utf-8")
        self.child.chmod(0o755)

    def tearDown(self):
        self.temp.cleanup()

    def run_wrapper(self, target, *, outside=True, extra_env=None):
        env = os.environ.copy()
        env.update({"T4B_N4B_CASE_RUNNER": str(self.child),
                    "T4B_FAKE_LOG": str(self.log), "TMPDIR": str(TMP_ROOT),
                    "TMP": str(TMP_ROOT), "TEMP": str(TMP_ROOT),
                    "PYTHONDONTWRITEBYTECODE": "1"})
        env.pop("BURST_MODELSIM_OUTSIDE_SANDBOX", None)
        if outside:
            env["BURST_MODELSIM_OUTSIDE_SANDBOX"] = "1"
        if extra_env:
            env.update(extra_env)
        return subprocess.run([str(WRAPPER), str(target)], cwd=ROOT, env=env,
                              text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                              check=False)

    @staticmethod
    def read_table(path):
        with path.open(encoding="utf-8", newline="") as stream:
            return list(csv.DictReader(stream, delimiter="\t"))

    def test_exact_36_rows_order_and_mode_contract(self):
        target = self.base / "n4b-success"
        result = self.run_wrapper(target)
        self.assertEqual(result.returncode, 0, result.stderr)
        manifest = self.read_table(target / "manifest.tsv")
        summary = self.read_table(target / "summary.tsv")
        self.assertEqual(len(manifest), 36)
        self.assertEqual(manifest, summary)
        expected = []
        for fixture in ("independent", "mulxor"):
            for n in (1024, 2048):
                expected.extend((fixture, n, "burst", o, 16, 256) for o in (1, 4, 5, 8, 16))
                expected.append((fixture, n, "legacy", 16, 16, 256))
                expected.extend((fixture, n, "scheduler-off", o, 16, 256) for o in (5, 8, 16))
        actual_log = [tuple([row["fixture"], int(row["N"]), row["mode"],
                             int(row["O_requested"]), int(row["B_requested"]),
                             int(row["D_requested"])]) for row in manifest]
        self.assertEqual(actual_log, expected)
        invocations = [line.rstrip("\n").split("\t") for line in self.log.read_text().splitlines()]
        self.assertEqual(len(invocations), 36)
        self.assertEqual([(x[0], int(x[1]), x[2], int(x[3])) for x in invocations],
                         [(x[0], x[1], x[2], x[3]) for x in expected])
        required = {"case_id", "comparison", "mode", "status", "B_requested",
                    "O_requested", "D_requested", "B_applied", "O_applied",
                    "D_applied", "C_total", "C_stream", "channel_latency",
                    "correctness", "IR_index", "DOT_index", "XML_index"}
        self.assertTrue(required.issubset(manifest[0]))
        legacy = next(row for row in manifest if row["mode"] == "legacy")
        self.assertEqual(legacy["comparison"], "burst-versus-legacy")
        self.assertEqual((legacy["B_applied"], legacy["O_applied"], legacy["D_applied"]),
                         ("not_applied", "not_applied", "not_applied"))
        self.assertEqual((legacy["C_total"], legacy["C_stream"], legacy["channel_latency"],
                          legacy["correctness"]), ("1136", "1130", "63", "PASS"))
        for key in ("first_wait_cycles", "exposed_wait_cycles", "state_stall_cycles",
                    "outstanding_peak", "n_desc"):
            self.assertEqual(legacy[key], "")
        scheduler = next(row for row in manifest if row["mode"] == "scheduler-off")
        self.assertEqual((scheduler["C_total"], scheduler["C_stream"],
                          scheduler["channel_latency"], scheduler["correctness"]),
                         ("", "", "", "NOT_SIMULATED"))
        for key in ("first_wait_cycles", "exposed_wait_cycles", "state_stall_cycles",
                    "outstanding_peak", "n_desc"):
            self.assertEqual(scheduler[key], "")
        for key in ("IR_index", "DOT_index", "XML_index"):
            self.assertIn("capture_index.tsv#", scheduler[key])
        index = target / "cases" / scheduler["case_id"] / "capture_index.tsv"
        index_rows = self.read_table(index)
        self.assertEqual({row["kind"] for row in index_rows}, {"IR", "DOT", "XML"})
        self.assertEqual(len(list(target.glob("*.wrapper.*.log"))), 72)

    def test_guard_and_existing_output_are_fail_closed(self):
        no_guard = self.base / "no-guard"
        result = self.run_wrapper(no_guard, outside=False)
        self.assertEqual(result.returncode, 2)
        self.assertFalse(no_guard.exists())
        self.assertIn("outside the sandbox", result.stderr)
        existing = self.base / "existing"
        existing.mkdir()
        (existing / "keep.txt").write_text("preserve", encoding="utf-8")
        result = self.run_wrapper(existing)
        self.assertEqual(result.returncode, 2)
        self.assertEqual((existing / "keep.txt").read_text(encoding="utf-8"), "preserve")
        self.assertIn("refusing existing output", result.stderr)

    def test_child_failure_retains_partial_logs_and_completed_rows(self):
        target = self.base / "n4b-partial"
        result = self.run_wrapper(target, extra_env={"T4B_FAKE_FAIL_MODE": "legacy",
            "T4B_FAKE_FAIL_FIXTURE": "independent", "T4B_FAKE_FAIL_N": "1024"})
        self.assertEqual(result.returncode, 2)
        manifest = self.read_table(target / "manifest.tsv")
        self.assertEqual(len(manifest), 6)
        self.assertEqual(manifest[-1]["mode"], "legacy")
        self.assertEqual(manifest[-1]["status"], "FAIL")
        self.assertTrue((target / "independent_N1024_legacy_O16.wrapper.stderr.log").is_file())
        self.assertIn("partial evidence preserved", result.stderr)

    def test_wrong_fixture_and_missing_required_metric_fail_closed(self):
        for suffix, extra_env in (
                ("wrong-fixture", {"T4B_FAKE_WRONG_FIXTURE": "1"}),
                ("missing-metric", {"T4B_FAKE_MISSING_METRIC": "outstanding_peak"}),
                ("missing-state-stall", {"T4B_FAKE_MISSING_METRIC": "state_stall_cycles"})):
            with self.subTest(case=suffix):
                target = self.base / suffix
                result = self.run_wrapper(target, extra_env=extra_env)
                self.assertEqual(result.returncode, 2)
                rows = self.read_table(target / "manifest.tsv")
                self.assertEqual(len(rows), 1)
                self.assertEqual(rows[0]["status"], "INVALID")
                self.assertIn("partial evidence preserved", result.stderr)

    def test_unavailable_state_stall_is_labeled_not_blank(self):
        target = self.base / "state-stall-unavailable"
        result = self.run_wrapper(target, extra_env={"T4B_FAKE_NULL_STATE_STALL": "1"})
        self.assertEqual(result.returncode, 0, result.stderr)
        rows = self.read_table(target / "manifest.tsv")
        self.assertEqual(rows[0]["state_stall_cycles"], "unavailable")


if __name__ == "__main__":
    unittest.main()
