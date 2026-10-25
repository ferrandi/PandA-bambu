#!/usr/bin/env python3
"""Behavioral checks for restarting the AXI burst regression in place."""

from __future__ import annotations

import os
import shlex
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[3]
WRAPPER = ROOT / "panda_regressions/hls/bambu_axi_burst.sh"
PARALLEL_HELPERS = ROOT / "panda_regressions/hls/bambu_axi_burst/parallel_helpers.sh"
MODELSIM_LOOKUP = ROOT / "etc/libtech/backend/simulation/modelsim/tool_lookup.sh"
TMP_ROOT = Path(os.environ.get("BURST_TEST_TMPDIR", Path.cwd())) / (
    f"out_{Path(__file__).stem}"
)


class RegressionRestartTests(unittest.TestCase):
    def setUp(self) -> None:
        TMP_ROOT.mkdir(parents=True, exist_ok=True)
        self.temp = tempfile.TemporaryDirectory(
            prefix="out_regression_restart_", dir=TMP_ROOT
        )
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.repo = self.root / "repo"
        family = self.repo / "panda_regressions/hls/bambu_axi_burst"
        family.mkdir(parents=True)
        shutil.copy2(WRAPPER, self.repo / "panda_regressions/hls/bambu_axi_burst.sh")
        shutil.copy2(PARALLEL_HELPERS, family / "parallel_helpers.sh")
        modelsim = self.repo / "etc/libtech/backend/simulation/modelsim"
        modelsim.mkdir(parents=True)
        shutil.copy2(MODELSIM_LOOKUP, modelsim / "tool_lookup.sh")

        install = self.root / "install"
        bindir = install / "bin"
        bindir.mkdir(parents=True)
        self._executable(bindir / "bambu", "#!/usr/bin/env bash\nexit 0\n")
        (install / "settings.sh").write_text(
            f"export BAMBU_HLS={shlex.quote(str(install))}\n", encoding="utf-8"
        )
        modelsim_bin = self.root / "modelsim/bin"
        modelsim_bin.mkdir(parents=True)
        for tool in ("vsim", "vlib", "vmap", "vlog"):
            self._executable(modelsim_bin / tool, "#!/usr/bin/env bash\nexit 0\n")

        self.fake_bin = self.root / "fake-bin"
        self.fake_bin.mkdir()
        self._executable(self.fake_bin / "python3", self._python_stub())
        self._executable(self.fake_bin / "bash", self._bash_stub())
        self.out_root = self.root / "out_bambu_axi_burst"

    @staticmethod
    def _executable(path: Path, contents: str) -> None:
        path.write_text(contents, encoding="utf-8")
        path.chmod(0o755)

    @staticmethod
    def _python_stub() -> str:
        return f"""#!/bin/bash
if [[ ${{1:-}} == -c ]]; then
  exec {sys.executable!r} "$@"
fi
case ${{1:-}} in
  */mantis.py)
    shift
    while (($#)); do
      if [[ $1 == -o ]]; then
        mkdir -p "$2/AXI-BURST"
        exit 0
      fi
      shift
    done
    echo 'fake mantis invocation omitted -o' >&2
    exit 90
    ;;
  *) exit 0 ;;
esac
"""

    @staticmethod
    def _bash_stub() -> str:
        return r"""#!/bin/bash
script=${1:-}
case $script in
  */run_compiler_tests.sh)
    mode=${2:-unknown}
    destination=${4:-}
    if [[ -z $destination ]]; then
      echo 'missing compiler-suite destination' >&2
      exit 91
    fi
    if [[ -e $destination ]]; then
      echo "compiler-suite destination already exists: $destination" >&2
      exit 92
    fi
    mkdir "$destination"
    printf 'mode=%s\n' "$mode" >"$destination/restart-sentinel.txt"
    if [[ ${FAKE_HELPER_FAIL:-0} == 1 ]]; then
      echo 'simulated compiler helper failure' >&2
      exit 9
    fi
    echo "simulated compiler helper $mode passed"
    ;;
  */run_read_burst_modelsim.sh|*/run_write_burst_engine.sh|*/run_vlog_portability.sh|*/run_generated_kernel_tests.sh|*/run_generated_wrapper_rid_tests.sh|*/test_compiler_runner_export_root.sh)
    exit 0
    ;;
  *)
    echo "unexpected bash child: $script" >&2
    exit 92
    ;;
esac
"""

    def run_regression(self, *args: str, helper_failure: bool = False) -> subprocess.CompletedProcess[str]:
        env = os.environ.copy()
        env.update(
            {
                "PATH": f"{self.fake_bin}:{self.root / 'modelsim/bin'}:{self.root / 'install/bin'}:{os.environ['PATH']}",
                "TMPDIR": str(self.root),
                "TMP": str(self.root),
                "TEMP": str(self.root),
                "FAKE_HELPER_FAIL": "1" if helper_failure else "0",
                "BAMBU_HLS_BACKEND_PATH": "",
            }
        )
        return subprocess.run(
            ["/bin/bash", str(self.repo / "panda_regressions/hls/bambu_axi_burst.sh"), *args],
            cwd=self.root,
            env=env,
            text=True,
            capture_output=True,
            check=False,
            timeout=90,
        )

    @staticmethod
    def is_within(path: Path, parent: Path) -> bool:
        try:
            path.resolve().relative_to(parent.resolve())
        except ValueError:
            return False
        return True

    def test_restart_uses_fresh_helper_tree_and_preserves_retained_evidence(self) -> None:
        first = self.run_regression("--no-clean", "-j1", "-o", str(self.out_root))
        self.assertEqual(first.returncode, 0, first.stderr)
        original = self.out_root / "compiler-pragma/restart-sentinel.txt"
        self.assertTrue(original.is_file())
        original_contents = original.read_text(encoding="utf-8")
        original_log = self.out_root / "jobs/compiler-pragma.log"
        self.assertTrue(original_log.is_file())
        original_log_contents = original_log.read_text(encoding="utf-8")

        restarted = self.run_regression(
            "--restart", "--no-clean", "-j1", "-o", str(self.out_root)
        )
        self.assertEqual(restarted.returncode, 0, restarted.stderr)
        helper_roots = list(self.out_root.glob("out_helpers_restart.*"))
        self.assertEqual(len(helper_roots), 1, helper_roots)
        restarted_helper = helper_roots[0] / "compiler-pragma/restart-sentinel.txt"
        self.assertTrue(restarted_helper.is_file())
        self.assertEqual(original.read_text(encoding="utf-8"), original_contents)
        self.assertEqual(original_log.read_text(encoding="utf-8"), original_log_contents)
        self.assertTrue(self.is_within(restarted_helper, self.out_root))
        self.assertTrue((self.out_root / "mantis/AXI-BURST").is_dir())
        self.assertIn("AXI_BURST_RETAIN_ARTIFACTS", restarted.stdout)

    def test_helper_failure_is_propagated_and_artifacts_are_retained(self) -> None:
        failed_root = self.root / "out_failed_burst"
        result = self.run_regression(
            "--no-clean", "-j1", "-o", str(failed_root), helper_failure=True
        )
        self.assertEqual(result.returncode, 1, result.stdout + result.stderr)
        self.assertIn("AXI burst helper suite failed", result.stderr)
        self.assertIn("simulated compiler helper failure", result.stderr)
        self.assertTrue((failed_root / "jobs/compiler-pragma.log").is_file())
        self.assertTrue((failed_root / "compiler-pragma/restart-sentinel.txt").is_file())
        self.assertTrue(
            self.is_within(
                failed_root / "compiler-pragma/restart-sentinel.txt", failed_root
            )
        )


if __name__ == "__main__":
    unittest.main()
