#!/usr/bin/env python3
"""Behavioral offline tests for the compiler-only duplex handoff runner."""
from __future__ import annotations

import hashlib
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[3]
HERE = Path(__file__).resolve().parent
TMP_ROOT = ROOT / "documentation/tmp"
TMP_ROOT.mkdir(parents=True, exist_ok=True)
os.environ.update(TMPDIR=str(TMP_ROOT), TMP=str(TMP_ROOT), TEMP=str(TMP_ROOT))
RUNNER = HERE / "run_duplex_handoff_tests.sh"
CASES = ("copy-symmetric", "copy-asymmetric", "independent-symmetric", "independent-asymmetric")


class DuplexHandoffRunnerTests(unittest.TestCase):
    def setUp(self) -> None:
        self.temp = tempfile.TemporaryDirectory(prefix="duplex-handoff-runner-", dir=TMP_ROOT)
        self.root = Path(self.temp.name)
        self.prefix = self.root / "install"
        (self.prefix / "bin").mkdir(parents=True)
        plugin = self.prefix / "lib/panda/clang-16/ASTAnalyzer.so"
        plugin.parent.mkdir(parents=True)
        plugin.write_text("fake plugin\n")
        (self.prefix / "settings.sh").write_text(f"export BAMBU_HLS='{self.prefix}'\n")
        self.bambu = self.prefix / "bin/bambu"
        self.bambu.write_text(self.compiler_stub())
        self.bambu.chmod(0o755)
        self.bin = self.root / "bin"
        self.bin.mkdir()
        self.python_stub = self.bin / "python3"
        self.python_stub.write_text(self.python_stub_source())
        self.python_stub.chmod(0o755)
        self.env = os.environ.copy()
        self.env.update(PATH=f"{self.bin}:{os.environ['PATH']}", TMPDIR=str(TMP_ROOT),
                        TMP=str(TMP_ROOT), TEMP=str(TMP_ROOT), PYTHONDONTWRITEBYTECODE="1")

    def tearDown(self) -> None:
        self.temp.cleanup()

    @staticmethod
    def compiler_stub() -> str:
        return r'''#!/usr/bin/env bash
set -eu
if [[ ${1:-} == --version ]]; then echo "stub bambu 1"; exit 0; fi
if [[ ${FAKE_BAMBU_FAIL:-} == 1 ]]; then echo "forced compiler failure" >&2; exit 9; fi
top= profile= source=$1
for arg in "$@"; do
  case $arg in --top-fname=*) top=${arg#*=};; -DDUPLEX_PROFILE=*) profile=${arg#*=};; -DDUPLEX_INDEPENDENT_PROFILE=*) profile=${arg#*=};; esac
done
mkdir -p panda-temp "HLS_output/dot/$top"
printf 'raw\n' > 'panda-temp/before_Frontend::InterfaceInfer.raw'
printf 'raw\n' > 'panda-temp/after_Frontend::InterfaceInfer.raw'
printf 'raw\n' > "panda-temp/after_Frontend::BuildVirtualPhi::${top}(1)[1].raw"
printf '<scheduling/>\n' > "${top}_scheduling.xml"
printf 'digraph {}\n' > "HLS_output/dot/$top/HLS_scheduling.dot"
printf 'digraph {}\n' > "HLS_output/dot/$top/OP_DFG.dot"
printf 'digraph {}\n' > "HLS_output/dot/$top/OP_ADG.dot"
printf 'module %s; endmodule\n' "$top" > "$top.v"
printf '<architecture/>\n' > panda-temp/architecture.xml
printf 'fake compiler stderr\n' >&2
'''

    @staticmethod
    def python_stub_source() -> str:
        return r'''#!/usr/bin/env bash
set -eu
for arg in "$@"; do
  if [[ $arg == */check_duplex_handoff.py ]]; then
    echo "HANDOFF_CHECKER_STUB_PASS"
    exit 0
  fi
done
exec /usr/bin/python3 "$@"
'''

    def invoke(self, *args: str, **extra_env: str) -> subprocess.CompletedProcess[str]:
        env = self.env.copy()
        env.update(extra_env)
        return subprocess.run(["bash", str(RUNNER), str(self.bambu), *args], cwd=ROOT,
                              env=env, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)

    def test_fresh_output_runs_four_profiles_and_records_digests(self) -> None:
        out = self.root / "fresh"
        result = self.invoke(str(out))
        self.assertEqual(result.returncode, 0, result.stdout)
        aggregate = (out / "aggregate.log").read_text().splitlines()
        self.assertEqual(aggregate[:4], [f"DUPLEX_HANDOFF_PASS CASE={case}" for case in CASES])
        self.assertEqual(aggregate[4].split()[0], "DUPLEX_HANDOFF_PASS")
        self.assertIn("profiles=copy-symmetric copy-asymmetric independent-symmetric independent-asymmetric", aggregate[4])
        for case in CASES:
            case_dir = out / case
            self.assertEqual((case_dir / "aggregate.log").read_text().strip(), f"DUPLEX_HANDOFF_PASS CASE={case}")
            manifest = (case_dir / "artifact-sha256.txt").read_text().splitlines()
            self.assertTrue(manifest)
            for row in manifest:
                expected, relative = row.split("  ", 1)
                actual = hashlib.sha256((case_dir / relative).read_bytes()).hexdigest()
                self.assertEqual(actual, expected, relative)
            provenance = (case_dir / "provenance.txt").read_text()
            self.assertIn(f"git_head=", provenance)
            self.assertIn("source_sha256=", provenance)
            self.assertIn("runner_sha256=", provenance)
        run_provenance = (out / "provenance.txt").read_text()
        self.assertIn("plugin_sha256=", run_provenance)
        self.assertIn("check_duplex_effect_analysis_artifacts.py", run_provenance)

    def test_default_output_is_under_documentation_tmp(self) -> None:
        result = self.invoke()
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertTrue((TMP_ROOT / "out_duplex_handoff" / "aggregate.log").is_file())
        shutil.rmtree(TMP_ROOT / "out_duplex_handoff")

    def test_preexisting_output_and_symlink_are_refused(self) -> None:
        existing = self.root / "existing"
        existing.mkdir()
        result = self.invoke(str(existing))
        self.assertNotEqual(result.returncode, 0)
        link = self.root / "link"
        link.symlink_to(existing, target_is_directory=True)
        result = self.invoke(str(link))
        self.assertNotEqual(result.returncode, 0)

    def test_invalid_jobs_fail_before_output_creation(self) -> None:
        out = self.root / "must-not-exist"
        result = self.invoke(str(out), DUPLEX_HANDOFF_JOBS="0")
        self.assertNotEqual(result.returncode, 0)
        self.assertFalse(out.exists())

    def test_nonzero_compiler_child_is_propagated(self) -> None:
        out = self.root / "compiler-fails"
        result = self.invoke(str(out), FAKE_BAMBU_FAIL="1")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("compiler invocation failed", result.stdout)
        self.assertFalse((out / "aggregate.log").read_text().startswith("DUPLEX_HANDOFF_PASS"))

    def test_missing_or_wrong_child_marker_fails_closed(self) -> None:
        rg = self.bin / "rg"
        rg.write_text(r'''#!/usr/bin/env bash
if [[ ${FAKE_RG_REJECT:-} == 1 && "$*" == *DUPLEX_HANDOFF_PASS* ]]; then exit 1; fi
exec /usr/bin/rg "$@"
''')
        rg.chmod(0o755)
        out = self.root / "marker-fails"
        result = self.invoke(str(out), FAKE_RG_REJECT="1")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("missing exact PASS marker", result.stdout)

    def test_four_profiles_and_one_exact_final_marker_are_mandatory(self) -> None:
        out = self.root / "markers"
        result = self.invoke(str(out))
        self.assertEqual(result.returncode, 0, result.stdout)
        lines = (out / "aggregate.log").read_text().splitlines()
        self.assertEqual(sum(line.startswith("DUPLEX_HANDOFF_PASS CASE=") for line in lines), 4)
        self.assertEqual(sum(line.startswith("DUPLEX_HANDOFF_PASS profiles=") for line in lines), 1)
        self.assertNotIn("DUPLEX_HANDOFF_PASS CASE=typo", lines)


if __name__ == "__main__":
    unittest.main()
