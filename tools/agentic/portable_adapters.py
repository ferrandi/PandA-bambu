#!/usr/bin/env python3
"""PAF-03B execution-path contracts, readiness, and PAF-05 descriptors."""
from __future__ import annotations

from datetime import datetime, timezone
from typing import Any, Mapping

import redaction

ACCESS = {"api-gateway", "native-account-client", "local-server", "native-local-client"}
FUNDING = {"project", "organization", "subscription", "personal-api", "local"}
AUTH = {"native-session", "environment-token", "token-helper", "none"}
PROTOCOLS = {"openai-responses", "anthropic-messages", "openai-chat-completions"}
READINESS = {
    "available",
    "authenticated-or-ready",
    "configuration-required",
    "unsupported-version",
    "execution-path-unsupported",
    "unavailable",
    "unknown",
}


class PortableAdapterError(ValueError):
    """Raised for a PAF-03B adapter contract violation."""


def _require(condition: bool, message: str) -> None:
    if not condition:
        raise PortableAdapterError(message)


def _id(value: object, field: str) -> str:
    _require(isinstance(value, str) and value and "/" not in value and "\\" not in value and ".." not in value, f"invalid {field}")
    return value


def _strings(value: object, field: str, allowed: set[str] | None = None) -> list[str]:
    _require(isinstance(value, list) and value and all(isinstance(item, str) and item for item in value), f"invalid {field}")
    _require(len(value) == len(set(value)), f"duplicate {field}")
    if allowed is not None:
        _require(set(value) <= allowed, f"unsupported {field}")
    return list(value)


def validate_adapter(value: Mapping[str, Any]) -> None:
    expected = {"schema", "schema_version", "adapter_id", "adapter_version", "execution_family", "execution_paths"}
    _require(set(value) == expected and value["schema"] == "evolvehls.agentic.portable-client-adapter" and value["schema_version"] == "1.0", "invalid adapter document")
    _id(value["adapter_id"], "adapter_id")
    _require(value["adapter_version"] is None or isinstance(value["adapter_version"], str), "invalid adapter_version")
    _id(value["execution_family"], "execution_family")
    _require(isinstance(value["execution_paths"], list) and value["execution_paths"], "execution paths required")
    path_ids = []
    for path in value["execution_paths"]:
        expected_path = {"path_id", "invocation_class", "access_classes", "funding_classes", "auth_modes", "protocols", "capabilities"}
        _require(isinstance(path, Mapping) and set(path) == expected_path, "invalid execution path")
        path_ids.append(_id(path["path_id"], "path_id"))
        _require(path["invocation_class"] in {"native-account-cli", "configured-api-cli", "configured-api-client", "http-api-client", "native-local-cli"}, "invalid invocation class")
        _strings(path["access_classes"], "access classes", ACCESS)
        _strings(path["funding_classes"], "funding classes", FUNDING)
        _strings(path["auth_modes"], "auth modes", AUTH)
        _strings(path["protocols"], "protocols", PROTOCOLS)
        _strings(path["capabilities"], "capabilities")
        if "native-account-client" in path["access_classes"]:
            _require(path["auth_modes"] == ["native-session"], "native execution may use native-session only")
    _require(len(path_ids) == len(set(path_ids)), "duplicate execution-path identifiers")


def validate_registry(value: Mapping[str, Any]) -> None:
    expected = {"schema", "schema_version", "registry_id", "version", "adapters", "profiles"}
    _require(set(value) == expected and value["schema"] == "evolvehls.agentic.portable-profile-registry" and value["schema_version"] == "1.0", "invalid registry document")
    _id(value["registry_id"], "registry_id"); _id(value["version"], "version")
    _require(isinstance(value["adapters"], list) and isinstance(value["profiles"], list), "registry lists required")
    adapters = {}
    for adapter in value["adapters"]:
        validate_adapter(adapter)
        _require(adapter["adapter_id"] not in adapters, "duplicate adapter identifiers")
        adapters[adapter["adapter_id"]] = adapter
    profile_ids = set()
    for profile in value["profiles"]:
        expected_profile = {"profile_id", "adapter_id", "execution_path_id", "access_class", "funding_class", "auth_mode", "protocol", "provider_or_runtime_binding", "model_binding", "capabilities", "priority"}
        _require(isinstance(profile, Mapping) and set(profile) == expected_profile, "invalid profile")
        _id(profile["profile_id"], "profile_id"); _require(profile["profile_id"] not in profile_ids, "duplicate profile identifiers"); profile_ids.add(profile["profile_id"])
        adapter = adapters.get(profile["adapter_id"]); _require(adapter is not None, "profile references unknown adapter")
        path = next((item for item in adapter["execution_paths"] if item["path_id"] == profile["execution_path_id"]), None)
        _require(path is not None, "profile references unknown execution path")
        _require(profile["access_class"] in path["access_classes"] and profile["funding_class"] in path["funding_classes"] and profile["auth_mode"] in path["auth_modes"] and profile["protocol"] in path["protocols"], "profile incompatible with execution path")
        _id(profile["provider_or_runtime_binding"], "provider_or_runtime_binding")
        _id(profile["model_binding"], "model_binding")
        _require(isinstance(profile["capabilities"], Mapping), "invalid profile capabilities")
        _require(isinstance(profile["priority"], int) and profile["priority"] >= 0, "invalid priority")


