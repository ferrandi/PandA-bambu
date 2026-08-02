#!/usr/bin/env python3
"""Focused PAF-05A1 execution contract tests; no execution occurs."""
from __future__ import annotations

import copy
import json
import re
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
TOOLS = ROOT / "tools" / "agentic"
sys.path.insert(0, str(TOOLS))

import contracts  # noqa: E402
import execution_contracts as execution  # noqa: E402
import local_state  # noqa: E402

try:
    import jsonschema
except ImportError:  # pragma: no cover
    jsonschema = None

D = "a" * 64
COMMIT = "b" * 40


def request() -> dict:
    return {
        "schema": "evolvehls.agentic.execution-request",
        "schema_version": "1.0",
        "execution_id": "fixture-run",
        "created_at": "2026-01-01T00:00:00+00:00",
        "task_id": "fixture-task",
        "task_version": "1.0",
        "task_digest": D,
        "role_id": "implementer",
        "role_version": "1.0",
        "role_digest": D,
        "base_commit": COMMIT,
        "profile_id": "fixture-profile",
        "adapter_id": "fixture-adapter",
        "runtime_map_id": "fixture-runtime-map",
        "runtime_map_version": "1.0",
        "runtime_entry_id": "fixture-entry",
        "invocation_descriptor_digest": D,
        "routing_provenance": {
            "schema_version": "1.0",
            "resolver_version": "1.0",
            "registry_id": "fixture-registry",
            "registry_version": "1.0",
            "policy_id": "fixture-policy",
            "policy_version": "1.0",
            "mode": "development",
            "task_id": "fixture-task",
            "role_id": "implementer",
        },
        "executor_id": "fixture-local",
        "executor_version": "1.0",
        "input_handoff": "fixture-json-file-v1",
        "working_directory_policy": "detached-worktree-root",
        "result_collection": "fixture-jsonl-and-artifact-v1",
        "wall_clock_timeout_seconds": 30,
        "stdout_limit_bytes": 1024,
        "stderr_limit_bytes": 1024,
        "total_output_limit_bytes": 2048,
        "retention_policy": "retain-for-inspection",
        "authorization_state": "previewed-requires-explicit-confirmation",
        "approved_operation": "write-fixture-execution-record-v1",
        "approved_output_path": execution.OUTPUT_PATH,
    }


def stream() -> dict:
    return {
        "captured_bytes": 0,
        "observed_bytes": 0,
        "truncated": False,
        "digest": D,
        "evidence_path": ".agentic-local/executions/fixture-run/stdout.bin",
    }


def receipt() -> dict:
    return {
        "schema": "evolvehls.agentic.execution-receipt",
        "schema_version": "1.0",
        "execution_id": "fixture-run",
        "execution_request_digest": D,
        "task_id": "fixture-task",
        "task_version": "1.0",
        "task_digest": D,
        "base_commit": COMMIT,
        "executor_id": "fixture-local",
        "executor_version": "1.0",
        "started_at": "2026-01-01T00:00:00+00:00",
        "completed_at": "2026-01-01T00:00:01+00:00",
        "execution_state": "completed",
        "exit_classification": "succeeded",
        "process_exit_code": 0,
        "timed_out": False,
        "cancelled": False,
        "worktree_path": ".agentic-local/worktrees/fixture-run",
        "input_handoff_digest": D,
        "stdout": stream(),
        "stderr": stream(),
        "result_path": ".agentic-local/executions/fixture-run/result.json",
        "result_digest": D,
        "changed_files": [
            {
                "path": execution.OUTPUT_PATH,
                "change": "added",
                "digest": D,
                "size": 100,
            }
        ],
        "caller_checkout_preserved": True,
        "detached_worktree": True,
        "base_commit_preserved": True,
        "git_metadata_preserved": True,
        "submodules_preserved": True,
        "retention_state": "retained",
        "diagnostics": ["fixture execution contract evidence"],
        "commit_attempted": False,
        "push_attempted": False,
        "pull_request_attempted": False,
        "merge_attempted": False,
    }


def handoff() -> dict:
    return {
        "schema": "evolvehls.agentic.fixture-handoff",
        "schema_version": "1.0",
        "execution_id": "fixture-run",
        "task": {
            "task_id": "fixture-task",
            "version": "1.0",
            "objective": "write deterministic fixture output",
            "inputs": ["tracked fixture input"],
            "constraints": ["no network"],
            "expected_outputs": ["fixture output"],
            "validation_requirements": ["exact bytes"],
        },
        "role": {
            "role_id": "implementer",
            "version": "1.0",
            "mandatory_capabilities": ["tool_calling"],
        },
        "route": {
            "profile_id": "fixture-profile",
            "adapter_id": "fixture-adapter",
            "runtime_map_id": "fixture-runtime-map",
            "runtime_entry_id": "fixture-entry",
        },
        "base_commit": COMMIT,
        "operation": "write-fixture-execution-record-v1",
        "output_path": execution.OUTPUT_PATH,
        "expected_content": "EvolveHLS fixture execution\n",
        "expected_digest": D,
    }


