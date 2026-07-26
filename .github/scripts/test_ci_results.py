#!/usr/bin/env python3
"""Focused tests for the PandA CI result protocol."""

from __future__ import annotations

import copy
import sys
import tempfile
import unittest
from pathlib import Path


SCRIPTS_DIRECTORY = Path(__file__).resolve().parent
if str(SCRIPTS_DIRECTORY) not in sys.path:
    sys.path.insert(0, str(SCRIPTS_DIRECTORY))

from ci_results.bundle import BundleValidationError, validate_bundle  # noqa: E402
from ci_results.constants import DOCUMENT_PATHS, STAGE_IDS  # noqa: E402
from ci_results.generate import generate_bundle  # noqa: E402
from ci_results.hashing import sha256_file  # noqa: E402
from ci_results.serialization import canonical_text, load_json, write_json  # noqa: E402
from ci_results.summary import render_bundle_summary  # noqa: E402


REPOSITORY = Path(__file__).resolve().parents[2]


def successful_environment() -> dict[str, str]:
    return {
        "PANDA_CI_ACTION_EXIT_STATUS": "0",
        "PANDA_CI_ACTION_START_EPOCH": "1700000000",
        "PANDA_CI_BAMBU_CC_EXISTS": "true",
        "PANDA_CI_BAMBU_CC_STARTS": "true",
        "PANDA_CI_BAMBU_EXISTS": "true",
        "PANDA_CI_BAMBU_STARTS": "true",
        "PANDA_CI_BASE_SHA": "b" * 40,
        "PANDA_CI_BUILD_OUTCOME": "success",
        "PANDA_CI_BUILD_SECONDS": "30",
        "PANDA_CI_BUILD_TIMEOUT_SECONDS": "6600",
        "PANDA_CI_CACHE_HIT": "false",
        "PANDA_CI_CACHE_MATCHED_KEY": "",
        "PANDA_CI_CACHE_PRIMARY_KEY": "panda-cache-test",
        "PANDA_CI_CACHE_RESTORE_OUTCOME": "success",
        "PANDA_CI_CACHE_SAVE_OUTCOME": "success",
        "PANDA_CI_CCACHE_CACHEABLE_CALLS": "100",
        "PANDA_CI_CCACHE_HIT_RATE": "25.0",
        "PANDA_CI_CCACHE_HITS": "25",
        "PANDA_CI_CCACHE_MISSES": "75",
        "PANDA_CI_CCACHE_SIZE_KIB": "4096",
        "PANDA_CI_CCACHE_VERSION": "ccache version 4.7.5",
        "PANDA_CI_CLANGXX_VERSION": "Debian clang version 16.0.6",
        "PANDA_CI_CLANG_VERSION": "Debian clang version 16.0.6",
        "PANDA_CI_CMAKE_VERSION": "cmake version 3.25.1",
        "PANDA_CI_COMMIT_SHA": "a" * 40,
        "PANDA_CI_COMPLETED_AT": "2023-11-14T22:15:00Z",
        "PANDA_CI_COMPLETION_EPOCH": "1700000100",
        "PANDA_CI_CONFIGURE_EXIT_STATUS": "0",
        "PANDA_CI_CONFIGURE_SECONDS": "2",
        "PANDA_CI_CONTAINER_SETUP_SECONDS": "10",
        "PANDA_CI_COSIMULATION_EXIT_STATUS": "0",
        "PANDA_CI_COSIMULATION_SECONDS": "5",
        "PANDA_CI_EUCALYPTUS_EXISTS": "true",
        "PANDA_CI_EUCALYPTUS_STARTS": "true",
        "PANDA_CI_EVENT_TYPE": "pull_request",
        "PANDA_CI_FAILURE_STAGE": "none",
        "PANDA_CI_FRONTEND_RESOLUTION_EXIT_STATUS": "0",
        "PANDA_CI_FRONTEND_RESOLUTION_SECONDS": "1",
        "PANDA_CI_GCC_VERSION": "gcc (Debian 12.2.0) 12.2.0",
        "PANDA_CI_GXX_VERSION": "g++ (Debian 12.2.0) 12.2.0",
        "PANDA_CI_INSTALLATION_EXIT_STATUS": "0",
        "PANDA_CI_INSTALL_SECONDS": "3",
        "PANDA_CI_JOB_CANCELLED": "false",
        "PANDA_CI_KILL_DETECTED": "no",
        "PANDA_CI_LLVM_VERSION": "16.0.6",
        "PANDA_CI_MEMORY_AVAILABLE_AFTER_KIB": "12000000",
        "PANDA_CI_MEMORY_AVAILABLE_BEFORE_KIB": "14000000",
        "PANDA_CI_OOM_DETECTED": "no",
        "PANDA_CI_PARALLELISM": "2",
        "PANDA_CI_PEAK_BUILD_CGROUP_KIB": "3000000",
        "PANDA_CI_PEAK_BUILD_RSS_KIB": "2800000",
        "PANDA_CI_PLUGIN_BUILD_EXIT_STATUS": "0",
        "PANDA_CI_PLUGIN_BUILD_SECONDS": "8",
        "PANDA_CI_PROJECT_BUILD_EXIT_STATUS": "0",
        "PANDA_CI_PROJECT_BUILD_SECONDS": "20",
        "PANDA_CI_PULL_REQUEST_NUMBER": "6",
        "PANDA_CI_REF": "refs/pull/6/merge",
        "PANDA_CI_REPOSITORY": "Antonyt80/PandA-bambu",
        "PANDA_CI_REQUEST_COMMIT_SHA": "d" * 40,
        "PANDA_CI_REQUESTED_FRONTEND": "I386_CLANG16",
        "PANDA_CI_RESULT_ARTIFACT_NAME": "panda-ci-results-12345-attempt-1",
        "PANDA_CI_RUN_ATTEMPT": "1",
        "PANDA_CI_RUNNER_ARCH": "X64",
        "PANDA_CI_RUNNER_IMAGE": "ubuntu-24.04",
        "PANDA_CI_RUNNER_OS": "Linux",
        "PANDA_CI_SELECTED_FRONTEND": "I386_CLANG16",
        "PANDA_CI_VERILATOR_VERSION": "Verilator 5.006",
        "PANDA_CI_VERIFY_EXIT_STATUS": "0",
        "PANDA_CI_VERIFY_OUTCOME": "success",
        "PANDA_CI_VERIFY_SECONDS": "2",
        "PANDA_CI_WORKFLOW_IMPLEMENTATION_COMMIT": "c" * 40,
        "PANDA_CI_WORKFLOW_RUN_ID": "12345",
        "PANDA_CI_WORKFLOW_START_EPOCH": "1700000000",
    }


