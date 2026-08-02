#!/usr/bin/env python3
"""Strict, validation-only PAF-05A execution document contracts."""
from __future__ import annotations

import re
from collections.abc import Mapping
from datetime import datetime
from typing import Any

import contracts
import local_state
import redaction

DIGEST_PATTERN = re.compile(r"^[0-9a-f]{64}$")
COMMIT_PATTERN = re.compile(r"^[0-9a-f]{40}$")
RESULT_IDENTITY_PATTERN = re.compile(
    r"^execution-request:([a-z][a-z0-9.-]*):sha256:([0-9a-f]{64})$"
)
OUTPUT_PATH = "agentic/fixtures/executions/fixture-local-output.txt"
EXIT_CLASSIFICATIONS = {
    "succeeded",
    "executor-failed",
    "timed-out",
    "cancelled",
    "output-limit",
    "invalid-result",
    "worktree-violation",
    "launch-failed",
    "executor-not-implemented",
}


class ExecutionContractError(ValueError):
    """Raised when a PAF-05 execution document violates its contract."""


def _require(condition: bool, message: str) -> None:
    if not condition:
        raise ExecutionContractError(message)


def _fields(value: Any, expected: set[str], name: str) -> None:
    _require(
        isinstance(value, Mapping) and set(value) == expected,
        f"{name} fields do not match schema",
    )


def _identifier(value: Any, field: str) -> None:
    try:
        local_state.validate_identifier(value, field)
    except local_state.LocalStateError as error:
        raise ExecutionContractError(str(error)) from None


def _digest(value: Any, field: str) -> None:
    _require(
        isinstance(value, str) and DIGEST_PATTERN.fullmatch(value) is not None,
        f"invalid {field}",
    )


def _commit(value: Any, field: str = "base_commit") -> None:
    _require(
        isinstance(value, str) and COMMIT_PATTERN.fullmatch(value) is not None,
        f"invalid {field}",
    )


def _timestamp(value: Any, field: str) -> datetime:
    try:
        return contracts.parse_timestamp(value, field)
    except contracts.ContractError as error:
        raise ExecutionContractError(str(error)) from None


def _bounded_string(value: Any, field: str, maximum: int) -> None:
    _require(
        isinstance(value, str) and 0 < len(value) <= maximum,
        f"invalid {field}",
    )


def _string_list(value: Any, field: str, maximum: int = 32) -> None:
    _require(
        isinstance(value, list)
        and len(value) <= maximum
        and len(value) == len(set(value))
        and all(isinstance(item, str) and 0 < len(item) <= 512 for item in value),
        f"invalid {field}",
    )


def _relative_path(value: Any, field: str, *, nullable: bool = False) -> None:
    if nullable and value is None:
        return
    _require(isinstance(value, str) and 0 < len(value) <= 512, f"invalid {field}")
    _require(
        not value.startswith(("/", "~"))
        and "\\" not in value
        and all(part not in {"", ".", ".."} for part in value.split("/")),
        f"invalid {field}",
    )


def _stream(value: Any, name: str) -> None:
    _fields(
        value,
        {"captured_bytes", "observed_bytes", "truncated", "digest", "evidence_path"},
        f"{name} metadata",
    )
    _require(
        isinstance(value["captured_bytes"], int)
        and not isinstance(value["captured_bytes"], bool)
        and isinstance(value["observed_bytes"], int)
        and not isinstance(value["observed_bytes"], bool)
        and 0 <= value["captured_bytes"] <= value["observed_bytes"] <= 524288
        and isinstance(value["truncated"], bool),
        f"invalid {name} metadata",
    )
    _digest(value["digest"], f"{name} digest")
    _relative_path(value["evidence_path"], f"{name} evidence_path", nullable=True)


