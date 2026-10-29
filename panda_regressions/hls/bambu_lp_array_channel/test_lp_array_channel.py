#!/usr/bin/env python3
# Copyright (C) 2026 Politecnico di Milano
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
"""Offline integration checks for the issue #434 Mantis/ModelSim launcher."""

import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import unittest
import xml.etree.ElementTree as ET


ROOT = Path(__file__).resolve().parents[3]
LAUNCHER = ROOT / "panda_regressions/hls/bambu_lp_array_channel.sh"
SUITE = ROOT / "panda_regressions/hls/bambu_lp_array_channel"
SCRATCH = ROOT / "documentation/tmp"


class LPArrayChannelIntegrationTest(unittest.TestCase):
    def setUp(self):
        SCRATCH.mkdir(parents=True, exist_ok=True)
        self.temp = tempfile.TemporaryDirectory(prefix="lp-array-channel-test-", dir=SCRATCH)
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.bin = self.root / "bin"
        self.bin.mkdir()
        self.cwd = self.root / "caller cwd"
        self.cwd.mkdir()
        self.invocations = self.root / "invocations"
        self.install_mocks()

    def executable(self, name, body):
        path = self.bin / name
        path.write_text("#!/usr/bin/env python3\n" + body, encoding="utf-8")
        path.chmod(0o755)
        return path

    def install_mocks(self):
        self.executable(
            "bambu",
            "import os, re, sys\n"
            "mode=os.environ.get('LP_MOCK_MODE','ok')\n"
            "with open(os.environ['LP_INVOCATIONS'],'a') as f: f.write('bambu\\n')\n"
            "fn=os.environ.get('MANTIS_EXPECT_FUNCTION','f')\n"
            "if mode != 'bad_schedule': print('II=1 solution found for loop (BB2)\\nInitiation Interval (II) for function '+fn+':BB2 = 1')\n"
            "if mode != 'bad_schedule': print('Iteration latency for function f:BB2 = 2')\n"
            "if mode == 'synth_nonzero': sys.exit(9)\n"
            "if mode != 'bad_schedule':\n"
            "  open('bambu_results.xml','w').write('<root><hls_results><function name=\"'+fn+'\"><loop_0 bb=\"2\" ii=\"1\"/></function></hls_results></root>')\n"
            "if mode != 'missing_rtl':\n"
            "  tb=os.environ.get('LP_TB','')\n"
            "  source=os.path.join(os.environ['LP_SUITE'],tb)\n"
            "  text=open(source).read()\n"
            "  duts=set(re.findall(r'^\\s*([A-Za-z_]\\w*)\\s+dut\\s*\\(',text,re.M))\n"
            "  if mode in ('shared_tb','missing_selected'): duts={'p_Z13dual_read_accRA16_iS0_R10ac_channelIiE'} if mode == 'missing_selected' else {'p_Z9dual_readRA16_iS0_R10ac_channelIiE'}\n"
            "  open('mock.v','w').write('\\n'.join('module '+dut+'(); endmodule' for dut in duts)+'\\n')\n",
        )
        for tool in ("vlib", "vlog", "vsim"):
            body = (
                "import os, sys\n"
                "mode=os.environ.get('LP_MOCK_MODE','ok')\n"
                "name=os.path.basename(sys.argv[0])\n"
                "if name == 'vlib': os.makedirs(sys.argv[-1],exist_ok=True)\n"
                "if name == 'vsim':\n"
                " bp=next((a.split('=',1)[1] for a in sys.argv if a.startswith('-gBP_MODE=')),'0')\n"
                " print('PASS [BP='+bp+']')\n"
                " if mode == 'sim_conflicting': print('FAIL [BP='+bp+']')\n"
                " if mode == 'sim_nonzero_pass': sys.exit(7)\n"
            )
            self.executable(tool, body)

    def run_suite(self, mode="ok", extra_args=(), path=None):
        env = os.environ.copy()
        env.update(
            PATH=path or (str(self.bin) + os.pathsep + env.get("PATH", "")),
            BAMBU_HLS_BACKEND_PATH="",
            LP_MOCK_MODE=mode,
            LP_INVOCATIONS=str(self.invocations),
            LP_SUITE=str(SUITE),
            TMPDIR=str(SCRATCH),
            TMP=str(SCRATCH),
            TEMP=str(SCRATCH),
        )
        return subprocess.run(
            [str(LAUNCHER), *extra_args], cwd=self.cwd, env=env,
            text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=120,
        )

    def output_dir(self):
        return self.cwd / "out_bambu_lp_array_channel_MODELSIM"

    def junit_failures(self):
        report = ET.parse(self.output_dir() / "test_report.xml").getroot()
        return len(report.findall(".//testcase/failure"))

    def case_dirs(self):
        return [path.parent for path in self.output_dir().rglob("execution.log")]

    def test_success_runs_all_cases_and_both_backpressure_modes(self):
        result = self.run_suite()
        details = "\n".join(
            f"{case.name}: {(case / 'execution.log').read_text()}"
            for case in self.case_dirs()
            if (case / "execution.log").exists()
        )
        self.assertEqual(result.returncode, 0, result.stdout + "\n" + details)
        self.assertEqual((self.invocations.read_text().count("bambu\n")), 3)
        report = ET.parse(self.output_dir() / "test_report.xml").getroot()
        self.assertEqual(report.get("failures"), "0")
        self.assertEqual(report.get("tests"), "3")
        for case in self.case_dirs():
            self.assertIn("PASS [BP=0]", (case / "vsim_BP0.log").read_text())
            self.assertIn("PASS [BP=2]", (case / "vsim_BP2.log").read_text())

    def test_synthesis_and_schedule_failures_override_plausible_messages(self):
        for mode in ("synth_nonzero", "bad_schedule"):
            with self.subTest(mode=mode):
                result = self.run_suite(mode)
                self.assertNotEqual(result.returncode, 0, result.stdout)
                root = self.output_dir()
                self.assertEqual(self.junit_failures(), 3)
                for case in self.case_dirs():
                    self.assertTrue((case / "failure.log").exists(), (case / "execution.log").read_text())
                    self.assertTrue((case / "execution.log").exists())
                    self.assertTrue((case / "bambu.log").exists())
                self.cleanup_output()

    def test_simulator_failure_and_conflicting_verdict_fail_junit(self):
        for mode in ("sim_nonzero_pass", "sim_conflicting"):
            with self.subTest(mode=mode):
                result = self.run_suite(mode)
                self.assertNotEqual(result.returncode, 0, result.stdout)
                self.assertEqual(self.junit_failures(), 3)
                for case in self.case_dirs():
                    self.assertTrue((case / "failure.log").exists(), (case / "execution.log").read_text())
                    self.assertTrue((case / "mock.v").exists())
                    self.assertTrue((case / "vlog_BP0.log").exists())
                    self.assertTrue((case / "vsim_BP0.log").exists())
                self.cleanup_output()

    def test_missing_rtl_and_modelsim_are_failures(self):
        result = self.run_suite("missing_rtl")
        self.assertNotEqual(result.returncode, 0, result.stdout)
        self.assertEqual(self.junit_failures(), 3)
        self.cleanup_output()
        for name in ("vlib", "vlog", "vsim"):
            (self.bin / name).unlink()
        isolated = self.root / "path-without-modelsim"
        isolated.mkdir()
        for name in ("bambu", "python3", "timeout", "dirname", "readlink", "mkdir", "grep", "sed", "awk", "find", "rm"):
            target = shutil.which(name)
            if name == "bambu":
                target = str(self.bin / name)
            if target:
                (isolated / name).symlink_to(target)
        result = self.run_suite("ok", path=str(isolated))
        self.assertNotEqual(result.returncode, 0, result.stdout)
        self.assertEqual(self.junit_failures(), 3)
        for case in self.case_dirs():
            self.assertIn(
                "ModelSim vsim not found in PATH or BAMBU_HLS_BACKEND_PATH",
                (case / "failure.log").read_text(),
            )

    def test_shared_testbench_checks_only_selected_module_dut(self):
        env = os.environ.copy()
        env.update(
            PATH=str(self.bin) + os.pathsep + env.get("PATH", ""),
            BAMBU_HLS_BACKEND_PATH="",
            LP_MOCK_MODE="shared_tb",
            LP_INVOCATIONS=str(self.invocations),
            LP_SUITE=str(SUITE),
            LP_TB="tb_lp_dual_array.v",
            LP_TB_TOP="tb_lp_dual_read",
            LP_LATENCY="2",
            TMPDIR=str(SCRATCH), TMP=str(SCRATCH), TEMP=str(SCRATCH),
        )
        result = subprocess.run(
            [str(SUITE / "run_case.sh")], cwd=self.cwd, env=env,
            text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=30,
        )
        self.assertEqual(result.returncode, 0, result.stdout)

        env["LP_MOCK_MODE"] = "missing_selected"
        result = subprocess.run(
            [str(SUITE / "run_case.sh")], cwd=self.cwd, env=env,
            text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=30,
        )
        self.assertNotEqual(result.returncode, 0, result.stdout)
        self.assertIn("generated RTL lacks DUT module p_Z9dual_readRA16_iS0_R10ac_channelIiE", result.stdout)

    def test_restart_reexecutes_failed_case_and_refuses_existing_output(self):
        failed = self.run_suite("bad_schedule")
        self.assertNotEqual(failed.returncode, 0, failed.stdout)
        self.assertEqual(self.junit_failures(), 3)
        for case in self.case_dirs():
            self.assertTrue((case / "failure.log").exists())
            self.assertTrue((case / "bambu.log").exists())
            self.assertTrue((case / "execution.log").exists())
        first_count = self.invocations.read_text().count("bambu\n")
        self.assertEqual(first_count, 3)
        refused = self.run_suite("ok")
        self.assertNotEqual(refused.returncode, 0, refused.stdout)
        restarted = self.run_suite("ok", ("--restart",))
        self.assertEqual(restarted.returncode, 0, restarted.stdout)
        self.assertEqual(self.invocations.read_text().count("bambu\n"), first_count + 3)
        self.assertFalse(any((case / "failure.log").exists() for case in self.case_dirs()))

    def test_existing_output_is_preserved_on_refusal(self):
        target = self.cwd / "out_bambu_lp_array_channel_MODELSIM"
        target.mkdir()
        sentinel = target / "keep"
        sentinel.write_text("preserve", encoding="utf-8")
        result = self.run_suite()
        self.assertNotEqual(result.returncode, 0, result.stdout)
        self.assertEqual(sentinel.read_text(), "preserve")

    def cleanup_output(self):
        shutil.rmtree(self.output_dir())


if __name__ == "__main__":
    unittest.main()
