#!/usr/bin/env python3
"""Offline contract tests for the writer Vivado synthesis gate."""

from __future__ import annotations

import os
import subprocess
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[4]
RUNNER = Path(__file__).with_name("run_write_burst_parameter_synth.sh")
TMP_ROOT = Path(os.environ.get("BURST_TEST_TMPDIR", Path.cwd())) / "out_writer_synth_runner"

GUARDS = {
    "B0": "B_MAX must be in [1,256]",
    "B257": "B_MAX must be in [1,256]",
    "O0": "MAX_OUTSTANDING must be in [1,16]",
    "O17": "MAX_OUTSTANDING must be in [1,16]",
    "FIFO-BELOW-B": "FIFO_DEPTH must be a power of two >= B_MAX and <= 4096",
    "FIFO-NONPOWER": "FIFO_DEPTH must be a power of two >= B_MAX and <= 4096",
}


class WriterSynthesisRunnerTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        TMP_ROOT.mkdir(parents=True, exist_ok=True)

    def setUp(self) -> None:
        self.case = Path(tempfile.mkdtemp(prefix="case-", dir=TMP_ROOT))
        self.tools = self.case / "fake-tools"
        self.tools.mkdir()
        self.vivado = self.tools / "vivado"
        self.vivado.write_text(self.vivado_stub(), encoding="utf-8")
        self.vivado.chmod(0o755)
        self.trace = self.case / "vivado-trace.txt"
        self.env = os.environ.copy()
        self.env.update(
            PATH=f"{self.tools}{os.pathsep}{self.env['PATH']}",
            BURST_WRITER_SYNTH_OUTSIDE_SANDBOX="1",
            BURST_TEST_TMPDIR=str(TMP_ROOT),
            TMPDIR=str(self.case),
            TMP=str(self.case),
            TEMP=str(self.case),
            VIVADO_PART="xc-test-part",
            VIVADO_BIN=str(self.case / "ignored-VIVADO_BIN"),
            WRITER_SYNTH_TRACE=str(self.trace),
        )

    def tearDown(self) -> None:
        import shutil

        shutil.rmtree(self.case)

    @staticmethod
    def vivado_stub() -> str:
        guards = "\n".join(f"  {label}) message={message!r} ;;" for label, message in GUARDS.items())
        return f'''#!/usr/bin/env bash
set -u
if [[ ${{1:-}} == -version ]]; then echo 'Vivado Fake 2026.10'; exit 0; fi
if [[ -n ${{WRITER_SYNTH_TRACE:-}} ]]; then
  printf '%s|%s\\n' "$PWD" "${{TMPDIR:-unset}}" >> "$WRITER_SYNTH_TRACE"
fi
tcl=
for ((i=1; i<=$#; ++i)); do
  if [[ ${{!i}} == -source ]]; then j=$((i+1)); tcl=${{!j}}; break; fi
done
[[ -n $tcl && -r $tcl ]] || exit 97
case ${{tcl##*/}} in
  synth-accept-legal-boundaries.tcl) label=ACCEPT ;;
  synth-reject-*.tcl) label=${{tcl##*synth-reject-}}; label=${{label%.tcl}} ;;
  *) exit 98 ;;
esac
mode=${{WRITER_SYNTH_FAKE_MODE:-auto}}
message=
sv=${{tcl%/*}}/MinimalAXI4MasterPipelinedWrite.sv
case $label in
{guards}
  ACCEPT) message='unexpected synthesis rejection' ;;
esac
emit_reject() {{
  if [[ $mode != generic-caught-error ]]; then
    if [[ $mode == guard-extra-text ]]; then
      echo "ERROR: [Synth 8-6058] Synth Error: $message unrelated [${{sv}}:454]"
    else
      echo "ERROR: [Synth 8-6058] Synth Error: $message  [$sv:454]"
    fi
    if [[ $mode == duplicate-guard ]]; then
      echo "ERROR: [Synth 8-6058] Synth Error: $message  [$sv:454]"
    fi
    echo "ERROR: [Synth 8-6156] failed synthesizing module 'MinimalAXI4MasterPipelinedWrite' [$sv:2]"
    if [[ $mode == unrelated-error ]]; then
      echo 'ERROR: [Synth 8-9999] unrelated synthesis failure'
    fi
  fi
  if [[ $mode != missing-outcome ]]; then echo 'SYNTH_TOOL_OUTCOME error'; fi
  if [[ $mode != missing-message ]]; then
    echo 'SYNTH_TOOL_MESSAGE ERROR: [Common 17-69] Command failed: Vivado Synthesis failed'
  fi
}}
case $mode in
  auto)
    if [[ $label == ACCEPT ]]; then echo 'SYNTH_TOOL_OUTCOME ok'; else emit_reject; fi
    ;;
  crash-with-guard) emit_reject; exit 139 ;;
  timeout-with-guard) emit_reject; exit 124 ;;
  duplicate-outcome) emit_reject; echo 'SYNTH_TOOL_OUTCOME error' ;;
  duplicate-message) emit_reject; echo 'SYNTH_TOOL_MESSAGE ERROR: duplicate' ;;
  missing-outcome|missing-message|wrong-guard|duplicate-guard|unrelated-error|generic-caught-error|guard-extra-text)
    [[ $mode != wrong-guard ]] || message='wrong guard text'
    emit_reject
    ;;
  legal-error)
    if [[ $label == ACCEPT ]]; then
      echo 'ERROR: [Synth 8-9999] unrelated synthesis failure'
      echo 'SYNTH_TOOL_OUTCOME error'
      echo 'SYNTH_TOOL_MESSAGE ERROR: [Common 17-69] Command failed: Vivado Synthesis failed'
    else emit_reject; fi
    ;;
  *) exit 99 ;;
esac
'''

    def run_gate(self, mode: str = "auto") -> tuple[subprocess.CompletedProcess[str], Path]:
        output = self.case / f"out-{mode}"
        env = self.env.copy()
        env["WRITER_SYNTH_FAKE_MODE"] = mode
        result = subprocess.run(
            ["bash", str(RUNNER), str(output)],
            cwd=ROOT,
            env=env,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            check=False,
        )
        return result, output

    def test_path_vivado_exact_rejects_and_legal_accept(self) -> None:
        result, output = self.run_gate()
        self.assertEqual(result.returncode, 0, result.stdout)
        rows = (output / "results.log").read_text(encoding="utf-8").splitlines()
        self.assertEqual(sum(row.startswith("SYNTH_REJECT_PASS ") for row in rows), 6)
        self.assertEqual(sum(row.startswith("SYNTH_ACCEPT_PASS ") for row in rows), 1)
        self.assertIn("WRITER_PARAMETER_SYNTH_PASS cases=7 rejects=6 accepts=1", result.stdout)
        provenance = (output / "provenance.txt").read_text(encoding="utf-8")
        self.assertIn(f"vivado={self.vivado.resolve()}", provenance)
        self.assertIn("Vivado Fake 2026.10", provenance)
        trace = self.trace.read_text(encoding="utf-8").splitlines()
        self.assertEqual(len(trace), 7)
        self.assertTrue(all(str((output / "tmp").resolve()) in line for line in trace))

    def test_crash_and_timeout_after_expected_guard_are_rejected(self) -> None:
        for mode, status in (("crash-with-guard", "139"), ("timeout-with-guard", "124")):
            with self.subTest(mode=mode):
                result, output = self.run_gate(mode)
                self.assertNotEqual(result.returncode, 0, result.stdout)
                self.assertIn("WRITER_PARAMETER_SYNTH_FAIL", result.stdout)
                self.assertTrue((output / "synth-reject-B0.log").is_file())
                self.assertEqual((output / "synth-reject-B0.log.status").read_text().strip(), status)
                self.assertNotIn("SYNTH_REJECT_PASS reject-B0", (output / "results.log").read_text())

    def test_duplicate_missing_wrong_unrelated_and_generic_errors_fail_closed(self) -> None:
        for mode in ("duplicate-outcome", "duplicate-message", "missing-outcome", "missing-message",
                     "wrong-guard", "duplicate-guard", "unrelated-error", "generic-caught-error",
                     "guard-extra-text"):
            with self.subTest(mode=mode):
                result, output = self.run_gate(mode)
                self.assertNotEqual(result.returncode, 0, result.stdout)
                self.assertIn("WRITER_PARAMETER_SYNTH_FAIL", result.stdout)
                self.assertNotIn("SYNTH_REJECT_PASS reject-B0", (output / "results.log").read_text())

    def test_legal_synthesis_error_is_not_accepted(self) -> None:
        result, output = self.run_gate("legal-error")
        self.assertNotEqual(result.returncode, 0, result.stdout)
        rows = (output / "results.log").read_text(encoding="utf-8").splitlines()
        self.assertEqual(sum(row.startswith("SYNTH_REJECT_PASS ") for row in rows), 6)
        self.assertFalse(any(row.startswith("SYNTH_ACCEPT_PASS ") for row in rows))

    def test_vivado_bin_override_cannot_bypass_path_requirement(self) -> None:
        no_vivado = self.case / "no-vivado"
        no_vivado.mkdir()
        env = self.env.copy()
        env["PATH"] = f"{no_vivado}{os.pathsep}{os.environ['PATH']}"
        output = self.case / "missing-vivado-output"
        result = subprocess.run(
            ["bash", str(RUNNER), str(output)], cwd=ROOT, env=env,
            text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, check=False,
        )
        self.assertNotEqual(result.returncode, 0, result.stdout)
        self.assertIn("required tool is missing from PATH: vivado", result.stdout)
        self.assertFalse(output.exists())


if __name__ == "__main__":
    unittest.main()