def readiness(registry: Mapping[str, Any], detected: list[Mapping[str, Any]], checked_at: str | None = None) -> dict[str, Any]:
    """Produce a normalized, genuinely redacted path-level readiness report."""
    validate_registry(registry)
    detections = {item["adapter_id"]: item for item in detected}
    adapters, paths, profiles, diagnostics = [], [], [], []
    for adapter in sorted(registry["adapters"], key=lambda item: item["adapter_id"]):
        found = detections.get(adapter["adapter_id"])
        adapter_state = "available" if found and found.get("available") else "unavailable"
        adapters.append({"ref": adapter["adapter_id"], "state": adapter_state})
        per_path = {(item.get("adapter_id"), item.get("execution_path_id")): item for item in (found or {}).get("paths", [])}
        for path in adapter["execution_paths"]:
            evidence = per_path.get((adapter["adapter_id"], path["path_id"]))
            paths.append({"adapter_id": adapter["adapter_id"], "execution_path_id": path["path_id"], "state": evidence.get("state", "unknown") if evidence else "unknown"})
        diagnostics.extend((found or {}).get("diagnostics", []))
    states = {(item["adapter_id"], item["execution_path_id"]): item["state"] for item in paths}
    for profile in sorted(registry["profiles"], key=lambda item: item["profile_id"]):
        profiles.append({"ref": profile["profile_id"], "state": states[(profile["adapter_id"], profile["execution_path_id"])]})
    return {
        "schema": "evolvehls.agentic.portable-readiness-report",
        "schema_version": "1.0",
        "registry_id": registry["registry_id"],
        "checked_at": checked_at or datetime.now(timezone.utc).isoformat(),
        "redacted": True,
        "adapters": adapters,
        "paths": paths,
        "profiles": profiles,
        "diagnostics": redaction.safe_diagnostics(diagnostics),
    }


def invocation_descriptor(registry: Mapping[str, Any], profile_id: str, report: Mapping[str, Any]) -> dict[str, Any]:
    """Generate a non-executing, normalized future PAF-05 handoff descriptor."""
    validate_registry(registry)
    profile = next((item for item in registry["profiles"] if item["profile_id"] == profile_id), None)
    _require(profile is not None, "unknown profile")
    adapter = next(item for item in registry["adapters"] if item["adapter_id"] == profile["adapter_id"])
    path = next(item for item in adapter["execution_paths"] if item["path_id"] == profile["execution_path_id"])
    ready = next((item["state"] for item in report["profiles"] if item["ref"] == profile_id), "unknown")
    return {
        "adapter_id": adapter["adapter_id"],
        "adapter_version": adapter["adapter_version"],
        "execution_path_id": path["path_id"],
        "invocation_class": path["invocation_class"],
        "execution_family": adapter["execution_family"],
        "supported_access_classes": sorted(path["access_classes"]),
        "supported_funding_classes": sorted(path["funding_classes"]),
        "supported_auth_modes": sorted(path["auth_modes"]),
        "supported_protocols": sorted(path["protocols"]),
        "provider_or_runtime_binding": profile["provider_or_runtime_binding"],
        "model_binding": profile["model_binding"],
        "input_handoff": "PAF-05-defined",
        "working_directory_behavior": "PAF-05-defined",
        "result_collection": "PAF-05-defined",
        "structured_output_support": "structured_output" in path["capabilities"],
        "resume_behavior": "supported" if "resume_session" in path["capabilities"] else "unsupported",
        "timeout_behavior": "supported" if "timeout" in path["capabilities"] else "unknown",
        "cancellation_behavior": "supported" if "cancellation" in path["capabilities"] else "unknown",
        "configuration_ownership": "framework-local-reference",
        "readiness": ready,
        "capability_evidence": {"source": "adapter-template-and-version"},
        "descriptor_version": "1.0",
    }