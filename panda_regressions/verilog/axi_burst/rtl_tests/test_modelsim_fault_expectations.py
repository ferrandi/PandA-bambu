#!/usr/bin/env python3
"""Offline contract tests for exact ModelSim AXI fault diagnostics."""

from __future__ import annotations

import os
import subprocess
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[4]
HELPER = Path(__file__).with_name("modelsim_helpers.sh")
TMP_ROOT = Path(os.environ.get("BURST_TEST_TMPDIR", Path.cwd())) / "out_modelsim_fault_expectations"
READ_FAULT = "AXI_BURST_FAULT engine=read reason=RRESP instance=tb.reader id=0"
PEER_FAULT = "AXI_BURST_PEER_FAULT engine=write reason=peer instance=tb.writer"
IGNORED_COMMAND = "AXI_BURST_COMMAND_IGNORED engine=read opcode=7 instance=tb.reader"


class ModelSimFaultExpectationTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        TMP_ROOT.mkdir(parents=True, exist_ok=True)

    def check_log(self, mode: str, raw: int, effective: int, transcript: str,
                  *expected: str, label: str = "case") -> subprocess.CompletedProcess[str]:
        with tempfile.TemporaryDirectory(prefix="case-", dir=TMP_ROOT) as directory:
            log = Path(directory) / "run.log"
            log.write_text(transcript, encoding="utf-8")
            Path(f"{log}.raw-status").write_text(f"{raw}\n", encoding="utf-8")
            Path(f"{log}.status").write_text(f"{effective}\n", encoding="utf-8")
            return subprocess.run(
                ["bash", "-c", 'source "$1"; shift; modelsim_expect_fault_log "$@"',
                 "modelsim_expect_fault_log", str(HELPER), str(log), mode, label, *expected],
                cwd=ROOT,
                env=os.environ.copy(),
                text=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                check=False,
            )

    def test_fail_fast_accepts_exact_diagnostic_and_no_pass(self) -> None:
        result = self.check_log("fatal", 0, 1, f"# ** Fatal: {READ_FAULT}\n", READ_FAULT)
        self.assertEqual(result.returncode, 0, result.stdout)

    def test_fail_fast_accepts_raw_runtime_failure_status(self) -> None:
        result = self.check_log("fatal", 1, 1, f"# ** Fatal: {READ_FAULT}\n", READ_FAULT)
        self.assertEqual(result.returncode, 0, result.stdout)

    def test_only_named_expected_fatal_control_accepts_aborted(self) -> None:
        transcript = f"# ** Fatal: {READ_FAULT}\n# Aborted\n"
        result = self.check_log("fatal-control", 134, 134, transcript, READ_FAULT,
                                label="AXI_BURST_EXPECT_FATAL_CONTROL")
        self.assertEqual(result.returncode, 0, result.stdout)
        ordinary = self.check_log("fatal", 134, 134, transcript, READ_FAULT)
        self.assertNotEqual(ordinary.returncode, 0, ordinary.stdout)
        continuation = self.check_log("continue", 134, 134, transcript, READ_FAULT)
        self.assertNotEqual(continuation.returncode, 0, continuation.stdout)

    def test_aborted_control_requires_its_exact_label(self) -> None:
        result = self.check_log("fatal-control", 134, 134, f"# ** Fatal: {READ_FAULT}\n# Aborted\n",
                                READ_FAULT, label="other-case")
        self.assertNotEqual(result.returncode, 0, result.stdout)

    def test_containment_accepts_exact_local_and_peer_diagnostic_set(self) -> None:
        result = self.check_log("continue", 0, 0, f"# {READ_FAULT}\n# {PEER_FAULT}\n",
                                READ_FAULT, PEER_FAULT)
        self.assertEqual(result.returncode, 0, result.stdout)

    def test_containment_accepts_ignored_command_diagnostic(self) -> None:
        result = self.check_log("continue", 0, 0, f"# {IGNORED_COMMAND}\n", IGNORED_COMMAND)
        self.assertEqual(result.returncode, 0, result.stdout)

    def test_rejects_wrong_reason_instance_count_extra_error_watchdog_timeout_crash_and_stale_pass(self) -> None:
        cases = (
            ("wrong reason", 0, 0, "# AXI_BURST_FAULT engine=read reason=RID instance=tb.reader id=0\n"),
            ("wrong instance", 0, 0, f"# {READ_FAULT.replace('tb.reader', 'tb.other')}\n"),
            ("extra diagnostic", 0, 0, f"# {READ_FAULT}\n# {PEER_FAULT}\n"),
            ("extra error", 0, 0, f"# {READ_FAULT}\n# ** Error: unrelated\n"),
            ("watchdog", 0, 0, f"# {READ_FAULT}\n# fixture watchdog expired\n"),
            ("timeout status", 124, 124, f"# {READ_FAULT}\n"),
            ("crash", 0, 1, f"# ** Fatal: {READ_FAULT}\n# Aborted\n"),
            ("stale pass", 0, 1,
             f"# ** Fatal: {READ_FAULT}\n# WRITE_BURST_ENGINE PASS CASE=old\n"),
        )
        for label, raw, effective, transcript in cases:
            with self.subTest(label=label):
                result = self.check_log("fatal" if effective else "continue", raw,
                                        effective, transcript, READ_FAULT)
                self.assertNotEqual(result.returncode, 0, result.stdout)

    def test_rejects_unexpected_fatal_in_continuation(self) -> None:
        result = self.check_log("continue", 0, 0, f"# ** Fatal: {READ_FAULT}\n", READ_FAULT)
        self.assertNotEqual(result.returncode, 0, result.stdout)

    def test_rejects_missing_status_files(self) -> None:
        with tempfile.TemporaryDirectory(prefix="missing-status-", dir=TMP_ROOT) as directory:
            log = Path(directory) / "run.log"
            log.write_text(f"# {READ_FAULT}\n", encoding="utf-8")
            result = subprocess.run(
                ["bash", "-c", 'source "$1"; modelsim_expect_fault_log "$2" continue case "$3"',
                 "modelsim_expect_fault_log", str(HELPER), str(log), READ_FAULT],
                cwd=ROOT,
                env=os.environ.copy(),
                text=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                check=False,
            )
            self.assertNotEqual(result.returncode, 0, result.stdout)


if __name__ == "__main__":
    unittest.main()
