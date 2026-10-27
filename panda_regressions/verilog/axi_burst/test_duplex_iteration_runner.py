#!/usr/bin/env python3
"""Offline fail-closed smoke tests for G3.3 runner preflight."""
from __future__ import annotations

import os
from pathlib import Path
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[3]
RUNNER = ROOT / "panda_regressions/verilog/axi_burst/run_duplex_iteration_tests.sh"
TMP_ROOT = Path(os.environ.get("BURST_TEST_TMPDIR", Path.cwd())) / f"out_{Path(__file__).stem}"


class DuplexIterationRunnerTests(unittest.TestCase):
    def setUp(self):
        TMP_ROOT.mkdir(parents=True, exist_ok=True)
        self.scratch = Path(tempfile.mkdtemp(prefix="g33_runner_", dir=TMP_ROOT))
        self.addCleanup(__import__("shutil").rmtree, self.scratch, ignore_errors=True)
        self.env = os.environ.copy()
        self.env.update({"TMPDIR": str(self.scratch), "TMP": str(self.scratch),
                         "TEMP": str(self.scratch), "PYTHONDONTWRITEBYTECODE": "1"})

    def _run(self, *args, **overrides):
        env = self.env.copy()
        env.update(overrides)
        return subprocess.run(["bash", str(RUNNER), *map(str, args)], cwd=ROOT, env=env,
                              text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                              check=False)

    def test_simulation_requires_explicit_outside_sandbox_opt_in(self):
        result = self._run()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("refusing compiler/build/simulation", result.stdout)

    def test_invalid_parallelism_fails_before_creating_output(self):
        output = TMP_ROOT / "out_g33_invalid_jobs_test"
        self.assertFalse(output.exists())
        result = self._run("/bin/true", output, DUPLEX_G3_SIM_OUTSIDE_SANDBOX="1", SIM_JOBS="0")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("SIM_JOBS must be a positive integer", result.stdout)
        self.assertFalse(output.exists())

    def test_preexisting_output_override_is_rejected(self):
        prefix = self.scratch / "install"
        bambu = prefix / "bin/bambu"
        bambu.parent.mkdir(parents=True)
        bambu.write_text("#!/bin/sh\nexit 0\n")
        bambu.chmod(0o755)
        settings = prefix / "settings.sh"
        settings.write_text(f"export BAMBU_HLS='{prefix}'\n")
        output = self.scratch / "already-exists"
        output.mkdir()
        result = self._run(str(bambu), str(output), DUPLEX_G3_SIM_OUTSIDE_SANDBOX="1")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("output directory must be fresh", result.stdout)


if __name__ == "__main__":
    unittest.main(verbosity=2)
