#!/usr/bin/env python3
"""Behavioral tests for generated-kernel and wrapper-RID ModelSim runners."""

from __future__ import annotations

import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest


REPO = Path(__file__).resolve().parents[4]
RTL_TESTS = Path(__file__).resolve().parent
TASK_ROOT = Path(os.environ.get("BURST_TEST_TMPDIR", Path.cwd())) / f"out_{Path(__file__).stem}"
TASK_ROOT.mkdir(parents=True, exist_ok=True)

KERNEL_RUNNER = RTL_TESTS / "run_generated_kernel_tests.sh"
WRAPPER_RUNNER = RTL_TESTS / "run_generated_wrapper_rid_tests.sh"


class GeneratedReadModelSimRunnerTests(unittest.TestCase):
    def setUp(self) -> None:
        self.temp = tempfile.TemporaryDirectory(prefix="generated-modelsim-", dir=TASK_ROOT)
        self.addCleanup(self.temp.cleanup)
        self.base = Path(self.temp.name)
        self.prefix = self.base / "prefix"
        (self.prefix / "bin").mkdir(parents=True)
        (self.prefix / "lib/panda/clang-16").mkdir(parents=True)
        (self.prefix / "lib/panda/clang-16/ASTAnalyzer.so").write_text("fake plugin\n")
        (self.prefix / "settings.sh").write_text(f"export BAMBU_HLS='{self.prefix}'\n")
        self._write_executable(self.prefix / "bin/bambu", self._bambu_script())
        self.tools = self.base / "tools"
        self.tools.mkdir()
        for name in ("vsim", "vlog", "vlib", "vmap"):
            self._write_executable(self.tools / name, self._modelsim_tool())
        self._write_executable(self.tools / "timeout", self._timeout_tool())
        self.verilator_calls = self.base / "verilator-called"
        self._write_executable(self.tools / "verilator", f"#!/bin/sh\nprintf called > '{self.verilator_calls}'\nexit 99\n")

    @staticmethod
    def _write_executable(path: Path, contents: str) -> None:
        path.write_text(contents)
        path.chmod(0o755)

    def _bambu_script(self) -> str:
        return r'''#!/usr/bin/env python3
import os, pathlib, sys
if "--version" in sys.argv:
    print("Bambu fake 1.0")
    print("fake build")
    raise SystemExit(0)
cwd = pathlib.Path.cwd()
(cwd / "kernel.v").write_text("""module kernel(\n  MinimalAXI4MasterPipelined #(.B_MAX(16), .MAX_OUTSTANDING(1), .FIFO_DEPTH(256), .BITSIZE_m_axi_araddr(32), .BITSIZE_m_axi_rdata(32), .BITSIZE_in5(32)) src_bambu_artificial_ParmMgr_modgen_16_i0 ();\nendmodule\nmodule src_bambu_artificial_ParmMgr_modgen();\nwire burst_rid = |p_m_axi_src_rid;\nendmodule\n""")
(cwd / "panda_libtech.v").write_text("""module MinimalAXI4MasterPipelined(\n);\nwire fifo_load_pop;\nreg [31:0] fifo_reg [0:FIFO_DEPTH-1];\nendmodule\n""")
print("Configured read burst regions for bundle src (B=16)", file=sys.stderr)
'''

    def _modelsim_tool(self) -> str:
        return r'''#!/usr/bin/env python3
import os, pathlib, sys
name = pathlib.Path(sys.argv[0]).name
args = sys.argv[1:]
mode = os.environ.get("FAKE_MODELSIM_MODE", "pass")
if name == "vsim" and args == ["-version"]:
    print("Questa fake 2026.1")
    raise SystemExit(0)
if name == "vmap":
    if "-c" in args:
        pathlib.Path("modelsim.ini").write_text("[Library]\n")
    raise SystemExit(0)
if name == "vlib":
    pathlib.Path(args[-1]).mkdir(parents=True, exist_ok=True)
    raise SystemExit(0)
if name == "vlog":
    if mode == "compile-fail":
        print("fake vlog compile error", file=sys.stderr)
        raise SystemExit(9)
    raise SystemExit(0)
if name == "vsim":
    top = next((arg.split(".", 1)[1] for arg in args if arg.startswith("work.")), "")
    negative = "+NEGATIVE_SENSITIVITY" in args
    if mode == "timeout":
        print("fake timeout requested", file=sys.stderr)
        raise SystemExit(124)
    if mode == "crash":
        print("Segmentation fault (fake)", file=sys.stderr)
        raise SystemExit(139)
    if mode == "fatal-zero":
        print("** Fatal: injected fatal with zero simulator exit")
        raise SystemExit(0)
    if top == "tb_generated_kernel":
        if negative:
            print("** Fatal: AXI response data order/value mismatch")
            raise SystemExit(1)
        for count in (0, 1, 15, 16, 17, 38, 1024, 2048, 4096):
            print(f"KERNEL_CASE PASS n={count} cycles=1")
        if mode != "missing-marker":
            print("GENERATED_KERNEL_ALL_CASES PASS")
        raise SystemExit(0)
    if top == "tb_generated_wrapper_rid":
        if negative:
            print("RID_CASE PASS rid=2 completion=1")
            print("RID_CASE PASS rid=32 completion=1")
            if mode != "missing-marker":
                print("RID_NEGATIVE_SENSITIVITY PASS (bit-0-only reduction accepted 2 and 32)")
        else:
            for rid, completion in ((0, 1), (1, 0), (2, 0), (32, 0), (63, 0)):
                print(f"RID_CASE PASS rid={rid} completion={completion}")
            if mode != "missing-marker":
                print("GENERATED_WRAPPER_RID PASS")
        raise SystemExit(0)
    print("unknown fake top", file=sys.stderr)
    raise SystemExit(2)
raise SystemExit(2)
'''

    def _timeout_tool(self) -> str:
        real_timeout = shutil.which("timeout")
        if real_timeout is None:
            raise RuntimeError("timeout is required for the ModelSim runner tests")
        return f'''#!/usr/bin/env python3
import os, sys
args = sys.argv[1:]
mode = os.environ.get("FAKE_MODELSIM_MODE", "pass")
if mode == "timeout" and args and args[0] in ("60s", "60"):
    print("fake timeout reached", file=sys.stderr)
    raise SystemExit(124)
os.execv("{real_timeout}", ["{real_timeout}", *args])
'''

    def _env(self, mode: str = "pass") -> dict[str, str]:
        env = os.environ.copy()
        env["PATH"] = f"{self.tools}:{self.prefix / 'bin'}:{env['PATH']}"
        env["FAKE_MODELSIM_MODE"] = mode
        for key in ("TMPDIR", "TMP", "TEMP", "BURST_TEST_TMPDIR"):
            if key != "BURST_TEST_TMPDIR":
                env[key] = str(self.base / "tmp")
        (self.base / "tmp").mkdir(exist_ok=True)
        return env

    def _run_kernel(self, name: str, mode: str = "pass") -> subprocess.CompletedProcess[str]:
        return subprocess.run(
            [str(KERNEL_RUNNER), str(self.prefix / "bin/bambu"), str(self.base / name)],
            cwd=REPO, env=self._env(mode), text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
            timeout=20,
        )

    def _run_wrapper(self, name: str, mode: str = "pass") -> subprocess.CompletedProcess[str]:
        return subprocess.run(
            [str(WRAPPER_RUNNER), str(self.prefix / "bin/bambu"), "", str(self.base / name)],
            cwd=REPO, env=self._env(mode), text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
            timeout=20,
        )

    def _assert_no_verilator(self) -> None:
        self.assertFalse(self.verilator_calls.exists(), "generated runner invoked Verilator")

    def test_kernel_pass_and_negative_scoreboard_witness(self) -> None:
        result = self._run_kernel("kernel-pass")
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        aggregate = (self.base / "kernel-pass/aggregate.log").read_text()
        self.assertIn("GENERATED_KERNEL_ALL_CASES PASS", aggregate)
        for count in (0, 1, 15, 16, 17, 38, 1024, 2048, 4096):
            self.assertEqual(aggregate.count(f"KERNEL_CASE PASS n={count} "), 1)
        self.assertIn("GENERATED_KERNEL_NEGATIVE_SENSITIVITY PASS", result.stdout)
        self._assert_no_verilator()

    def test_kernel_rejects_missing_marker_compile_fatal_timeout_and_crash(self) -> None:
        for mode, diagnostic in (
            ("missing-marker", "aggregate PASS marker"),
            ("compile-fail", "ModelSim compile failed"),
            ("fatal-zero", "positive simulation failed"),
            ("timeout", "positive simulation failed"),
            ("crash", "positive simulation failed"),
        ):
            with self.subTest(mode=mode):
                result = self._run_kernel(f"kernel-{mode}", mode)
                self.assertNotEqual(result.returncode, 0)
                self.assertIn(diagnostic, result.stdout + result.stderr)
        self._assert_no_verilator()

    def test_wrapper_rid_positive_and_mutated_rid_are_both_witnessed(self) -> None:
        result = self._run_wrapper("wrapper-pass")
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        positive = (self.base / "wrapper-pass/positive/run.log.normalized").read_text()
        for rid, completion in ((0, 1), (1, 0), (2, 0), (32, 0), (63, 0)):
            self.assertEqual(positive.count(f"RID_CASE PASS rid={rid} completion={completion}"), 1)
        self.assertEqual(positive.count("GENERATED_WRAPPER_RID PASS"), 1)
        negative = (self.base / "wrapper-pass/negative/run.log.normalized").read_text()
        self.assertEqual(negative.count("RID_CASE PASS rid=2 completion=1"), 1)
        self.assertEqual(negative.count("RID_CASE PASS rid=32 completion=1"), 1)
        self.assertEqual(negative.count("RID_NEGATIVE_SENSITIVITY PASS (bit-0-only reduction accepted 2 and 32)"), 1)
        self._assert_no_verilator()

    def test_wrapper_rejects_missing_marker_compile_fatal_timeout_and_crash(self) -> None:
        for mode, diagnostic in (
            ("missing-marker", "exact aggregate marker"),
            ("compile-fail", "ModelSim compile failed"),
            ("fatal-zero", "simulation failed"),
            ("timeout", "simulation failed"),
            ("crash", "simulation failed"),
        ):
            with self.subTest(mode=mode):
                result = self._run_wrapper(f"wrapper-{mode}", mode)
                self.assertNotEqual(result.returncode, 0)
                self.assertIn(diagnostic, result.stdout + result.stderr)
        self._assert_no_verilator()


if __name__ == "__main__":
    unittest.main(verbosity=2)