def validate_execution_request(value: Mapping[str, Any]) -> None:
    """Validate the closed PAF-05A execution-request 1.0 contract."""
    expected = {
        "schema",
        "schema_version",
        "execution_id",
        "created_at",
        "task_id",
        "task_version",
        "task_digest",
        "role_id",
        "role_version",
        "role_digest",
        "base_commit",
        "profile_id",
        "adapter_id",
        "runtime_map_id",
        "runtime_map_version",
        "runtime_entry_id",
        "invocation_descriptor_digest",
        "routing_provenance",
        "executor_id",
        "executor_version",
        "input_handoff",
        "working_directory_policy",
        "result_collection",
        "wall_clock_timeout_seconds",
        "stdout_limit_bytes",
        "stderr_limit_bytes",
        "total_output_limit_bytes",
        "retention_policy",
        "authorization_state",
        "approved_operation",
        "approved_output_path",
    }
    _fields(value, expected, "execution request")
    _require(
        value["schema"] == "evolvehls.agentic.execution-request"
        and value["schema_version"] == "1.0",
        "unsupported execution request schema",
    )
    for field in (
        "execution_id",
        "task_id",
        "role_id",
        "profile_id",
        "adapter_id",
        "runtime_map_id",
        "runtime_entry_id",
        "executor_id",
    ):
        _identifier(value[field], field)
    for field in (
        "task_version",
        "role_version",
        "runtime_map_version",
        "executor_version",
    ):
        _bounded_string(value[field], field, 64)
    _timestamp(value["created_at"], "created_at")
    for field in ("task_digest", "role_digest", "invocation_descriptor_digest"):
        _digest(value[field], field)
    _commit(value["base_commit"])

    provenance_fields = {
        "schema_version",
        "resolver_version",
        "registry_id",
        "registry_version",
        "policy_id",
        "policy_version",
        "mode",
        "task_id",
        "role_id",
    }
    provenance = value["routing_provenance"]
    _fields(provenance, provenance_fields, "routing provenance")
    _require(
        provenance["schema_version"] == "1.0"
        and provenance["mode"] in {"development", "evaluation", "ablation"},
        "invalid routing provenance",
    )
    for field in ("registry_id", "policy_id", "task_id", "role_id"):
        _identifier(provenance[field], f"routing_provenance.{field}")
    for field in ("resolver_version", "registry_version", "policy_version"):
        _bounded_string(provenance[field], f"routing_provenance.{field}", 64)

    _require(
        value["executor_id"] == "fixture-local"
        and value["executor_version"] == "1.0"
        and value["input_handoff"] == "fixture-json-file-v1"
        and value["working_directory_policy"] == "detached-worktree-root"
        and value["result_collection"] == "fixture-jsonl-and-artifact-v1"
        and value["retention_policy"] == "retain-for-inspection"
        and value["authorization_state"]
        == "previewed-requires-explicit-confirmation"
        and value["approved_operation"] == "write-fixture-execution-record-v1"
        and value["approved_output_path"] == OUTPUT_PATH,
        "unsupported fixture execution constants",
    )

    for field, maximum in (
        ("wall_clock_timeout_seconds", 300),
        ("stdout_limit_bytes", 262144),
        ("stderr_limit_bytes", 262144),
        ("total_output_limit_bytes", 524288),
    ):
        _require(
            isinstance(value[field], int)
            and not isinstance(value[field], bool)
            and 1 <= value[field] <= maximum,
            f"invalid {field}",
        )
    total = value["total_output_limit_bytes"]
    stdout = value["stdout_limit_bytes"]
    stderr = value["stderr_limit_bytes"]
    _require(
        max(stdout, stderr) <= total <= stdout + stderr,
        "invalid total_output_limit_bytes relationship",
    )
    _relative_path(value["approved_output_path"], "approved_output_path")


