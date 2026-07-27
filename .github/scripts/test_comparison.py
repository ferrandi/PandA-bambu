#!/usr/bin/env python3
"""Focused tests for deterministic v1.1 candidate-versus-baseline comparison."""

from __future__ import annotations

import copy
import subprocess
import tempfile
import unittest
from pathlib import Path

from ci_results.comparison import (
    ComparisonError,
    ComparisonInputError,
    _policy_reasons,
    _task_record,
    compare_bundles,
    render_comparison_file,
    validate_comparison,
)
from ci_results.__main__ import main
from ci_results.generate import generate_bundle
from ci_results.hashing import sha256_file
from ci_results.regressions import REGRESSION_SPECS
from ci_results.serialization import load_json, write_json
from test_ci_results import successful_environment
from test_fast_regressions import regression_task

REPOSITORY = Path(__file__).resolve().parents[2]
TASK_ID = "regression-scalar"


class ComparisonTests(unittest.TestCase):
    def setUp(self) -> None:
        self.temporary_directory = tempfile.TemporaryDirectory(
            prefix=".ci-comparison-fixture-", dir=REPOSITORY
        )
        self.root = Path(self.temporary_directory.name)
        self.baseline = self.root / "baseline"
        self.candidate = self.root / "candidate"
        self.output = self.root / "comparison.json"
        self._generate(self.baseline, "a" * 40)
        self._generate(self.candidate, "b" * 40)

    def tearDown(self) -> None:
        self.temporary_directory.cleanup()

    def _generate(self, output: Path, commit: str, failures=None, environment_updates=None) -> None:
        raw = self.root / f"raw-{output.name}"
        evidence = self.root / f"evidence-{output.name}"
        failures = failures or {}
        for spec in REGRESSION_SPECS:
            write_json(raw / "tasks" / f"{spec.task_id}.json", regression_task(spec, failures.get(spec.task_id)))
            task_evidence = evidence / spec.task_id
            task_evidence.mkdir(parents=True, exist_ok=True)
            for name in ("bambu.log", "bambu_results.xml", "rtl-files.txt", "simulation.log"):
                (task_evidence / name).write_text("fixture\n", encoding="utf-8")
        passed = len(REGRESSION_SPECS) - len(failures)
        write_json(raw / "suite.json", {
            "completed_at": "2023-11-14T22:14:50Z", "duration_seconds": 30,
            "exit_status": 1 if failures else 0, "failed_count": len(failures),
            "outcome": "fail" if failures else "pass", "passed_count": passed,
            "started_at": "2023-11-14T22:14:20Z", "task_count": len(REGRESSION_SPECS),
            "task_ids": [spec.task_id for spec in REGRESSION_SPECS],
        })
        environment = successful_environment()
        environment.update({
            "PANDA_CI_COMMIT_SHA": commit,
            "PANDA_CI_REGRESSION_ARTIFACT_NAME": f"evidence-{output.name}",
            "PANDA_CI_REGRESSION_EVIDENCE_DIR": evidence.relative_to(REPOSITORY).as_posix(),
            "PANDA_CI_OPEN_BUILD_COMPLETION_EPOCH": "1700000060",
            "PANDA_CI_WORKFLOW_FILE": ".github/workflows/fast-regressions-hosted.yml",
            "PANDA_CI_REGRESSION_ACTION_EXIT_STATUS": "1" if failures else "0",
            "PANDA_CI_REGRESSION_ACTION_COMPLETION_EPOCH": "1700000090",
            "PANDA_CI_REGRESSION_ACTION_OUTCOME": "failure" if failures else "success",
            "PANDA_CI_REGRESSION_ACTION_START_EPOCH": "1700000060",
            "PANDA_CI_REGRESSION_CONTAINER_SETUP_SECONDS": "1",
            "PANDA_CI_REGRESSION_FAILED_COUNT": str(len(failures)),
            "PANDA_CI_REGRESSION_FAILURE_STAGE": "fast-regressions" if failures else "none",
            "PANDA_CI_REGRESSION_OUTCOME": "fail" if failures else "pass",
            "PANDA_CI_REGRESSION_PASSED_COUNT": str(passed),
            "PANDA_CI_REGRESSION_SECONDS": "30",
            "PANDA_CI_REGRESSION_TASK_COUNT": str(len(REGRESSION_SPECS)),
            "PANDA_CI_REGRESSION_TIMEOUT_SECONDS": "2100",
        })
        environment.update(environment_updates or {})
        generate_bundle(output, environment=environment, repository=REPOSITORY, regression_results=raw)

    def _rehash(self, bundle: Path, relative: str) -> None:
        manifest = load_json(bundle / "manifest.json")
        next(item for item in manifest["documents"] if item["path"] == relative)["sha256"] = sha256_file(bundle / relative)
        write_json(bundle / "manifest.json", manifest)

    def _mutate_manifest(self, bundle: Path, mutation) -> None:
        manifest = load_json(bundle / "manifest.json")
        mutation(manifest)
        write_json(bundle / "manifest.json", manifest)

    def _mutate_open_build(self, bundle: Path, mutation) -> None:
        relative = "tasks/open-build.json"
        task = load_json(bundle / relative)
        mutation(task)
        write_json(bundle / relative, task)
        self._rehash(bundle, relative)

    def _mutate_request(self, bundle: Path, mutation) -> None:
        request = load_json(bundle / "request.json")
        mutation(request)
        write_json(bundle / "request.json", request)
        self._rehash(bundle, "request.json")

    def _mutate_task(self, bundle: Path, mutation, task_id: str = TASK_ID) -> None:
        relative = f"tasks/{task_id}.json"
        task = load_json(bundle / relative)
        mutation(task)
        write_json(bundle / relative, task)
        request = load_json(bundle / "request.json")
        requested = next(item for item in request["tasks"] if item["task_id"] == task_id)
        requested["configuration"] = copy.deepcopy(task["configuration"])
        requested["configuration"]["frontend"].pop("selected", None)
        write_json(bundle / "request.json", request)
        self._rehash(bundle, relative)
        self._rehash(bundle, "request.json")

    def _compare(self):
        return compare_bundles(self.baseline, self.candidate, self.output)

    @staticmethod
    def _task(result, task_id: str = TASK_ID):
        return next(task for task in result["tasks"] if task["task_id"] == task_id)

    def _assert_tamper_rejected(self, mutation, pattern: str | None = None) -> None:
        value = load_json(self.output)
        mutation(value)
        write_json(self.output, value)
        context = (
            self.assertRaisesRegex(Exception, pattern)
            if pattern
            else self.assertRaises(Exception)
        )
        with context:
            validate_comparison(self.output)

    def _rejecting_comparison(self, stage: str = "hls-synthesis"):
        self._generate(self.candidate, "b" * 40, {TASK_ID: stage})
        return self._compare()

    def _manual_review_comparison(self):
        self._mutate_manifest(
            self.candidate,
            lambda manifest: manifest["effective_build_profile"].update(
                optimized_flags="-Ofast -march=native -mtune=native",
                cpu_target_profile="native",
            ),
        )
        return self._compare()

    def test_equal_real_v11_tasks_are_accepted(self):
        result = self._compare()
        self.assertEqual(result["policy"]["decision"], "accept")
        self.assertEqual(result["summary"]["comparable_tasks"], 5)

    def test_output_has_versioned_schema_and_bundle_identities(self):
        result = self._compare()
        self.assertEqual((result["schema"], result["schema_version"]), ("panda.ci.comparison", "1.0"))
        self.assertEqual(result["baseline"]["commit_sha"], "a" * 40)
        self.assertEqual(result["candidate"]["commit_sha"], "b" * 40)

    def test_comparison_is_byte_deterministic(self):
        first = self.root / "first.json"
        second = self.root / "second.json"
        compare_bundles(self.baseline, self.candidate, first)
        compare_bundles(self.baseline, self.candidate, second)
        self.assertEqual(first.read_bytes(), second.read_bytes())
        subprocess.run(["cmp", first, second], check=True)

    def test_cycle_improvement_has_negative_delta(self):
        self._mutate_task(self.candidate, lambda task: task["results"]["simulation"].update(total_cycles=21))
        metric = self._task(self._compare())["metrics"][0]
        self.assertEqual((metric["absolute_delta"], metric["percentage_delta"]), (-21, -50.0))

    def test_cycle_regression_is_measured_but_not_policy_rejection(self):
        self._mutate_task(self.candidate, lambda task: task["results"]["simulation"].update(total_cycles=84))
        result = self._compare()
        self.assertEqual(result["policy"]["decision"], "accept")
        self.assertEqual(self._task(result)["metrics"][0]["percentage_delta"], 100.0)

    def test_zero_baseline_has_absolute_but_null_percentage(self):
        self._mutate_task(self.baseline, lambda task: task["results"]["simulation"].update(total_cycles=0))
        metric = self._task(self._compare())["metrics"][0]
        self.assertEqual(metric["absolute_delta"], 42)
        self.assertIsNone(metric["percentage_delta"])

    def test_duration_metrics_are_compared(self):
        def change(task):
            next(m for m in task["metrics"] if m["metric_id"] == "duration.hls-synthesis")["value"] = 2
            next(s for s in task["stages"] if s["stage_id"] == "hls-synthesis")["duration_seconds"] = 2
        self._mutate_task(self.candidate, change)
        metrics = {m["metric_id"]: m for m in self._task(self._compare())["metrics"]}
        self.assertEqual(metrics["duration.hls-synthesis"]["absolute_delta"], 1)

    def test_frontend_change_is_not_comparable(self):
        def change(task):
            task["configuration"]["frontend"].update(requested="I386_CLANG17", selected="I386_CLANG17")
            task["configuration"]["options"]["compiler"] = "I386_CLANG17"
            task["configuration"]["invocation"]["arguments"] = [a.replace("I386_CLANG16", "I386_CLANG17") for a in task["configuration"]["invocation"]["arguments"]]
        self._mutate_task(self.candidate, change)
        result = self._compare()
        self.assertEqual(self._task(result)["classification"], "not-comparable")
        self.assertEqual(result["policy"]["decision"], "manual-review")

    def test_invocation_change_is_not_comparable(self):
        self._mutate_task(self.candidate, lambda task: task["configuration"]["invocation"]["arguments"].append("--fixture-option"))
        self.assertEqual(self._task(self._compare())["classification"], "not-comparable")

    def test_device_change_is_not_comparable(self):
        def change(task):
            task["configuration"]["options"]["device"] = "xc7z020"
            task["configuration"]["invocation"]["arguments"].append("--device-name=xc7z020")
        self._mutate_task(self.candidate, change)
        self.assertEqual(self._compare()["policy"]["decision"], "manual-review")

    def assert_profile_change_requires_review(self, mutation):
        self._mutate_manifest(self.candidate, mutation)
        result = self._compare()
        self.assertEqual(result["policy"]["decision"], "manual-review")
        self.assertEqual(self._task(result)["classification"], "not-comparable")

    def test_generic_versus_native_flags_requires_review(self):
        def change(manifest):
            profile = manifest["effective_build_profile"]
            profile["optimized_flags"] = "-Ofast -march=native -mtune=native"
            profile["cpu_target_profile"] = "native"
        self.assert_profile_change_requires_review(change)

    def test_changed_workflow_hash_requires_review(self):
        def change(manifest):
            manifest["effective_build_profile"]["workflow_file_sha256"] = "d" * 64
            manifest["workflow"]["file_sha256"] = "d" * 64
        self.assert_profile_change_requires_review(change)

    def test_changed_dockerfile_hash_requires_review(self):
        def change(manifest):
            manifest["effective_build_profile"]["dockerfile_sha256"] = "d" * 64
            manifest["container"]["dockerfile_sha256"] = "d" * 64
        self.assert_profile_change_requires_review(change)

    def test_changed_compiler_version_requires_review(self):
        def change(manifest):
            for tools in (manifest["tools"], manifest["effective_build_profile"]["tool_versions"]):
                next(item for item in tools if item["tool_id"] == "clang")["version"] = "clang version 17.0.0"
        self.assert_profile_change_requires_review(change)

    def test_rejects_cross_document_workflow_provenance_mismatch(self):
        self._mutate_manifest(
            self.candidate,
            lambda manifest: manifest["effective_build_profile"].update(
                workflow_file_sha256="d" * 64
            ),
        )
        with self.assertRaisesRegex(
            ComparisonInputError, "effective build profile/workflow mismatch"
        ):
            self._compare()

    def test_rejects_cross_document_tool_provenance_mismatch(self):
        self._mutate_manifest(
            self.candidate,
            lambda manifest: manifest["effective_build_profile"][
                "tool_versions"
            ][0].update(version="tampered tool version"),
        )
        with self.assertRaisesRegex(
            ComparisonInputError, "effective build profile/manifest mismatch"
        ):
            self._compare()

    def test_rejects_cross_document_open_build_provenance_mismatch(self):
        self._mutate_manifest(
            self.candidate,
            lambda manifest: manifest["effective_build_profile"].update(
                selected_frontend="I386_CLANG17"
            ),
        )
        with self.assertRaisesRegex(
            ComparisonInputError, "effective build profile/open-build mismatch"
        ):
            self._compare()

    def test_missing_effective_profile_requires_review(self):
        self.assert_profile_change_requires_review(
            lambda manifest: manifest.pop("effective_build_profile")
        )

    def test_empty_effective_profile_value_requires_review(self):
        self.assert_profile_change_requires_review(
            lambda manifest: manifest["effective_build_profile"].update(
                optimized_flags=""
            )
        )

    def test_whitespace_only_optimized_flags_requires_review(self):
        self.assert_profile_change_requires_review(
            lambda manifest: manifest["effective_build_profile"].update(
                optimized_flags=" \t\n"
            )
        )

    def test_whitespace_only_cpu_target_profile_requires_review(self):
        self.assert_profile_change_requires_review(
            lambda manifest: manifest["effective_build_profile"].update(
                cpu_target_profile=" \t\n"
            )
        )

    def test_empty_profile_tool_list_requires_review(self):
        def change(manifest):
            manifest["tools"] = []
            manifest["effective_build_profile"]["tool_versions"] = []

        self.assert_profile_change_requires_review(change)

    def test_duplicate_profile_tool_ids_requires_review(self):
        def change(manifest):
            tools = manifest["tools"]
            duplicate = copy.deepcopy(tools[0])
            manifest["tools"] = tools + [duplicate]
            manifest["effective_build_profile"]["tool_versions"] = manifest["tools"]

        self.assert_profile_change_requires_review(change)

    def test_incomplete_effective_profile_tool_set_requires_review(self):
        def change(manifest):
            manifest["tools"] = manifest["tools"][:-1]
            manifest["effective_build_profile"]["tool_versions"] = manifest["tools"]

        self.assert_profile_change_requires_review(change)

    def test_false_assertions_remain_valid_profile_evidence(self):
        for bundle in (self.baseline, self.candidate):
            self._mutate_manifest(
                bundle,
                lambda manifest: manifest["effective_build_profile"].update(
                    assertions_enabled=False
                ),
            )
            self._mutate_request(
                bundle,
                lambda request: request["build_parameters"].update(
                    assertions_enabled=False
                ),
            )
            self._mutate_open_build(
                bundle,
                lambda task: task["configuration"].update(assertions_enabled=False),
            )
        result = self._compare()
        self.assertEqual(result["policy"]["decision"], "accept")
        self.assertEqual(self._task(result)["classification"], "comparable")

    def test_false_warnings_as_errors_remain_valid_profile_evidence(self):
        for bundle in (self.baseline, self.candidate):
            self._mutate_manifest(
                bundle,
                lambda manifest: manifest["effective_build_profile"].update(
                    warnings_as_errors=False
                ),
            )
            self._mutate_request(
                bundle,
                lambda request: request["build_parameters"].update(
                    warnings_as_errors=False
                ),
            )
            self._mutate_open_build(
                bundle,
                lambda task: task["configuration"].update(warnings_as_errors=False),
            )
        result = self._compare()
        self.assertEqual(result["policy"]["decision"], "accept")
        self.assertEqual(self._task(result)["classification"], "comparable")

    def test_different_workflow_run_ids_remain_comparable(self):
        self._generate(self.candidate, "b" * 40, environment_updates={"PANDA_CI_WORKFLOW_RUN_ID": "98765"})
        self.assertEqual(self._compare()["policy"]["decision"], "accept")

    def test_different_run_attempts_remain_comparable(self):
        self._generate(self.candidate, "b" * 40, environment_updates={"PANDA_CI_RUN_ATTEMPT": "2"})
        self.assertEqual(self._compare()["policy"]["decision"], "accept")

    def test_different_timestamps_remain_comparable(self):
        self._mutate_manifest(self.candidate, lambda manifest: manifest.update(completed_at="2023-11-14T22:16:00Z"))
        self.assertEqual(self._compare()["policy"]["decision"], "accept")

    def test_cache_hit_versus_miss_remains_comparable(self):
        self._generate(self.candidate, "b" * 40, environment_updates={
            "PANDA_CI_CACHE_HIT": "true",
            "PANDA_CI_CACHE_MATCHED_KEY": "panda-cache-test",
        })
        self.assertEqual(self._compare()["policy"]["decision"], "accept")

    def test_different_commit_shas_remain_comparable(self):
        self.assertNotEqual(
            load_json(self.baseline / "manifest.json")["commit_sha"],
            load_json(self.candidate / "manifest.json")["commit_sha"],
        )
        self.assertEqual(self._compare()["policy"]["decision"], "accept")

    def test_synthesis_regression_is_rejected(self):
        self._generate(self.candidate, "b" * 40, {TASK_ID: "hls-synthesis"})
        result = self._compare()
        self.assertEqual(result["policy"]["decision"], "reject")
        self.assertIn("candidate-synthesis-regression", {r["code"] for r in result["policy"]["reasons"]})

    def test_simulation_regression_is_rejected(self):
        self._generate(self.candidate, "b" * 40, {TASK_ID: "rtl-simulation"})
        self.assertIn("candidate-simulation-regression", {r["code"] for r in self._compare()["policy"]["reasons"]})

    def test_verification_failure_is_rejected(self):
        self._generate(self.candidate, "b" * 40, {TASK_ID: "result-verification"})
        self.assertIn("candidate-verification-failure", {r["code"] for r in self._compare()["policy"]["reasons"]})

    def test_profile_mismatch_plus_synthesis_failure_is_reject(self):
        self._generate(self.candidate, "b" * 40, {TASK_ID: "hls-synthesis"})
        self._mutate_manifest(
            self.candidate,
            lambda manifest: manifest["effective_build_profile"].update(
                optimized_flags="-Ofast -march=native -mtune=native",
                cpu_target_profile="native",
            ),
        )
        self.assertEqual(self._compare()["policy"]["decision"], "reject")

    def test_profile_mismatch_plus_simulation_failure_is_reject(self):
        self._generate(self.candidate, "b" * 40, {TASK_ID: "rtl-simulation"})
        self._mutate_manifest(
            self.candidate,
            lambda manifest: manifest["effective_build_profile"].update(
                optimized_flags="-Ofast -march=native -mtune=native",
                cpu_target_profile="native",
            ),
        )
        self.assertEqual(self._compare()["policy"]["decision"], "reject")

    def test_profile_mismatch_plus_verification_failure_is_reject(self):
        self._generate(self.candidate, "b" * 40, {TASK_ID: "result-verification"})
        self._mutate_manifest(
            self.candidate,
            lambda manifest: manifest["effective_build_profile"].update(
                optimized_flags="-Ofast -march=native -mtune=native",
                cpu_target_profile="native",
            ),
        )
        self.assertEqual(self._compare()["policy"]["decision"], "reject")

    def test_missing_profile_plus_correctness_regression_is_reject(self):
        self._generate(self.candidate, "b" * 40, {TASK_ID: "hls-synthesis"})
        self._mutate_manifest(
            self.candidate, lambda manifest: manifest.pop("effective_build_profile")
        )
        self.assertEqual(self._compare()["policy"]["decision"], "reject")

    def test_profile_mismatch_without_correctness_regression_is_manual_review(self):
        self.assertEqual(self._manual_review_comparison()["policy"]["decision"], "manual-review")

    def test_correctness_rejection_takes_precedence_when_not_comparable(self):
        self._generate(self.candidate, "b" * 40, {TASK_ID: "hls-synthesis"})
        self._mutate_task(
            self.candidate,
            lambda task: task["configuration"]["invocation"]["arguments"].append(
                "--different-configuration"
            ),
        )
        result = self._compare()
        codes = {reason["code"] for reason in result["policy"]["reasons"]}
        self.assertEqual(self._task(result)["classification"], "not-comparable")
        self.assertEqual(result["policy"]["decision"], "reject")
        self.assertIn("configuration-not-comparable", codes)
        self.assertIn("candidate-synthesis-regression", codes)

    def test_preexisting_baseline_failure_requires_manual_review(self):
        self._generate(self.baseline, "a" * 40, {TASK_ID: "hls-synthesis"})
        self._generate(self.candidate, "b" * 40, {TASK_ID: "hls-synthesis"})
        result = self._compare()
        self.assertEqual(result["policy"]["decision"], "manual-review")
        self.assertIn("cycle-information-unavailable", {r["code"] for r in result["policy"]["reasons"]})

    def test_fixed_baseline_failure_is_manual_review_and_recorded(self):
        self._generate(self.baseline, "a" * 40, {TASK_ID: "hls-synthesis"})
        result = self._compare()
        self.assertEqual(result["policy"]["decision"], "manual-review")
        self.assertEqual(result["summary"]["correctness_improvements"], 1)

    def test_missing_candidate_policy_is_reject(self):
        task = load_json(self.baseline / f"tasks/{TASK_ID}.json")
        record = _task_record(TASK_ID, task, None, True)
        build = {
            "baseline": "pass",
            "candidate": "pass",
            "evidence": [
                "baseline:tasks/open-build.json#outcome",
                "candidate:tasks/open-build.json#outcome",
            ],
            "transition": "pass → pass",
        }
        reasons = _policy_reasons(build, [record])
        self.assertEqual(reasons[0]["code"], "required-regression-missing")

    def test_added_candidate_policy_is_manual_review(self):
        task = load_json(self.candidate / f"tasks/{TASK_ID}.json")
        record = _task_record(TASK_ID, None, task, True)
        build = {
            "baseline": "pass",
            "candidate": "pass",
            "evidence": [
                "baseline:tasks/open-build.json#outcome",
                "candidate:tasks/open-build.json#outcome",
            ],
            "transition": "pass → pass",
        }
        reasons = _policy_reasons(build, [record])
        self.assertEqual(reasons[0]["code"], "new-candidate-regression")

    def test_malformed_baseline_names_baseline(self):
        (self.baseline / "manifest.json").write_text("{}\n", encoding="utf-8")
        with self.assertRaisesRegex(ComparisonInputError, "baseline bundle validation failed"):
            self._compare()

    def test_malformed_candidate_names_candidate(self):
        (self.candidate / "manifest.json").write_text("{}\n", encoding="utf-8")
        with self.assertRaisesRegex(ComparisonInputError, "candidate bundle validation failed"):
            self._compare()

    def test_legacy_bundle_is_rejected_with_named_protocol_error(self):
        legacy = self.root / "legacy"
        generate_bundle(legacy, environment=successful_environment(), repository=REPOSITORY)
        with self.assertRaisesRegex(ComparisonInputError, "baseline bundle uses unsupported"):
            compare_bundles(legacy, self.candidate, self.output)

    def test_comparison_semantic_validation_rejects_bad_delta(self):
        self._compare()
        value = load_json(self.output)
        value["tasks"][0]["metrics"][0]["absolute_delta"] = 7
        write_json(self.output, value)
        with self.assertRaisesRegex(ComparisonError, "invalid absolute delta"):
            validate_comparison(self.output)

    def test_standalone_validation_reconstructs_policy_and_rejects_tampering(self):
        self._compare()
        value = load_json(self.output)
        value["policy"] = {
            "decision": "manual-review",
            "reasons": [
                {
                    "code": "fabricated-review",
                    "decision": "manual-review",
                    "evidence": ["candidate:tasks/regression-scalar.json"],
                    "task_id": TASK_ID,
                }
            ],
        }
        value["overall_comparison_outcome"] = "manual-review"
        value["summary"]["manual_review_items"] = 1
        write_json(self.output, value)
        with self.assertRaisesRegex(ComparisonError, "policy reasons do not match"):
            validate_comparison(self.output)

    def test_tamper_removed_reject_reason_fails(self):
        self._rejecting_comparison()
        self._assert_tamper_rejected(lambda value: value["policy"]["reasons"].pop())

    def test_tamper_decision_reject_to_accept_fails(self):
        self._rejecting_comparison()
        self._assert_tamper_rejected(
            lambda value: value["policy"].update(decision="accept")
        )

    def test_tamper_decision_reject_to_manual_review_fails(self):
        self._rejecting_comparison()
        self._assert_tamper_rejected(
            lambda value: value["policy"].update(decision="manual-review")
        )

    def test_tamper_build_transition_fails(self):
        self._compare()
        self._assert_tamper_rejected(
            lambda value: value["build"].update(transition="pass -> pass")
        )

    def test_tamper_correctness_transition_fails(self):
        self._compare()
        self._assert_tamper_rejected(
            lambda value: self._task(value)["correctness"]["overall"].update(
                transition="pass -> fail"
            )
        )

    def test_tamper_execution_transition_fails(self):
        self._compare()
        self._assert_tamper_rejected(lambda value: self._task(value)["execution"].update(transition="queued -> done"))

    def test_tamper_failure_transition_fails(self):
        self._compare()
        self._assert_tamper_rejected(lambda value: self._task(value)["failure"].update(transition="none -> hls-synthesis"))

    def test_tamper_introduced_failure_fails(self):
        self._rejecting_comparison()
        self._assert_tamper_rejected(lambda value: self._task(value)["failure"].update(introduced_failure=False))

    def test_tamper_fixed_failure_fails(self):
        self._generate(self.baseline, "a" * 40, {TASK_ID: "hls-synthesis"})
        self._compare()
        self._assert_tamper_rejected(lambda value: self._task(value)["failure"].update(fixed_failure=False))

    def test_tamper_summary_regression_counter_fails(self):
        self._rejecting_comparison()
        self._assert_tamper_rejected(lambda value: value["summary"].update(correctness_regressions=0))

    def test_tamper_summary_improvement_counter_fails(self):
        self._generate(self.baseline, "a" * 40, {TASK_ID: "hls-synthesis"})
        self._compare()
        self._assert_tamper_rejected(lambda value: value["summary"].update(correctness_improvements=0))

    def test_tamper_task_classification_fails(self):
        self._compare()
        self._assert_tamper_rejected(lambda value: self._task(value).update(classification="not-comparable"))

    def test_tamper_failed_task_id_check_relabel_fails(self):
        self._compare()

        def relabel(value):
            task = self._task(value)
            check = task["comparability_checks"][0]
            check["candidate"] = "different-task"
            check["reason_code"] = "configuration-differs"
            task["classification"] = "not-comparable"
            task["comparability_reasons"] = [
                {"code": "configuration-differs", "field": "task_id"}
            ]
            task["metrics"] = []
            value["policy"] = {
                "decision": "manual-review",
                "reasons": [
                    {
                        "code": "configuration-not-comparable",
                        "decision": "manual-review",
                        "evidence": task["evidence"],
                        "task_id": TASK_ID,
                    }
                ],
            }
            value["overall_comparison_outcome"] = "manual-review"
            value["summary"]["comparable_tasks"] -= 1
            value["summary"]["manual_review_items"] = 1

        self._assert_tamper_rejected(relabel, "reason code is not canonical")

    def test_tamper_failed_configuration_check_relabel_fails(self):
        self._compare()

        def relabel(value):
            task = self._task(value)
            check = next(
                item
                for item in task["comparability_checks"]
                if item["field"] == "options.device"
            )
            check["candidate"] = "different-device"
            check["reason_code"] = "task-type-differs"
            task["classification"] = "not-comparable"
            task["comparability_reasons"] = [
                {"code": "task-type-differs", "field": "options.device"}
            ]
            task["metrics"] = []
            value["policy"] = {
                "decision": "manual-review",
                "reasons": [
                    {
                        "code": "configuration-not-comparable",
                        "decision": "manual-review",
                        "evidence": task["evidence"],
                        "task_id": TASK_ID,
                    }
                ],
            }
            value["overall_comparison_outcome"] = "manual-review"
            value["summary"]["comparable_tasks"] -= 1
            value["summary"]["manual_review_items"] = 1

        self._assert_tamper_rejected(relabel, "reason code is not canonical")

    def test_tamper_passed_equal_check_relabel_fails(self):
        self._compare()
        self._assert_tamper_rejected(
            lambda value: self._task(value)["comparability_checks"][0].update(
                reason_code="configuration-differs"
            ),
            "reason code is not canonical",
        )

    def test_tamper_unknown_comparability_check_field_fails(self):
        self._compare()
        self._assert_tamper_rejected(
            lambda value: self._task(value)["comparability_checks"].append(
                {
                    "baseline": "a",
                    "candidate": "b",
                    "field": "unknown.field",
                    "reason_code": "configuration-differs",
                }
            ),
            "comparability checks are not canonical",
        )

    def test_tamper_duplicate_comparability_check_field_fails(self):
        self._compare()
        self._assert_tamper_rejected(
            lambda value: self._task(value)["comparability_checks"].__setitem__(
                1, copy.deepcopy(self._task(value)["comparability_checks"][0])
            ),
            "comparability checks are not canonical",
        )

    def test_tamper_reordered_comparability_check_fields_fails(self):
        self._compare()

        def reorder(value):
            checks = self._task(value)["comparability_checks"]
            checks[0], checks[1] = checks[1], checks[0]

        self._assert_tamper_rejected(reorder, "comparability checks are not canonical")

    def test_tamper_wrong_relabel_with_matching_reason_fails(self):
        self._manual_review_comparison()

        def relabel(value):
            task = self._task(value)
            check = next(
                item
                for item in task["comparability_checks"]
                if item["field"] == "build_profile"
            )
            check["reason_code"] = "configuration-differs"
            task["comparability_reasons"] = [
                {"code": "configuration-differs", "field": "build_profile"}
            ]

        self._assert_tamper_rejected(relabel, "reason code is not canonical")

    def test_tamper_comparability_reason_fails(self):
        self._manual_review_comparison()
        self._assert_tamper_rejected(lambda value: self._task(value)["comparability_reasons"][0].update(field="wrong"))

    def test_tamper_duplicate_comparability_reason_fails(self):
        self._manual_review_comparison()
        self._assert_tamper_rejected(
            lambda value: self._task(value)["comparability_reasons"].append(
                copy.deepcopy(self._task(value)["comparability_reasons"][0])
            )
        )

    def test_tamper_unknown_comparability_reason_fails(self):
        self._compare()
        self._assert_tamper_rejected(
            lambda value: self._task(value)["comparability_reasons"].append(
                {"code": "unknown-reason", "field": "task_id"}
            )
        )

    def test_tamper_primitive_comparability_check_without_derived_update_fails(self):
        self._compare()
        self._assert_tamper_rejected(
            lambda value: self._task(value)["comparability_checks"][0].update(
                candidate="different-task"
            )
        )

    def test_tamper_metric_evidence_fails(self):
        self._compare()
        self._assert_tamper_rejected(lambda value: self._task(value)["metrics"][0]["evidence"].append("candidate:elsewhere.json"))

    def test_tamper_metric_unit_fails(self):
        self._compare()
        self._assert_tamper_rejected(lambda value: self._task(value)["metrics"][0].update(unit="seconds"))

    def test_tamper_unknown_metric_id_fails(self):
        self._compare()
        self._assert_tamper_rejected(lambda value: self._task(value)["metrics"][0].update(metric_id="unknown.metric"))

    def test_tamper_duplicate_metric_id_fails(self):
        self._compare()
        self._assert_tamper_rejected(lambda value: self._task(value)["metrics"][1].update(metric_id="simulation.total-cycles"))

    def test_tamper_missing_required_metric_fails(self):
        self._compare()
        self._assert_tamper_rejected(lambda value: self._task(value)["metrics"].pop())

    def test_tamper_absolute_delta_fails(self):
        self._compare()
        self._assert_tamper_rejected(
            lambda value: self._task(value)["metrics"][0].update(absolute_delta=7),
            "invalid absolute delta",
        )

    def test_tamper_percentage_delta_fails(self):
        self._compare()
        self._assert_tamper_rejected(
            lambda value: self._task(value)["metrics"][0].update(percentage_delta=7),
            "invalid percentage delta",
        )

    def test_tamper_noncanonical_task_evidence_fails(self):
        self._compare()
        self._assert_tamper_rejected(
            lambda value: self._task(value).update(
                evidence=["candidate:tasks/regression-scalar.json"]
            )
        )

    def test_tamper_unsupported_policy_reason_fails(self):
        self._compare()
        self._assert_tamper_rejected(
            lambda value: value["policy"]["reasons"].append(
                {
                    "code": "unsupported-policy-reason",
                    "decision": "manual-review",
                    "evidence": ["candidate:tasks/regression-scalar.json"],
                    "task_id": TASK_ID,
                }
            )
        )

    def test_compare_cli_uses_distinct_nonzero_policy_exit_codes(self):
        self.assertEqual(
            main(
                [
                    "compare",
                    "--baseline",
                    str(self.baseline),
                    "--candidate",
                    str(self.candidate),
                    "--output",
                    str(self.output),
                ]
            ),
            0,
        )
        self._mutate_manifest(
            self.candidate,
            lambda manifest: manifest.pop("effective_build_profile"),
        )
        self.assertEqual(
            main(
                [
                    "compare",
                    "--baseline",
                    str(self.baseline),
                    "--candidate",
                    str(self.candidate),
                    "--output",
                    str(self.output),
                ]
            ),
            2,
        )
        self._generate(self.candidate, "b" * 40, {TASK_ID: "hls-synthesis"})
        self.assertEqual(
            main(
                [
                    "compare",
                    "--baseline",
                    str(self.baseline),
                    "--candidate",
                    str(self.candidate),
                    "--output",
                    str(self.output),
                ]
            ),
            1,
        )

    def test_compare_main_malformed_baseline_returns_nonzero(self):
        (self.baseline / "manifest.json").write_text("{}\n", encoding="utf-8")
        result = main(
            [
                "compare",
                "--baseline",
                str(self.baseline),
                "--candidate",
                str(self.candidate),
                "--output",
                str(self.output),
            ]
        )
        self.assertNotEqual(result, 0)
        self.assertFalse(self.output.exists())

    def test_compare_main_malformed_candidate_returns_nonzero(self):
        (self.candidate / "manifest.json").write_text("{}\n", encoding="utf-8")
        result = main(
            [
                "compare",
                "--baseline",
                str(self.baseline),
                "--candidate",
                str(self.candidate),
                "--output",
                str(self.output),
            ]
        )
        self.assertNotEqual(result, 0)
        self.assertFalse(self.output.exists())

    def test_render_comparison_main_malformed_json_returns_nonzero(self):
        self.output.write_text("{\n", encoding="utf-8")
        self.assertNotEqual(
            main(["render-comparison", "--input", str(self.output)]),
            0,
        )

    def test_render_comparison_main_semantic_inconsistency_returns_nonzero(self):
        self._compare()
        value = load_json(self.output)
        value["tasks"][0]["metrics"][0]["absolute_delta"] = 7
        write_json(self.output, value)
        self.assertNotEqual(
            main(["render-comparison", "--input", str(self.output)]),
            0,
        )

    def test_malformed_compare_main_does_not_create_accepted_output(self):
        (self.candidate / "manifest.json").write_text("{}\n", encoding="utf-8")
        result = main(
            [
                "compare",
                "--baseline",
                str(self.baseline),
                "--candidate",
                str(self.candidate),
                "--output",
                str(self.output),
            ]
        )
        self.assertNotEqual(result, 0)
        self.assertFalse(self.output.exists())

    def test_renderer_uses_only_comparison_document(self):
        result = self._compare()
        text = render_comparison_file(self.output)
        self.assertIn("Comparable tasks: 5", text)
        self.assertIn(f"Policy decision: {result['policy']['decision']}", text)


if __name__ == "__main__":
    unittest.main()
