#!/usr/bin/env python3
"""Check AXI master selection and the complete matrix scheduled by CI."""

import os
from pathlib import Path
import shlex
import subprocess
import sys
import tempfile
import unittest

from check_axi_master_specialization import check


HERE = Path(__file__).resolve().parent


def instance(write=False, burst=16, outstanding=3):
    name = "MinimalAXI4MasterPipelined" + ("Write" if write else "")
    return f"{name} #(.B_MAX({burst}), .MAX_OUTSTANDING({outstanding})) engine{int(write)} ();\n"


class AXIMasterSpecializationTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(dir=os.environ.get("BURST_TEST_TMPDIR"))
        self.addCleanup(self.temp.cleanup)
        self.output = Path(self.temp.name)

    def test_scalar_profile_rejects_a_configured_engine(self):
        (self.output / "kernel.v").write_text(instance(burst=1, outstanding=1))
        with self.assertRaisesRegex(ValueError, "historical single-beat"):
            check(self.output, "read", 0)

    def test_duplex_checks_both_parameter_sets(self):
        (self.output / "kernel.v").write_text(instance() + instance(write=True, outstanding=1))
        with self.assertRaisesRegex(ValueError, "MAX_OUTSTANDING=3"):
            check(self.output, "duplex", 3)

    def test_missing_direction_and_commented_instances_are_rejected(self):
        (self.output / "kernel.v").write_text(instance() + "/* " + instance(write=True) + " */")
        with self.assertRaisesRegex(ValueError, "wrong AXI master selection"):
            check(self.output, "duplex", 3)

    def test_checker_cli_propagates_failure(self):
        result = subprocess.run(
            [sys.executable, str(HERE / "check_axi_master_specialization.py"),
             "--output", str(self.output), "--direction=read", "--profile=0"],
            capture_output=True, text=True)
        self.assertEqual(result.returncode, 1)
        self.assertIn("AXI_MASTER_SPECIALIZATION_FAIL", result.stderr)

    def test_ci_manifest_schedules_all_twelve_cosimulations_and_post_checks(self):
        rows = [shlex.split(line) for line in (HERE.parent / "bambu_axi_burst_list").read_text().splitlines()
                if "axi_specialization.cpp" in line]
        cases = set()
        for row in rows:
            env = next(arg for arg in row if arg.startswith("BENCHMARK_ENV="))
            command = shlex.split(env.removeprefix("BENCHMARK_ENV=MANTIS_POST_CHECK="))
            self.assertEqual(command[:3], ["python3", "-B", "BENCHMARKS_ROOT/check_axi_master_specialization.py"])
            direction = next(arg.split("=", 1)[1] for arg in command if arg.startswith("--direction="))
            profile = int(next(arg.split("=", 1)[1] for arg in command if arg.startswith("--profile=")))
            self.assertNotIn((direction, profile), cases)
            cases.add((direction, profile))
            self.assertIn(f"--extra-cc-options=-DAXI_PROFILE={profile}", row)
            self.assertIn("--tb-arg=37", row)
            bench = {"read": "bambu_axi_burst_tb.cpp", "write": "bambu_axi_burst_write_tb.cpp",
                     "duplex": "bambu_axi_burst_duplex_copy_tb.cpp"}[direction]
            self.assertIn(f"--generate-tb=BENCHMARKS_ROOT/{bench}", row)
            if direction != "duplex":
                self.assertIn(f"--extra-cc-options=-DAXI_{direction.upper()}", row)
            if direction != "write":
                self.assertIn("--tb-param-size=src:16384", row)
            if direction != "read":
                self.assertIn("--tb-param-size=dst:16384", row)
            if profile == 0:
                rtl = "MinimalAXI4AdapterSingleBeat #(.BITSIZE_in1(32)) adapter ();"
            else:
                burst, outstanding = (16 if profile in (1, 3) else 1), (3 if profile in (2, 3) else 1)
                rtl = (instance(burst=burst, outstanding=outstanding) if direction != "write" else "")
                rtl += instance(write=True, burst=burst, outstanding=outstanding) if direction != "read" else ""
            (self.output / "kernel.v").write_text(rtl)
            check(self.output, direction, profile)
        self.assertEqual(cases, {(direction, profile) for direction in ("read", "write", "duplex")
                                 for profile in range(4)})


if __name__ == "__main__":
    unittest.main()
