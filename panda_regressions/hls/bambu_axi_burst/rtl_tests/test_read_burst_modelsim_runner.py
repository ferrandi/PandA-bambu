#!/usr/bin/env python3
"""Behavioral smoke tests for the read-core ModelSim runner using fake PATH tools."""

from __future__ import annotations

import os
from pathlib import Path
import subprocess
import tempfile
import unittest


REPO = Path(__file__).resolve().parents[4]
RTL_TESTS = Path(__file__).resolve().parent
RUNNER = RTL_TESTS / "run_read_burst_modelsim.sh"
TMP_ROOT = Path(os.environ.get(
    "BURST_TEST_TMPDIR", REPO / "documentation/tmp/out_read_burst_modelsim_runner"
))


VSIM = r'''#!/usr/bin/env python3
import os, pathlib, sys
args = sys.argv[1:]
if "-version" in args:
    print("QuestaSim fake 2026")
    raise SystemExit(0)
with open(os.environ["FAKE_MODELSIM_CALLS"], "a", encoding="utf-8") as stream:
    stream.write("vsim " + " ".join(args) + "\n")
case = next((arg.split("=", 1)[1] for arg in args if arg.startswith("+CASE=")), "")
top = next((arg for arg in args if arg.startswith("work.")), "")
if case == "SMOKE":
    max_out = next((arg.split("=", 1)[1] for arg in args if arg.startswith("-gMAX_OUTSTANDING=")), "1")
    depth = next((arg.split("=", 1)[1] for arg in args if arg.startswith("-gFIFO_DEPTH=")), "256")
    if max_out not in [str(n) for n in range(1, 17)]:
        print("** Fatal: MAX_OUTSTANDING must be in [1,16]")
        if os.environ.get("FAKE_MODELSIM_MODE") == "extra-fatal": print("** Fatal: second fatal")
        marker_case = "SMOKE" if os.environ.get("FAKE_MODELSIM_MODE") == "invalid-final-smoke" else ""
        print("DUPLEX_READ PASS CASE=" + marker_case)
        raise SystemExit({"invalid-timeout": 124, "invalid-killed": 137, "invalid-crash": 139}.get(
            os.environ.get("FAKE_MODELSIM_MODE"), 1))
    try: d = int(depth)
    except ValueError: d = 0
    if d < 1 or d > 4096 or d & (d - 1):
        print("** Fatal: FIFO_DEPTH must be a power of two in [1,4096]")
        if os.environ.get("FAKE_MODELSIM_MODE") == "extra-fatal": print("** Fatal: second fatal")
        marker_case = "SMOKE" if os.environ.get("FAKE_MODELSIM_MODE") == "invalid-final-smoke" else ""
        print("DUPLEX_READ PASS CASE=" + marker_case)
        raise SystemExit({"invalid-timeout": 124, "invalid-killed": 137, "invalid-crash": 139}.get(
            os.environ.get("FAKE_MODELSIM_MODE"), 1))
if os.environ.get("FAKE_MODELSIM_MODE") == "unrelated-error":
    print("** Error: unrelated simulator error")
    raise SystemExit(1)
if top == "work.tb_configure_read_pipeline":
    print("READ_CONFIG_PIPELINE PASS CASE=" + case)
else:
    marker_case = "WRONG" if os.environ.get("FAKE_MODELSIM_MODE") == "wrong-marker" else case
    print("DUPLEX_READ PASS CASE=" + marker_case)
    if case == "READY_CREDIT_PROOF":
        print("PASS READY_CREDIT_PROOF FULL O=16 D=32 AR=16")
        print("PASS READY_CREDIT_PROOF O=16 D=32 full-LOAD-credit-AR-response-drain N=1")
''' 


