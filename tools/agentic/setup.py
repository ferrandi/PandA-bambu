#!/usr/bin/env python3
"""Non-executing PAF-03B setup and repository-local configuration generation."""
from __future__ import annotations

import hashlib
import json
from pathlib import Path
from typing import Any, Mapping

import adapters
import local_state
import portable_adapters

SETUP_SCHEMA = "evolvehls.agentic.portable-setup-spec"


class SetupError(ValueError):
    """Raised for invalid setup input or unsafe setup behavior."""


def _canonical(value: Mapping[str, Any]) -> bytes:
    return json.dumps(value, sort_keys=True, separators=(",", ":"), ensure_ascii=True).encode("utf-8")


def validate_spec(spec: Mapping[str, Any]) -> None:
    expected = {"schema", "schema_version", "registry", "authorized_probes"}
    if set(spec) != expected or spec["schema"] != SETUP_SCHEMA or spec["schema_version"] != "1.0":
        raise SetupError("invalid setup specification")
    if not isinstance(spec["authorized_probes"], bool):
        raise SetupError("invalid probe authorization")
    portable_adapters.validate_registry(spec["registry"])


def _documents(spec: Mapping[str, Any], detected: list[Mapping[str, Any]]) -> list[local_state.PlannedWrite]:
    registry = spec["registry"]
    report = portable_adapters.readiness(registry, detected, "2026-01-01T00:00:00+00:00")
    descriptor_docs = {
        profile["profile_id"]: portable_adapters.invocation_descriptor(registry, profile["profile_id"], report)
        for profile in registry["profiles"]
    }
    plans = [
        local_state.PlannedWrite(local_state.LOCAL_DIR, ("setup", "registry"), local_state.canonical_json(registry)),
        local_state.PlannedWrite(local_state.STATE_DIR, ("readiness", registry["registry_id"]), local_state.canonical_json(report)),
    ]
    for profile_id, descriptor in sorted(descriptor_docs.items()):
        plans.append(local_state.PlannedWrite(local_state.LOCAL_DIR, ("generated", profile_id), local_state.canonical_json(descriptor)))
    receipt = {
        "schema": "evolvehls.agentic.portable-setup-receipt",
        "schema_version": "1.0",
        "registry_id": registry["registry_id"],
        "registry_digest": hashlib.sha256(_canonical(registry)).hexdigest(),
        "authorized_probes": spec["authorized_probes"],
        "writes": len(plans),
    }
    plans.append(local_state.PlannedWrite(local_state.STATE_DIR, ("setup", registry["registry_id"]), local_state.canonical_json(receipt)))
    return plans


def preview(repository: Path, spec: Mapping[str, Any], detected: list[Mapping[str, Any]] | None = None) -> dict[str, Any]:
    validate_spec(spec)
    observed = detected if detected is not None else adapters.detect_all()
    return {
        "registry_id": spec["registry"]["registry_id"],
        "dry_run": True,
        "probes": "not-run" if spec["authorized_probes"] else "not-authorized",
        "writes": local_state.preview(repository, _documents(spec, observed)),
    }


def apply(
    repository: Path,
    spec: Mapping[str, Any],
    *,
    detected: list[Mapping[str, Any]] | None = None,
    dry_run: bool = False,
    replace: bool = False,
) -> dict[str, Any]:
    validate_spec(spec)
    observed = detected if detected is not None else adapters.detect_all()
    planned = _documents(spec, observed)
    if dry_run:
        return preview(repository, spec, observed)
    written = local_state.commit(repository, planned, replace=replace)
    return {
        "registry_id": spec["registry"]["registry_id"],
        "dry_run": False,
        "changed": [str(path.relative_to(repository.resolve())) for path in written],
        "idempotent": not written,
    }


def config_preview(repository: Path, spec: Mapping[str, Any], detected: list[Mapping[str, Any]] | None = None) -> list[dict[str, object]]:
    validate_spec(spec)
    observed = detected if detected is not None else adapters.detect_all()
    plans = [item for item in _documents(spec, observed) if item.parts[0] == "generated"]
    return local_state.preview(repository, plans)


def config_generate(repository: Path, spec: Mapping[str, Any], *, detected: list[Mapping[str, Any]] | None = None, dry_run: bool = False, replace: bool = False) -> list[str]:
    validate_spec(spec)
    observed = detected if detected is not None else adapters.detect_all()
    plans = [item for item in _documents(spec, observed) if item.parts[0] == "generated"]
    paths = local_state.commit(repository, plans, replace=replace, dry_run=dry_run)
    return [str(path.relative_to(repository.resolve())) for path in paths]