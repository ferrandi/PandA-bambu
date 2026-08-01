#!/usr/bin/env python3
"""Focused tests for hosted fast-regression result production."""

from __future__ import annotations

import copy
import io
import stat
import struct
import subprocess
import sys
import tempfile
import unittest
from contextlib import redirect_stdout
from pathlib import Path
from unittest.mock import patch


SCRIPTS_DIRECTORY = Path(__file__).resolve().parent
if str(SCRIPTS_DIRECTORY) not in sys.path:
    sys.path.insert(0, str(SCRIPTS_DIRECTORY))

from ci_results.bundle import BundleValidationError, validate_bundle  # noqa: E402
from ci_results.generate import generate_bundle  # noqa: E402
from ci_results.hashing import sha256_file  # noqa: E402
from ci_results.regressions import (  # noqa: E402
    REGRESSION_SPECS,
    _actual_arguments,
    _artifact_ids,
    _checks,
    _classify_nonzero,
    _failure,
    _metrics,
    _missing_rtl_authenticity_instances,
    _normalized_arguments,
    _stage_records,
    _task_configuration,
    _write_verilator_wrapper,
    run_regression,
    run_regression_suite,
)
from ci_results.serialization import canonical_text, load_json, write_json  # noqa: E402
from ci_results.summary import render_bundle_summary  # noqa: E402
from test_ci_results import successful_environment  # noqa: E402


REPOSITORY = Path(__file__).resolve().parents[2]
STAGE_IDS = (
    "input-validation",
    "hls-synthesis",
    "rtl-generation",
    "simulator-preparation",
    "rtl-simulation",
    "result-verification",
)
FAILURES = {
    "hls-synthesis": ("compilation", "hls-synthesis-failed", "HLS synthesis failed."),
    "rtl-simulation": ("execution", "rtl-simulation-failed", "RTL simulation failed."),
    "result-verification": (
        "verification",
        "result-mismatch",
        "Simulated output did not match the established vectors.",
    ),
}


def regression_task(spec, failing_stage: str | None = None) -> dict:
    artifact_ids = _artifact_ids(spec.task_id)
    failure = None
    outcome = "pass"
    status = 0
    if failing_stage is not None:
        category, code, message = FAILURES[failing_stage]
        artifact_index = {
            "hls-synthesis": 0,
            "rtl-simulation": 3,
            "result-verification": 1,
        }[failing_stage]
        failure = _failure(
            category,
            code,
            failing_stage,
            message,
            artifact_ids[artifact_index],
        )
        outcome = "fail"
        status = 1
    durations = {stage_id: 1 for stage_id in STAGE_IDS}
    stages = _stage_records(
        durations, artifact_ids, outcome, failure, status, None
    )
    stage_outcomes = {stage["stage_id"]: stage["outcome"] for stage in stages}
    arguments = _normalized_arguments(spec, "I386_CLANG16", 2)
    return {
        "artifacts": list(artifact_ids),
        "checks": _checks(stages),
        "configuration": _task_configuration(
            REPOSITORY,
            REPOSITORY / "panda_dist/bin/bambu",
            spec,
            arguments,
            "I386_CLANG16",
            2,
            "I386_CLANG16",
        ),
        "execution": {
            "completed_at": "2023-11-14T22:14:26Z",
            "exit_status": status,
            "started_at": "2023-11-14T22:14:20Z",
            "state": "completed",
        },
        "failure": failure,
        "metrics": _metrics(durations, 6),
        "outcome": outcome,
        "results": {
            "simulation": {
                "completed": stage_outcomes["rtl-simulation"] == "pass",
                "execution_count": 1
                if stage_outcomes["rtl-simulation"] == "pass"
                else None,
                "total_cycles": 42
                if stage_outcomes["rtl-simulation"] == "pass"
                else None,
                "verified": stage_outcomes["result-verification"] == "pass",
            },
            "synthesis": {
                "completed": stage_outcomes["rtl-generation"] == "pass",
                "rtl_artifact_count": 1
                if stage_outcomes["rtl-generation"] == "pass"
                else 0,
            },
        },
        "schema": "panda.ci.task-result",
        "schema_version": "1.1",
        "stages": stages,
        "task_id": spec.task_id,
        "task_type": "regression",
    }