def validate_execution_receipt(value: Mapping[str, Any]) -> None:
    """Validate the closed PAF-05A execution-receipt 1.0 contract."""
    expected = {
        "schema",
        "schema_version",
        "execution_id",
        "execution_request_digest",
        "task_id",
        "task_version",
        "task_digest",
        "base_commit",
        "executor_id",
        "executor_version",
        "started_at",
        "completed_at",
        "execution_state",
        "exit_classification",
        "process_exit_code",
        "timed_out",
        "cancelled",
        "worktree_path",
        "input_handoff_digest",
        "stdout",
        "stderr",
        "result_path",
        "result_digest",
        "changed_files",
        "caller_checkout_preserved",
        "detached_worktree",
        "base_commit_preserved",
        "git_metadata_preserved",
        "submodules_preserved",
        "retention_state",
        "diagnostics",
        "commit_attempted",
        "push_attempted",
        "pull_request_attempted",
        "merge_attempted",
    }
    _fields(value, expected, "execution receipt")
    _require(
        value["schema"] == "evolvehls.agentic.execution-receipt"
        and value["schema_version"] == "1.0",
        "unsupported execution receipt schema",
    )
    for field in ("execution_id", "task_id", "executor_id"):
        _identifier(value[field], field)
    for field in ("task_version", "executor_version"):
        _bounded_string(value[field], field, 64)
    for field in (
        "execution_request_digest",
        "task_digest",
        "input_handoff_digest",
        "result_digest",
    ):
        _digest(value[field], field)
    _commit(value["base_commit"])
    _require(
        value["executor_id"] == "fixture-local"
        and value["executor_version"] == "1.0",
        "unsupported receipt executor",
    )

    started = _timestamp(value["started_at"], "started_at")
    completed = _timestamp(value["completed_at"], "completed_at")
    _require(completed >= started, "completed_at must not precede started_at")
    _require(
        value["execution_state"] in {"completed", "failed", "cancelled"}
        and value["exit_classification"] in EXIT_CLASSIFICATIONS,
        "invalid execution receipt state",
    )
    _require(
        value["process_exit_code"] is None
        or (
            isinstance(value["process_exit_code"], int)
            and not isinstance(value["process_exit_code"], bool)
        ),
        "invalid process_exit_code",
    )
    _require(
        isinstance(value["timed_out"], bool)
        and isinstance(value["cancelled"], bool),
        "invalid timeout or cancellation state",
    )
    _relative_path(value["worktree_path"], "worktree_path")
    _relative_path(value["result_path"], "result_path")
    _stream(value["stdout"], "stdout")
    _stream(value["stderr"], "stderr")

    changed = value["changed_files"]
    _require(isinstance(changed, list) and len(changed) <= 64, "invalid changed_files")
    paths: list[str] = []
    for item in changed:
        _fields(item, {"path", "change", "digest", "size"}, "changed file")
        _relative_path(item["path"], "changed file path")
        _require(
            item["change"] in {"added", "modified", "deleted", "type-changed"},
            "invalid changed file change",
        )
        if item["change"] == "deleted":
            _require(
                item["digest"] is None and item["size"] is None,
                "deleted changed file evidence must be null",
            )
        else:
            _digest(item["digest"], "changed file digest")
            _require(
                isinstance(item["size"], int)
                and not isinstance(item["size"], bool)
                and 0 <= item["size"] <= 1073741824,
                "invalid changed file size",
            )
        paths.append(item["path"])
    _require(paths == sorted(paths), "changed_files must be sorted by path")
    _require(len(paths) == len(set(paths)), "changed file paths must be unique")

    evidence_fields = (
        "caller_checkout_preserved",
        "detached_worktree",
        "base_commit_preserved",
        "git_metadata_preserved",
        "submodules_preserved",
    )
    _require(
        all(isinstance(value[field], bool) for field in evidence_fields),
        "invalid preservation evidence",
    )
    _require(
        value["retention_state"] in {"retained", "cleanup-pending", "safely-removed"},
        "invalid retention_state",
    )
    _require(
        isinstance(value["diagnostics"], list)
        and len(value["diagnostics"]) <= 32
        and all(
            isinstance(item, str) and 0 < len(item) <= 512
            for item in value["diagnostics"]
        )
        and redaction.is_safe(value["diagnostics"]),
        "unsafe diagnostics",
    )
    audit_fields = (
        "commit_attempted",
        "push_attempted",
        "pull_request_attempted",
        "merge_attempted",
    )
    _require(
        all(value[field] is False for field in audit_fields),
        "framework operation attestations must be false",
    )

    classification = value["exit_classification"]
    if classification == "succeeded":
        _require(
            value["execution_state"] == "completed"
            and value["process_exit_code"] == 0
            and value["timed_out"] is False
            and value["cancelled"] is False
            and value["stdout"]["truncated"] is False
            and value["stderr"]["truncated"] is False
            and all(value[field] is True for field in evidence_fields),
            "invalid succeeded receipt",
        )
    if classification == "timed-out":
        _require(value["timed_out"] is True, "timed-out receipt must record timeout")
    if classification == "cancelled":
        _require(
            value["cancelled"] is True and value["execution_state"] == "cancelled",
            "cancelled receipt is inconsistent",
        )


def validate_fixture_handoff(value: Mapping[str, Any]) -> None:
    """Validate the bounded fixture handoff without executing it."""
    _fields(
        value,
        {
            "schema",
            "schema_version",
            "execution_id",
            "task",
            "role",
            "route",
            "base_commit",
            "operation",
            "output_path",
            "expected_content",
            "expected_digest",
        },
        "fixture handoff",
    )
    _require(
        value["schema"] == "evolvehls.agentic.fixture-handoff"
        and value["schema_version"] == "1.0",
        "unsupported fixture handoff schema",
    )
    _identifier(value["execution_id"], "execution_id")
    _commit(value["base_commit"])

    _fields(
        value["task"],
        {
            "task_id",
            "version",
            "objective",
            "inputs",
            "constraints",
            "expected_outputs",
            "validation_requirements",
        },
        "handoff task",
    )
    _identifier(value["task"]["task_id"], "task.task_id")
    _bounded_string(value["task"]["version"], "task.version", 64)
    _bounded_string(value["task"]["objective"], "task.objective", 512)
    for field in (
        "inputs",
        "constraints",
        "expected_outputs",
        "validation_requirements",
    ):
        _string_list(value["task"][field], f"task.{field}")

    _fields(
        value["role"],
        {"role_id", "version", "mandatory_capabilities"},
        "handoff role",
    )
    _identifier(value["role"]["role_id"], "role.role_id")
    _bounded_string(value["role"]["version"], "role.version", 64)
    _string_list(value["role"]["mandatory_capabilities"], "role.mandatory_capabilities")

    _fields(
        value["route"],
        {"profile_id", "adapter_id", "runtime_map_id", "runtime_entry_id"},
        "handoff route",
    )
    for field in value["route"]:
        _identifier(value["route"][field], f"route.{field}")

    _require(
        value["operation"] == "write-fixture-execution-record-v1"
        and value["output_path"] == OUTPUT_PATH,
        "unsupported fixture handoff operation or output",
    )
    _relative_path(value["output_path"], "output_path")
    _require(
        isinstance(value["expected_content"], str)
        and 0 < len(value["expected_content"].encode("utf-8")) <= 4096,
        "invalid expected_content",
    )
    _digest(value["expected_digest"], "expected_digest")


