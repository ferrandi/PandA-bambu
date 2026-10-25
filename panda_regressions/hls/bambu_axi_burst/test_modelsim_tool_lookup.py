#!/usr/bin/env python3
"""Behavioral tests for the shared Bambu ModelSim tool lookup."""

from __future__ import annotations

import os
import shutil
import subprocess
import unittest
import uuid
from pathlib import Path
import shlex


ROOT = Path(__file__).resolve().parents[3]
LOOKUP = ROOT / "etc/libtech/backend/simulation/modelsim/tool_lookup.sh"
LAUNCHER = ROOT / "etc/libtech/backend/simulation/modelsim/launch.sh"
SCRATCH_ROOT = Path(os.environ.get("BURST_TEST_TMPDIR", Path.cwd())) / (
    "out_modelsim_tool_lookup"
)


class ModelSimToolLookupTests(unittest.TestCase):
    def setUp(self) -> None:
        SCRATCH_ROOT.mkdir(parents=True, exist_ok=True)
        self.scratch = SCRATCH_ROOT / uuid.uuid4().hex
        self.scratch.mkdir()
        self.addCleanup(shutil.rmtree, self.scratch, ignore_errors=True)
        self.tools = self.scratch / "host-tools"
        self.tools.mkdir()
        self.env = os.environ.copy()
        self.env.update(
            PATH=f"{self.tools}:/usr/bin:/bin",
            TMPDIR=str(self.scratch),
            TMP=str(self.scratch),
            TEMP=str(self.scratch),
        )
        self.env.pop("BAMBU_HLS_BACKEND_PATH", None)

    @staticmethod
    def write_executable(path: Path, body: str = "exit 0\n") -> None:
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(f"#!/bin/sh\n{body}", encoding="utf-8")
        path.chmod(0o755)

    def run_lookup(self, env: dict[str, str] | None = None, cwd: Path | None = None):
        command = f"""\
source {shlex.quote(str(LOOKUP))}
if bambu_modelsim_resolve; then
  printf 'SOURCE=%s\\nPATH=%s\\nBIN=%s\\nROOT=%s\\n' \\
    "$BAMBU_MODELSIM_SOURCE" "$BAMBU_MODELSIM_PATH" \\
    "$BAMBU_MODELSIM_BIN" "$BAMBU_MODELSIM_ROOT"
else
  exit 73
fi
"""
        return subprocess.run(
            ["/bin/bash", "-c", command],
            cwd=cwd or self.scratch,
            env=env or self.env,
            text=True,
            capture_output=True,
            check=False,
        )

    def test_path_vsim_precedes_backend_roots(self) -> None:
        path_bin = self.scratch / "path-bin"
        self.write_executable(path_bin / "vsim")
        backend_bin = self.scratch / "backend" / "questa" / "bin"
        self.write_executable(backend_bin / "vsim")
        env = self.env.copy()
        env["PATH"] = f"{path_bin}:/usr/bin:/bin"
        env["BAMBU_HLS_BACKEND_PATH"] = str(backend_bin.parent.parent)

        result = self.run_lookup(env)

        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("SOURCE=PATH\n", result.stdout)
        self.assertIn(f"PATH={path_bin / 'vsim'}\n", result.stdout)
        self.assertIn(f"BIN={path_bin}\n", result.stdout)

    def test_relative_path_and_backend_roots_resolve_to_usable_absolute_paths(self) -> None:
        launch = self.scratch / "relative launch"
        launch.mkdir()
        path_bin = launch / "relative tools" / "bin"
        self.write_executable(path_bin / "vsim", "printf '%s\\n' \"${0##*/}\"\n")
        env = self.env.copy()
        env["PATH"] = "relative tools/bin:/usr/bin:/bin"
        result = self.run_lookup(env, launch)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn(f"PATH={path_bin / 'vsim'}\n", result.stdout)
        self.assertIn(f"BIN={path_bin}\n", result.stdout)
        invoked = subprocess.run(
            [str(path_bin / "vsim")], cwd=self.scratch, text=True,
            capture_output=True, check=False,
        )
        self.assertEqual(invoked.returncode, 0, invoked.stderr)
        self.assertEqual(invoked.stdout.strip(), "vsim")

        backend_root = launch / "relative backend"
        backend_bin = backend_root / "questa" / "bin"
        self.write_executable(backend_bin / "vco", "printf '%s\\n' \"${0##*/}\"\n")
        (backend_bin / "vsim").symlink_to("vco")
        env["PATH"] = "/usr/bin:/bin"
        env["BAMBU_HLS_BACKEND_PATH"] = "relative backend"
        result = self.run_lookup(env, launch)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn(f"PATH={backend_bin / 'vsim'}\n", result.stdout)
        self.assertIn(f"BIN={backend_bin}\n", result.stdout)
        invoked = subprocess.run(
            [str(backend_bin / "vsim")], cwd=self.scratch, text=True,
            capture_output=True, check=False,
        )
        self.assertEqual(invoked.returncode, 0, invoked.stderr)
        self.assertEqual(invoked.stdout.strip(), "vsim")

    def test_backend_fallback_handles_spaces_symlinked_root_and_vco_dispatcher(self) -> None:
        physical_root = self.scratch / "backend root with spaces"
        bin_dir = physical_root / "questa" / "bin"
        self.write_executable(bin_dir / "vco", "printf '%s\\n' \"$(basename \"$0\")\"\n")
        (bin_dir / "vsim").symlink_to("vco")
        linked_root = self.scratch / "backend root link"
        linked_root.symlink_to(physical_root, target_is_directory=True)
        env = self.env.copy()
        env["BAMBU_HLS_BACKEND_PATH"] = str(linked_root)

        result = self.run_lookup(env)

        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("SOURCE=BAMBU_HLS_BACKEND_PATH\n", result.stdout)
        expected_path = linked_root / "questa/bin/vsim"
        self.assertIn(f"PATH={expected_path}\n", result.stdout)
        self.assertIn(f"BIN={linked_root / 'questa/bin'}\n", result.stdout)
        self.assertIn(f"ROOT={linked_root / 'questa'}\n", result.stdout)
        invoked = subprocess.run(
            [str(expected_path)], text=True, capture_output=True, check=False
        )
        self.assertEqual(invoked.returncode, 0, invoked.stderr)
        self.assertEqual(invoked.stdout.strip(), "vsim")

    def test_searches_roots_in_backend_path_order_without_version_sorting(self) -> None:
        first_root = self.scratch / "root one"
        second_root = self.scratch / "root two"
        later_root = self.scratch / "root three"
        second_bin = second_root / "release-a" / "bin"
        self.write_executable(second_bin / "vsim")
        self.write_executable(later_root / "release-z" / "bin" / "vsim")
        env = self.env.copy()
        env["BAMBU_HLS_BACKEND_PATH"] = ":".join(
            (str(first_root), str(second_root), str(later_root))
        )

        result = self.run_lookup(env)

        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn(f"BIN={second_bin}\n", result.stdout)

    def test_keeps_match_printed_before_find_reports_traversal_error(self) -> None:
        first_root = self.scratch / "unreadable earlier root"
        second_root = self.scratch / "valid later root"
        candidate = second_root / "release" / "bin" / "vsim"
        self.write_executable(candidate)
        # Model an earlier-root traversal error after find has emitted a
        # valid later-root match. Command substitution must preserve stdout
        # even though the producer exits nonzero.
        self.write_executable(
            self.tools / "find",
            "printf '%s\\n' \"$EXPECTED_VSIM\"\nexit 1\n",
        )
        env = self.env.copy()
        env["BAMBU_HLS_BACKEND_PATH"] = f"{first_root}:{second_root}"
        env["EXPECTED_VSIM"] = str(candidate)

        result = self.run_lookup(env)

        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("SOURCE=BAMBU_HLS_BACKEND_PATH\n", result.stdout)
        self.assertIn(f"PATH={candidate}\n", result.stdout)

    def test_first_non_executable_candidate_fails_instead_of_falling_through(self) -> None:
        first_bin = self.scratch / "first" / "bin"
        first_bin.mkdir(parents=True)
        (first_bin / "vsim").write_text("#!/bin/sh\nexit 0\n", encoding="utf-8")
        later_bin = self.scratch / "later" / "bin"
        self.write_executable(later_bin / "vsim")
        env = self.env.copy()
        env["BAMBU_HLS_BACKEND_PATH"] = f"{first_bin.parent}:{later_bin.parent}"

        result = self.run_lookup(env)

        self.assertEqual(result.returncode, 73)
        self.assertIn("ModelSim vsim candidate is not executable", result.stderr)
        self.assertIn(str(first_bin / "vsim"), result.stderr)

    def test_empty_backend_components_never_search_the_current_directory(self) -> None:
        cwd = self.scratch / "cwd"
        self.write_executable(cwd / "bin" / "vsim")
        env = self.env.copy()
        env["BAMBU_HLS_BACKEND_PATH"] = "::"

        result = self.run_lookup(env, cwd)

        self.assertEqual(result.returncode, 73)
        self.assertIn("has no existing roots", result.stderr)

    def test_missing_backend_roots_have_a_specific_failure(self) -> None:
        env = self.env.copy()
        env["BAMBU_HLS_BACKEND_PATH"] = str(self.scratch / "does-not-exist")

        result = self.run_lookup(env)

        self.assertEqual(result.returncode, 73)
        self.assertIn("has no existing roots", result.stderr)

    def test_launcher_prepends_backend_bin_and_invokes_vsim_name(self) -> None:
        install = self.scratch / "install/lib/panda/libtech/backend/simulation/modelsim"
        install.mkdir(parents=True)
        shutil.copy2(LAUNCHER, install / "launch.sh")
        shutil.copy2(LOOKUP, install / "tool_lookup.sh")
        (install / "setup.sh").write_text("# test setup\n", encoding="utf-8")
        wrapper_log = self.scratch / "wrapper.log"
        invocation_log = self.scratch / "invocation.log"
        (install / "simulation_wrapper.sh").write_text(
            "printf 'PATH=%s\\n' \"$PATH\" >\"$WRAPPER_LOG\"\n"
            "printf 'VSIM=%s\\n' \"$(command -v vsim)\" >>\"$WRAPPER_LOG\"\n"
            "vsim --from-wrapper\n",
            encoding="utf-8",
        )
        self.write_executable(
            self.tools / "bambu_results", "printf '1\\n'\n"
        )
        backend_bin = self.scratch / "Questa install" / "questa" / "bin"
        self.write_executable(
            backend_bin / "vco",
            "printf '%s|%s\\n' \"$(basename \"$0\")\" \"$*\" >>\"$INVOCATION_LOG\"\n",
        )
        (backend_bin / "vsim").symlink_to("vco")
        env = self.env.copy()
        env["BAMBU_HLS_BACKEND_PATH"] = str(backend_bin.parent.parent)
        env["WRAPPER_LOG"] = str(wrapper_log)
        env["INVOCATION_LOG"] = str(invocation_log)

        result = subprocess.run(
            ["/bin/bash", str(install / "launch.sh"), "-c"],
            cwd=self.scratch,
            env=env,
            text=True,
            capture_output=True,
            check=False,
        )

        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn(f"Siemens Modelsim path: {backend_bin}", result.stdout)
        wrapper_contents = wrapper_log.read_text(encoding="utf-8")
        self.assertTrue(wrapper_contents.startswith(f"PATH={backend_bin}:"))
        self.assertIn(f"VSIM={backend_bin / 'vsim'}\n", wrapper_contents)
        self.assertEqual(invocation_log.read_text(encoding="utf-8"), "vsim|--from-wrapper\n")


if __name__ == "__main__":
    unittest.main(verbosity=2)
