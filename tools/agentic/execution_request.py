#!/usr/bin/env python3
"""Non-executing construction of the fixture-only PAF-05 execution request."""
from __future__ import annotations

import copy
from collections.abc import Mapping
from typing import Any

import contracts
import execution_contracts
import local_state
import portable_adapters
import routing

FIXTURE_PROFILE_ID = "fixture-local-profile"
FIXTURE_ADAPTER_ID = "fixture-local-adapter"

def fixture_templates() -> dict[str, dict[str, Any]]:
    """Return fresh trusted fixture metadata; callers cannot mutate shared state."""
    return {
        "fixture-local": {
            "command": None,
            "execution_family": "fixture-local",
            "paths": {
                "fixture": {
                    "invocation_class": "native-local-cli",
                    "access_classes": ["native-local-client"],
                    "funding_classes": ["local"],
                    "auth_modes": ["none"],
                    "protocols": ["openai-chat-completions"],
                    "capabilities": [
                        "working_directory",
                        "noninteractive_mode",
                        "timeout",
                        "machine_readable_result",
                    ],
                }
            },
            "versions": {"builtin": {"fixture"}},
        }
    }


class ExecutionRequestError(ValueError):
    """Raised when fixture request construction cannot remain canonical."""


def _require(condition: bool, message: str) -> None:
    if not condition:
        raise ExecutionRequestError(message)


def build_fixture_execution_request(
    task: Mapping[str, Any],
    role: Mapping[str, Any],
    registry: Mapping[str, Any],
    policy: Mapping[str, Any],
    readiness: Mapping[str, Any],
    runtime_map: Mapping[str, Any],
    *,
    execution_id: str,
    base_commit: str,
    created_at: str,
    wall_clock_timeout_seconds: int = 30,
    stdout_limit_bytes: int = 65536,
    stderr_limit_bytes: int = 65536,
    total_output_limit_bytes: int = 131072,
) -> dict[str, Any]:
    """Build one isolated fixture request; perform no persistence or execution."""
    task = copy.deepcopy(task)
    role = copy.deepcopy(role)
    registry = copy.deepcopy(registry)
    policy = copy.deepcopy(policy)
    readiness = copy.deepcopy(readiness)
    runtime_map = copy.deepcopy(runtime_map)
    templates = fixture_templates()
    try:
        contracts.validate_task(task)
        contracts.validate_role(role)
        contracts.validate_registry(registry)
        contracts.validate_policy(policy)
        contracts.validate_readiness(readiness, registry)
        portable_adapters.validate_runtime_map(
            runtime_map,
            registry,
            templates=templates,
        )
    except (contracts.ContractError, portable_adapters.PortableAdapterError) as error:
        raise ExecutionRequestError(str(error)) from None

    _require(task["role"] == role["role_id"], "fixture task role mismatch")
    _require(
        task["reproducibility"]["base_revision"] == base_commit,
        "fixture base commit does not match task reproducibility",
    )
    _require(
        registry["registry_id"] == "fixture-execution-registry"
        and policy["policy_id"] == "fixture-execution-policy",
        "fixture request requires explicit fixture registry and policy",
    )

    try:
        decision = routing.resolve(
            task,
            role,
            registry,
            policy,
            readiness,
            mode="development",
            required_profile_id=FIXTURE_PROFILE_ID,
        )
        contracts.validate_decision(decision)
    except (routing.RoutingError, contracts.ContractError, ValueError) as error:
        raise ExecutionRequestError(str(error)) from None
    _require(
        decision["selected"]["profile_id"] == FIXTURE_PROFILE_ID
        and decision["selected"]["adapter_id"] == FIXTURE_ADAPTER_ID
        and decision["fallback_authorized"] is False,
        "canonical routing did not select the fixture-only route",
    )

    try:
        descriptor = portable_adapters.invocation_descriptor(
            registry,
            decision,
            readiness,
            runtime_map,
            templates=templates,
        )
    except portable_adapters.PortableAdapterError as error:
        raise ExecutionRequestError(str(error)) from None

    request = {
        "schema": "evolvehls.agentic.execution-request",
        "schema_version": "1.0",
        "execution_id": execution_id,
        "created_at": created_at,
        "task_id": task["task_id"],
        "task_version": task["version"],
        "task_digest": local_state.canonical_digest(
            task, domain="evolvehls.agentic.task"
        ),
        "role_id": role["role_id"],
        "role_version": role["version"],
        "role_digest": local_state.canonical_digest(
            role, domain="evolvehls.agentic.role"
        ),
        "base_commit": base_commit,
        "profile_id": descriptor["profile_id"],
        "adapter_id": descriptor["adapter_id"],
        "runtime_map_id": descriptor["runtime_map_id"],
        "runtime_map_version": descriptor["runtime_map_version"],
        "runtime_entry_id": descriptor["runtime_entry_id"],
        "invocation_descriptor_digest": local_state.canonical_digest(
            descriptor, domain="evolvehls.agentic.invocation-descriptor"
        ),
        "routing_provenance": copy.deepcopy(descriptor["routing_provenance"]),
        "executor_id": "fixture-local",
        "executor_version": "1.0",
        "input_handoff": "fixture-json-file-v1",
        "working_directory_policy": "detached-worktree-root",
        "result_collection": "fixture-jsonl-and-artifact-v1",
        "wall_clock_timeout_seconds": wall_clock_timeout_seconds,
        "stdout_limit_bytes": stdout_limit_bytes,
        "stderr_limit_bytes": stderr_limit_bytes,
        "total_output_limit_bytes": total_output_limit_bytes,
        "retention_policy": "retain-for-inspection",
        "authorization_state": "previewed-requires-explicit-confirmation",
        "approved_operation": "write-fixture-execution-record-v1",
        "approved_output_path": execution_contracts.OUTPUT_PATH,
    }
    try:
        execution_contracts.validate_execution_request(request)
        execution_contracts.validate_execution_request_context(
            request,
            task,
            role,
            decision,
            descriptor,
            runtime_map,
        )
    except execution_contracts.ExecutionContractError as error:
        raise ExecutionRequestError(str(error)) from None
    return request