def validate_execution_request_context(
    request: Mapping[str, Any],
    task: Mapping[str, Any],
    role: Mapping[str, Any],
    decision: Mapping[str, Any],
    descriptor: Mapping[str, Any],
    runtime_map: Mapping[str, Any],
) -> None:
    """Validate immutable request bindings without re-resolving the route."""
    validate_execution_request(request)
    try:
        contracts.validate_task(task)
        contracts.validate_role(role)
        contracts.validate_decision(decision)
    except contracts.ContractError as error:
        raise ExecutionContractError(str(error)) from None

    _require(
        task["role"] == role["role_id"]
        and request["task_id"] == task["task_id"]
        and request["task_version"] == task["version"]
        and request["task_digest"] == local_state.canonical_digest(task),
        "task execution-request context mismatch",
    )
    _require(
        request["role_id"] == role["role_id"]
        and request["role_version"] == role["version"]
        and request["role_digest"] == local_state.canonical_digest(role),
        "role execution-request context mismatch",
    )
    _require(
        request["base_commit"] == task["reproducibility"]["base_revision"],
        "execution-request base revision mismatch",
    )
    _require(
        decision["task_id"] == task["task_id"]
        and decision["role_id"] == role["role_id"]
        and decision["selected"]["profile_id"] == "fixture-local-profile"
        and decision["selected"]["adapter_id"] == "fixture-local-adapter"
        and decision["fallback_authorized"] is False,
        "fixture routing decision mismatch or fallback",
    )

    _require(
        runtime_map.get("runtime_map_id") == request["runtime_map_id"]
        and runtime_map.get("version") == request["runtime_map_version"],
        "runtime-map execution-request context mismatch",
    )
    entries = [
        item
        for item in runtime_map.get("entries", [])
        if item.get("runtime_entry_id") == request["runtime_entry_id"]
    ]
    _require(len(entries) == 1, "runtime entry execution-request context mismatch")
    entry = entries[0]
    _require(
        entry["profile_id"] == request["profile_id"]
        and entry["adapter_id"] == request["adapter_id"],
        "runtime entry route mismatch",
    )

    for field in (
        "profile_id",
        "adapter_id",
        "runtime_map_id",
        "runtime_map_version",
        "runtime_entry_id",
    ):
        _require(
            descriptor.get(field) == request[field],
            f"descriptor {field} mismatch",
        )
    _require(
        descriptor.get("protocol") == decision["selected"]["protocol"]
        and descriptor.get("profile_id") == decision["selected"]["profile_id"]
        and descriptor.get("adapter_id") == decision["selected"]["adapter_id"],
        "descriptor canonical route mismatch",
    )
    _require(
        request["invocation_descriptor_digest"]
        == local_state.canonical_digest(descriptor),
        "invocation descriptor digest mismatch",
    )
    _require(
        request["routing_provenance"] == descriptor.get("routing_provenance"),
        "routing provenance mismatch",
    )


def result_execution_identity(execution_id: str, request_digest: str) -> str:
    """Return the portable result 1.0 execution-plan identity."""
    _identifier(execution_id, "execution_id")
    _digest(request_digest, "request_digest")
    return f"execution-request:{execution_id}:sha256:{request_digest}"


def validate_result_execution_identity(
    result: Mapping[str, Any],
    execution_id: str,
    request_digest: str,
) -> None:
    """Validate a result 1.0 document and its immutable request provenance."""
    try:
        contracts.validate_result(result)
    except contracts.ContractError as error:
        raise ExecutionContractError(str(error)) from None
    expected = result_execution_identity(execution_id, request_digest)
    _require(
        result["execution_plan"] == expected
        and RESULT_IDENTITY_PATTERN.fullmatch(result["execution_plan"]) is not None,
        "result execution identity mismatch",
    )