class FastRegressionResultTests(unittest.TestCase):
    def test_synthesis_smoke_outputs_are_normalized_for_artifact_upload(self) -> None:
        helper = SCRIPTS_DIRECTORY.parent / "actions" / "build-panda" / "normalize-output-permissions.sh"
        with tempfile.TemporaryDirectory() as temporary:
            output = Path(temporary) / "synthesis-smoke"
            nested = output / "nested"
            nested.mkdir(parents=True)
            report = nested / "bambu_results.xml"
            report.write_text("<application/>\n", encoding="utf-8")
            output.chmod(0o700)
            nested.chmod(0o700)
            report.chmod(0o600)
            subprocess.run([str(helper), str(output)], check=True)
            self.assertTrue(stat.S_IMODE(output.stat().st_mode) & stat.S_IXOTH)
            self.assertTrue(stat.S_IMODE(nested.stat().st_mode) & stat.S_IXOTH)
            self.assertTrue(stat.S_IMODE(report.stat().st_mode) & stat.S_IROTH)
        entrypoint = helper.with_name("entrypoint.sh").read_text(encoding="utf-8")
        dockerfile = helper.with_name("Dockerfile").read_text(encoding="utf-8")
        self.assertIn("normalize-output-permissions.sh", entrypoint)
        self.assertIn("COPY normalize-output-permissions.sh", dockerfile)

    def setUp(self) -> None:
        self.temporary_directory = tempfile.TemporaryDirectory(
            prefix=".ci-regression-fixture-", dir=REPOSITORY
        )
        self.root = Path(self.temporary_directory.name)
        self.raw = self.root / "raw"
        self.evidence = self.root / "evidence"
        self.bundle = self.root / "bundle"
        self.environment = successful_environment()
        self.environment.update(
            {
                "PANDA_CI_REGRESSION_ARTIFACT_NAME": "fast-regression-evidence-test",
                "PANDA_CI_REGRESSION_EVIDENCE_DIR": self.evidence.relative_to(
                    REPOSITORY
                ).as_posix(),
                "PANDA_CI_OPEN_BUILD_COMPLETION_EPOCH": "1700000060",
                "PANDA_CI_WORKFLOW_FILE": ".github/workflows/fast-regressions-hosted.yml",
            }
        )

    def tearDown(self) -> None:
        self.temporary_directory.cleanup()

    def write_raw(self, failures: dict[str, str] | None = None) -> None:
        failures = failures or {}
        failed_count = len(failures)
        passed_count = len(REGRESSION_SPECS) - failed_count
        self.environment.update(
            {
                "PANDA_CI_REGRESSION_ACTION_EXIT_STATUS": "1" if failures else "0",
                "PANDA_CI_REGRESSION_ACTION_COMPLETION_EPOCH": "1700000090",
                "PANDA_CI_REGRESSION_ACTION_OUTCOME": "failure"
                if failures
                else "success",
                "PANDA_CI_REGRESSION_ACTION_START_EPOCH": "1700000060",
                "PANDA_CI_REGRESSION_CONTAINER_SETUP_SECONDS": "1",
                "PANDA_CI_REGRESSION_FAILED_COUNT": str(failed_count),
                "PANDA_CI_REGRESSION_FAILURE_STAGE": "fast-regressions"
                if failures
                else "none",
                "PANDA_CI_REGRESSION_OUTCOME": "fail" if failures else "pass",
                "PANDA_CI_REGRESSION_PASSED_COUNT": str(passed_count),
                "PANDA_CI_REGRESSION_SECONDS": "30",
                "PANDA_CI_REGRESSION_TASK_COUNT": str(len(REGRESSION_SPECS)),
                "PANDA_CI_REGRESSION_TIMEOUT_SECONDS": "2100",
            }
        )
        for spec in REGRESSION_SPECS:
            write_json(
                self.raw / "tasks" / f"{spec.task_id}.json",
                regression_task(spec, failures.get(spec.task_id)),
            )
            directory = self.evidence / spec.task_id
            directory.mkdir(parents=True, exist_ok=True)
            (directory / "bambu.log").write_text("bambu fixture\n", encoding="utf-8")
            (directory / "bambu_results.xml").write_text(
                '<bambu_results><application><timing><simulation return_value="0">'
                "<run>42</run></simulation></timing></application></bambu_results>\n",
                encoding="utf-8",
            )
            (directory / "rtl-files.txt").write_text("top.v\t128\n", encoding="utf-8")
            (directory / "simulation.log").write_text("42\n", encoding="utf-8")
            if spec.task_id == "regression-graphsage":
                (directory / "runtime-linkage.txt").write_text(
                    "runtime-linkage-report-v1\nverification\tpass\n", encoding="utf-8"
                )
            if spec.task_id in {"regression-graphsage-serial", "regression-graphsage"}:
                write_json(directory / "output-comparison.json", {
                    "schema": "panda.ci.graphsage-comparison", "schema_version": "1.0",
                    "outcome": "pass", "mismatch_count": 0, "cases": [],
                })
        write_json(
            self.raw / "suite.json",
            {
                "completed_at": "2023-11-14T22:14:50Z",
                "duration_seconds": 30,
                "exit_status": 1 if failures else 0,
                "failed_count": failed_count,
                "outcome": "fail" if failures else "pass",
                "passed_count": passed_count,
                "started_at": "2023-11-14T22:14:20Z",
                "task_count": len(REGRESSION_SPECS),
                "task_ids": [spec.task_id for spec in REGRESSION_SPECS],
            },
        )

    def generate(self, failures: dict[str, str] | None = None, output: Path | None = None):
        self.write_raw(failures)
        return generate_bundle(
            output or self.bundle,
            environment=self.environment,
            repository=REPOSITORY,
            regression_results=self.raw,
        )

    def rehash(self, relative_path: str) -> None:
        manifest = load_json(self.bundle / "manifest.json")
        reference = next(
            item for item in manifest["documents"] if item["path"] == relative_path
        )
        reference["sha256"] = sha256_file(self.bundle / relative_path)
        write_json(self.bundle / "manifest.json", manifest)

    def test_selected_inputs_exist_and_use_established_vectors(self) -> None:
        self.assertEqual(len(REGRESSION_SPECS), 8)
        for spec in REGRESSION_SPECS:
            self.assertTrue((REPOSITORY / spec.source_path).is_file(), spec.source_path)
            if spec.test_vector_kind in {"xml", "cxx"}:
                self.assertTrue((REPOSITORY / spec.test_vector).is_file(), spec.test_vector)
        loop_task = regression_task(REGRESSION_SPECS[2])
        callgraph_task = regression_task(REGRESSION_SPECS[4])
        self.assertEqual(loop_task["configuration"]["options"]["language_standard"], "c++17")
        self.assertEqual(
            callgraph_task["configuration"]["options"]["bambu_parameters"],
            ["function-opt=0"],
        )
        sparta_task = regression_task(REGRESSION_SPECS[5])
        self.assertEqual(sparta_task["configuration"]["input"]["top_function"], "vector_add")
        self.assertEqual(
            sparta_task["configuration"]["invocation"]["arguments"],
            [
                "examples/OpenMP/functional/src/vector_add.cpp",
                "-lm",
                "-fopenmp",
                "--context_switch=2",
                "--channels-type=MEM_ACC_11",
                "--memory-allocation-policy=GLSS",
                "--simulate",
                "--simulator=VERILATOR",
                "--generate-tb=examples/OpenMP/functional/src/vector.xml",
                "--top-fname=vector_add",
                "--compiler=I386_CLANG16",
                "--parallel-backend=2",
                "--output-directory=.ci-regression-work/regression-sparta/output",
                "--no-clean",
            ],
        )
        vectors = (REPOSITORY / REGRESSION_SPECS[5].test_vector).read_text(encoding="utf-8")
        self.assertIn('a="{1,2,3,4,5,6,7,8,9,10', vectors)

        graphsage_spec = REGRESSION_SPECS[7]
        graphsage_task = regression_task(graphsage_spec)
        self.assertEqual(graphsage_task["configuration"]["input"]["test_vector_kind"], "cxx")
        self.assertEqual(
            graphsage_task["configuration"]["invocation"]["arguments"],
            [
                "examples/GraphSAGE/graphsage_mean.cpp",
                "-fopenmp",
                "--context_switch=2",
                "--channels-type=MEM_ACC_11",
                "--memory-allocation-policy=GLSS",
                "--tb-extra-cc-options=-DBAMBU_SIM_DUMP_OUTPUT",
                "--simulate",
                "--simulator=VERILATOR",
                "--generate-tb=examples/GraphSAGE/graphsage_mean_test.cpp",
                "--top-fname=graphsage_mean",
                "--compiler=I386_CLANG16",
                "--parallel-backend=2",
                "--output-directory=.ci-regression-work/regression-graphsage/output",
                "--no-clean",
            ],
        )
        actual = _actual_arguments(
            REPOSITORY,
            graphsage_spec,
            graphsage_task["configuration"]["invocation"]["arguments"],
        )
        self.assertIn(f"--generate-tb={REPOSITORY / graphsage_spec.test_vector}", actual)
        for task_id in ("regression-graphsage-serial", "regression-graphsage"):
            self.assertIn(f"{task_id}.output-comparison", _artifact_ids(task_id))
        self.assertEqual(
            graphsage_spec.rtl_authenticity_instances,
            REGRESSION_SPECS[5].rtl_authenticity_instances,
        )

    def test_graphsage_mismatch_is_failed_verification(self) -> None:
        failure = _classify_nonzero(
            "irregular mismatch vertex=3 feature=1 expected=0 observed=7\n",
            "regression-graphsage",
            _artifact_ids("regression-graphsage"),
            "Bambu simulation returned '1', not zero.",
        )
        self.assertEqual(failure["code"], "result-mismatch")
        self.assertEqual(failure["stage"], "result-verification")

    def test_missing_graphsage_cpp_testbench_is_invalid_input(self) -> None:
        repository = self.root / "missing-graphsage-testbench"
        spec = REGRESSION_SPECS[7]
        source = repository / spec.source_path
        source.parent.mkdir(parents=True)
        source.write_text('extern "C" void graphsage_mean(const int*, const int*, const int*, int*) {}\n')
        bambu = repository / "panda_dist/bin/bambu"
        bambu.parent.mkdir(parents=True)
        bambu.write_text("#!/bin/sh\nexit 0\n", encoding="utf-8")
        bambu.chmod(0o755)

        task = run_regression(
            repository,
            bambu,
            repository / ".ci-regression-results",
            repository / ".ci-regression-evidence",
            spec,
            "I386_CLANG16",
            2,
            30,
            "/bin/true",
            repository / ".ci-regression-work/.tools/bin",
        )

        self.assertEqual(task["outcome"], "fail")
        self.assertEqual(task["failure"]["code"], "invalid-regression-input")
        self.assertEqual(task["failure"]["stage"], "input-validation")
        self.assertIn("C++ testbench", task["failure"]["message"])

    def test_sparta_authenticity_requires_context_switch_components(self) -> None:
        rtl = self.root / "sparta.v"
        rtl.write_text(
            "kmp_bambu_cs_manager cs_manager (.clock(clock));\n"
            "kmp_bambu_omp_start_cs omp_start_cs (.clock(clock));\n"
            "kmp_bambu_omp_done_cs omp_done_cs (.clock(clock));\n",
            encoding="utf-8",
        )
        instances = REGRESSION_SPECS[5].rtl_authenticity_instances
        self.assertEqual(_missing_rtl_authenticity_instances([rtl], instances), ())
        rtl.write_text(
            "module kmp_bambu_cs_manager; endmodule\n"
            "module kmp_bambu_omp_start_cs; endmodule\n"
            "module kmp_bambu_omp_done_cs; endmodule\n",
            encoding="utf-8",
        )
        self.assertEqual(
            _missing_rtl_authenticity_instances([rtl], instances), instances
        )

    def test_success_without_sparta_authenticity_evidence_fails(self) -> None:
        repository = self.root / "missing-sparta-evidence-repository"
        spec = REGRESSION_SPECS[5]
        source = repository / spec.source_path
        vectors = repository / spec.test_vector
        source.parent.mkdir(parents=True)
        source.write_text("void vector_add(float *a, float *b, float *c) {}\n", encoding="utf-8")
        vectors.parent.mkdir(parents=True, exist_ok=True)
        vectors.write_text("<function/>\n", encoding="utf-8")

        output = repository / ".ci-regression-work" / spec.task_id / "output"
        output.mkdir(parents=True)
        (output / "ordinary.v").write_text(
            "module ordinary_fsmd; endmodule\n", encoding="utf-8"
        )
        (output / "bambu_results.xml").write_text(
            '<bambu_results><application><timing><simulation return_value="0">'
            "<run>42</run></simulation></timing></application></bambu_results>\n",
            encoding="utf-8",
        )

        bambu = repository / "panda_dist/bin/bambu"
        bambu.parent.mkdir(parents=True)
        bambu.write_text(
            '#!/bin/sh\ndate +%s%N > "${PANDA_CI_VERILATOR_MARKER}"\n',
            encoding="utf-8",
        )
        bambu.chmod(0o755)

        task = run_regression(
            repository,
            bambu,
            repository / ".ci-regression-results",
            repository / ".ci-regression-evidence",
            spec,
            "I386_CLANG16",
            2,
            30,
            "/bin/true",
            repository / ".ci-regression-work/.tools/bin",
        )

        self.assertEqual(task["outcome"], "fail")
        self.assertEqual(task["failure"]["code"], "rtl-generation-failed")
        self.assertEqual(task["failure"]["stage"], "rtl-generation")
        inventory = (
            repository
            / ".ci-regression-evidence"
            / spec.task_id
            / "rtl-files.txt"
        ).read_text(encoding="utf-8")
        self.assertIn("authenticity\tkmp_bambu_cs_manager\tcs_manager\tmissing", inventory)

    def test_verilator_timing_wrapper_preserves_tool_root_layout(self) -> None:
        tool_root = self.root / "tool-root"
        real_verilator = tool_root / "bin" / "verilator"
        real_verilator.parent.mkdir(parents=True)
        real_verilator.write_text("#!/bin/sh\n", encoding="utf-8")
        (tool_root / "share" / "verilator").mkdir(parents=True)
        shadow_root = self.root / "shadow-root"
        wrapper_bin = _write_verilator_wrapper(shadow_root, str(real_verilator))
        self.assertEqual(wrapper_bin, shadow_root / "bin")
        self.assertTrue((wrapper_bin / "verilator").is_file())
        self.assertEqual(
            (shadow_root / "share" / "verilator").resolve(),
            (tool_root / "share" / "verilator").resolve(),
        )

    def test_suite_rejects_arbitrary_output_directories_without_deleting_them(self) -> None:
        repository = self.root / "managed-output-repository"
        repository.mkdir()
        for unsafe_name in (".git", "src"):
            with self.subTest(path=unsafe_name):
                unsafe = repository / unsafe_name
                unsafe.mkdir(exist_ok=True)
                sentinel = unsafe / "must-survive"
                sentinel.write_text("preserve me\n", encoding="utf-8")
                with self.assertRaisesRegex(ValueError, "dedicated repository path"):
                    run_regression_suite(
                        repository,
                        repository / "panda_dist/bin/bambu",
                        unsafe,
                        repository / ".ci-regression-evidence",
                    )
                self.assertEqual(sentinel.read_text(encoding="utf-8"), "preserve me\n")

    def test_incomplete_report_does_not_hide_simulator_build_failure(self) -> None:
        failure = _classify_nonzero(
            "%Error: Verilator compilation failed\n",
            "regression-control",
            _artifact_ids("regression-control"),
            "bambu_results.xml contains no simulation result.",
        )
        self.assertEqual(failure["code"], "simulator-build-failed")
        self.assertEqual(failure["stage"], "simulator-preparation")

    def test_success_without_verilator_marker_is_infrastructure_error(self) -> None:
        repository = self.root / "missing-marker-repository"
        spec = REGRESSION_SPECS[1]
        source = repository / spec.source_path
        vectors = repository / spec.test_vector
        source.parent.mkdir(parents=True)
        source.write_text("void duff(void) {}\n", encoding="utf-8")
        vectors.write_text("<function/>\n", encoding="utf-8")
        bambu = repository / "panda_dist/bin/bambu"
        bambu.parent.mkdir(parents=True)
        bambu.write_text("#!/bin/sh\nexit 0\n", encoding="utf-8")
        bambu.chmod(0o755)

        task = run_regression(
            repository,
            bambu,
            repository / ".ci-regression-results",
            repository / ".ci-regression-evidence",
            spec,
            "I386_CLANG16",
            2,
            30,
            "/bin/true",
            repository / ".ci-regression-work/.tools/bin",
        )

        self.assertEqual(task["execution"]["state"], "infrastructure_error")
        self.assertEqual(task["outcome"], "unknown")
        self.assertEqual(task["failure"]["stage"], "simulator-preparation")
        self.assertIsNone(task["results"]["simulation"]["execution_count"])
        self.assertIsNone(task["results"]["simulation"]["total_cycles"])

    def test_unexpected_runner_exception_does_not_fabricate_zero_duration(self) -> None:
        repository = self.root / "runner-exception-repository"
        repository.mkdir()
        with redirect_stdout(io.StringIO()), patch(
            "ci_results.regressions.run_regression",
            side_effect=RuntimeError("fixture runner failure"),
        ):
            suite = run_regression_suite(
                repository,
                repository / "panda_dist/bin/bambu",
                repository / ".ci-regression-results",
                repository / ".ci-regression-evidence",
            )

        self.assertEqual(suite["outcome"], "fail")
        for spec in REGRESSION_SPECS:
            task = load_json(
                repository / ".ci-regression-results/tasks" / f"{spec.task_id}.json"
            )
            metrics = {metric["metric_id"]: metric for metric in task["metrics"]}
            self.assertIsNone(metrics["duration.regression-total"]["value"])
            self.assertEqual(task["execution"]["state"], "infrastructure_error")
            self.assertIsNone(task["execution"]["exit_status"])

    def test_producer_outputs_hosted_results_readable_to_validator_step(self) -> None:
        repository = self.root / "hosted-data-shape-repository"
        repository.mkdir()
        results = repository / ".ci-regression-results"
        evidence = repository / ".ci-regression-evidence"

        def fake_run_regression(*args):
            results_directory, evidence_directory, spec = args[2], args[3], args[4]
            task = regression_task(spec)
            write_json(results_directory / "tasks" / f"{spec.task_id}.json", task)
            if spec.task_id in {"regression-graphsage-serial", "regression-graphsage"}:
                directory = evidence_directory / spec.task_id / "mdpi-output-dumps"
                directory.mkdir(parents=True, exist_ok=True)
                for call in range(1, 5):
                    for parameter, length in enumerate((7, 12, 18, 18)):
                        values = range(parameter * 20, parameter * 20 + length)
                        payload = struct.pack(f"={length}i", *values)
                        for kind in ("gold", "sim"):
                            (directory / f"P{parameter}.{kind}.{call}.dat").write_bytes(payload)
            return task

        with patch("ci_results.regressions.run_regression", side_effect=fake_run_regression):
            suite = run_regression_suite(
                repository,
                repository / "panda_dist/bin/bambu",
                results,
                evidence,
            )

        self.assertEqual(suite["outcome"], "pass")
        produced = [results / "suite.json"] + [
            results / "tasks" / f"{spec.task_id}.json" for spec in REGRESSION_SPECS
        ]
        for path in produced:
            mode = stat.S_IMODE(path.stat().st_mode)
            self.assertTrue(mode & stat.S_IROTH, f"{path} is not world-readable")

    def test_all_regressions_pass_and_summary_is_derived_from_bundle(self) -> None:
        documents = self.generate()
        regression_paths = [
            path
            for path, task in documents.items()
            if path.startswith("tasks/") and task["task_type"] == "regression"
        ]
        self.assertEqual(len(regression_paths), 8)
        self.assertTrue(all(documents[path]["outcome"] == "pass" for path in regression_paths))
        self.assertEqual(documents["verdict.json"]["overall_outcome"], "pass")
        self.assertEqual(
            documents["tasks/open-build.json"]["execution"]["completed_at"],
            "2023-11-14T22:14:20Z",
        )
        build_metrics = {
            metric["metric_id"]: metric["value"]
            for metric in documents["tasks/open-build.json"]["metrics"]
        }
        self.assertEqual(build_metrics["duration.workflow-total"], 60)
        suite = documents["manifest.json"]["hosted_regression_suite"]
        self.assertEqual(
            suite,
            {
                "action_exit_status": 0,
                "action_outcome": "success",
                "completed_at": "2023-11-14T22:14:50Z",
                "container_setup_seconds": 1,
                "duration_measurement_method": "suite-monotonic-clock",
                "duration_seconds": 30,
                "execution_state": "completed",
                "exit_status": 0,
                "failed_count": 0,
                "failure_stage": None,
                "outcome": "pass",
                "passed_count": 8,
                "started_at": "2023-11-14T22:14:20Z",
                "task_count": 8,
            },
        )
        summary = render_bundle_summary(self.bundle)
        self.assertIn("## GitHub-hosted fast regressions", summary)
        self.assertIn("| `regression-callgraph` |", summary)
        self.assertIn("Regression suite elapsed: 30 s", summary)

    def test_synthesis_failure_skips_every_downstream_stage(self) -> None:
        documents = self.generate({"regression-scalar": "hls-synthesis"})
        task = documents["tasks/regression-scalar.json"]
        stages = {stage["stage_id"]: stage for stage in task["stages"]}
        self.assertEqual(stages["hls-synthesis"]["outcome"], "fail")
        for stage_id in STAGE_IDS[2:]:
            self.assertEqual(stages[stage_id]["outcome"], "skipped")
            self.assertIsNone(stages[stage_id]["duration_seconds"])
        self.assertEqual(task["failure"]["code"], "hls-synthesis-failed")

    def test_simulation_failure_is_not_reported_as_verification(self) -> None:
        documents = self.generate({"regression-control": "rtl-simulation"})
        task = documents["tasks/regression-control.json"]
        self.assertFalse(task["results"]["simulation"]["completed"])
        self.assertFalse(task["results"]["simulation"]["verified"])
        self.assertEqual(task["failure"]["code"], "rtl-simulation-failed")
        self.assertEqual(task["stages"][-1]["outcome"], "skipped")

    def test_output_mismatch_preserves_successful_simulation(self) -> None:
        documents = self.generate({"regression-loop-cxx": "result-verification"})
        task = documents["tasks/regression-loop-cxx.json"]
        self.assertTrue(task["results"]["simulation"]["completed"])
        self.assertFalse(task["results"]["simulation"]["verified"])
        self.assertEqual(task["failure"]["code"], "result-mismatch")

    def test_mixed_results_make_hosted_rule_and_overall_verdict_fail(self) -> None:
        documents = self.generate({"regression-callgraph": "rtl-simulation"})
        rules = {rule["rule_id"]: rule for rule in documents["verdict.json"]["rules"]}
        self.assertEqual(rules["hosted-fast-regressions-success"]["outcome"], "fail")
        self.assertEqual(rules["hosted-fast-regressions-success"]["severity"], "blocking")
        self.assertEqual(documents["verdict.json"]["merge_recommendation"], "do-not-merge")
        self.assertEqual(
            documents["manifest.json"]["hosted_regression_suite"]["execution_state"],
            "completed",
        )
        self.assertEqual(
            documents["manifest.json"]["hosted_regression_suite"]["outcome"],
            "fail",
        )

    def test_post_task_action_failure_cannot_report_merge(self) -> None:
        self.write_raw()
        environment = copy.deepcopy(self.environment)
        environment.update(
            {
                "PANDA_CI_REGRESSION_ACTION_EXIT_STATUS": "2",
                "PANDA_CI_REGRESSION_ACTION_OUTCOME": "failure",
                "PANDA_CI_REGRESSION_FAILURE_STAGE": "fast-regressions",
            }
        )
        documents = generate_bundle(
            self.bundle,
            environment=environment,
            repository=REPOSITORY,
            regression_results=self.raw,
        )
        suite = documents["manifest.json"]["hosted_regression_suite"]
        rules = {rule["rule_id"]: rule for rule in documents["verdict.json"]["rules"]}
        self.assertEqual(suite["execution_state"], "infrastructure_error")
        self.assertEqual(suite["outcome"], "unknown")
        self.assertEqual(rules["hosted-fast-regressions-success"]["outcome"], "neutral")
        self.assertEqual(documents["verdict.json"]["merge_recommendation"], "manual-review")

    def test_exported_suite_count_mismatch_is_infrastructure_error(self) -> None:
        self.write_raw()
        environment = copy.deepcopy(self.environment)
        environment["PANDA_CI_REGRESSION_PASSED_COUNT"] = "4"
        documents = generate_bundle(
            self.bundle,
            environment=environment,
            repository=REPOSITORY,
            regression_results=self.raw,
        )
        suite = documents["manifest.json"]["hosted_regression_suite"]
        self.assertEqual(suite["execution_state"], "infrastructure_error")
        self.assertEqual(suite["outcome"], "unknown")

    def test_regression_action_cancellation_is_preserved(self) -> None:
        self.raw.mkdir(parents=True)
        environment = copy.deepcopy(self.environment)
        environment.update(
            {
                "PANDA_CI_JOB_CANCELLED": "true",
                "PANDA_CI_REGRESSION_ACTION_EXIT_STATUS": "",
                "PANDA_CI_REGRESSION_ACTION_COMPLETION_EPOCH": "1700000100",
                "PANDA_CI_REGRESSION_ACTION_OUTCOME": "cancelled",
                "PANDA_CI_REGRESSION_ACTION_START_EPOCH": "1700000060",
                "PANDA_CI_REGRESSION_FAILURE_STAGE": "fast-regressions",
                "PANDA_CI_REGRESSION_TIMEOUT_SECONDS": "2100",
            }
        )
        documents = generate_bundle(
            self.bundle,
            environment=environment,
            repository=REPOSITORY,
            regression_results=self.raw,
        )
        suite = documents["manifest.json"]["hosted_regression_suite"]
        self.assertEqual(suite["execution_state"], "canceled")
        self.assertEqual(suite["outcome"], "unknown")
        self.assertEqual(suite["duration_measurement_method"], "action-wall-clock")
        self.assertEqual(suite["duration_seconds"], 40)
        self.assertEqual(documents["manifest.json"]["execution_state"], "canceled")
        self.assertTrue(
            all(
                documents[f"tasks/{spec.task_id}.json"]["outcome"] == "skipped"
                for spec in REGRESSION_SPECS
            )
        )

    def test_regression_action_timeout_is_preserved(self) -> None:
        self.raw.mkdir(parents=True)
        environment = copy.deepcopy(self.environment)
        environment.update(
            {
                "PANDA_CI_COMPLETION_EPOCH": "1700002200",
                "PANDA_CI_JOB_CANCELLED": "false",
                "PANDA_CI_REGRESSION_ACTION_EXIT_STATUS": "",
                "PANDA_CI_REGRESSION_ACTION_COMPLETION_EPOCH": "1700002200",
                "PANDA_CI_REGRESSION_ACTION_OUTCOME": "failure",
                "PANDA_CI_REGRESSION_ACTION_START_EPOCH": "1700000060",
                "PANDA_CI_REGRESSION_FAILURE_STAGE": "fast-regressions",
                "PANDA_CI_REGRESSION_TIMEOUT_SECONDS": "2100",
            }
        )
        documents = generate_bundle(
            self.bundle,
            environment=environment,
            repository=REPOSITORY,
            regression_results=self.raw,
        )
        suite = documents["manifest.json"]["hosted_regression_suite"]
        self.assertEqual(suite["execution_state"], "timed_out")
        self.assertEqual(suite["outcome"], "unknown")
        self.assertEqual(documents["manifest.json"]["execution_state"], "timed_out")

    def test_timed_out_suite_must_match_action_telemetry(self) -> None:
        self.raw.mkdir(parents=True)
        environment = copy.deepcopy(self.environment)
        environment.update(
            {
                "PANDA_CI_COMPLETION_EPOCH": "1700002200",
                "PANDA_CI_JOB_CANCELLED": "false",
                "PANDA_CI_REGRESSION_ACTION_COMPLETION_EPOCH": "1700002200",
                "PANDA_CI_REGRESSION_ACTION_EXIT_STATUS": "",
                "PANDA_CI_REGRESSION_ACTION_OUTCOME": "failure",
                "PANDA_CI_REGRESSION_ACTION_START_EPOCH": "1700000060",
                "PANDA_CI_REGRESSION_FAILURE_STAGE": "fast-regressions",
                "PANDA_CI_REGRESSION_TIMEOUT_SECONDS": "2100",
            }
        )
        generate_bundle(
            self.bundle,
            environment=environment,
            repository=REPOSITORY,
            regression_results=self.raw,
        )
        manifest = load_json(self.bundle / "manifest.json")
        manifest["hosted_regression_suite"]["action_outcome"] = "success"
        manifest["hosted_regression_suite"]["action_exit_status"] = 0
        write_json(self.bundle / "manifest.json", manifest)
        with self.assertRaises(BundleValidationError) as context:
            validate_bundle(self.bundle)
        self.assertIn(
            "timed-out hosted_regression_suite contradicts action telemetry",
            str(context.exception),
        )

    def test_runner_only_duration_fallback_has_no_action_timestamps(self) -> None:
        self.raw.mkdir(parents=True)
        environment = copy.deepcopy(self.environment)
        environment.update(
            {
                "PANDA_CI_REGRESSION_ACTION_EXIT_STATUS": "1",
                "PANDA_CI_REGRESSION_ACTION_OUTCOME": "failure",
                "PANDA_CI_REGRESSION_ACTION_START_EPOCH": "",
                "PANDA_CI_REGRESSION_FAILURE_STAGE": "fast-regressions",
                "PANDA_CI_REGRESSION_SECONDS": "5",
            }
        )
        documents = generate_bundle(
            self.bundle,
            environment=environment,
            repository=REPOSITORY,
            regression_results=self.raw,
        )
        suite = documents["manifest.json"]["hosted_regression_suite"]
        self.assertEqual(suite["duration_measurement_method"], "runner-wall-clock")
        self.assertEqual(suite["duration_seconds"], 5)
        self.assertIsNone(suite["started_at"])
        self.assertIsNone(suite["completed_at"])

    def test_broken_artifact_reference_is_rejected(self) -> None:
        self.generate()
        task_path = "tasks/regression-memory-interface.json"
        task = load_json(self.bundle / task_path)
        task["artifacts"][0] = "regression-memory-interface.missing"
        write_json(self.bundle / task_path, task)
        self.rehash(task_path)
        with self.assertRaises(BundleValidationError) as context:
            validate_bundle(self.bundle)
        self.assertIn("unknown artifact", str(context.exception))

    def test_passing_simulation_requires_observations(self) -> None:
        self.generate()
        task_path = "tasks/regression-control.json"
        task = load_json(self.bundle / task_path)
        task["results"]["simulation"]["execution_count"] = None
        write_json(self.bundle / task_path, task)
        self.rehash(task_path)
        with self.assertRaises(BundleValidationError) as context:
            validate_bundle(self.bundle)
        self.assertIn("passing simulation requires an execution count", str(context.exception))

    def test_available_regression_artifact_requires_metadata(self) -> None:
        self.generate()
        artifacts = load_json(self.bundle / "artifacts.json")
        artifact = next(
            item
            for item in artifacts["artifacts"]
            if item["artifact_id"] == "regression-control.result-report"
        )
        artifact["sha256"] = None
        write_json(self.bundle / "artifacts.json", artifacts)
        self.rehash("artifacts.json")
        with self.assertRaises(BundleValidationError) as context:
            validate_bundle(self.bundle)
        self.assertIn("available evidence requires SHA-256", str(context.exception))

    def test_passing_regression_requires_synthesis_and_simulation_timings(self) -> None:
        self.generate()
        task_path = "tasks/regression-scalar.json"
        task = load_json(self.bundle / task_path)
        stages = {stage["stage_id"]: stage for stage in task["stages"]}
        metrics = {metric["metric_id"]: metric for metric in task["metrics"]}
        stages["hls-synthesis"]["duration_seconds"] = None
        metrics["duration.hls-synthesis"]["value"] = None
        write_json(self.bundle / task_path, task)
        self.rehash(task_path)
        with self.assertRaises(BundleValidationError) as context:
            validate_bundle(self.bundle)
        self.assertIn("passing regression requires 'duration.hls-synthesis'", str(context.exception))

    def test_serialization_is_deterministic(self) -> None:
        first = self.root / "bundle-first"
        second = self.root / "bundle-second"
        self.generate(output=first)
        generate_bundle(
            second,
            environment=copy.deepcopy(self.environment),
            repository=REPOSITORY,
            regression_results=self.raw,
        )
        first_documents = {
            path.relative_to(first).as_posix(): canonical_text(load_json(path))
            for path in first.rglob("*.json")
        }
        second_documents = {
            path.relative_to(second).as_posix(): canonical_text(load_json(path))
            for path in second.rglob("*.json")
        }
        self.assertEqual(first_documents, second_documents)


if __name__ == "__main__":
    unittest.main()
