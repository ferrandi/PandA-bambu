#!/usr/bin/env python3
"""Fail-closed contract tests for the AXI width sweep using fake ModelSim tools."""

from __future__ import annotations

import os
from pathlib import Path
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[4]
RUNNER = Path(__file__).with_name("run_axi_width_contract.sh")
TMP_ROOT = Path(os.environ.get("BURST_TEST_TMPDIR", ROOT / "documentation/tmp/wp7-w1-width-test"))

VSIM = r'''#!/usr/bin/env python3
import os, sys

args = sys.argv[1:]
if "-version" in args:
    print("QuestaSim fake width-contract")
    raise SystemExit(0)
with open(os.environ["FAKE_MODELSIM_CALLS"], "a", encoding="utf-8") as stream:
    stream.write("vsim " + " ".join(args) + "\n")

mode = os.environ.get("FAKE_MODELSIM_MODE", "pass")
top = next((arg for arg in args if arg.startswith("work.")), "")
width = int(next(arg.split("=", 1)[1] for arg in args if arg.startswith("-gDATA_BITS=")))
reader = top == "work.tb_reader_width_contract"
invalid = (reader and width in (2048, 4096)) or (not reader and width != 32)
if invalid:
    expected = ("BITSIZE_m_axi_rdata must be a power-of-two width in [8,1024]"
                if reader else "F1 writer supports 32-bit data beats only")
    if not reader and mode in ("neg-writer-elab12", "neg-writer-elab12-extra-error", "neg-writer-elab13"):
        print("** Fatal: " + expected)
        if mode == "neg-writer-elab12-extra-error":
            print("** Error: unrelated simulator error")
        raise SystemExit(12 if mode != "neg-writer-elab13" else 13)
    diagnostic = "wrong guard diagnostic" if mode == "neg-wrong-diagnostic" else expected
    if mode == "neg-missing-fatal":
        print(expected)
    else:
        print("** Fatal: " + diagnostic)
    if mode == "neg-duplicate-fatal":
        print("** Fatal: " + expected)
    if mode == "neg-extra-error":
        print("** Error: unrelated simulator error")
    if mode == "neg-extra-fatal":
        print("** Fatal: unrelated fatal")
    if mode == "neg-pass-marker":
        print("AXI_READER_WIDTH_PASS DATA_BITS=2048 ARSIZE=8")
    if mode == "neg-timeout":
        print("Simulation timeout")
        raise SystemExit(124)
    if mode == "neg-crash":
        print("Segmentation fault")
        raise SystemExit(139)
    raise SystemExit(0)

if reader:
    size = 0 if width <= 8 else (width // 8).bit_length() - 1
    marker = f"AXI_READER_WIDTH_PASS DATA_BITS={width} ARSIZE={size}"
else:
    marker = f"AXI_WRITER_WIDTH_PASS DATA_BITS={width}"
if mode == "pass-missing-marker":
    pass
else:
    if mode == "pass-wrong-marker":
        marker = marker.replace(f"DATA_BITS={width}", f"DATA_BITS={width + 1}")
    if mode == "pass-wrong-size" and reader:
        marker = marker.replace(f"ARSIZE={size}", f"ARSIZE={size + 1}")
    print(marker)
    if mode == "pass-duplicate-marker":
        print(marker)
if mode == "pass-extra-error":
    print("** Error: unrelated simulator error")
if mode == "pass-timeout":
    print("Simulation timeout")
    raise SystemExit(124)
if mode == "pass-crash":
    print("Segmentation fault")
    raise SystemExit(139)
raise SystemExit(0)
'''


class WidthContractRunnerTests(unittest.TestCase):
    def setUp(self) -> None:
        TMP_ROOT.mkdir(parents=True, exist_ok=True)
        self.temp = tempfile.TemporaryDirectory(prefix="width-contract-", dir=TMP_ROOT)
        self.root = Path(self.temp.name)
        self.bin = self.root / "bin"
        self.bin.mkdir()
        self.calls = self.root / "calls.log"
        for name in ("vsim", "vlib", "vmap", "vlog"):
            path = self.bin / name
            body = VSIM if name == "vsim" else "#!/usr/bin/env bash\nexit 0\n"
            path.write_text(body, encoding="utf-8")
            path.chmod(0o755)
        self.env = os.environ.copy()
        self.env.update(
            PATH=f"{self.bin}:{self.env['PATH']}",
            TMPDIR=str(self.root), TMP=str(self.root), TEMP=str(self.root),
            FAKE_MODELSIM_CALLS=str(self.calls),
        )

    def tearDown(self) -> None:
        self.temp.cleanup()

    def run_runner(self, name: str, mode: str = "pass") -> subprocess.CompletedProcess[str]:
        env = self.env.copy()
        env["FAKE_MODELSIM_MODE"] = mode
        return subprocess.run(
            ["bash", str(RUNNER), str(self.root / name)],
            cwd=ROOT,
            env=env,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            timeout=90,
            check=False,
        )

    def test_full_width_sweep_and_expected_parameter_failures_pass(self) -> None:
        result = self.run_runner("complete")
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        results = (self.root / "complete" / "results.log").read_text(encoding="utf-8")
        self.assertEqual(results.count("AXI_READER_WIDTH_PASS width="), 8)
        self.assertIn("AXI_WIDTH_EXPECTED_FAILURE PASS name=reader-2048", results)
        self.assertIn("AXI_WIDTH_EXPECTED_FAILURE PASS name=reader-4096", results)
        self.assertIn("AXI_WRITER_WIDTH_PASS width=32", results)
        self.assertIn("AXI_WIDTH_CONTRACT_PASS", results)
        self.assertNotIn("verilator ", self.calls.read_text(encoding="utf-8"))

    def test_writer_elaboration_fatal_status_12_is_accepted_for_exact_guard(self) -> None:
        result = self.run_runner("writer-elaboration-12", "neg-writer-elab12")
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        output = self.root / "writer-elaboration-12"
        self.assertIn("AXI_WIDTH_EXPECTED_FAILURE PASS name=writer-8 width=8",
                      (output / "results.log").read_text(encoding="utf-8"))
        self.assertEqual((output / "writer-8.raw.log.raw-status").read_text().strip(), "12")
        self.assertEqual((output / "writer-8.raw.log.status").read_text().strip(), "12")

    def test_positive_widths_reject_bad_markers_errors_timeouts_and_crashes(self) -> None:
        for mode in ("pass-missing-marker", "pass-duplicate-marker", "pass-wrong-marker", "pass-wrong-size",
                     "pass-extra-error", "pass-timeout", "pass-crash"):
            with self.subTest(mode=mode):
                result = self.run_runner(mode, mode)
                self.assertNotEqual(result.returncode, 0, result.stdout)

    def test_negative_widths_require_one_exact_fatal_and_reject_noise(self) -> None:
        for mode in ("neg-missing-fatal", "neg-wrong-diagnostic", "neg-duplicate-fatal",
                     "neg-extra-fatal", "neg-extra-error", "neg-pass-marker",
                     "neg-timeout", "neg-crash", "neg-writer-elab12-extra-error",
                     "neg-writer-elab13"):
            with self.subTest(mode=mode):
                result = self.run_runner(mode, mode)
                self.assertNotEqual(result.returncode, 0, result.stdout)


if __name__ == "__main__":
    unittest.main(verbosity=2)
