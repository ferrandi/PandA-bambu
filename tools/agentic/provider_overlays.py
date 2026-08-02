#!/usr/bin/env python3
"""Deterministic composition of approved local provider overlays.

Effective documents are computed in memory.  They retain the built-in canonical
registry/runtime-map identities and are consumed directly by PAF-03A/03B.
"""
from __future__ import annotations

import copy
import hashlib
import json
from collections.abc import Mapping, Sequence
from typing import Any

import contracts
import portable_adapters


class OverlayError(ValueError):
    """Raised when a local overlay cannot safely extend canonical documents."""


def _canonical(value: Mapping[str, Any]) -> bytes:
    return (json.dumps(value, ensure_ascii=True, sort_keys=True, separators=(",", ":"))).encode("utf-8")


def digest(value: Mapping[str, Any]) -> str:
    return hashlib.sha256(_canonical(value)).hexdigest()


def _require(condition: bool, message: str) -> None:
    if not condition:
        raise OverlayError(message)


def _same(left: Mapping[str, Any], right: Mapping[str, Any]) -> bool:
    return _canonical(left) == _canonical(right)


def _sorted_documents(documents: Sequence[Mapping[str, Any]]) -> list[Mapping[str, Any]]:
    return sorted(
        documents,
        key=lambda value: (
            str(value.get("provider_id", "")),
            str(value.get("overlay_id", "")),
            digest(value),
        ),
    )


def _overlay_items(overlay: Mapping[str, Any], field: str) -> list[Mapping[str, Any]]:
    singular = "profile" if field == "profiles" else "entry"
    values = overlay.get(field)
    if values is None:
        values = [overlay[singular]]
    return [item for item in values if isinstance(item, Mapping)]


def compose(
    registry: Mapping[str, Any],
    runtime_map: Mapping[str, Any],
    profile_overlays: Sequence[Mapping[str, Any]] = (),
    runtime_overlays: Sequence[Mapping[str, Any]] = (),
) -> tuple[dict[str, Any], dict[str, Any], dict[str, Any]]:
    """Return validated canonical effective documents and a provenance sidecar.

    The supplied built-in documents are never modified.  Exact duplicate local
    entries are idempotent; every incompatible ID or semantic collision fails
    closed instead of using a last-writer-wins policy.
    """
    try:
        contracts.validate_registry(registry)
        portable_adapters.validate_runtime_map(runtime_map, registry)
    except (contracts.ContractError, portable_adapters.PortableAdapterError) as error:
        raise OverlayError(f"invalid built-in canonical document: {error}") from None

    profiles = _sorted_documents(profile_overlays)
    runtimes = _sorted_documents(runtime_overlays)
    for overlay in profiles:
        try:
            contracts.validate_provider_profile_overlay(overlay, registry)
        except contracts.ContractError as error:
            raise OverlayError(str(error)) from None
    for overlay in runtimes:
        try:
            contracts.validate_provider_runtime_overlay(overlay, registry, runtime_map)
        except contracts.ContractError as error:
            raise OverlayError(str(error)) from None

    effective_registry = copy.deepcopy(dict(registry))
    effective_runtime = copy.deepcopy(dict(runtime_map))
    profile_by_id = {item["profile_id"]: item for item in effective_registry["profiles"]}
    profile_source: dict[str, str] = {item["profile_id"]: "builtin" for item in effective_registry["profiles"]}
    for overlay in profiles:
        for raw_profile in _overlay_items(overlay, "profiles"):
            profile = copy.deepcopy(dict(raw_profile))
            profile_id = profile["profile_id"]
            existing = profile_by_id.get(profile_id)
            if existing is not None:
                if _same(existing, profile) and profile_source[profile_id] != "builtin":
                    continue
                raise OverlayError(f"profile identifier collision: {profile_id}")
            profile_by_id[profile_id] = profile
            profile_source[profile_id] = overlay["overlay_id"]

    effective_registry["profiles"] = [profile_by_id[key] for key in sorted(profile_by_id)]
    try:
        contracts.validate_registry(effective_registry)
    except contracts.ContractError as error:
        raise OverlayError(f"effective registry is invalid: {error}") from None
    adapter_protocols = {item["adapter_id"]: set(item["protocols"]) for item in effective_registry["adapters"]}
    for profile in effective_registry["profiles"]:
        _require(
            profile["protocol"] in adapter_protocols[profile["adapter_id"]],
            f"canonical adapter does not support profile protocol: {profile['profile_id']}",
        )

    runtime_by_id = {item["runtime_entry_id"]: item for item in effective_runtime["entries"]}
    runtime_source: dict[str, str] = {item["runtime_entry_id"]: "builtin" for item in effective_runtime["entries"]}
    pair_paths = {(item["profile_id"], item["execution_path"]): item for item in effective_runtime["entries"]}
    provider_profiles = {item["profile_id"] for item in effective_registry["profiles"] if profile_source.get(item["profile_id"]) != "builtin"}
    for overlay in runtimes:
        for raw_entry in _overlay_items(overlay, "entries"):
            entry = copy.deepcopy(dict(raw_entry))
            entry_id = entry["runtime_entry_id"]
            existing = runtime_by_id.get(entry_id)
            if existing is not None:
                if _same(existing, entry) and runtime_source[entry_id] != "builtin":
                    continue
                raise OverlayError(f"runtime entry identifier collision: {entry_id}")
            key = (entry["profile_id"], entry["execution_path"])
            collision = pair_paths.get(key)
            if collision is not None and not _same(collision, entry):
                raise OverlayError(
                    "runtime semantic collision for canonical profile and execution path: "
                    f"{entry['profile_id']} / {entry['execution_path']}"
                )
            _require(entry["profile_id"] in provider_profiles, "runtime overlay must reference a composed local provider profile")
            runtime_by_id[entry_id] = entry
            runtime_source[entry_id] = overlay["overlay_id"]
            pair_paths[key] = entry

    effective_runtime["entries"] = [runtime_by_id[key] for key in sorted(runtime_by_id)]
    try:
        portable_adapters.validate_runtime_map(effective_runtime, effective_registry)
    except portable_adapters.PortableAdapterError as error:
        raise OverlayError(f"effective runtime map is invalid: {error}") from None

    provenance = {
        "composition_version": "1.0",
        "materialized": False,
        "registry": {
            "registry_id": registry["registry_id"],
            "version": registry["version"],
            "source": "builtin",
            "digest": digest(registry),
        },
        "runtime_map": {
            "runtime_map_id": runtime_map["runtime_map_id"],
            "version": runtime_map["version"],
            "source": "builtin",
            "digest": digest(runtime_map),
        },
        "profile_overlays": [
            {"overlay_id": item["overlay_id"], "provider_id": item["provider_id"], "digest": digest(item)}
            for item in profiles
        ],
        "runtime_overlays": [
            {"overlay_id": item["overlay_id"], "provider_id": item["provider_id"], "digest": digest(item)}
            for item in runtimes
        ],
    }
    return effective_registry, effective_runtime, provenance