#!/usr/bin/env python3
"""Behavioral offline tests for the writer ModelSim runner."""

from __future__ import annotations

import os
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[4]
RTL_TESTS = Path(__file__).resolve().parent
RUNNER = RTL_TESTS / "run_write_burst_engine.sh"
TMP_ROOT = Path(
    os.environ.get("BURST_TEST_TMPDIR", Path.cwd())
) / f"out_{Path(__file__).stem}"

FAKE_MODELSIM = r'''#!/usr/bin/env bash
set -u
tool=${0##*/}
if [[ $tool == vsim && ${1:-} == -version ]]; then
  echo "QuestaSim Fake 2026.4"
  exit 0
fi
if [[ -n ${WRITER_TRACE:-} ]]; then
  printf '%s\t%s\t%s\n' "$tool" "$PWD" "$*" >> "$WRITER_TRACE"
fi
case $tool in
  vmap)
    if [[ ${1:-} == -c ]]; then
      printf 'fake work mapping\n' > modelsim.ini
    fi
    exit 0
    ;;
  vlib)
    mkdir -p work
    exit 0
    ;;
  vlog)
    case ${WRITER_VLOG_MODE:-pass} in
      fail) echo 'FAKE_VLOG_DIAGNOSTIC'; exit 7 ;;
      timeout) echo 'FAKE_VLOG_TIMEOUT'; exit 124 ;;
    esac
    exit 0
    ;;
  vsim)
    selected_case=
    top=
    expect_fault=0
    for arg in "$@"; do
      case $arg in
        +CASE=*) selected_case=${arg#'+CASE='} ;;
        +AXI_BURST_EXPECT_FAULT) expect_fault=1 ;;
        work.*) top=${arg#work.} ;;
      esac
    done
    mode=${WRITER_VSIM_MODE:-pass}
    if [[ $selected_case == PARAM && -n ${WRITER_VSIM_PARAM_MODE:-} ]]; then
      mode=$WRITER_VSIM_PARAM_MODE
    fi
    case $mode in
      pass|fault-missing|fault-wrong|fault-extra|fault-instance|fault-malformed-payload|fault-wrong-payload)
        expected_reason=
        diagnostic_class=FAULT
        case $selected_case in
          BAD_CONFIG_SIZE|BAD_CONFIG_ALIGN|COUNT_MSB|OVERFLOW32|OVERFLOW64|CONFIG_PIPELINE_INVALID|\
          CONFIGURE64_BAD_ALIGN_ZERO|CFG_*|PREMATURE_CONFIGURE*|ASYNC_RESET)
            expected_reason=CONFIGURE_INVALID ;;
          BAD_STORE_SIZE|BAD_STORE_SIZE8|BAD_STORE_ADDRESS|FAST_STORE_ACK_INVALID|FAST_STORE_ACK_ZERO_REGION|\
          FAST_STORE_ACK_WRAP)
            expected_reason=STORE_INVALID ;;
          PENDING_OPERAND_CHANGE*) expected_reason=PENDING_STORE_CHANGED ;;
          EARLY_DRAIN|DRAIN_PENDING_SHORT_COUNT) expected_reason=DRAIN_EARLY ;;
          BID) expected_reason=BID ;;
          BRESP|BRESP_*|FAST_STORE_ACK_BFAULT) expected_reason=BRESP ;;
          PEER_FAULT_SPECULATIVE) diagnostic_class=PEER_FAULT; expected_reason=peer ;;
        esac
        if [[ $top == tb_configure_writer_pipeline && $selected_case == PEER_FAULT_E1 ]]; then
          diagnostic_class=PEER_FAULT
          expected_reason=peer
        fi
        if [[ $expect_fault == 1 && -n $expected_reason && $mode != fault-missing ]]; then
          if [[ $mode == fault-wrong ]]; then expected_reason=WRONG_REASON; fi
          record_instance=$top.dut.report_axi_write_diagnostic
          record_payload=00000000
          record_opcode=2
          record_size=32
          record_count=1
          if [[ $selected_case == FAST_STORE_ACK_WRAP ]]; then
            record_opcode=1
            record_count=0
            record_payload=ca770000
          fi
          if [[ $mode == fault-instance ]]; then record_instance=$top.other; fi
          if [[ $mode == fault-malformed-payload ]]; then record_payload=xyz; fi
          if [[ $mode == fault-wrong-payload ]]; then record_payload=deadbeef; fi
          record_address=00001000
          if [[ $top == tb_configure_writer_pipeline ]]; then
            record_opcode=-1
            record_size=0
            record_count=0
            record_address=00000000
          fi
          if [[ $selected_case == FAST_STORE_ACK_WRAP ]]; then record_address=0000000000000000; fi
          echo "# AXI_BURST_${diagnostic_class} engine=write reason=$expected_reason instance=$record_instance opcode=$record_opcode size=$record_size address=$record_address count=$record_count payload=$record_payload resp=00 id=0 descriptors=0 remaining=0"
          if [[ $mode == fault-extra ]]; then
            echo '# AXI_BURST_FAULT engine=write reason=EXTRA instance=tb_write_burst_engine.dut opcode=2 size=32 address=00001000 count=1 payload=00000000 resp=00 id=0 descriptors=0 remaining=0'
          fi
        fi
        if [[ $top == tb_configure_writer_pipeline ]]; then
          echo "# CONFIGURE_WRITER_PIPELINE PASS CASE=$selected_case"
        else
          echo "# WRITE_BURST_ENGINE PASS CASE=$selected_case B=16 FIFO=16 AW=1 W=1 BRESP=1"
        fi
        if [[ $selected_case == FAST_STORE_ACK_STREAM ]]; then
          echo '# WRITE_FAST_STORE_ACK_STREAM beats=64 interval=1'
        fi
        if [[ $selected_case == FAST_STORE_ACK_BACKPRESSURE ]]; then
          echo '# WRITE_FAST_STORE_ACK_BACKPRESSURE_PASS held=1'
        fi
        exit 0
        ;;
      no-marker)
        echo '# Simulation completed without a test marker'
        exit 0
        ;;
      wrong-marker)
        echo '# WRITE_BURST_ENGINE PASS CASE=WRONG B=16 FIFO=16 AW=1 W=1 BRESP=1'
        exit 0
        ;;
      nonzero)
        echo 'FAKE_VSIM_DIAGNOSTIC'
        exit 9
        ;;
      timeout)
        echo "# ** Fatal: ${WRITER_FATAL_TEXT:-MAX_OUTSTANDING must be in [1,16]}"
        exit 124
        ;;
      vopt-fatal)
        echo "# ** Fatal: ${WRITER_FATAL_TEXT:-MAX_OUTSTANDING must be in [1,16]}"
        echo '# Error loading design'
        exit 12
        ;;
      runtime-fatal)
        echo "# ** Fatal: ${WRITER_FATAL_TEXT:-WRITE_DATA_MISMATCH}"
        exit 0
        ;;
      crash-after-fatal)
        echo "# ** Fatal: ${WRITER_FATAL_TEXT:-MAX_OUTSTANDING must be in [1,16]}"
        echo '# Segmentation fault'
        exit 0
        ;;
      *)
        echo "unknown fake vsim mode: $mode" >&2
        exit 99
        ;;
    esac
    ;;
  *)
    echo "unexpected fake ModelSim entry point: $tool" >&2
    exit 98
    ;;
esac
'''


