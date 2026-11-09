#!/usr/bin/env python3
"""Offline contract tests for the bounded TestbenchMEMAXI ModelSim runner."""

from __future__ import annotations

import os
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[4]
SCRIPT = ROOT / "panda_regressions/verilog/axi_burst/rtl_tests/run_testbench_memaxi_model.sh"
SCRATCH = Path(os.environ.get("BURST_TEST_TMPDIR", Path.cwd() / "out_testbench_memaxi_model_runner"))


class ModelRunnerContract(unittest.TestCase):
    def setUp(self) -> None:
        SCRATCH.mkdir(parents=True, exist_ok=True)
        self.case = Path(tempfile.mkdtemp(prefix="case-", dir=SCRATCH))
        self.bin = self.case / "modelsim/questasim/bin"
        self.bin.mkdir(parents=True)
        self.env = os.environ.copy()
        for name in ("TMPDIR", "TMP", "TEMP"):
            self.env[name] = str(self.case / "tmp")
        Path(self.env["TMPDIR"]).mkdir()
        self.env["MODELSIM_ROOT"] = str(self.case / "modelsim")
        self.env["BURST_MODELSIM_OUTSIDE_SANDBOX"] = "1"
        self.env["MOCK_MODE"] = "pass"
        self.write_tool("vlib", "mkdir -p work")
        self.write_tool("vmap", "mkdir -p work")
        self.write_tool("vlog", "exit 0")
        self.write_tool("vsim", self.vsim_body())

    def tearDown(self) -> None:
        shutil.rmtree(self.case, ignore_errors=True)

    def write_tool(self, name: str, body: str) -> None:
        path = self.bin / name
        path.write_text("#!/bin/bash\nset -eu\n" + body + "\n", encoding="utf-8")
        path.chmod(0o755)

    @staticmethod
    def vsim_body() -> str:
        return r'''if [[ ${1:-} == -version ]]; then echo "Questa mock version"; exit 0; fi
args=" $* "
delay=$(sed -n 's/.*-gWRITE_DELAY=\([0-9]*\).*/\1/p' <<<"$args")
case_name=$(sed -n 's/.*+NEGATIVE=\([^ ]*\).*/\1/p' <<<"$args")
if [[ ${MOCK_MODE:-pass} == timeout && -n $case_name ]]; then
    echo "F0_NEGATIVE_START case=$case_name delay=$delay"
    echo "** Fatal: AXI WLAST mismatch at beat 0 of 2"
    exit 124
fi
if [[ -n $case_name ]]; then
    echo "F0_NEGATIVE_START case=$case_name delay=$delay"
    case $case_name in
      early_wlast) diagnostic='AXI WLAST mismatch at beat 0 of 2' ;;
      missing_final_wlast) diagnostic='AXI WLAST mismatch at beat 1 of 2' ;;
      read_wrap) diagnostic='Unsupported AXI read burst type: 2' ;;
      read_reserved) diagnostic='Unsupported AXI read burst type: 3' ;;
      write_wrap) diagnostic='Unsupported AXI write burst type: 2' ;;
      write_reserved) diagnostic='Unsupported AXI write burst type: 3' ;;
    esac
    echo "** Fatal: $diagnostic"
    echo "  Process: /memory/independent_axi_model"
    echo "  File: $PWD/hdl/TestbenchMEMAXI.sv"
    if [[ ${MOCK_MODE:-pass} == aborted ]]; then
        echo "Aborted (core dumped)"
        exit 134
    fi
    if [[ ${MOCK_MODE:-pass} == aborted-missing ]]; then
        exit 134
    fi
    exit 1
fi
if [[ ${MOCK_MODE:-pass} == positive-error ]]; then
    echo "F0_QUALIFY_PASS delay=$delay"
    echo "** Error: injected failure"
    exit 0
fi
echo "F0_QUALIFY_PASS delay=$delay"
if [[ ${MOCK_MODE:-pass} != missing-response-injection-marker ]]; then
    echo "# F0_RESPONSE_INJECTION_PASS read=SLVERR,DECERR,OKAY write=SLVERR,DECERR,OKAY reset=cleared stall=stable"
fi
echo "F0_WITNESS_PASS R_MULTI_BEAT_DRAIN"
for order in 0 1 2; do echo "F0_WRITE_TIMING_PASS order=$order delay=$delay beat=0"; done
exit 0'''

    def run_qualify(self, mode: str = "pass") -> subprocess.CompletedProcess[str]:
        self.env["MOCK_MODE"] = mode
        output = self.case / f"out-{mode}"
        return subprocess.run(
            [str(SCRIPT), "qualify", str(output)], cwd=self.case, env=self.env,
            text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, check=False,
        )

    def test_qualify_accepts_positive_and_all_exact_negative_diagnostics(self) -> None:
        result = self.run_qualify()
        self.assertEqual(result.returncode, 0, result.stdout)
        out = self.case / "out-pass"
        logs = sorted((out / "logs").glob("*.log"))
        self.assertEqual(len(logs), 19)  # extraction, version, tool logs, 2 positive + 12 negative
        for log in logs:
            if log.name in {"extract.log", "vlib.log", "vmap.log", "vlog.log"}:
                continue
            self.assertTrue(log.with_suffix(log.suffix + ".raw-status").exists())
            self.assertTrue(log.with_suffix(log.suffix + ".effective-status").exists())

    def test_timeout_rejects_negative_even_after_expected_fatal(self) -> None:
        result = self.run_qualify("timeout")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("exact-fatal contract", result.stdout)
        logs = list((self.case / "out-timeout/logs").glob("negative-*.log"))
        self.assertEqual(len(logs), 1)
        self.assertEqual(logs[0].with_suffix(logs[0].suffix + ".raw-status").read_text().strip(), "124")

    def test_positive_error_rejects_pass_marker(self) -> None:
        result = self.run_qualify("positive-error")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("failed despite any transcript markers", result.stdout)

    def test_missing_response_injection_marker_fails_closed(self) -> None:
        result = self.run_qualify("missing-response-injection-marker")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("did not prove TestbenchMEMAXI response injection", result.stdout)
        log = self.case / "out-missing-response-injection-marker/logs/qualify-delay-1.log"
        self.assertTrue(log.is_file())
        self.assertIn("F0_QUALIFY_PASS", log.read_text(encoding="utf-8"))
        self.assertNotIn("F0_RESPONSE_INJECTION_PASS", log.read_text(encoding="utf-8"))

    def test_raw_134_accepts_exact_fatal_with_single_aborted_and_context(self) -> None:
        result = self.run_qualify("aborted")
        self.assertEqual(result.returncode, 0, result.stdout)
        log = self.case / "out-aborted/logs/negative-early_wlast-delay-1.log"
        self.assertEqual(log.with_suffix(log.suffix + ".raw-status").read_text().strip(), "134")
        self.assertEqual(log.with_suffix(log.suffix + ".effective-status").read_text().strip(), "134")

    def test_raw_134_without_aborted_transcript_is_rejected(self) -> None:
        result = self.run_qualify("aborted-missing")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("unexpected Aborted transcript", result.stdout)
        log = self.case / "out-aborted-missing/logs/negative-early_wlast-delay-1.log"
        self.assertEqual(log.with_suffix(log.suffix + ".raw-status").read_text().strip(), "134")


if __name__ == "__main__":
    unittest.main()
