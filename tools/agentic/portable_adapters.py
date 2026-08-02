#!/usr/bin/env python3
"""PAF-03B runtime supplements and PAF-03A routed descriptors."""
from __future__ import annotations

from datetime import datetime, timedelta, timezone
import re
from typing import Any, Callable, Mapping

import adapters
import contracts
import redaction


_IDENTIFIER = re.compile(r"^[a-z][a-z0-9.-]*$")


class PortableAdapterError(ValueError):
    """Raised when portable runtime metadata cannot safely supplement PAF-03A."""


def _require(condition: bool, message: str) -> None:
    if not condition:
        raise PortableAdapterError(message)


def _id(value: object, field: str) -> str:
    _require(isinstance(value, str) and _IDENTIFIER.fullmatch(value) is not None, f"invalid {field}")
    return value


def _template_string_list(value: object, field: str) -> list[str]:
    _require(
        isinstance(value, list)
        and all(isinstance(item, str) and item for item in value)
        and len(value) == len(set(value)),
        f"invalid template {field}",
    )
    return value


def _template_path(
    entry: Mapping[str, Any],
    templates: Mapping[str, Mapping[str, Any]] | None = None,
    adapter: Mapping[str, Any] | None = None,
) -> Mapping[str, Any]:
    available = templates if templates is not None else adapters.TEMPLATES
    _require(isinstance(available, Mapping), "templates must be a mapping")
    template_name = entry["client_template"]
    _require(template_name in available, "unknown client template")
    template = available[template_name]
    _require(isinstance(template, Mapping), "client template must be a mapping")
    execution_family = template.get("execution_family")
    _require(
        isinstance(execution_family, str) and execution_family,
        "invalid template execution_family",
    )
    paths = template.get("paths")
    _require(isinstance(paths, Mapping), "template paths must be a mapping")
    path = paths.get(entry["execution_path"])
    _require(isinstance(path, Mapping), "unknown or invalid client execution path")
    for field in ("access_classes", "funding_classes", "auth_modes", "protocols"):
        _template_string_list(path.get(field), field)
    _require(
        isinstance(path.get("invocation_class"), str) and path["invocation_class"],
        "invalid template invocation_class",
    )
    if adapter is not None:
        _require(
            execution_family == adapter["execution_family"],
            "template execution family does not match canonical adapter",
        )
        _require(
            path["invocation_class"] == adapter["invocation_class"],
            "template invocation class does not match canonical adapter",
        )
    return path


def validate_runtime_map(
    value: Mapping[str, Any],
    registry: Mapping[str, Any],
    *,
    templates: Mapping[str, Mapping[str, Any]] | None = None,
) -> None:
    """Validate a non-routing runtime supplement against canonical PAF-03A IDs."""
    try:
        contracts.validate_registry(registry)
    except contracts.ContractError as error:
        raise PortableAdapterError(str(error)) from None
    expected = {"schema", "schema_version", "runtime_map_id", "version", "entries"}
    _require(
        set(value) == expected
        and value["schema"] == "evolvehls.agentic.client-runtime-map"
        and value["schema_version"] == "1.0",
        "invalid client runtime map",
    )
    _id(value["runtime_map_id"], "runtime_map_id")
    _require(isinstance(value["version"], str) and value["version"].strip(), "invalid runtime-map version")
    _require(isinstance(value["entries"], list) and value["entries"], "runtime-map entries required")
    canonical_profiles = {item["profile_id"]: item for item in registry["profiles"]}
    canonical_adapters = {item["adapter_id"]: item for item in registry["adapters"]}
    entry_ids: set[str] = set()
    matches: set[tuple[str, str]] = set()
    for entry in value["entries"]:
        expected_entry = {
            "runtime_entry_id",
            "profile_id",
            "adapter_id",
            "client_template",
            "execution_path",
        }
        _require(isinstance(entry, Mapping) and set(entry) == expected_entry, "invalid runtime-map entry")
        entry_id = _id(entry["runtime_entry_id"], "runtime_entry_id")
        _require(entry_id not in entry_ids, "duplicate runtime entry identifiers")
        entry_ids.add(entry_id)
        profile_id = _id(entry["profile_id"], "profile_id")
        adapter_id = _id(entry["adapter_id"], "adapter_id")
        profile = canonical_profiles.get(profile_id)
        _require(profile is not None, "runtime entry references unknown canonical profile")
        _require(adapter_id in canonical_adapters, "runtime entry references unknown canonical adapter")
        _require(profile["adapter_id"] == adapter_id, "runtime entry adapter does not match canonical profile")
        adapter = canonical_adapters[adapter_id]
        path = _template_path(
            entry, templates, adapter if templates is not None else None
        )
        _require(profile["access_class"] in path["access_classes"], "runtime path does not support canonical access class")
        _require(profile["funding_class"] in path["funding_classes"], "runtime path does not support canonical funding class")
        _require(profile["auth_mode"] in path["auth_modes"], "runtime path does not support canonical auth mode")
        _require(profile["protocol"] in path["protocols"], "runtime path does not support canonical protocol")
        key = (profile_id, entry["execution_path"])
        _require(key not in matches, "duplicate runtime entry for canonical profile and execution path")
        matches.add(key)