def result(identity: str) -> dict:
    return {
        "schema": "evolvehls.agentic.result",
        "schema_version": "1.0",
        "task_id": "fixture-task",
        "task_version": "1.0",
        "outcome": "succeeded",
        "execution_plan": identity,
        "artifacts": [{"path": execution.OUTPUT_PATH, "digest": D}],
        "validation": [{"name": "fixture", "status": "passed", "evidence": D}],
        "diagnostics": [],
    }


class CanonicalDigestTests(unittest.TestCase):
    def test_deterministic_order_nested_change_and_shape(self):
        first = {"b": [{"z": True, "a": None}], "a": 1}
        second = {"a": 1, "b": [{"a": None, "z": True}]}
        digest = local_state.canonical_digest(first)
        self.assertEqual(digest, local_state.canonical_digest(first))
        self.assertEqual(digest, local_state.canonical_digest(second))
        self.assertRegex(digest, r"^[0-9a-f]{64}$")
        changed = copy.deepcopy(first)
        changed["a"] = 2
        self.assertNotEqual(digest, local_state.canonical_digest(changed))

    def test_unsupported_value_fails_consistently(self):
        with self.assertRaises(TypeError):
            local_state.canonical_digest({"bad": object()})


@unittest.skipIf(jsonschema is None, "jsonschema unavailable")
class SchemaTests(unittest.TestCase):
    def test_new_schemas_are_strict_and_validate_examples(self):
        examples = {
            "execution-request": request(),
            "execution-receipt": receipt(),
            "fixture-handoff": handoff(),
        }
        for name, value in examples.items():
            schema = contracts.load_schema(name)
            jsonschema.Draft202012Validator.check_schema(schema)
            jsonschema.validate(value, schema)
            bad = copy.deepcopy(value)
            bad["command"] = "echo unsafe"
            with self.assertRaises(jsonschema.ValidationError):
                jsonschema.validate(bad, schema)


class ExecutionRequestTests(unittest.TestCase):
    def test_valid_request(self):
        execution.validate_execution_request(request())

    def test_missing_unknown_and_prohibited_fields(self):
        for field in ("task_id", "approved_output_path"):
            value = request()
            del value[field]
            with self.assertRaises(execution.ExecutionContractError):
                execution.validate_execution_request(value)
        for field, attempted in (
            ("command", "echo x"),
            ("argv", ["echo", "x"]),
            ("environment", {"TOKEN": "secret"}),
            ("credential", "secret"),
        ):
            value = request()
            value[field] = attempted
            with self.assertRaises(execution.ExecutionContractError):
                execution.validate_execution_request(value)

    def test_ids_digests_commits_timestamp_and_constants(self):
        mutations = (
            ("execution_id", "../bad"),
            ("task_digest", "A" * 64),
            ("task_digest", "a" * 63),
            ("base_commit", "HEAD"),
            ("base_commit", "B" * 40),
            ("created_at", "2026-01-01"),
            ("executor_id", "cline"),
            ("approved_operation", "arbitrary"),
            ("approved_output_path", "/tmp/output"),
            ("approved_output_path", "../output"),
        )
        for field, attempted in mutations:
            value = request()
            value[field] = attempted
            with self.subTest(field=field, attempted=attempted):
                with self.assertRaises(execution.ExecutionContractError):
                    execution.validate_execution_request(value)

    def test_numeric_limits_and_relationships(self):
        mutations = (
            ("wall_clock_timeout_seconds", 0),
            ("wall_clock_timeout_seconds", 301),
            ("stdout_limit_bytes", 0),
            ("stderr_limit_bytes", 262145),
            ("total_output_limit_bytes", 1000),
            ("total_output_limit_bytes", 2049),
        )
        for field, attempted in mutations:
            value = request()
            value[field] = attempted
            with self.subTest(field=field, attempted=attempted):
                with self.assertRaises(execution.ExecutionContractError):
                    execution.validate_execution_request(value)