class CIResultProtocolTests(unittest.TestCase):
    def setUp(self) -> None:
        self.temporary_directory = tempfile.TemporaryDirectory()
        self.root = Path(self.temporary_directory.name)
        self.bundle = self.root / ".ci-results"
        self.environment = successful_environment()

    def tearDown(self) -> None:
        self.temporary_directory.cleanup()

    def generate(self, environment: dict[str, str] | None = None) -> dict[str, dict]:
        return generate_bundle(
            self.bundle,
            environment=environment or self.environment,
            repository=REPOSITORY,
        )

    def task(self) -> dict:
        return load_json(self.bundle / "tasks/open-build.json")

    def rewrite(self, relative_path: str, value: dict) -> None:
        write_json(self.bundle / relative_path, value)

    def rehash(self, relative_path: str) -> None:
        manifest = load_json(self.bundle / "manifest.json")
        reference = next(
            item for item in manifest["documents"] if item["path"] == relative_path
        )
        reference["sha256"] = sha256_file(self.bundle / relative_path)
        self.rewrite("manifest.json", manifest)

    def assert_invalid(self, expected: str) -> None:
        with self.assertRaises(BundleValidationError) as context:
            validate_bundle(self.bundle)
        self.assertIn(expected, str(context.exception))

    def test_successful_complete_run_and_summary(self) -> None:
        documents = self.generate()
        task = documents["tasks/open-build.json"]
        self.assertEqual(task["outcome"], "pass")
        self.assertTrue(all(stage["outcome"] == "pass" for stage in task["stages"]))
        self.assertEqual(documents["verdict.json"]["merge_recommendation"], "merge")
        summary = render_bundle_summary(self.bundle)
        self.assertIn("## Open build performance", summary)
        self.assertIn("cache_restore=miss", summary)
        self.assertIn("Merge recommendation: `merge`", summary)

    def test_request_target_and_executed_commit_are_distinct(self) -> None:
        documents = self.generate()
        self.assertEqual(documents["request.json"]["commit_sha"], "d" * 40)
        self.assertEqual(documents["manifest.json"]["commit_sha"], "a" * 40)

    def test_observed_frontend_and_ccache_version_are_recorded(self) -> None:
        environment = successful_environment()
        environment["PANDA_CI_SELECTED_FRONTEND"] = "I386_CLANG17"
        documents = self.generate(environment)
        self.assertEqual(
            documents["request.json"]["build_parameters"]["selected_frontend"],
            "I386_CLANG16",
        )
        self.assertEqual(
            documents["tasks/open-build.json"]["configuration"]["selected_frontend"],
            "I386_CLANG17",
        )
        tools = {
            item["tool_id"]: item["version"]
            for item in documents["manifest.json"]["tools"]
        }
        self.assertEqual(tools["ccache"], "ccache version 4.7.5")

    def test_compilation_failure(self) -> None:
        environment = successful_environment()
        environment.update(
            {
                "PANDA_CI_ACTION_EXIT_STATUS": "2",
                "PANDA_CI_BUILD_OUTCOME": "failure",
                "PANDA_CI_FAILURE_STAGE": "project-build",
                "PANDA_CI_PROJECT_BUILD_EXIT_STATUS": "2",
                "PANDA_CI_VERIFY_OUTCOME": "skipped",
                "PANDA_CI_VERIFY_EXIT_STATUS": "",
                "PANDA_CI_VERIFY_SECONDS": "",
            }
        )
        documents = self.generate(environment)
        task = documents["tasks/open-build.json"]
        project = next(stage for stage in task["stages"] if stage["stage_id"] == "project-build")
        self.assertEqual(project["failure"]["category"], "compilation")
        self.assertEqual(project["failure"]["code"], "compiler-exit-nonzero")
        self.assertEqual(task["outcome"], "fail")

    def test_configure_failure_keeps_unavailable_observations_null(self) -> None:
        environment = successful_environment()
        environment.update(
            {
                "PANDA_CI_ACTION_EXIT_STATUS": "1",
                "PANDA_CI_BUILD_OUTCOME": "failure",
                "PANDA_CI_CONFIGURE_EXIT_STATUS": "1",
                "PANDA_CI_FAILURE_STAGE": "configure",
                "PANDA_CI_KILL_DETECTED": "unknown",
                "PANDA_CI_MEMORY_AVAILABLE_AFTER_KIB": "",
                "PANDA_CI_MEMORY_AVAILABLE_BEFORE_KIB": "",
                "PANDA_CI_OOM_DETECTED": "unknown",
                "PANDA_CI_PEAK_BUILD_CGROUP_KIB": "",
                "PANDA_CI_PEAK_BUILD_RSS_KIB": "",
                "PANDA_CI_SELECTED_FRONTEND": "",
                "PANDA_CI_VERIFY_OUTCOME": "skipped",
            }
        )
        documents = self.generate(environment)
        task = documents["tasks/open-build.json"]
        self.assertIsNone(task["configuration"]["selected_frontend"])
        metrics = {item["metric_id"]: item["value"] for item in task["metrics"]}
        for metric_id in (
            "memory.build.peak-cgroup-kib",
            "memory.build.peak-aggregate-rss-kib",
            "memory.build.available-before-kib",
            "memory.build.available-after-kib",
            "memory.build.oom-detected",
            "memory.build.kill-detected",
        ):
            self.assertIsNone(metrics[metric_id])

    def test_skipped_downstream_stages_use_nulls(self) -> None:
        environment = successful_environment()
        environment.update(
            {
                "PANDA_CI_ACTION_EXIT_STATUS": "2",
                "PANDA_CI_BUILD_OUTCOME": "failure",
                "PANDA_CI_FAILURE_STAGE": "plugin-build",
                "PANDA_CI_PLUGIN_BUILD_EXIT_STATUS": "2",
                "PANDA_CI_VERIFY_OUTCOME": "skipped",
            }
        )
        documents = self.generate(environment)
        task = documents["tasks/open-build.json"]
        for stage_id in (
            "project-build",
            "installation",
            "installed-executable-validation",
            "xml-verilator-cosimulation",
        ):
            stage = next(item for item in task["stages"] if item["stage_id"] == stage_id)
            self.assertEqual(stage["outcome"], "skipped")
            self.assertIsNone(stage["duration_seconds"])
            self.assertIsNone(stage["exit_status"])
            self.assertIsNone(stage["failure"])

    def test_executable_validation_failure(self) -> None:
        environment = successful_environment()
        environment.update(
            {
                "PANDA_CI_BAMBU_EXISTS": "false",
                "PANDA_CI_BAMBU_STARTS": "unknown",
                "PANDA_CI_VERIFY_EXIT_STATUS": "1",
                "PANDA_CI_VERIFY_OUTCOME": "failure",
            }
        )
        documents = self.generate(environment)
        task = documents["tasks/open-build.json"]
        check = task["checks"][0]
        self.assertEqual(check["outcome"], "fail")
        self.assertEqual(check["failure"]["code"], "executable-missing")
        self.assertEqual(task["outcome"], "fail")

    def test_successful_verification_requires_detailed_outputs(self) -> None:
        environment = successful_environment()
        environment["PANDA_CI_BAMBU_STARTS"] = ""
        with self.assertRaises(BundleValidationError) as context:
            self.generate(environment)
        self.assertIn(
            "passing installed executable stage requires every check to pass",
            str(context.exception),
        )

    def test_cosimulation_failure(self) -> None:
        environment = successful_environment()
        environment.update(
            {
                "PANDA_CI_ACTION_EXIT_STATUS": "1",
                "PANDA_CI_BUILD_OUTCOME": "failure",
                "PANDA_CI_COSIMULATION_EXIT_STATUS": "1",
                "PANDA_CI_FAILURE_STAGE": "cosimulation",
                "PANDA_CI_VERIFY_OUTCOME": "skipped",
            }
        )
        documents = self.generate(environment)
        task = documents["tasks/open-build.json"]
        check = next(
            item for item in task["checks"] if item["check_id"] == "xml-verilator-cosimulation"
        )
        self.assertEqual(check["outcome"], "fail")
        self.assertEqual(check["failure"]["code"], "cosimulation-exit-nonzero")

    def test_infrastructure_failure(self) -> None:
        environment = successful_environment()
        for key in (
            "PANDA_CI_ACTION_EXIT_STATUS",
            "PANDA_CI_CONTAINER_SETUP_SECONDS",
            "PANDA_CI_FAILURE_STAGE",
        ):
            environment[key] = ""
        environment["PANDA_CI_BUILD_OUTCOME"] = "failure"
        environment["PANDA_CI_SELECTED_FRONTEND"] = ""
        environment["PANDA_CI_VERIFY_OUTCOME"] = "skipped"
        documents = self.generate(environment)
        task = documents["tasks/open-build.json"]
        self.assertEqual(task["execution"]["state"], "infrastructure_error")
        self.assertEqual(task["outcome"], "unknown")
        self.assertEqual(task["failure"]["category"], "infrastructure")

    def test_failed_stage_never_reports_zero_exit_status(self) -> None:
        environment = successful_environment()
        environment.update(
            {
                "PANDA_CI_ACTION_EXIT_STATUS": "0",
                "PANDA_CI_BUILD_OUTCOME": "failure",
                "PANDA_CI_FAILURE_STAGE": "project-build",
                "PANDA_CI_PROJECT_BUILD_EXIT_STATUS": "0",
                "PANDA_CI_VERIFY_OUTCOME": "skipped",
            }
        )
        documents = self.generate(environment)
        task = documents["tasks/open-build.json"]
        project = next(stage for stage in task["stages"] if stage["stage_id"] == "project-build")
        self.assertEqual(project["outcome"], "fail")
        self.assertIsNone(project["exit_status"])
        self.assertIsNone(task["execution"]["exit_status"])

    def test_step_timeout_preserves_completed_stage_evidence(self) -> None:
        environment = successful_environment()
        environment.update(
            {
                "PANDA_CI_ACTION_EXIT_STATUS": "124",
                "PANDA_CI_BUILD_OUTCOME": "failure",
                "PANDA_CI_COMPLETION_EPOCH": "1700006700",
                "PANDA_CI_FAILURE_STAGE": "project-build",
                "PANDA_CI_PROJECT_BUILD_EXIT_STATUS": "",
                "PANDA_CI_VERIFY_OUTCOME": "skipped",
            }
        )
        documents = self.generate(environment)
        task = documents["tasks/open-build.json"]
        stages = {stage["stage_id"]: stage for stage in task["stages"]}
        self.assertEqual(documents["manifest.json"]["execution_state"], "timed_out")
        self.assertEqual(stages["plugin-build"]["outcome"], "pass")
        self.assertEqual(stages["project-build"]["execution_state"], "timed_out")
        self.assertEqual(stages["project-build"]["exit_status"], 124)
        self.assertEqual(stages["installation"]["outcome"], "skipped")
        environment["PANDA_CI_BUILD_OUTCOME"] = "cancelled"
        documents = self.generate(environment)
        self.assertEqual(documents["manifest.json"]["execution_state"], "timed_out")

    def test_cancellation_before_build_is_not_infrastructure_failure(self) -> None:
        environment = successful_environment()
        environment.update(
            {
                "PANDA_CI_ACTION_EXIT_STATUS": "",
                "PANDA_CI_BUILD_OUTCOME": "skipped",
                "PANDA_CI_CONTAINER_SETUP_SECONDS": "",
                "PANDA_CI_FAILURE_STAGE": "",
                "PANDA_CI_JOB_CANCELLED": "true",
                "PANDA_CI_SELECTED_FRONTEND": "",
                "PANDA_CI_VERIFY_OUTCOME": "skipped",
            }
        )
        documents = self.generate(environment)
        task = documents["tasks/open-build.json"]
        self.assertEqual(task["execution"]["state"], "canceled")
        self.assertEqual(documents["manifest.json"]["execution_state"], "canceled")

    def test_cancellation_during_executable_verification_is_preserved(self) -> None:
        environment = copy.deepcopy(self.environment)
        environment.update(
            {
                "PANDA_CI_JOB_CANCELLED": "true",
                "PANDA_CI_VERIFY_EXIT_STATUS": "",
                "PANDA_CI_VERIFY_OUTCOME": "cancelled",
                "PANDA_CI_VERIFY_SECONDS": "",
            }
        )
        documents = self.generate(environment)
        task = documents["tasks/open-build.json"]
        stages = {stage["stage_id"]: stage for stage in task["stages"]}
        self.assertEqual(task["execution"]["state"], "canceled")
        self.assertEqual(task["outcome"], "unknown")
        self.assertEqual(task["failure"]["stage"], "installed-executable-validation")
        self.assertEqual(
            stages["installed-executable-validation"]["execution_state"], "canceled"
        )
        self.assertEqual(
            stages["installed-executable-validation"]["outcome"], "unknown"
        )
        self.assertEqual(documents["manifest.json"]["execution_state"], "canceled")
        diagnostics = documents["artifacts.json"]["artifacts"][1:]
        self.assertTrue(all(item["github_artifact_name"] is None for item in diagnostics))

    def test_unavailable_metrics_are_null(self) -> None:
        environment = successful_environment()
        for key in list(environment):
            if any(
                token in key
                for token in (
                    "_SECONDS",
                    "_KIB",
                    "_CALLS",
                    "_HITS",
                    "_MISSES",
                    "_HIT_RATE",
                    "OOM_DETECTED",
                    "KILL_DETECTED",
                )
            ):
                environment[key] = ""
        documents = self.generate(environment)
        task = documents["tasks/open-build.json"]
        observed = {
            item["metric_id"]: item["value"]
            for item in task["metrics"]
            if item["metric_id"].startswith(("memory.", "ccache."))
        }
        self.assertTrue(observed)
        self.assertTrue(all(value is None for value in observed.values()))

    def test_rejects_numeric_string(self) -> None:
        self.generate()
        task = self.task()
        task["metrics"][0]["value"] = "10"
        self.rewrite("tasks/open-build.json", task)
        self.assert_invalid("expected type number, null")

    def test_rejects_negative_duration(self) -> None:
        self.generate()
        task = self.task()
        task["stages"][0]["duration_seconds"] = -1
        task["metrics"][0]["value"] = -1
        self.rewrite("tasks/open-build.json", task)
        self.assert_invalid("value must be at least 0")

    def test_rejects_unknown_required_enum(self) -> None:
        self.generate()
        task = self.task()
        task["stages"][0]["outcome"] = "surprise"
        self.rewrite("tasks/open-build.json", task)
        self.assert_invalid("allowed enum")

    def test_rejects_unsupported_major_schema_version(self) -> None:
        self.generate()
        request = load_json(self.bundle / "request.json")
        request["schema_version"] = "2.0"
        self.rewrite("request.json", request)
        self.assert_invalid("unsupported major schema version")

    def test_deterministic_serialization(self) -> None:
        value = {"z": [3, 2, 1], "a": "µ", "nested": {"b": 2, "a": 1}}
        first = canonical_text(value)
        second = canonical_text(copy.deepcopy(value))
        self.assertEqual(first, second)
        self.assertTrue(first.startswith('{\n  "a"'))
        self.assertIn("µ", first)
        self.assertTrue(first.endswith("\n"))

    def test_regeneration_keeps_bundle_metadata_non_recursive(self) -> None:
        self.generate()
        (self.bundle / "extra.json").write_text("{}\n", encoding="utf-8")
        documents = self.generate()
        bundle_artifact = documents["artifacts.json"]["artifacts"][0]
        self.assertEqual(bundle_artifact["artifact_id"], "structured-result-bundle")
        self.assertTrue(bundle_artifact["available"])
        self.assertIsNone(bundle_artifact["size_bytes"])
        self.assertIsNone(bundle_artifact["sha256"])
        self.assertFalse((self.bundle / "extra.json").exists())

    def test_rejects_requested_artifact_that_does_not_resolve(self) -> None:
        self.generate()
        request = load_json(self.bundle / "request.json")
        request["requested_artifact_ids"][0] = "missing-artifact"
        self.rewrite("request.json", request)
        self.rehash("request.json")
        self.assert_invalid("requested_artifact_ids must contain every v1 artifact")

    def test_rejects_metric_contract_mismatch(self) -> None:
        self.generate()
        task = self.task()
        task["metrics"][0]["unit"] = "kibibytes"
        self.rewrite("tasks/open-build.json", task)
        self.rehash("tasks/open-build.json")
        self.assert_invalid("expected unit/aggregation/scope")

    def test_rejects_task_pass_that_contradicts_failed_stage(self) -> None:
        environment = successful_environment()
        environment.update(
            {
                "PANDA_CI_ACTION_EXIT_STATUS": "2",
                "PANDA_CI_BUILD_OUTCOME": "failure",
                "PANDA_CI_FAILURE_STAGE": "project-build",
                "PANDA_CI_PROJECT_BUILD_EXIT_STATUS": "2",
                "PANDA_CI_VERIFY_OUTCOME": "skipped",
            }
        )
        self.generate(environment)
        task = self.task()
        task["execution"]["exit_status"] = 0
        task["failure"] = None
        task["outcome"] = "pass"
        self.rewrite("tasks/open-build.json", task)
        self.rehash("tasks/open-build.json")
        self.assert_invalid("passing open-build task requires every stage to pass")

    def test_rejects_executable_check_that_contradicts_details(self) -> None:
        self.generate()
        task = self.task()
        task["checks"][0]["details"]["exists"] = False
        self.rewrite("tasks/open-build.json", task)
        self.rehash("tasks/open-build.json")
        self.assert_invalid("outcome contradicts executable details")

    def test_rejects_schema_check_that_claims_failure(self) -> None:
        self.generate()
        task = self.task()
        task["checks"][-1].update(
            {
                "failure": {
                    "category": "schema",
                    "code": "schema-validation-failed",
                    "evidence": [],
                    "message": "The bundle did not validate.",
                    "retryable": False,
                    "stage": None,
                },
                "outcome": "fail",
            }
        )
        self.rewrite("tasks/open-build.json", task)
        self.rehash("tasks/open-build.json")
        self.assert_invalid("must confirm this validated bundle")

    def test_rejects_manifest_task_state_mismatch(self) -> None:
        self.generate()
        manifest = load_json(self.bundle / "manifest.json")
        manifest["execution_state"] = "canceled"
        self.rewrite("manifest.json", manifest)
        self.assert_invalid("manifest execution_state does not match task")

    def test_missing_required_documents(self) -> None:
        for relative_path in ("manifest.json",) + DOCUMENT_PATHS:
            with self.subTest(relative_path=relative_path):
                self.bundle = self.root / relative_path.replace("/", "-")
                self.generate()
                (self.bundle / relative_path).unlink()
                self.assert_invalid("required document is missing")

    def test_broken_document_reference(self) -> None:
        self.generate()
        manifest = load_json(self.bundle / "manifest.json")
        manifest["documents"][0]["path"] = "missing-artifacts.json"
        self.rewrite("manifest.json", manifest)
        self.assert_invalid("broken path reference")

    def test_broken_document_hash(self) -> None:
        self.generate()
        manifest = load_json(self.bundle / "manifest.json")
        manifest["documents"][0]["sha256"] = "0" * 64
        self.rewrite("manifest.json", manifest)
        self.assert_invalid("SHA-256 mismatch")

    def test_broken_artifact_reference(self) -> None:
        self.generate()
        task = self.task()
        task["stages"][1]["artifact_ids"].append("missing-artifact")
        self.rewrite("tasks/open-build.json", task)
        self.assert_invalid("unknown artifact reference")

    def test_manifest_hashes_canonical_documents(self) -> None:
        self.generate()
        manifest = load_json(self.bundle / "manifest.json")
        references = {item["path"]: item["sha256"] for item in manifest["documents"]}
        for relative_path, expected in references.items():
            self.assertEqual(sha256_file(self.bundle / relative_path), expected)

    def test_all_required_stage_ids_are_present(self) -> None:
        self.generate()
        self.assertEqual(
            [item["stage_id"] for item in self.task()["stages"]],
            list(STAGE_IDS),
        )


if __name__ == "__main__":
    unittest.main()