def _detection_by_template(
    runtime_map: Mapping[str, Any], detected: list[Mapping[str, Any]]
) -> dict[str, Mapping[str, Any]]:
    known = {entry["client_template"] for entry in runtime_map["entries"]}
    result = {item.get("adapter_id"): item for item in detected if item.get("adapter_id") in known}
    return result


def readiness(
    registry: Mapping[str, Any],
    runtime_map: Mapping[str, Any],
    detected: list[Mapping[str, Any]],
    checked_at: str | None = None,
    *,
    clock: Callable[[], datetime] | None = None,
) -> dict[str, Any]:
    """Translate bounded client detection into the sole canonical readiness shape."""
    validate_runtime_map(runtime_map, registry)
    if checked_at is None:
        instant = (clock or (lambda: datetime.now(timezone.utc)))()
        _require(instant.tzinfo is not None and instant.utcoffset() is not None, "clock must return timezone-aware UTC time")
        checked_at = instant.astimezone(timezone.utc).isoformat()
    try:
        contracts.parse_timestamp(checked_at, "checked_at")
    except contracts.ContractError as error:
        raise PortableAdapterError(str(error)) from None

    detected_by_template = _detection_by_template(runtime_map, detected)
    profile_states: dict[str, str] = {}
    adapter_states: dict[str, list[str]] = {}
    diagnostics: list[str] = []
    for entry in runtime_map["entries"]:
        observation = detected_by_template.get(entry["client_template"], {})
        diagnostics.extend(observation.get("diagnostics", []))
        path = next(
            (
                item
                for item in observation.get("paths", [])
                if item.get("execution_path_id") == entry["execution_path"]
            ),
            None,
        )
        state = path.get("state") if isinstance(path, Mapping) else "unknown"
        canonical_state = "ready" if state == "authenticated-or-ready" else "unavailable" if state == "unavailable" else "unknown"
        profile_states[entry["profile_id"]] = canonical_state
        adapter_states.setdefault(entry["adapter_id"], []).append(canonical_state)

    report = {
        "schema": "evolvehls.agentic.readiness-report",
        "schema_version": "1.0",
        "registry_id": registry["registry_id"],
        "checked_at": checked_at,
        "redacted": True,
        "adapters": [
            {
                "ref": adapter["adapter_id"],
                "status": "ready" if "ready" in adapter_states.get(adapter["adapter_id"], []) else "unavailable" if adapter_states.get(adapter["adapter_id"]) and all(state == "unavailable" for state in adapter_states[adapter["adapter_id"]]) else "unknown",
            }
            for adapter in sorted(registry["adapters"], key=lambda item: item["adapter_id"])
        ],
        "profiles": [
            {"ref": profile["profile_id"], "status": profile_states.get(profile["profile_id"], "unknown")}
            for profile in sorted(registry["profiles"], key=lambda item: item["profile_id"])
        ],
        "resources": [],
        "diagnostics": redaction.safe_diagnostics(diagnostics),
    }
    try:
        contracts.validate_readiness(report, registry)
    except contracts.ContractError as error:
        raise PortableAdapterError(str(error)) from None
    return report


