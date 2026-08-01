#!/usr/bin/env python3
"""Deterministic PAF-02 catalog, role query, and persisted-selection services."""
from __future__ import annotations

import copy
import hashlib
import json
import os
import tempfile
from datetime import datetime, timezone
from pathlib import Path
from typing import Any, Mapping

import contracts
import discovery
import resolver

UNIT_FIELDS = {"client", "provider", "model", "protocol", "effort"}
CONFIDENCE_ORDER = {"unknown": 0, "declared": 1, "inferred": 2, "observed": 3, "historically-validated": 4}


class CatalogError(ValueError):
    """Raised when catalog assembly, query, or persistence is invalid."""


def _canonical(value: Any) -> bytes:
    return json.dumps(value, sort_keys=True, separators=(",", ":"), ensure_ascii=True).encode("utf-8")


def _atomic_json(path: Path, value: Mapping[str, Any]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.NamedTemporaryFile("w", encoding="utf-8", dir=path.parent, delete=False) as handle:
        json.dump(value, handle, sort_keys=True, indent=2)
        handle.write("\n")
        temporary = Path(handle.name)
    os.replace(temporary, path)


def _unit_valid(unit: Any, model_id: str) -> bool:
    return isinstance(unit, Mapping) and set(unit) == UNIT_FIELDS and unit.get("client") in {"codex", "claude-code", "cline"} and unit.get("protocol") in {"openai-responses", "anthropic-messages", "openai-chat-completions"} and unit.get("model") == model_id and all(isinstance(unit.get(key), str) and unit[key] for key in UNIT_FIELDS)


def _evidence(status: str, confidence: str, source: str) -> dict[str, Any]:
    return {
        "status": status,
        "confidence": confidence,
        "provenance": [{"source": source, "confidence": confidence, "observed_at": None, "expires_at": None}],
    }


def apply_overlay(models: list[Mapping[str, Any]], profile_id: str, overlay: Mapping[str, Any] | None = None) -> list[dict[str, Any]]:
    if overlay is not None:
        contracts.validate_overlay(overlay)
        if overlay["profile_id"] != profile_id:
            raise CatalogError("overlay profile_id does not match catalog profile")
        rules = {item["model_id"]: item for item in overlay["rules"]}
    else:
        rules = {}
    output: list[dict[str, Any]] = []
    for raw in models:
        model = copy.deepcopy(dict(raw))
        model_id = model.get("model_id")
        if not isinstance(model_id, str) or not model_id:
            raise CatalogError("discovery returned an invalid model identifier")
        rule = rules.get(model_id)
        if rule is None:
            model["eligible"] = False
            model["rejection_reasons"] = ["awaiting local policy overlay"]
            model["execution_units"] = []
        else:
            units = rule["execution_units"]
            if not all(_unit_valid(unit, model_id) for unit in units):
                raise CatalogError("overlay contains an invalid execution unit")
            model["eligible"] = rule["eligible"]
            model["rejection_reasons"] = list(rule["rejection_reasons"])
            if model["eligible"] and model["rejection_reasons"]:
                raise CatalogError("eligible overlay rule cannot include rejection reasons")
            if not model["eligible"] and not model["rejection_reasons"]:
                model["rejection_reasons"] = ["local policy marks candidate ineligible"]
            model["execution_units"] = copy.deepcopy(units)
            model["metadata"] = {**model.get("metadata", {}), **rule["metadata"]}
            capabilities = dict(model.get("capabilities", {}))
            for name, declared in rule["capabilities"].items():
                capabilities[name] = copy.deepcopy(declared)
                for provenance in capabilities[name]["provenance"]:
                    provenance["source"] = "policy-overlay"
            model["capabilities"] = capabilities
        output.append(model)
    unknown_rules = sorted(set(rules) - {item["model_id"] for item in output})
    if unknown_rules:
        raise CatalogError("overlay references undiscovered model identifiers: " + ", ".join(unknown_rules))
    return sorted(output, key=lambda item: item["model_id"])


def synchronize(profile: Mapping[str, Any], endpoint: str, headers: Mapping[str, str], transport=discovery.urllib_get, imported: Mapping[str, Any] | None = None, overlay: Mapping[str, Any] | None = None, created_at: str | None = None) -> dict[str, Any]:
    result = discovery.discover(endpoint, headers, profile["discovery"]["methods"], profile["timeout"]["seconds"], transport, imported)
    models = apply_overlay(list(result.models), profile["profile_id"], overlay)
    body = {
        "schema": "evolvehls.agentic.catalog",
        "schema_version": "1.0",
        "profile_id": profile["profile_id"],
        "snapshot_id": "",
        "created_at": created_at or datetime.now(timezone.utc).isoformat(),
        "models": models,
    }
    identity = {key: value for key, value in body.items() if key not in {"snapshot_id", "created_at"}}
    body["snapshot_id"] = hashlib.sha256(_canonical(identity)).hexdigest()
    return body | {"discovery": {"adapter": result.adapter, "attempted": list(result.attempted), "requires_model_id": result.requires_model_id, "diagnostics": list(result.diagnostics)}}


def persist_catalog(catalog: Mapping[str, Any], directory: Path) -> Path:
    required = {"schema", "schema_version", "profile_id", "snapshot_id", "created_at", "models"}
    if not isinstance(catalog, Mapping) or not required <= set(catalog):
        raise CatalogError("catalog does not match the portable catalog contract")
    path = directory / f"{catalog['snapshot_id']}.json"
    _atomic_json(path, {key: catalog[key] for key in required})
    return path


def query(catalog: Mapping[str, Any], role: Mapping[str, Any]) -> dict[str, Any]:
    contracts.validate_role(role)
    candidates = []
    minimum = CONFIDENCE_ORDER[role["minimum_confidence"]]
    for model in catalog.get("models", []):
        capabilities = model.get("capabilities", {})
        missing, probe_needed = [], []
        for name in role["mandatory_capabilities"]:
            evidence = capabilities.get(name)
            if not isinstance(evidence, Mapping) or evidence.get("status") != "supported":
                missing.append(name)
            elif CONFIDENCE_ORDER.get(evidence.get("confidence"), -1) < minimum:
                probe_needed.append(name)
        candidates.append({
            "model_id": model.get("model_id"),
            "eligible": bool(model.get("eligible")),
            "rejection_reasons": list(model.get("rejection_reasons", [])),
            "mandatory_unavailable": missing,
            "probe_needed": probe_needed,
            "capabilities": copy.deepcopy(capabilities),
        })
    return {"role_id": role["role_id"], "role_version": role["version"], "catalog_snapshot": catalog.get("snapshot_id"), "candidates": candidates}


def select(catalog: Mapping[str, Any], role: Mapping[str, Any], objective: str | None = None, mode: str = "development", override: Mapping[str, Any] | None = None, pins: Mapping[str, Any] | None = None) -> dict[str, Any]:
    contracts.validate_role(role)
    if mode not in role["permitted_modes"]:
        raise CatalogError("role does not permit requested resolver mode")
    plan = resolver.resolve(list(catalog.get("models", [])), role["role_id"], objective or role["default_objective"], set(role["mandatory_capabilities"]), mode=mode, override=override, pins=pins)
    plan["explanation"].extend([
        f"catalog snapshot: {catalog['snapshot_id']}",
        f"role version: {role['version']}",
        f"minimum evidence confidence: {role['minimum_confidence']}",
    ])
    return plan


def persist_selection(plan: Mapping[str, Any], catalog: Mapping[str, Any], role: Mapping[str, Any], directory: Path, created_at: str | None = None) -> Path:
    contracts.validate_role(role)
    created = created_at or datetime.now(timezone.utc).isoformat()
    identity = _canonical({"catalog_snapshot": catalog["snapshot_id"], "role_id": role["role_id"], "role_version": role["version"], "execution_plan": plan})
    selection = {
        "schema": "evolvehls.agentic.selection",
        "schema_version": "1.0",
        "selection_id": hashlib.sha256(identity).hexdigest(),
        "created_at": created,
        "catalog_snapshot": catalog["snapshot_id"],
        "role_id": role["role_id"],
        "role_version": role["version"],
        "execution_plan": dict(plan),
    }
    contracts.validate_selection(selection)
    path = directory / f"{selection['selection_id']}.json"
    _atomic_json(path, selection)
    return path


def latest_selection(directory: Path) -> dict[str, Any]:
    values = []
    for path in directory.glob("*.json"):
        try:
            value = contracts.load_contract(path, "selection")
        except contracts.ContractError:
            continue
        values.append(value)
    if not values:
        raise CatalogError("no valid persisted selection exists")
    return sorted(values, key=lambda item: (item["created_at"], item["selection_id"]))[-1]