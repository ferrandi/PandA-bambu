#!/usr/bin/env python3
"""Behavioral tests for the AXI-core ModelSim/Questa compile gate."""

from __future__ import annotations

import os
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[4]
RTL_TESTS = Path(__file__).resolve().parent
RUNNER = RTL_TESTS / "run_vlog_portability.sh"
TMP_ROOT = Path(os.environ.get("BURST_TEST_TMPDIR", Path.cwd())) / f"out_{Path(__file__).stem}"
VARIANTS = [
    f"{target}_{reset}_{polarity}"
    for target in ("MinimalAXI4MasterPipelined", "MinimalAXI4MasterPipelinedWrite")
    for reset in ("sync", "async")
    for polarity in ("active-low", "active-high")
]


def _write_executable(path: Path, contents: str) -> None:
    path.write_text(contents, encoding="utf-8")
    path.chmod(0o755)


class VlogPortabilityRunnerTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        TMP_ROOT.mkdir(parents=True, exist_ok=True)

    def setUp(self) -> None:
        self.case_root = Path(tempfile.mkdtemp(prefix="case-", dir=TMP_ROOT))
        self.tools = self.case_root / "fake tools"
        self.tools.mkdir()
        self.trace = self.case_root / "tool calls.txt"
        self._install_fake_tools()
        self.env = os.environ.copy()
        self.env.update(
            PATH=f"{self.tools}{os.pathsep}{self.env['PATH']}",
            VLOG_TRACE=str(self.trace),
            TMPDIR=str(self.case_root),
            TMP=str(self.case_root),
            TEMP=str(self.case_root),
        )

    def tearDown(self) -> None:
        shutil.rmtree(self.case_root)

    def _install_fake_tools(self) -> None:
        _write_executable(
            self.tools / "vco",
            "#!/bin/bash\n"
            "tool=$(basename \"$0\")\n"
            "case $tool in vlib) expected='Fake vlib 1.0';; vlog) expected='Fake vlog 1.0';; *) echo \"unexpected dispatch name: $tool\" >&2; exit 12;; esac\n"
            "if [[ ${1:-} == -version ]]; then echo \"$expected\"; exit 0; fi\n"
            "printf '%s|%s|%s\\n' \"$tool\" \"$PWD\" \"$*\" >> \"$VLOG_TRACE\"\n"
            "if [[ $tool == vlib && ${FAIL_VLIB_DIR:-} == \"$(basename \"$(dirname \"$PWD\")\")\" ]]; then echo 'FAKE VLIB DIAGNOSTIC' >&2; exit 9; fi\n"
            "if [[ $tool == vlog && ${FAIL_VLOG_DIR:-} == \"$(basename \"$(dirname \"$PWD\")\")\" ]]; then echo 'FAKE VLOG DIAGNOSTIC' >&2; exit 7; fi\n"
            "if [[ $tool == vlog ]]; then [[ \"$*\" == *-sv* ]] || exit 8; for arg in \"$@\"; do [[ $arg == /* ]] || continue; [[ -s $arg ]] || exit 6; done; fi\n"
            "exit 0\n",
        )
        (self.tools / "vlib").symlink_to("vco")
        (self.tools / "vlog").symlink_to("vco")

    def _run(self, output: Path | None, *, env: dict[str, str] | None = None,
             path: str | None = None, cwd: Path | None = None,
             extra_args: tuple[str, ...] = ()) -> subprocess.CompletedProcess[str]:
        run_env = (env or self.env).copy()
        if path is not None:
            run_env["PATH"] = path
        return subprocess.run(
            [shutil.which("bash") or "/bin/bash", str(RUNNER), *([str(output)] if output is not None else []), *extra_args],
            cwd=cwd or ROOT,
            env=run_env,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            check=False,
        )

    def test_default_output_is_created_under_external_launch_directory(self) -> None:
        launch_root = self.case_root / "external launch cwd"
        launch_root.mkdir()
        env = self.env.copy()
        env.pop("BURST_TEST_TMPDIR", None)
        result = self._run(None, env=env, cwd=launch_root)
        self.assertEqual(result.returncode, 0, result.stdout)
        output = launch_root / "out_axi_core_vlog_portability"
        self.assertTrue((output / "aggregate.log").is_file())

    def test_explicit_output_under_parent_root_is_honored(self) -> None:
        parent_root = self.case_root / "inherited parent"
        parent_root.mkdir()
        env = self.env.copy()
        env["BURST_TEST_TMPDIR"] = str(parent_root)
        output = parent_root / "out_vlog_explicit"
        result = self._run(output, env=env, cwd=self.case_root)
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertTrue((output / "aggregate.log").is_file())

    def test_compiles_all_extracted_variants_in_isolated_default_mode_libraries(self) -> None:
        output = self.case_root / "output with spaces"
        result = self._run(output)
        self.assertEqual(result.returncode, 0, result.stdout)
        results = (output / "results.log").read_text(encoding="utf-8").splitlines()
        self.assertEqual(
            [line for line in results if line.startswith("AXI_CORE_VLOG PASS VARIANT=")],
            [f"AXI_CORE_VLOG PASS VARIANT={variant}" for variant in VARIANTS],
        )
        self.assertEqual(results[-1], f"AXI_CORE_VLOG PASS VARIANT={VARIANTS[-1]}")
        aggregate = (output / "aggregate.log").read_text(encoding="utf-8").splitlines()
        self.assertEqual(aggregate, ["AXI_CORE_VLOG_PORTABILITY_PASS: 8 variants"])

        calls = self.trace.read_text(encoding="utf-8").splitlines()
        vlib_calls = [line for line in calls if line.startswith("vlib|")]
        vlog_calls = [line for line in calls if line.startswith("vlog|")]
        self.assertEqual(len(vlib_calls), 8)
        self.assertEqual(len(vlog_calls), 8)
        self.assertEqual(len({line.split("|", 2)[1] for line in vlib_calls}), 8)
        self.assertEqual(len({line.split("|", 2)[1] for line in vlog_calls}), 8)
        self.assertTrue(all(line.endswith("|work") for line in vlib_calls))
        self.assertTrue(all("|-sv -work work " in line for line in vlog_calls))
        self.assertEqual(len(list(output.glob("*.v"))), 8)

        for variant in VARIANTS:
            hdl = (output / f"{variant}.v").read_text(encoding="utf-8")
            reset_mode = variant.split("_")[-2]
            polarity = variant.split("_")[-1]
            self.assertIn("module " + variant.rsplit("_", 2)[0], hdl)
            if reset_mode == "async":
                if variant.startswith("MinimalAXI4MasterPipelinedWrite_"):
                    sync_hdl = (output / f"{variant.replace('_async_', '_sync_')}.v").read_text()
                    self.assertNotEqual(hdl, sync_hdl)
            expected_reset = "1'b0" if polarity == "active-low" else "1'b1"
            self.assertIn(f"reset == {expected_reset}", hdl)

        provenance = (output / "provenance.txt").read_text(encoding="utf-8")
        for entry in (
            "git_head=", "python3_version=", "vlib_version=", "vlog_version=",
            "sha256_xml=", "sha256_extractor=", "sha256_runner=",
            "sha256_timeout=", "sha256_sha256sum=",
        ):
            self.assertIn(entry, provenance)
        escaped_tools = str(self.tools).replace(" ", "\\ ")
        vlib_invocation = f"vlib_invocation={escaped_tools}/vlib"
        vlog_invocation = f"vlog_invocation={escaped_tools}/vlog"
        vco_resolved = f"{escaped_tools}/vco"
        self.assertIn(vlib_invocation, provenance)
        self.assertIn(vlog_invocation, provenance)
        self.assertIn(f"vlib_resolved={vco_resolved}", provenance)
        self.assertIn(f"vlog_resolved={vco_resolved}", provenance)
        self.assertIn("sha256_vlib_invocation=", provenance)
        self.assertIn("sha256_vlib_resolved=", provenance)
        self.assertIn("sha256_vlog_invocation=", provenance)
        self.assertIn("sha256_vlog_resolved=", provenance)
        self.assertEqual(len((output / "extracted_hdl_sha256.txt").read_text().splitlines()), 8)
        commands = (output / "commands.txt").read_text().splitlines()
        self.assertEqual(len(commands), 24)
        self.assertEqual(sum(" -sv " in line for line in commands), 8)
        self.assertIn("Fake\\ vlog\\ 1.0", provenance)
        self.assertIn("vlog_flags=-sv", provenance)

    def test_vlib_failure_is_reported_without_success_marker(self) -> None:
        output = self.case_root / "vlib-fails"
        env = self.env | {"FAIL_VLIB_DIR": "MinimalAXI4MasterPipelined_sync_active-low"}
        result = self._run(output, env=env)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("stage=vlib", result.stdout)
        self.assertIn("FAKE VLIB DIAGNOSTIC", result.stdout)
        self.assertFalse((output / "results.log").exists())
        self.assertFalse((output / "aggregate.log").exists())

    def test_vlog_failure_is_reported_without_final_marker(self) -> None:
        output = self.case_root / "vlog-fails"
        env = self.env | {"FAIL_VLOG_DIR": "MinimalAXI4MasterPipelined_sync_active-low"}
        result = self._run(output, env=env)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("stage=vlog", result.stdout)
        self.assertIn("FAKE VLOG DIAGNOSTIC", result.stdout)
        results_path = output / "results.log"
        if results_path.exists():
            self.assertNotIn("AXI_CORE_VLOG_PORTABILITY_PASS:", results_path.read_text(encoding="utf-8"))

    def test_existing_and_symlink_outputs_are_rejected(self) -> None:
        existing = self.case_root / "existing"
        existing.mkdir()
        result = self._run(existing)
        self.assertEqual(result.returncode, 2, result.stdout)
        self.assertIn("must be fresh", result.stdout)
        target = self.case_root / "symlink-target"
        target.mkdir()
        link = self.case_root / "symlink-output"
        link.symlink_to(target, target_is_directory=True)
        result = self._run(link)
        self.assertEqual(result.returncode, 2, result.stdout)
        self.assertTrue(link.is_symlink())

    def test_extra_positional_arguments_fail_before_creating_output(self) -> None:
        output = self.case_root / "too-many-args"
        result = self._run(output, extra_args=("unexpected",))
        self.assertEqual(result.returncode, 2, result.stdout)
        self.assertIn("usage:", result.stdout)
        self.assertFalse(output.exists())

    def test_missing_each_required_tool_fails_before_output_creation(self) -> None:
        required = ("python3", "vlib", "vlog", "timeout", "sha256sum", "tail")
        for missing in required:
            with self.subTest(tool=missing):
                bin_dir = self.case_root / f"path-without-{missing}"
                bin_dir.mkdir(exist_ok=True)
                # Provide the runner's non-optional shell utilities while omitting
                # exactly the selected required executable.
                utilities = (
                    "python3", "vlib", "vlog", "timeout", "sha256sum", "realpath",
                    "dirname", "mkdir", "git", "awk", "head", "tr", "tee", "bash", "tail",
                )
                for utility in utilities:
                    if utility == missing:
                        continue
                    found = shutil.which(utility, path=self.env["PATH"])
                    if found:
                        link = bin_dir / utility
                        if not link.exists():
                            link.symlink_to(found)
                output = self.case_root / f"missing-{missing}"
                result = self._run(output, path=str(bin_dir))
                self.assertEqual(result.returncode, 2, result.stdout)
                self.assertIn(f"missing required tool: {missing}", result.stdout)
                self.assertFalse(output.exists())


if __name__ == "__main__":
    unittest.main(verbosity=2)