def invocation_descriptor(
    registry: Mapping[str, Any],
    decision: Mapping[str, Any],
    readiness_report: Mapping[str, Any],
    runtime_map: Mapping[str, Any],
    *,
    now: datetime | None = None,
    max_readiness_age: timedelta | None = None,
    composition_provenance: Mapping[str, Any] | None = None,
    templates: Mapping[str, Mapping[str, Any]] | None = None,
) -> dict[str, Any]:
    """Build a future PAF-05 descriptor directly from a PAF-03A decision."""
    try:
        contracts.validate_registry(registry)
        contracts.validate_decision(decision)
        contracts.validate_readiness(readiness_report, registry)
    except contracts.ContractError as error:
        raise PortableAdapterError(str(error)) from None
    validate_runtime_map(runtime_map, registry, templates=templates)
    _require(
        decision["registry_id"] == registry["registry_id"] and decision["registry_version"] == registry["version"],
        "routing decision does not match canonical registry",
    )
    selected = decision["selected"]
    profile = next((item for item in registry["profiles"] if item["profile_id"] == selected["profile_id"]), None)
    _require(profile is not None, "selected canonical profile is missing")
    adapter = next((item for item in registry["adapters"] if item["adapter_id"] == profile["adapter_id"]), None)
    _require(adapter is not None, "selected canonical adapter is missing")
    _require(selected["adapter_id"] == adapter["adapter_id"], "routing decision adapter does not match canonical profile")
    for field in ("access_class", "funding_class", "auth_mode", "protocol"):
        _require(selected[field] == profile[field], "routing decision does not match canonical profile")

    profile_readiness = next((item["status"] for item in readiness_report["profiles"] if item["ref"] == profile["profile_id"]), None)
    adapter_readiness = next((item["status"] for item in readiness_report["adapters"] if item["ref"] == adapter["adapter_id"]), None)
    _require(profile_readiness is not None and adapter_readiness is not None, "readiness does not cover selected canonical profile and adapter")
    if max_readiness_age is not None:
        instant = now or datetime.now(timezone.utc)
        _require(instant.tzinfo is not None and instant.utcoffset() is not None, "now must be timezone-aware")
        observed = contracts.parse_timestamp(readiness_report["checked_at"], "checked_at")
        _require(observed <= instant and instant - observed <= max_readiness_age, "canonical readiness is stale or from the future")

    entries = [
        item
        for item in runtime_map["entries"]
        if item["profile_id"] == profile["profile_id"] and item["adapter_id"] == adapter["adapter_id"]
    ]
    _require(len(entries) == 1, "selected canonical profile has ambiguous or missing runtime metadata")
    entry = entries[0]
    path = _template_path(
        entry, templates, adapter if templates is not None else None
    )
    return {
        "descriptor_version": "1.0",
        "profile_id": profile["profile_id"],
        "adapter_id": adapter["adapter_id"],
        "runtime_map_id": runtime_map["runtime_map_id"],
        "runtime_map_version": runtime_map["version"],
        "runtime_entry_id": entry["runtime_entry_id"],
        "client_template": entry["client_template"],
        "execution_path": entry["execution_path"],
        "invocation_class": path["invocation_class"],
        "execution_family": adapter["execution_family"],
        "protocol": profile["protocol"],
        "binding": profile["binding"],
        "model": profile["model"],
        "readiness": {"profile": profile_readiness, "adapter": adapter_readiness, "checked_at": readiness_report["checked_at"]},
        "routing_provenance": {
            key: decision[key]
            for key in ("schema_version", "resolver_version", "registry_id", "registry_version", "policy_id", "policy_version", "mode", "task_id", "role_id")
        },
        "input_handoff": "PAF-05-defined",
        "working_directory_behavior": "PAF-05-defined",
        "result_collection": "PAF-05-defined",
        "capability_evidence": {"source": "static-version-capability-map"},
        "configuration_ownership": "framework-local-reference",
        **({"composition_provenance": dict(composition_provenance)} if composition_provenance is not None else {}),
    }