class ExecutionReceiptTests(unittest.TestCase):
    def test_valid_success_failed_timeout_and_cancelled(self):
        execution.validate_execution_receipt(receipt())
        failed = receipt()
        failed.update(
            execution_state="failed",
            exit_classification="executor-failed",
            process_exit_code=2,
        )
        execution.validate_execution_receipt(failed)
        timed = copy.deepcopy(failed)
        timed.update(exit_classification="timed-out", timed_out=True, process_exit_code=None)
        execution.validate_execution_receipt(timed)
        cancelled = copy.deepcopy(failed)
        cancelled.update(
            execution_state="cancelled",
            exit_classification="cancelled",
            cancelled=True,
            process_exit_code=None,
        )
        execution.validate_execution_receipt(cancelled)

    def test_time_success_and_classification_invariants(self):
        cases = []
        value = receipt()
        value["completed_at"] = "2025-12-31T23:59:59+00:00"
        cases.append(value)
        for field, attempted in (
            ("process_exit_code", 1),
            ("timed_out", True),
            ("cancelled", True),
            ("caller_checkout_preserved", False),
        ):
            value = receipt()
            value[field] = attempted
            cases.append(value)
        for stream_name in ("stdout", "stderr"):
            value = receipt()
            value[stream_name]["truncated"] = True
            cases.append(value)
        value = receipt()
        value.update(
            execution_state="failed",
            exit_classification="timed-out",
            process_exit_code=None,
        )
        cases.append(value)
        value = receipt()
        value.update(
            execution_state="failed",
            exit_classification="cancelled",
            process_exit_code=None,
        )
        cases.append(value)
        for value in cases:
            with self.assertRaises(execution.ExecutionContractError):
                execution.validate_execution_receipt(value)

    def test_paths_changed_inventory_diagnostics_digests_and_audit(self):
        cases = []
        for field, attempted in (
            ("worktree_path", "/tmp/worktree"),
            ("worktree_path", "../worktree"),
            ("result_path", ".agentic-local/../result"),
            ("result_digest", "A" * 64),
            ("commit_attempted", True),
        ):
            value = receipt()
            value[field] = attempted
            cases.append(value)
        value = receipt()
        item = copy.deepcopy(value["changed_files"][0])
        item["path"] = "a"
        value["changed_files"] = [copy.deepcopy(value["changed_files"][0]), item]
        cases.append(value)
        value = receipt()
        value["changed_files"] = value["changed_files"] * 2
        cases.append(value)
        value = receipt()
        value["diagnostics"] = ["token=ghp_abcdefghijklmnopqrstuvwxyz123456"]
        cases.append(value)
        for value in cases:
            with self.assertRaises(execution.ExecutionContractError):
                execution.validate_execution_receipt(value)


class FixtureHandoffTests(unittest.TestCase):
    def test_valid_handoff(self):
        execution.validate_fixture_handoff(handoff())

    def test_closed_shape_constants_paths_digest_content_and_nested_fields(self):
        cases = []
        value = handoff()
        del value["role"]
        cases.append(value)
        for field, attempted in (
            ("operation", "run-command"),
            ("output_path", "other/output"),
            ("output_path", "/tmp/output"),
            ("output_path", "../output"),
            ("expected_digest", "A" * 64),
            ("expected_content", "x" * 4097),
        ):
            value = handoff()
            value[field] = attempted
            cases.append(value)
        for container, field in (
            ("task", "command"),
            ("role", "environment"),
            ("route", "credential"),
        ):
            value = handoff()
            value[container][field] = "unsafe"
            cases.append(value)
        value = handoff()
        value["command"] = "unsafe"
        cases.append(value)
        for value in cases:
            with self.assertRaises(execution.ExecutionContractError):
                execution.validate_fixture_handoff(value)


class ResultIdentityTests(unittest.TestCase):
    def test_identity_generation_and_validation(self):
        identity = execution.result_execution_identity("fixture-run", D)
        self.assertEqual(identity, f"execution-request:fixture-run:sha256:{D}")
        execution.validate_result_execution_identity(result(identity), "fixture-run", D)

    def test_mismatch_malformed_and_invalid_result(self):
        identity = execution.result_execution_identity("fixture-run", D)
        for attempted_id, attempted_digest, plan in (
            ("other-run", D, identity),
            ("fixture-run", "b" * 64, identity),
            ("fixture-run", D, "malformed"),
        ):
            with self.assertRaises(execution.ExecutionContractError):
                execution.validate_result_execution_identity(
                    result(plan), attempted_id, attempted_digest
                )
        invalid = result(identity)
        del invalid["outcome"]
        with self.assertRaises(execution.ExecutionContractError):
            execution.validate_result_execution_identity(invalid, "fixture-run", D)


if __name__ == "__main__":
    unittest.main()