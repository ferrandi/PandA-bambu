from __future__ import annotations

import os
import hashlib
import shutil
import subprocess
import unittest
import uuid
from pathlib import Path


ROOT = Path(__file__).resolve().parents[3]
RUNNER = ROOT / "panda_regressions/hls/bambu_axi_burst/run_write_development_tests.sh"
SCRATCH_ROOT = Path(os.environ.get("BURST_TEST_TMPDIR", Path.cwd())) / f"out_{Path(__file__).stem}"


class DevelopmentRunnerTests(unittest.TestCase):
    def setUp(self):
        SCRATCH_ROOT.mkdir(parents=True, exist_ok=True)
        self.scratch = SCRATCH_ROOT / f"runner-test-{uuid.uuid4().hex}"
        self.scratch.mkdir()
        self.addCleanup(shutil.rmtree, self.scratch, ignore_errors=True)
        self.export = self.scratch / "export"
        self.script_dir = self.export / "panda_regressions/hls/bambu_axi_burst"
        self.script_dir.mkdir(parents=True)
        shutil.copy2(RUNNER, self.script_dir / RUNNER.name)
        self.script_dir.joinpath(RUNNER.name).chmod(0o755)
        self.rtl_tests = self.export / "panda_regressions/verilog/axi_burst/rtl_tests"
        self.rtl_tests.mkdir(parents=True)
        for name in (
            "run_write_compiler_tests.sh",
            "run_frontend_compatibility_tests.sh",
            "run_source_restrict_provenance_tests.sh",
            "run_duplex_fast_store_ack_tests.sh",
            "run_mdpi_extent_tests.sh",
            "parallel_helpers.sh",
            "test_write_compiler_artifacts.py",
            "check_copy_baseline_results.py",
            "copy_baseline.cpp",
            "copy_baseline_tb.cpp",
            "copy_cycle_expectations.tsv",
            "check_copy_handoff.py",
            "write_compiler_cases/write_cases.cpp",
            "run_copy_handoff_tests.sh",
        ):
            path = self.script_dir / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text("# exported stub target\n")
        (self.rtl_tests / "run_generated_write_kernel_tests.sh").write_text(
            "# exported stub target\n"
        )
        (self.rtl_tests / "test_extract_write_engine.py").write_text(
            "# exported stub target\n"
        )
        self.bin_dir = self.scratch / "bin"
        self.bin_dir.mkdir()
        self.calls = self.scratch / "calls.log"
        self._stub("python3", "printf 'python3|%s|%s|%s|%s|%s|%s\\n' \"$TMPDIR\" \"$TMP\" \"$TEMP\" \"${BURST_G2_SIM_OUTSIDE_SANDBOX:-}\" \"${BURST_TEST_TMPDIR:-}\" \"$*\" >>\"$CALL_LOG\"\n")
        self._stub("bash", "printf 'bash|%s|%s|%s|%s|%s|%s|%s\\n' \"$TMPDIR\" \"$TMP\" \"$TEMP\" \"${BURST_G2_SIM_OUTSIDE_SANDBOX:-}\" \"${MDPI_EXTENT_TESTS_OUTSIDE_SANDBOX:-}\" \"${BURST_TEST_TMPDIR:-}\" \"$*\" >>\"$CALL_LOG\"\nif [[ $* == *run_source_restrict_provenance_tests.sh* ]]; then\n  [[ ${SOURCE_RESTRICT_FAIL:-0} != 1 ]] || exit 23\n  output=${!#}\n  mkdir -p \"$output\"\n  printf 'SOURCE-RESTRICT-PROVENANCE PASS: 4 compiler cases\\nSOURCE_RESTRICT_PROVENANCE OUT=%s\\n' \"$output\" >\"$output/aggregate.log\"\nfi\n")
        self.bambu = self.scratch / "installed/bin/bambu"
        self.bambu.parent.mkdir(parents=True)
        self.bambu.write_text("#!/bin/sh\nexit 0\n")
        self.bambu.chmod(0o755)
        prefix = self.bambu.parent.parent
        (prefix / "settings.sh").write_text("export BAMBU_HLS=\"$(cd \"$(dirname \"${BASH_SOURCE[0]}\")\" && pwd)\"\n")
        plugin = prefix / "lib/panda/clang-16/ASTAnalyzer.so"
        plugin.parent.mkdir(parents=True)
        plugin.write_text("plugin stub\n")
        self.env = os.environ.copy()
        self.env["PATH"] = f"{self.bin_dir}:{self.env['PATH']}"
        self.env["CALL_LOG"] = str(self.calls)
        temp_base = SCRATCH_ROOT.resolve()
        self.env.update(TMPDIR=str(temp_base), TMP=str(temp_base), TEMP=str(temp_base))
        self.env["BURST_TEST_TMPDIR"] = str(self.scratch / "stale-inherited-test-output")

    def _stub(self, name, body):
        path = self.bin_dir / name
        path.write_text("#!/bin/bash\nset -euo pipefail\n" + body)
        path.chmod(0o755)

    def _run(self, mode, output, *extra_args, outside_sandbox=False):
        env = self.env.copy()
        if outside_sandbox:
            env["BURST_G2_SIM_OUTSIDE_SANDBOX"] = "1"
            env["MDPI_EXTENT_TESTS_OUTSIDE_SANDBOX"] = "1"
        return subprocess.run(
            ["/bin/bash", str(self.script_dir / RUNNER.name), mode, str(self.bambu), *map(str, extra_args), str(output)],
            cwd=self.export,
            env=env,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            check=False,
        )

    def _calls(self):
        if not self.calls.exists():
            return []
        return self.calls.read_text().splitlines()

    def _baseline(self, path):
        path.mkdir()
        (path / "provenance.txt").write_text(f"baseline={path.name}\n")
        for name in ("copy-burst-two-bundles", "copy-legacy-two-bundles", "copy-legacy-shared-bundle"):
            case_dir = path / name
            case_dir.mkdir()
            (case_dir / "artifact-sha256.txt").write_text(f"manifest={name}\n")
        return path

    def test_offline_dispatches_only_the_two_manual_offline_checks(self):
        output = self.scratch / "offline-output"
        result = self._run("offline", output)
        self.assertEqual(result.returncode, 0, result.stderr)
        calls = self._calls()
        self.assertEqual(len(calls), 2)
        self.assertIn("test_write_compiler_artifacts.py", calls[0])
        self.assertIn("test_extract_write_engine.py", calls[1])
        for call in calls:
            self.assertTrue(call.startswith("python3|"))
            fields = call.split("|", 6)
            self.assertEqual(fields[1:4], [str(output / "offline/tmp")] * 3)
            self.assertEqual(fields[5], str(output / "offline"))
            self.assertNotEqual(fields[5], self.env["BURST_TEST_TMPDIR"])
        self.assertTrue((output / "offline").is_dir())

    def test_output_must_be_fresh_and_g2_is_opt_in(self):
        existing = self.scratch / "existing"
        existing.mkdir()
        marker = existing / "keep"
        marker.write_text("safe")
        refused = self._run("offline", existing)
        self.assertNotEqual(refused.returncode, 0)
        self.assertEqual(marker.read_text(), "safe")
        self.assertEqual(self._calls(), [])

        output = self.scratch / "g2-without-opt-in"
        refused = self._run("g2", output)
        self.assertNotEqual(refused.returncode, 0)
        self.assertFalse(output.exists())
        self.assertIn("BURST_G2_SIM_OUTSIDE_SANDBOX=1", refused.stderr)
        self.assertEqual(self._calls(), [])

    def test_mdpi_extent_dispatches_the_manual_simulation_runner(self):
        output = self.scratch / "mdpi-extent-output"
        refused = self._run("mdpi-extent", output)
        self.assertNotEqual(refused.returncode, 0)
        self.assertIn("MDPI_EXTENT_TESTS_OUTSIDE_SANDBOX=1", refused.stderr)
        self.assertFalse(output.exists())
        self.assertEqual(self._calls(), [])

        result = self._run("mdpi-extent", output, outside_sandbox=True)
        self.assertEqual(result.returncode, 0, result.stderr)
        calls = self._calls()
        self.assertEqual(len(calls), 1)
        self.assertTrue(calls[0].startswith("bash|"))
        self.assertIn("run_mdpi_extent_tests.sh", calls[0])
        fields = calls[0].split("|", 6)
        self.assertTrue(fields[6].endswith(str(output / "mdpi-extent/results")))
        self.assertEqual(fields[1:4], [str(output / "mdpi-extent/tmp")] * 3)
        self.assertEqual(fields[5], "1")

    def test_all_dispatches_compiler_then_all_three_g2_tops(self):
        output = self.scratch / "all-output"
        result = self._run("all", output, outside_sandbox=True)
        self.assertEqual(result.returncode, 0, result.stderr)
        calls = self._calls()
        self.assertEqual(len(calls), 9)
        self.assertTrue(calls[0].startswith("python3|"))
        self.assertIn("test_write_compiler_artifacts.py", calls[0])
        self.assertIn("test_extract_write_engine.py", calls[1])
        self.assertIn("run_write_compiler_tests.sh all", calls[2])
        self.assertIn("run_frontend_compatibility_tests.sh", calls[3])
        self.assertIn("run_source_restrict_provenance_tests.sh", calls[4])
        self.assertTrue(calls[4].endswith(str(output / "compiler/source-restrict-provenance")))
        self.assertIn("run_duplex_fast_store_ack_tests.sh", calls[5])
        self.assertTrue(calls[5].endswith(str(output / "compiler/duplex-fast-store-ack")))
        aggregate = output / "compiler/source-restrict-provenance/aggregate.log"
        self.assertIn("SOURCE-RESTRICT-PROVENANCE PASS", aggregate.read_text())
        for call, top in zip(calls[6:], (
            "write_count_seed",
            "write_with_unused_same_bundle",
            "write_with_unused_same_bundle_reversed",
        )):
            self.assertIn("run_generated_write_kernel_tests.sh", call)
            self.assertTrue(call.endswith(f"{top} {top}"))
            self.assertEqual(call.split("|", 5)[4], "1")
        for call in calls:
            self.assertTrue(call.split("|", 5)[1].startswith(str(output)))
            self.assertNotIn("run_copy_handoff_tests.sh", call)

    def test_all_propagates_source_restrict_runner_failure(self):
        output = self.scratch / "all-source-restrict-failure"
        env = self.env.copy()
        env["BURST_G2_SIM_OUTSIDE_SANDBOX"] = "1"
        env["SOURCE_RESTRICT_FAIL"] = "1"
        result = subprocess.run(
            ["/bin/bash", str(self.script_dir / RUNNER.name), "all", str(self.bambu), str(output)],
            cwd=self.export,
            env=env,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            check=False,
        )
        self.assertNotEqual(result.returncode, 0)
        calls = self._calls()
        self.assertTrue(any("run_source_restrict_provenance_tests.sh" in call for call in calls))
        self.assertFalse(any("run_generated_write_kernel_tests.sh" in call for call in calls))

    def test_handoff_validates_two_baselines_then_dispatches_checker_runner(self):
        baseline1 = self.scratch / "baseline-one"
        baseline2 = self.scratch / "baseline-two"
        self._baseline(baseline1)
        self._baseline(baseline2)
        output = self.scratch / "handoff-output"
        result = self._run("handoff", output, baseline1, baseline2)
        self.assertEqual(result.returncode, 0, result.stderr)
        calls = self._calls()
        self.assertEqual(len(calls), 3)
        self.assertTrue(calls[0].startswith("python3|"))
        self.assertIn("check_copy_baseline_results.py", calls[0])
        self.assertIn(str(baseline1), calls[0])
        self.assertTrue(calls[1].startswith("python3|"))
        self.assertIn("check_copy_baseline_results.py", calls[1])
        self.assertIn(str(baseline2), calls[1])
        self.assertTrue(calls[2].startswith("bash|"))
        self.assertIn("run_copy_handoff_tests.sh", calls[2])
        self.assertTrue(calls[2].endswith(str(output / "handoff")))
        provenance = (output / "handoff-provenance.txt").read_text()
        self.assertIn("mode=handoff", provenance)
        self.assertIn("check_copy_handoff.py", provenance)
        for baseline in (baseline1, baseline2):
            self.assertIn(f"baseline_run{1 if baseline == baseline1 else 2}={baseline.resolve()}", provenance)
            for name in ("provenance.txt", "copy-burst-two-bundles/artifact-sha256.txt",
                         "copy-legacy-two-bundles/artifact-sha256.txt",
                         "copy-legacy-shared-bundle/artifact-sha256.txt"):
                file_path = baseline / name
                digest = hashlib.sha256(file_path.read_bytes()).hexdigest()
                self.assertIn(f"{digest}  {file_path.resolve()}", provenance)
        self.assertFalse((output / "all").exists())

    def test_handoff_rejects_same_canonical_baseline_before_creating_output_or_dispatching(self):
        baseline = self._baseline(self.scratch / "baseline-one")
        output = self.scratch / "duplicate-baseline-output"
        aliased_baseline = baseline.parent / baseline.name / ".." / baseline.name
        refused = self._run("handoff", output, baseline, aliased_baseline)
        self.assertNotEqual(refused.returncode, 0)
        self.assertIn("two distinct baseline run directories", refused.stderr)
        self.assertFalse(output.exists())
        self.assertEqual(self._calls(), [])

    def test_handoff_fails_closed_for_missing_baseline_or_existing_output(self):
        baseline = self.scratch / "baseline"
        self._baseline(baseline)
        output = self.scratch / "missing-baseline-output"
        refused = self._run("handoff", output, baseline, self.scratch / "absent")
        self.assertNotEqual(refused.returncode, 0)
        self.assertFalse(output.exists())
        self.assertEqual(self._calls(), [])

        existing = self.scratch / "existing-handoff-output"
        existing.mkdir()
        marker = existing / "keep"
        marker.write_text("safe")
        refused = self._run("handoff", existing, baseline, baseline)
        self.assertNotEqual(refused.returncode, 0)
        self.assertEqual(marker.read_text(), "safe")
        self.assertEqual(self._calls(), [])


if __name__ == "__main__":
    unittest.main()