class ReadBurstModelSimRunnerTests(unittest.TestCase):
    def setUp(self) -> None:
        TMP_ROOT.mkdir(parents=True, exist_ok=True)
        self.temp = tempfile.TemporaryDirectory(prefix="read-runner-", dir=TMP_ROOT)
        self.root = Path(self.temp.name)
        self.bin = self.root / "bin"
        self.bin.mkdir()
        self.calls = self.root / "calls.log"
        for name in ("vsim", "vlib", "vmap", "vlog", "verilator"):
            path = self.bin / name
            if name == "vsim":
                path.write_text(VSIM, encoding="utf-8")
            else:
                path.write_text(
                    "#!/usr/bin/env bash\n"
                    "printf '%s %s\\n' \"$(basename \"$0\")\" \"$*\" >> \"$FAKE_MODELSIM_CALLS\"\n"
                    "exit 0\n",
                    encoding="utf-8",
                )
            path.chmod(0o755)
        self.env = os.environ.copy()
        self.env.update(
            PATH=f"{self.bin}:{self.env['PATH']}",
            TMPDIR=str(self.root), TMP=str(self.root), TEMP=str(self.root),
            FAKE_MODELSIM_CALLS=str(self.calls),
        )

    def tearDown(self) -> None:
        self.temp.cleanup()

    def run_runner(self, name: str, suite: str, mode: str = "") -> subprocess.CompletedProcess[str]:
        out = self.root / name
        env = self.env.copy()
        if mode:
            env["FAKE_MODELSIM_MODE"] = mode
        return subprocess.run(
            ["bash", str(RUNNER), str(out), suite],
            cwd=REPO,
            env=env,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            timeout=60,
            check=False,
        )

    def test_credit_witness_markers_and_configure_child_pass(self) -> None:
        result = self.run_runner("credits", "credits")
        self.assertEqual(result.returncode, 0, result.stderr)
        aggregate = (self.root / "credits" / "aggregate.log").read_text(encoding="utf-8")
        self.assertIn("PASS READY_CREDIT_PROOF FULL", aggregate)
        self.assertIn("full-LOAD-credit-AR-response-drain", aggregate)
        self.assertIn("DUPLEX_READ PASS CASE=CREDIT_LIMIT", aggregate)
        self.assertNotIn("verilator ", self.calls.read_text(encoding="utf-8"))

        configure = subprocess.run(
            ["bash", str(RTL_TESTS / "run_configure_read_pipeline.sh"), str(self.root / "configure")],
            cwd=REPO, env=self.env, text=True, stdout=subprocess.PIPE,
            stderr=subprocess.PIPE, timeout=30, check=False,
        )
        self.assertEqual(configure.returncode, 0, configure.stderr)
        self.assertIn("READ_CONFIG_PIPELINE PASS CASE=READ_RESET", configure.stdout)
        self.assertIn("READ_CONFIG_PIPELINE PASS CASE=READ_PEER_FAULT", configure.stdout)

    def test_invalid_parameter_cases_require_named_single_fatal_and_smoke_selector(self) -> None:
        for suffix, mode in (("", ""), ("-final-smoke", "invalid-final-smoke")):
            result = self.run_runner("bounds" + suffix, "bounds", mode)
            self.assertEqual(result.returncode, 0, result.stderr)
            for name in ("O0", "O17", "D0", "D3", "D8192"):
                self.assertIn(f"PASS invalid parameters {name} rejected", result.stdout)
        calls = self.calls.read_text(encoding="utf-8")
        for parameter_case in ("-gMAX_OUTSTANDING=0", "-gMAX_OUTSTANDING=17", "-gFIFO_DEPTH=0",
                               "-gFIFO_DEPTH=3", "-gFIFO_DEPTH=8192"):
            self.assertIn(parameter_case, calls)
        for line in calls.splitlines():
            if "-gMAX_OUTSTANDING=0" in line or "-gMAX_OUTSTANDING=17" in line or \
                    any(value in line for value in ("-gFIFO_DEPTH=0", "-gFIFO_DEPTH=3", "-gFIFO_DEPTH=8192")):
                self.assertIn("+CASE=SMOKE", line)
        self.assertIn("+CASE=COUNT_TRUNC_64", calls)
        self.assertIn("-gADDR_BITS=64", calls)
        self.assertIn("-gIN_BITS=32", calls)
        self.assertIn("-gIN_BITS=65", calls)
        self.assertNotIn("verilator ", calls)

    def test_missing_marker_unrelated_error_and_multiple_fatals_fail_closed(self) -> None:
        marker = self.run_runner("bad-marker", "credits", "wrong-marker")
        self.assertNotEqual(marker.returncode, 0)
        self.assertIn("missing exact PASS marker", marker.stderr)

        error = self.run_runner("bad-error", "bounds", "unrelated-error")
        self.assertNotEqual(error.returncode, 0)
        self.assertIn("unrelated simulator error", error.stderr)

        extra = self.run_runner("bad-fatal", "bounds", "extra-fatal")
        self.assertNotEqual(extra.returncode, 0)
        self.assertIn("fatals=2", extra.stderr)

    def test_invalid_profile_fatal_with_timeout_or_crash_status_is_rejected(self) -> None:
        for mode, status in (("invalid-timeout", 124), ("invalid-killed", 124), ("invalid-crash", 139)):
            with self.subTest(status=status):
                result = self.run_runner(f"invalid-status-{mode}", "bounds", mode)
                self.assertNotEqual(result.returncode, 0)
                self.assertIn(f"status={status}", result.stderr)


if __name__ == "__main__":
    unittest.main(verbosity=2)
