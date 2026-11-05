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
        shutil.copy2(ROOT / "panda_regressions/hls/bambu_axi_burst/bambu_axi_burst_sum_profile.cpp", family)
        shutil.copy2(WRAPPER, self.repo / "panda_regressions/hls/bambu_axi_burst.sh")
        shutil.copy2(PARALLEL_HELPERS, family / "parallel_helpers.sh")
        regressions = self.repo / "panda_regressions/hls"
        shutil.copy2(ROOT / "panda_regressions/hls/bambu_axi_burst_list", regressions)
        fixed_device_dir = self.repo / "etc/libtech/xilinx"
        fixed_device_dir.mkdir(parents=True)
        shutil.copy2(ROOT / "etc/libtech/xilinx/xcu250-2Lfigd2104.xml", fixed_device_dir)
        modelsim = self.repo / "etc/libtech/backend/simulation/modelsim"
        modelsim.mkdir(parents=True)
        shutil.copy2(MODELSIM_LOOKUP, modelsim / "tool_lookup.sh")

        install = self.root / "install"
        self.install = install
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
        self.mantis_args_file = self.root / "mantis-args"
        self.mantis_argv_file = self.root / "mantis-argv"

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
    if [[ -n ${{FAKE_MANTIS_ARGS_FILE:-}} ]]; then
      printf '%s' "$*" >>"$FAKE_MANTIS_ARGS_FILE"
    fi
    if [[ -n ${{FAKE_MANTIS_ARGV_FILE:-}} ]]; then
      printf '%s\\0' "$@" >"$FAKE_MANTIS_ARGV_FILE"
    fi
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
    if [[ -n ${FAKE_BURST_SIM_FILE:-} ]]; then
      printf '%s' "${BURST_SIMULATE:-unset}" >>"$FAKE_BURST_SIM_FILE"
    fi
    printf 'mode=%s\n' "$mode" >"$destination/restart-sentinel.txt"
    if [[ ${FAKE_HELPER_FAIL:-0} == 1 ]]; then
      echo 'simulated compiler helper failure' >&2
      exit 9
    fi
    echo "simulated compiler helper $mode passed"
    ;;
  */run_write_compiler_tests.sh|*/run_frontend_compatibility_tests.sh)
    mode=${2:-unknown}
    destination=${4:-}
    mkdir -p "$destination"
    printf '%s\n' "${script##*/}:$mode" >>"${FAKE_JOB_LOG_FILE:?}"
    [[ ${FAKE_FAIL_JOB:-} != "${script##*/}" ]] || exit 9
    ;;
  */run_read_burst_modelsim.sh|*/run_write_burst_engine.sh|*/run_vlog_portability.sh|*/run_generated_kernel_tests.sh|*/run_generated_wrapper_rid_tests.sh|*/test_compiler_runner_export_root.sh|*/run_burst_fault_diagnostics.sh|*/run_axi_width_contract.sh|*/run_duplex_core_tests.sh|*/run_generated_duplex_tests.sh|*/run_generated_duplex_kernel_tests.sh|*/run_generated_write_kernel_tests.sh|*/run_duplex_alias_tests.sh|*/run_duplex_fallback_tests.sh|*/run_duplex_independent_tests.sh|*/run_adapter_size_guard.sh|*/run_mdpi_extent_tests.sh|*/run_testbench_memaxi_model.sh|*/run_generated_memaxi_response_tests.sh|*/run_vector_payload_tests.sh|*/run_copy_comparison_tests.sh|*/run_default_burst_tests.sh|*/run_default_duplex_reduction_modelsims.sh)
    name=${script##*/}
    printf '%s\n' "$name" >>"${FAKE_JOB_LOG_FILE:?}"
    if [[ $name == run_default_duplex_reduction_modelsims.sh ]]; then
      printf '%s|%s|%s|%s\n' "$name" "${2:-}" "${3:-}" \
        "${BURST_DUPLEX_DEFAULT_SIM_OUTSIDE_SANDBOX:-unset}" >>"${FAKE_JOB_ARGS_FILE:?}"
    fi
    if [[ $name == run_write_burst_engine.sh || $name == run_vector_payload_tests.sh || \
          $name == run_generated_memaxi_response_tests.sh || $name == run_copy_comparison_tests.sh || \
          $name == run_generated_duplex_tests.sh || $name == run_generated_duplex_kernel_tests.sh || \
          $name == run_duplex_alias_tests.sh || $name == run_duplex_fallback_tests.sh || \
          $name == run_duplex_independent_tests.sh || $name == run_mdpi_extent_tests.sh ]]; then
      printf '%s|%s|%s|%s|%s|%s|%s|%s|%s|%s\n' "$name" \
        "${BURST_WRITER_GROUP_JOBS:-unset}" "${SIM_JOBS:-unset}" \
        "${COPY_COMPARISON_JOBS:-unset}" \
        "${BURST_COPY_COMPARISON_OUTSIDE_SANDBOX:-${BURST_MODELSIM_OUTSIDE_SANDBOX:-unset}}" \
        "${DUPLEX_G2_JOBS:-unset}" "${DUPLEX_ALIAS_JOBS:-unset}" \
        "${DUPLEX_FALLBACK_JOBS:-unset}" "${DUPLEX_INDEPENDENT_JOBS:-unset}" \
        "${MDPI_EXTENT_JOBS:-unset}" \
        >>"${FAKE_JOB_ARGS_FILE:?}"
    fi
    destination=${!#}
    mkdir -p "$destination"
    if [[ ${FAKE_FAIL_JOB:-} == "$name" ]]; then
      printf 'failure marker\n' >"$destination/failure-marker.txt"
      exit 9
    fi
    if [[ -n ${FAKE_WRITE_OUTPUT_SENTINELS:-} ]]; then
      printf '%s\n' "$name" >"$destination/workflow-retention-sentinel.txt"
    fi
    exit 0
    ;;
  *)
    echo "unexpected bash child: $script" >&2
    exit 92
    ;;
esac
"""

    def run_regression(
        self,
        *args: str,
        helper_failure: bool = False,
        env_overrides: dict[str, str] | None = None,
    ) -> subprocess.CompletedProcess[str]:
        env = os.environ.copy()
        env.update(
            {
                "PATH": f"{self.fake_bin}:{self.root / 'modelsim/bin'}:{self.root / 'install/bin'}:{os.environ['PATH']}",
                "TMPDIR": str(self.root),
                "TMP": str(self.root),
                "TEMP": str(self.root),
                "FAKE_HELPER_FAIL": "1" if helper_failure else "0",
                "FAKE_MANTIS_ARGS_FILE": str(self.mantis_args_file),
                "FAKE_MANTIS_ARGV_FILE": str(self.mantis_argv_file),
                "FAKE_BURST_SIM_FILE": str(self.root / "burst-sim-values"),
                "FAKE_JOB_LOG_FILE": str(self.root / "job-calls"),
                "FAKE_JOB_ARGS_FILE": str(self.root / "job-args"),
                "BAMBU_HLS_BACKEND_PATH": "",
            }
        )
        env.update(env_overrides or {})
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

    def test_compiler_group_skips_mantis_and_modelsim_preflight(self) -> None:
        lookup_sentinel = self.root / "modelsim-lookup-called"
        lookup = self.repo / "etc/libtech/backend/simulation/modelsim/tool_lookup.sh"
        lookup.write_text(
            'printf called >>"$FAKE_MODELSIM_LOOKUP_FILE"\n'
            'bambu_modelsim_resolve() { printf resolved >>"$FAKE_MODELSIM_LOOKUP_FILE"; return 91; }\n',
            encoding="utf-8",
        )
        output = self.root / "out_compiler_group"
        result = self.run_regression(
            "--group=compiler",
            "--no-clean",
            "-j1",
            "-o",
            str(output),
            env_overrides={"FAKE_MODELSIM_LOOKUP_FILE": str(lookup_sentinel)},
        )
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertFalse(lookup_sentinel.exists())
        self.assertFalse(self.mantis_args_file.exists())
        self.assertEqual((self.root / "burst-sim-values").read_text(), "0000000")
        calls = (self.root / "job-calls").read_text(encoding="utf-8")
        self.assertIn("run_write_compiler_tests.sh:all", calls)
        self.assertIn("run_frontend_compatibility_tests.sh", calls)
        self.assertIn("run_default_burst_tests.sh", calls)
        self.assertTrue((output / "compiler-default-burst").is_dir())
        self.assertIn("compiler-shared-resource", result.stdout)

    def test_functional_group_strips_selector_before_mantis(self) -> None:
        output = self.root / "out_functional_group"
        result = self.run_regression(
            "--group=functional", "--no-clean", "-j1", "-o", str(output),
            "--timeout=4m", "--commonargs=--max-sim-cycles=1200000",
        )
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertTrue((output / "mantis/AXI-BURST").is_dir())
        self.assertTrue(self.mantis_args_file.is_file())
        mantis_args = self.mantis_args_file.read_text(encoding="utf-8")
        self.assertNotIn("--group", mantis_args)
        self.assertIn("--parallel=1", mantis_args)
        self.assertNotIn("--bambu-parameter=m-axi-burst=1", mantis_args)
        self.assertIn("--max-sim-cycles=1000000", mantis_args)
        self.assertLess(mantis_args.index("--timeout=30m"), mantis_args.index("--timeout=4m"))
        self.assertLess(
            mantis_args.index("--max-sim-cycles=1000000"),
            mantis_args.index("--commonargs=--max-sim-cycles=1200000"),
        )
        self.assertFalse((output / "compiler-pragma").exists())
        calls = (self.root / "job-calls").read_text(encoding="utf-8")
        for runner in (
            "run_burst_fault_diagnostics.sh", "run_axi_width_contract.sh",
            "run_duplex_core_tests.sh", "run_generated_duplex_tests.sh",
            "run_generated_duplex_kernel_tests.sh", "run_generated_write_kernel_tests.sh",
            "run_default_duplex_reduction_modelsims.sh",
            "run_duplex_alias_tests.sh", "run_duplex_fallback_tests.sh",
            "run_duplex_independent_tests.sh", "run_adapter_size_guard.sh",
            "run_mdpi_extent_tests.sh", "run_testbench_memaxi_model.sh",
            "run_write_compiler_tests.sh",
        ):
            self.assertIn(runner, calls)
        self.assertEqual(calls.count("run_generated_write_kernel_tests.sh"), 3)
        self.assertEqual(calls.count("run_duplex_alias_tests.sh"), 2)
        job_args = (self.root / "job-args").read_text(encoding="utf-8").splitlines()
        reduction_args = [row for row in job_args if row.startswith("run_default_duplex_reduction_modelsims.sh|")]
        self.assertEqual(len(reduction_args), 1, job_args)
        runner, bambu, destination, guard = reduction_args[0].split("|")
        self.assertEqual(runner, "run_default_duplex_reduction_modelsims.sh")
        self.assertEqual(bambu, str((self.install / "bin/bambu").resolve()))
        self.assertEqual(destination, str((output / "rtl-default-duplex-reduction").resolve()))
        self.assertEqual(guard, "1")
        self.assertTrue(Path(destination).is_dir())

    def test_functional_workflow_keeps_evidence_under_uploaded_report_dir(self) -> None:
        workflow = (ROOT / ".github/workflows/minimal.yml").read_text(encoding="utf-8")
        job_start = workflow.index("\n  bambu-axi-burst:\n")
        next_job = workflow.index("\n  AXI4-verification:\n", job_start)
        job = workflow[job_start:next_job]
        step_start = job.index("      - name: Launch AXI burst regression\n")
        run_start = job.index("        run: |\n", step_start) + len("        run: |\n")
        run_end = job.index("      - name: Collect post-mortem data\n", run_start)
        run_block = "\n".join(
            line[10:] if line.startswith("          ") else line
            for line in job[run_start:run_end].splitlines()
        )

        invocation = next(line for line in run_block.splitlines() if "--group=functional" in line)
        self.assertEqual(invocation.count("--output="), 1)
        self.assertIn("--no-clean", invocation)
        self.assertIn("--returnfail", invocation)
        self.assertIn("--junitdir=", invocation)
        self.assertIn("--summary=", invocation)
        self.assertIn("-c=--simulator=MODELSIM", invocation)
        self.assertNotRegex(invocation, r"(?:--parallel(?:=|\s)|(?:^|\s)-j\d*)")
        output_assignment = next(line for line in run_block.splitlines() if line.startswith("output="))
        self.assertIn("GITHUB_RUN_ID", output_assignment)
        self.assertIn("GITHUB_RUN_ATTEMPT", output_assignment)
        self.assertIn("path: ${{env.report-dir}}", job)

        report_dir = self.root / "uploaded-reports"
        report_dir.mkdir()
        substitutions = {
            "${{env.dist-dir}}": str(self.install),
            "${{env.report-dir}}": str(report_dir),
        }
        for expression, value in substitutions.items():
            run_block = run_block.replace(expression, shlex.quote(value))
        run_block = run_block.replace(
            "${{env.script}}", "/bin/bash ./panda_regressions/hls/bambu_axi_burst.sh"
        )
        syntax = subprocess.run(
            ["/bin/bash", "-n"], input=run_block, text=True, capture_output=True, check=False
        )
        self.assertEqual(syntax.returncode, 0, syntax.stderr)

        env = os.environ.copy()
        env.update(
            {
                "PATH": f"{self.fake_bin}:{self.root / 'modelsim/bin'}:{self.install / 'bin'}:{os.environ['PATH']}",
                "TMPDIR": str(self.root), "TMP": str(self.root), "TEMP": str(self.root),
                "FAKE_MANTIS_ARGS_FILE": str(self.mantis_args_file),
                "FAKE_MANTIS_ARGV_FILE": str(self.mantis_argv_file),
                "FAKE_JOB_LOG_FILE": str(self.root / "job-calls"),
                "FAKE_JOB_ARGS_FILE": str(self.root / "job-args"),
                "FAKE_WRITE_OUTPUT_SENTINELS": "1",
                "GITHUB_RUN_ID": "regression-restart-test",
                "GITHUB_RUN_ATTEMPT": "3",
                "BAMBU_HLS_BACKEND_PATH": "",
            }
        )
        result = subprocess.run(
            ["/bin/bash", "-e", "-o", "pipefail", "-c", run_block],
            cwd=self.repo, env=env, text=True, capture_output=True, check=False, timeout=90,
        )
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

        retained = report_dir / "axi-burst-functional-regression-restart-test-3"
        self.assertTrue((retained / "jobs/rtl-adapter-size.log").is_file())
        sentinels = list(retained.rglob("workflow-retention-sentinel.txt"))
        self.assertTrue(sentinels)
        self.assertTrue(all(self.is_within(path, report_dir) for path in sentinels))
        self.assertTrue(self.is_within(retained / "jobs/rtl-adapter-size.log", report_dir))
        self.assertTrue((retained / "mantis/AXI-BURST").is_dir())
        self.assertTrue(self.is_within(retained, report_dir))
        self.assertIn("--no-clean", self.mantis_args_file.read_text(encoding="utf-8"))

    def test_functional_dispatch_honors_j20_within_weighted_helper_budget(self) -> None:
        output = self.root / "out_functional_concurrency"
        result = self.run_regression(
            "--group=functional", "--no-clean", "-j20", "-o", str(output),
        )
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        mantis_args = self.mantis_args_file.read_text(encoding="utf-8")
        self.assertIn("--parallel=20", mantis_args)
        self.assertRegex(result.stdout, r"Starting \d+ burst helper jobs \(parallel limit 20\)")
        calls = (self.root / "job-calls").read_text(encoding="utf-8")
        for runner in (
            "run_vector_payload_tests.sh", "run_generated_memaxi_response_tests.sh",
            "run_copy_comparison_tests.sh",
        ):
            self.assertIn(runner, calls)
        self.assertTrue((output / "rtl-vector-payload").is_dir())
        self.assertTrue((output / "rtl-generated-memaxi-response").is_dir())
        self.assertTrue((output / "rtl-copy-comparison").is_dir())
        rows = (self.root / "job-args").read_text(encoding="utf-8").splitlines()
        settings = {row.split("|", 1)[0]: row.split("|") for row in rows}
        self.assertEqual(settings["run_write_burst_engine.sh"][1], "20")
        self.assertEqual(settings["run_generated_duplex_tests.sh"][2], "2")
        self.assertEqual(settings["run_generated_duplex_kernel_tests.sh"][2], "6")
        self.assertEqual(settings["run_generated_duplex_kernel_tests.sh"][5], "6")
        self.assertEqual(settings["run_duplex_alias_tests.sh"][6], "12")
        self.assertEqual(settings["run_duplex_fallback_tests.sh"][7], "4")
        self.assertEqual(settings["run_duplex_independent_tests.sh"][8], "2")
        self.assertEqual(settings["run_mdpi_extent_tests.sh"][9], "13")
        self.assertEqual(settings["run_generated_memaxi_response_tests.sh"][2], "6")
        self.assertEqual(settings["run_copy_comparison_tests.sh"][3], "2")
        self.assertEqual(settings["run_copy_comparison_tests.sh"][4], "1")

    def test_functional_dispatch_caps_every_nested_pool_at_j1(self) -> None:
        output = self.root / "out_functional_j1_budget"
        result = self.run_regression(
            "--group=functional", "--no-clean", "-j1", "-o", str(output),
        )
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        rows = (self.root / "job-args").read_text(encoding="utf-8").splitlines()
        settings = {row.split("|", 1)[0]: row.split("|") for row in rows}
        self.assertEqual(settings["run_write_burst_engine.sh"][1], "1")
        self.assertEqual(settings["run_generated_duplex_tests.sh"][2], "1")
        self.assertEqual(settings["run_generated_duplex_kernel_tests.sh"][5], "1")
        self.assertEqual(settings["run_duplex_alias_tests.sh"][6], "1")
        self.assertEqual(settings["run_duplex_fallback_tests.sh"][7], "1")
        self.assertEqual(settings["run_duplex_independent_tests.sh"][8], "1")
        self.assertEqual(settings["run_mdpi_extent_tests.sh"][9], "1")
        self.assertEqual(settings["run_generated_memaxi_response_tests.sh"][2], "1")
        self.assertEqual(settings["run_copy_comparison_tests.sh"][3], "1")

    def test_all_dispatch_honors_j20(self) -> None:
        output = self.root / "out_all_concurrency"
        result = self.run_regression("--group=all", "--no-clean", "-j20", "-o", str(output))
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertIn("--parallel=20", self.mantis_args_file.read_text(encoding="utf-8"))
        self.assertRegex(result.stdout, r"Starting \d+ burst helper jobs \(parallel limit 20\)")

    def test_functional_dispatch_cleans_new_runner_outputs_by_default(self) -> None:
        output = self.root / "out_functional_cleanup"
        result = self.run_regression(
            "--group=functional", "-j2", "-o", str(output),
        )
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        for name in (
            "rtl-vector-payload", "rtl-generated-memaxi-response", "rtl-copy-comparison",
        ):
            self.assertFalse((output / name).exists(), name)

    def test_fixed_fallback_arguments_preserve_shell_special_paths(self) -> None:
        marker = self.root / "shell-injection-marker"
        special_repo = self.root / "repo space ' ;$(touch shell-injection-marker)"
        self.repo.rename(special_repo)
        self.repo = special_repo
        output = self.root / "out space ' ;$(touch shell-injection-marker)"
        output.mkdir()
        result = self.run_regression("--group=functional", "--restart", "--no-clean", "-j1", "-o", str(output))
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertFalse(marker.exists(), "path metadata was evaluated as a shell command")

        argv = self.mantis_argv_file.read_bytes().split(b"\0")
        argv = [item.decode() for item in argv if item]
        self.assertIn("--restart", argv)
        self.assertIn("--no-clean", argv)
        self.assertIn("--returnfail", argv)
        list_path = Path(argv[argv.index("-l") + 1])
        lines = list_path.read_text(encoding="utf-8").splitlines()
        self.assertGreater(len(lines), 1)
        legacy_write = shlex.split(next(line for line in lines if "--benchmark-name=plain-m-axi-write-n1024" in line))
        self.assertIn("--bambu-parameter=m-axi-burst=0", legacy_write)
        default_read = shlex.split(next(line for line in lines if "--benchmark-name=plain-m-axi-no-latency" in line))
        self.assertNotIn("--bambu-parameter=m-axi-burst=0", default_read)
        fixed_case = shlex.split(lines[-1])
        expected_source = str(self.repo.resolve() / "panda_regressions/hls/bambu_axi_burst/bambu_axi_burst_sum_profile.cpp")
        self.assertTrue(Path(expected_source).is_file())
        expected_tb = str(self.repo.resolve() / "panda_regressions/hls/bambu_axi_burst/bambu_axi_burst_tb.cpp")
        expected_device = str(output.resolve() / "fixed-burst-fallback-inputs/xcu250-fixed.xml")
        self.assertTrue(Path(expected_device).is_file())
        self.assertFalse((self.repo / "panda_regressions/hls/fixed-burst-fallback-inputs").exists())
        expected_post_argv = [
            "python3", "-B", str(self.repo.resolve() / "panda_regressions/hls/bambu_axi_burst/check_fixed_burst_fallback.py"),
            "--output", ".", "--target-file", expected_device,
        ]
        expected_fixed_case = [
            "BENCHMARK_ENV=MANTIS_POST_CHECK=" + shlex.join(expected_post_argv),
            expected_source, "--top-fname=kernel", f"--generate-tb={expected_tb}",
            "--tb-arg=1024", "--tb-param-size=src:16384", "--extra-cc-options=-DBAMBU",
            "--extra-cc-options=-DBAMBU_AXI_SUM_O5", "--extra-cc-options=-DBURST_TEST_DEFAULT_N=1024",
            f"--target-file={expected_device}",
            "--benchmark-name=fixed-burst-fallback",
        ]
        self.assertEqual(fixed_case, expected_fixed_case)
        post_argv = shlex.split(fixed_case[0].split("MANTIS_POST_CHECK=", 1)[1])
        self.assertEqual(post_argv, expected_post_argv)
        self.assertFalse(marker.exists(), "post-check argv triggered shell expansion")

        bambu_args = next(item.split("=", 1)[1] for item in argv if item.startswith("--args="))
        parsed_bambu_args = shlex.split(bambu_args)
        self.assertEqual(parsed_bambu_args, [
            "--configuration-name=AXI-BURST", "--compiler=I386_CLANG16",
            "--experimental-setup=BAMBU", "--generate-interface=INFER",
            "--device-name=xcu250,-2L,figd2104", "--AXI-burst-type=INCREMENTAL",
            "--max-sim-cycles=1000000",
            "--mem-delay-read=64", "--mem-delay-write=2", "--tb-queue-size=32",
            "--memory-allocation-policy=NO_BRAM", "--distram-threshold=0", "--clock-period=3.33",
            "--generate-vcd", "--no-clean", "--extra-cc-options=-DNOCACHE", "--simulate",
            "--simulator=MODELSIM", f"--mentor-root={self.root / 'modelsim'}",
        ])

    def test_functional_gate_failure_retains_no_clean_output(self) -> None:
        output = self.root / "out_failed_functional_gate"
        result = self.run_regression(
            "--group=functional", "--no-clean", "-j1", "-o", str(output),
            env_overrides={"FAKE_FAIL_JOB": "run_adapter_size_guard.sh"},
        )
        self.assertEqual(result.returncode, 1, result.stdout + result.stderr)
        self.assertIn("AXI burst helper suite failed", result.stderr)
        markers = list(output.rglob("failure-marker.txt"))
        self.assertEqual(len(markers), 1, markers)
        self.assertTrue((output / "jobs/rtl-adapter-size.log").is_file())

    def test_invalid_group_fails_before_creating_output(self) -> None:
        output = self.root / "out_invalid_group"
        result = self.run_regression("--group=invalid", "-o", str(output))
        self.assertEqual(result.returncode, 2)
        self.assertIn("invalid group", result.stderr)
        self.assertFalse(output.exists())


if __name__ == "__main__":
    unittest.main()
