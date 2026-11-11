#!/usr/bin/env python3
"""Check the RTL-only CI entrypoint and failure propagation without Bambu."""

import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
import xml.etree.ElementTree as ET


ROOT = Path(__file__).resolve().parents[3]
SUITE = ROOT / "panda_regressions/verilog/axi_burst.sh"


class ComponentSuiteTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(dir=os.environ.get("BURST_TEST_TMPDIR"))
        self.addCleanup(self.temp.cleanup)
        self.directory = Path(self.temp.name)
        self.bin = self.directory / "bin"
        self.bin.mkdir()
        self.calls = self.directory / "calls"
        for name in ("vsim", "vlib", "vlog", "vmap"):
            self.executable(name, "#!/bin/sh\nexit 0\n")
        self.executable("bambu", '#!/bin/sh\necho UNEXPECTED_BAMBU >> "$CALL_LOG"\nexit 99\n')
        self.executable("bash", '''#!/bin/sh
case "$1" in
  */rtl_tests/run_*.sh)
    name=$(basename "$1")
    echo "$name" >> "$CALL_LOG"
    if [ "$name" = "${FAIL_RTL:-}" ]; then exit 7; fi
    exit 0 ;;
  *) exec /bin/bash "$@" ;;
esac
''')
        self.executable("python3", f'''#!/bin/sh
if [ "$1" = -B ]; then shift; fi
if [ "$1" = -m ]; then exit 0; fi
exec "{sys.executable}" "$@"
''')

    def executable(self, name, body):
        path = self.bin / name
        path.write_text(body)
        path.chmod(0o755)

    def run_suite(self, failure=""):
        output = self.directory / "output"
        env = os.environ | {"PATH": str(self.bin) + os.pathsep + os.environ["PATH"],
                            "CALL_LOG": str(self.calls), "FAIL_RTL": failure}
        result = subprocess.run(["/bin/bash", str(SUITE), str(output)], env=env,
                                capture_output=True, text=True, timeout=30)
        report = ET.parse(output / "junit.xml").getroot()
        return result, report

    def test_all_rtl_groups_are_scheduled_without_bambu(self):
        result, report = self.run_suite()
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(report.get("failures"), "0")
        self.assertEqual({case.get("name") for case in report.findall("testcase")}, {
            "fifo", "memory", "openmp", "offline-rtl", "offline-runners", "read-master", "write-master",
            "vlog", "diagnostics", "widths", "duplex-core", "adapter-size", "memory-model"})
        calls = self.calls.read_text().splitlines()
        self.assertNotIn("UNEXPECTED_BAMBU", calls)
        self.assertEqual(len(calls), 11)
        for name in ("run_read_burst_modelsim.sh", "run_write_burst_engine.sh", "run_duplex_core_tests.sh",
                     "run_adapter_size_guard.sh", "run_axi_width_contract.sh", "run_burst_fault_diagnostics.sh",
                     "run_testbench_memaxi_model.sh", "run_vlog_portability.sh"):
            self.assertIn(name, calls)
        workflow = (ROOT / ".github/workflows/minimal.yml").read_text()
        job = workflow.split("  verilog-axi-burst:", 1)[1].split("  bambu-axi-burst:", 1)[0]
        self.assertIn("bash panda_regressions/verilog/axi_burst.sh", job)
        self.assertEqual(workflow.count("  verilog-axi-burst:\n"), 1)
        self.assertEqual(workflow.count("  bambu-axi-burst:\n"), 1)
        self.assertNotIn("bambu-axi-burst-compiler", workflow)
        self.assertIn("--group=all --no-clean", workflow)
        self.assertNotIn("run_axi_area_matrix.py", job)
        self.assertNotIn("run_write_burst_parameter_synth.sh", job)
        hls = (ROOT / "panda_regressions/hls/bambu_axi_burst.sh").read_text()
        self.assertNotIn("run_read_burst_modelsim.sh", hls)
        self.assertNotIn("run_write_burst_engine.sh", hls)
        family = ROOT / "panda_regressions/hls/bambu_axi_burst"
        verilog = ROOT / "panda_regressions/verilog/axi_burst"
        runners = ["run_generated_kernel_tests.sh", "run_generated_write_kernel_tests.sh",
                   "run_generated_duplex_tests.sh", "run_generated_duplex_kernel_tests.sh",
                   "run_generated_memaxi_response_tests.sh", "run_generated_wrapper_rid_tests.sh",
                   "run_default_duplex_reduction_modelsims.sh", "run_duplex_iteration_tests.sh"]
        for runner in runners:
            self.assertTrue((family / runner).is_file(), runner)
            self.assertFalse(list(verilog.rglob(runner)), runner)
        for check in ("test_generated_read_modelsim_runner.py", "test_generated_memaxi_response_runner.py",
                      "test_generated_duplex_runner.py", "test_generated_duplex_kernel_runner.py",
                      "test_duplex_iteration_results.py", "test_duplex_iteration_runner.py"):
            self.assertEqual(hls.count('/' + check + '\"'), 1, check)
            self.assertFalse(list(verilog.rglob(check)), check)

    def test_rtl_failure_fails_ci_and_is_kept_in_junit(self):
        result, report = self.run_suite("run_read_burst_modelsim.sh")
        self.assertEqual(result.returncode, 1)
        self.assertEqual(report.get("failures"), "1")
        failures = [case.get("name") for case in report.findall("testcase") if case.find("failure") is not None]
        self.assertEqual(failures, ["read-master"])
        self.assertIn("run_testbench_memaxi_model.sh", self.calls.read_text())


if __name__ == "__main__":
    unittest.main()