def _write_executable(path: Path, contents: str) -> None:
    path.write_text(contents, encoding="utf-8")
    path.chmod(0o755)


class WriterModelSimRunnerTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        TMP_ROOT.mkdir(parents=True, exist_ok=True)

    def setUp(self) -> None:
        self.case_root = Path(tempfile.mkdtemp(prefix="writer-modelsim-", dir=TMP_ROOT))
        self.launch = self.case_root / "launch cwd"
        self.launch.mkdir()
        self.tools = self.case_root / "fake ModelSim tools"
        self.tools.mkdir()
        self.utilities = self.case_root / "host utilities without verilator"
        self.utilities.mkdir()
        self.trace = self.case_root / "tool calls.log"
        self._install_fake_tools(self.tools)
        self._install_utilities(self.utilities)
        self.env = os.environ.copy()
        self.env.update(
            PATH=os.pathsep.join((str(self.tools), str(self.utilities))),
            BURST_WRITER_SIM_OUTSIDE_SANDBOX="1",
            BURST_WRITER_GROUP_JOBS="2",
            WRITER_TRACE=str(self.trace),
            TMPDIR=str(self.case_root),
            TMP=str(self.case_root),
            TEMP=str(self.case_root),
        )
        self.env.pop("BAMBU_HLS_BACKEND_PATH", None)
        self.env.pop("WRITER_VLOG_MODE", None)
        self.env.pop("WRITER_VSIM_MODE", None)
        self.env.pop("WRITER_VSIM_PARAM_MODE", None)
        self.env.pop("WRITER_FATAL_TEXT", None)

    def tearDown(self) -> None:
        shutil.rmtree(self.case_root)

    @staticmethod
    def _install_fake_tools(path: Path) -> None:
        vco = path / "vco"
        _write_executable(vco, FAKE_MODELSIM)
        for tool in ("vmap", "vlib", "vlog", "vsim"):
            (path / tool).symlink_to(vco.name)

    @staticmethod
    def _install_utilities(path: Path) -> None:
        utilities = (
            "bash", "env", "python3", "sha256sum", "timeout", "grep", "sed", "tail",
            "tr", "realpath", "dirname", "basename", "mkdir", "git", "find", "tee",
            "cat", "rm", "sleep", "head", "cut", "awk", "sort", "wc", "chmod",
        )
        for utility in utilities:
            found = shutil.which(utility, path=os.environ.get("PATH"))
            if found:
                (path / utility).symlink_to(found)

    def _run(
        self,
        output: Path,
        groups: str,
        *,
        env: dict[str, str] | None = None,
        path: str | None = None,
        tools: Path | None = None,
        jobs: int = 2,
    ) -> subprocess.CompletedProcess[str]:
        run_env = (env or self.env).copy()
        selected_tools = tools or self.tools
        run_env["PATH"] = path or os.pathsep.join((str(selected_tools), str(self.utilities)))
        run_env["BURST_WRITER_GROUP_JOBS"] = str(jobs)
        run_env["BURST_WRITER_GROUP_FILTER"] = groups
        run_env["WRITER_TRACE"] = str(self.trace)
        run_env["TMPDIR"] = str(self.case_root)
        run_env["TMP"] = str(self.case_root)
        run_env["TEMP"] = str(self.case_root)
        return subprocess.run(
            [str(self.utilities / "bash"), str(RUNNER), str(output)],
            cwd=self.launch,
            env=run_env,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            check=False,
        )

    def _new_output(self, name: str) -> Path:
        return self.case_root / name

    def test_fresh_output_directory_is_required(self) -> None:
        result = subprocess.run(
            [str(self.utilities / "bash"), str(RUNNER)],
            cwd=self.launch,
            env=self.env,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            check=False,
        )
        self.assertEqual(result.returncode, 2, result.stdout)
        self.assertIn("usage:", result.stdout)
        self.assertFalse((self.launch / "out_write_burst_engine").exists())

    def _backend(self, *, include_vmap: bool = True) -> Path:
        backend = self.case_root / "backend with spaces" / "questa" / "bin"
        backend.mkdir(parents=True)
        for tool in ("vsim", "vlib", "vlog", "vmap"):
            if include_vmap or tool != "vmap":
                (backend / tool).symlink_to(self.tools / tool)
        return backend.parent.parent

    def test_path_resolution_and_prefixed_pass_work_without_verilator(self) -> None:
        output = self._new_output("output with spaces")
        result = self._run(output, "core-B16-D16")
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn("WRITE_BURST_ENGINE_SUBSET_PASS", result.stdout)
        self.assertTrue((output / "provenance.txt").is_file())
        self.assertIn("modelsim_lookup_source=PATH", (output / "provenance.txt").read_text())
        calls = self.trace.read_text(encoding="utf-8").splitlines()
        self.assertTrue(any(line.startswith("vlog\t") for line in calls))
        self.assertTrue(any(line.startswith("vsim\t") for line in calls))
        self.assertFalse(any(line.startswith("verilator\t") for line in calls))
        self.assertIn("WRITE_BURST_ENGINE PASS CASE=CORE", (output / "results.log").read_text())

    def test_backend_fallback_uses_shared_resolver(self) -> None:
        backend_root = self._backend()
        output = self._new_output("backend output")
        result = self._run(
            output,
            "core-B16-D16",
            path=str(self.utilities),
            env=self.env | {"BAMBU_HLS_BACKEND_PATH": str(backend_root)},
        )
        self.assertEqual(result.returncode, 0, result.stdout)
        provenance = (output / "provenance.txt").read_text(encoding="utf-8")
        self.assertIn("modelsim_lookup_source=BAMBU_HLS_BACKEND_PATH", provenance)
        self.assertIn(f"modelsim_bin={backend_root / 'questa' / 'bin'}", provenance)
        calls = self.trace.read_text(encoding="utf-8").splitlines()
        self.assertTrue(any(line.startswith("vsim\t") for line in calls))

    def test_missing_sibling_fails_before_output_creation(self) -> None:
        backend_root = self._backend(include_vmap=False)
        output = self._new_output("must not be created")
        result = self._run(
            output,
            "core-B16-D16",
            path=str(self.utilities),
            env=self.env | {"BAMBU_HLS_BACKEND_PATH": str(backend_root)},
        )
        self.assertEqual(result.returncode, 2, result.stdout)
        self.assertIn("required ModelSim sibling tool is not executable", result.stdout)
        self.assertFalse(output.exists())

    def test_existing_and_symlink_outputs_are_rejected(self) -> None:
        existing = self._new_output("existing")
        existing.mkdir()
        result = self._run(existing, "core-B16-D16")
        self.assertEqual(result.returncode, 2, result.stdout)
        self.assertTrue(existing.is_dir())
        target = self._new_output("symlink-target")
        target.mkdir()
        link = self._new_output("output-link")
        link.symlink_to(target)
        result = self._run(link, "core-B16-D16")
        self.assertEqual(result.returncode, 2, result.stdout)
        self.assertTrue(link.is_symlink())

    def test_vlog_failure_and_timeout_are_not_qualified(self) -> None:
        for mode in ("fail", "timeout"):
            with self.subTest(mode=mode):
                output = self._new_output(f"vlog-{mode}")
                result = self._run(output, "core-B16-D16", env=self.env | {"WRITER_VLOG_MODE": mode})
                self.assertNotEqual(result.returncode, 0, result.stdout)
                self.assertNotIn("WRITE_BURST_ENGINE_SUBSET_PASS", result.stdout)
                self.assertFalse((output / "results.log").exists())
                if mode == "fail":
                    self.assertIn("FAKE_VLOG_DIAGNOSTIC", result.stdout)
                else:
                    self.assertIn("ModelSim vlog timed out", result.stdout)

    def test_simulator_nonzero_timeout_missing_and_wrong_markers_fail_closed(self) -> None:
        cases = (
            ("nonzero", "FAKE_VSIM_DIAGNOSTIC"),
            ("timeout", "ModelSim simulation timed out"),
            ("no-marker", "missing exact PASS marker"),
            ("wrong-marker", "missing exact PASS marker"),
        )
        for mode, message in cases:
            with self.subTest(mode=mode):
                output = self._new_output(f"vsim-{mode}")
                result = self._run(output, "core-B16-D16", env=self.env | {"WRITER_VSIM_MODE": mode})
                self.assertNotEqual(result.returncode, 0, result.stdout)
                self.assertIn(message, result.stdout)
                self.assertNotIn("WRITE_BURST_ENGINE_SUBSET_PASS", result.stdout)

    def test_intentional_faults_select_and_validate_exact_diagnostics(self) -> None:
        output = self._new_output("intentional-fault-diagnostics")
        result = self._run(output, "core-B16-D16")
        self.assertEqual(result.returncode, 0, result.stdout)
        calls = [line for line in self.trace.read_text(encoding="utf-8").splitlines()
                 if line.startswith("vsim\t")]
        config_call = next(line for line in calls if "+CASE=BAD_CONFIG_SIZE" in line)
        normal_call = next(line for line in calls if "+CASE=CORE" in line)
        self.assertIn("+AXI_BURST_EXPECT_FAULT", config_call)
        self.assertNotIn("+AXI_BURST_EXPECT_FAULT", normal_call)
        results = (output / "results.log").read_text(encoding="utf-8")
        self.assertIn("AXI_BURST_FAULT engine=write reason=CONFIGURE_INVALID", results)
        self.assertIn("AXI_BURST_FAULT engine=write reason=STORE_INVALID", results)
        self.assertIn("AXI_BURST_FAULT engine=write reason=PENDING_STORE_CHANGED", results)
        self.assertIn("AXI_BURST_FAULT engine=write reason=DRAIN_EARLY", results)
        self.assertIn("AXI_BURST_FAULT engine=write reason=BID", results)
        self.assertIn("AXI_BURST_FAULT engine=write reason=BRESP", results)

        address_output = self._new_output("intentional-wrap-fault")
        address_result = self._run(address_output, "fast-store-ack-address64")
        self.assertEqual(address_result.returncode, 0, address_result.stdout)
        address_calls = [line for line in self.trace.read_text(encoding="utf-8").splitlines()
                         if line.startswith("vsim\t")]
        wrap_call = next(line for line in address_calls if "+CASE=FAST_STORE_ACK_WRAP" in line)
        carry_call = next(line for line in address_calls if "+CASE=FAST_STORE_ACK_CARRY" in line)
        self.assertIn("+AXI_BURST_EXPECT_FAULT", wrap_call)
        self.assertNotIn("+AXI_BURST_EXPECT_FAULT", carry_call)
        address_results = (address_output / "results.log").read_text(encoding="utf-8")
        self.assertIn("FAST_STORE_ACK_WRAP", address_results)
        self.assertIn(
            "AXI_BURST_FAULT engine=write reason=STORE_INVALID "
            "instance=tb_write_burst_engine.dut.report_axi_write_diagnostic "
            "opcode=1 size=32 address=0000000000000000 count=0 payload=ca770000 "
            "resp=00 id=0 descriptors=0 remaining=0",
            address_results,
        )

        for mode in ("fault-instance", "fault-malformed-payload"):
            with self.subTest(mode=mode):
                invalid_output = self._new_output(f"intentional-fault-{mode}")
                invalid_result = self._run(
                    invalid_output,
                    "core-B16-D16",
                    env=self.env | {"WRITER_VSIM_MODE": mode},
                )
                self.assertNotEqual(invalid_result.returncode, 0, invalid_result.stdout)
                self.assertIn("schema-valid FAULT", invalid_result.stdout)

        wrong_value_output = self._new_output("intentional-wrap-wrong-value")
        wrong_value_result = self._run(
            wrong_value_output,
            "fast-store-ack-address64",
            env=self.env | {"WRITER_VSIM_MODE": "fault-wrong-payload"},
        )
        self.assertNotEqual(wrong_value_result.returncode, 0, wrong_value_result.stdout)
        self.assertIn("unexpected stable STORE_INVALID fields", wrong_value_result.stdout)

    def test_intentional_faults_reject_missing_wrong_or_extra_diagnostics(self) -> None:
        for mode in ("fault-missing", "fault-wrong", "fault-extra"):
            with self.subTest(mode=mode):
                output = self._new_output(f"intentional-fault-{mode}")
                result = self._run(
                    output,
                    "core-B16-D16",
                    env=self.env | {"WRITER_VSIM_MODE": mode},
                )
                self.assertNotEqual(result.returncode, 0, result.stdout)
                self.assertNotIn("WRITE_BURST_ENGINE_SUBSET_PASS", result.stdout)

    def test_parameter_vopt_fatal_requires_exact_diagnostic(self) -> None:
        output = self._new_output("invalid-vopt-pass")
        result = self._run(
            output,
            "invalid-O0",
            env=self.env | {"WRITER_VSIM_PARAM_MODE": "vopt-fatal"},
        )
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn("EXPECTED_PARAMETER_FATAL O0 status=12", (output / "results.log").read_text())

        for mode, fatal in (
            ("vopt-fatal", "WRONG PARAMETER DIAGNOSTIC"),
            ("timeout", "MAX_OUTSTANDING must be in [1,16]"),
            ("crash-after-fatal", "MAX_OUTSTANDING must be in [1,16]"),
        ):
            with self.subTest(mode=mode, fatal=fatal):
                output = self._new_output(f"invalid-{mode}-{len(list(self.case_root.iterdir()))}")
                result = self._run(
                    output,
                    "invalid-O0",
                    env=self.env | {"WRITER_VSIM_PARAM_MODE": mode, "WRITER_FATAL_TEXT": fatal},
                )
                self.assertNotEqual(result.returncode, 0, result.stdout)
                self.assertNotIn("WRITE_BURST_ENGINE_SUBSET_PASS", result.stdout)

    def test_runtime_invalid_fatal_is_accepted_only_with_exact_message(self) -> None:
        output = self._new_output("invalid-runtime-pass")
        result = self._run(
            output,
            "invalid-O0",
            env=self.env | {
                "WRITER_VSIM_PARAM_MODE": "runtime-fatal",
                "WRITER_FATAL_TEXT": "MAX_OUTSTANDING must be in [1,16]",
            },
        )
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn("EXPECTED_PARAMETER_FATAL O0 status=1", (output / "results.log").read_text())

    def test_mutation_requires_the_named_runtime_fatal(self) -> None:
        output = self._new_output("mutation-fatal-pass")
        result = self._run(
            output,
            "mutation-data-order",
            env=self.env | {"WRITER_VSIM_MODE": "runtime-fatal"},
        )
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn("MUTATION_REJECTED data-order status=1: WRITE_DATA_MISMATCH", (output / "results.log").read_text())

        output = self._new_output("mutation-no-fatal")
        result = self._run(output, "mutation-data-order")
        self.assertNotEqual(result.returncode, 0, result.stdout)
        self.assertIn("expected one named ModelSim fatal", result.stdout)
        self.assertNotIn("WRITE_BURST_ENGINE_SUBSET_PASS", result.stdout)

    def test_async_profiles_and_configure_pipeline_use_isolated_libraries(self) -> None:
        output = self._new_output("async-and-configure")
        result = self._run(
            output,
            "async-reset-active-low,fast-async-reset,configure-pipeline",
            jobs=3,
        )
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertTrue((output / "build-ASYNC_RESET_active-low_FAST0" / "modelsim.ini").is_file())
        self.assertTrue((output / "build-ASYNC_RESET_active-low_FAST1" / "modelsim.ini").is_file())
        self.assertIn("CONFIGURE_WRITER_PIPELINE_SUITE_PASS cases=2", (output / "configure-writer-pipeline" / "aggregate.log").read_text())
        calls = self.trace.read_text(encoding="utf-8")
        self.assertIn("+CASE=PEER_FAULT_E1 +AXI_BURST_EXPECT_FAULT", calls)
        self.assertIn(
            "AXI_BURST_PEER_FAULT engine=write reason=peer "
            "instance=tb_configure_writer_pipeline.dut.report_axi_write_diagnostic "
            "opcode=-1 size=0 address=00000000 count=0 payload=00000000 "
            "resp=00 id=0 descriptors=0 remaining=0",
            (output / "configure-writer-pipeline" / "aggregate.log").read_text(),
        )
        vlog_dirs = {
            line.split("\t", 2)[1]
            for line in self.trace.read_text(encoding="utf-8").splitlines()
            if line.startswith("vlog\t")
        }
        self.assertTrue(any("ASYNC_RESET_active-low_FAST0" in directory for directory in vlog_dirs))
        self.assertTrue(any("ASYNC_RESET_active-low_FAST1" in directory for directory in vlog_dirs))
        self.assertTrue(any("configure-writer-pipeline" in directory for directory in vlog_dirs))

    def test_configure_writer_peer_fault_rejects_missing_or_wrong_diagnostic(self) -> None:
        for mode in ("fault-missing", "fault-instance"):
            with self.subTest(mode=mode):
                output = self._new_output("configure-peer-" + mode)
                result = self._run(
                    output,
                    "configure-pipeline",
                    env=self.env | {"WRITER_VSIM_MODE": mode},
                )
                self.assertNotEqual(result.returncode, 0, result.stdout)

    def test_parallel_group_failure_is_propagated(self) -> None:
        output = self._new_output("parallel-failure")
        result = self._run(
            output,
            "core-B16-D16,invalid-O0",
            jobs=2,
            env=self.env | {
                "WRITER_VSIM_MODE": "nonzero",
                "WRITER_VSIM_PARAM_MODE": "vopt-fatal",
            },
        )
        self.assertNotEqual(result.returncode, 0, result.stdout)
        self.assertIn("writer RTL test group failed", result.stdout)
        self.assertNotIn("WRITE_BURST_ENGINE_SUBSET_PASS", result.stdout)
        self.assertTrue((output / "job-logs" / "core-B16-D16.log").is_file())


if __name__ == "__main__":
    unittest.main(verbosity=2)
