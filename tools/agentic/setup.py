#!/usr/bin/env python3
"""Non-executing PAF-03B setup and canonical descriptor generation."""
from __future__ import annotations

import hashlib
import json
from datetime import datetime, timezone
from pathlib import Path
from typing import Any, Callable, Mapping

import adapters
import contracts
import local_state
import portable_adapters

SETUP_SCHEMA = "evolvehls.agentic.portable-setup-spec"


class SetupError(ValueError):
    """Raised for invalid setup input or unsafe setup behavior."""


def _canonical(value: Mapping[str, Any]) -> bytes:
    return json.dumps(value, sort_keys=True, separators=(",", ":"), ensure_ascii=True).encode("utf-8")


def _checked_at(clock: Callable[[], datetime] | None) -> str:
    instant = (clock or (lambda: datetime.now(timezone.utc)))()
    if instant.tzinfo is None or instant.utcoffset() is None:
        raise SetupError("clock must return a timezone-aware timestamp")
    return instant.astimezone(timezone.utc).isoformat()


def validate_spec(spec: Mapping[str, Any]) -> None:
    expected = {
        "schema",
        "schema_version",
        "registry",
        "runtime_map",
        "routing_decision",
        "probe_authorization",
    }
    if set(spec) != expected or spec["schema"] != SETUP_SCHEMA or spec["schema_version"] != "1.0":
        raise SetupError("invalid setup specification")
    if spec["probe_authorization"] not in {"not-requested", "authorized-not-performed"}:
        raise SetupError("invalid probe authorization")
    try:
        contracts.validate_registry(spec["registry"])
        contracts.validate_decision(spec["routing_decision"])
    except contracts.ContractError as error:
        raise SetupError(str(error)) from None
    try:
        portable_adapters.validate_runtime_map(spec["runtime_map"], spec["registry"])
    except portable_adapters.PortableAdapterError as error:
        raise SetupError(str(error)) from None
    decision = spec["routing_decision"]
    registry = spec["registry"]
    if decision["registry_id"] != registry["registry_id"] or decision["registry_version"] != registry["version"]:
        raise SetupError("routing decision does not match canonical registry")


def _documents(
    spec: Mapping[str, Any],
    detected: list[Mapping[str, Any]],
    checked_at: str,
) -> list[local_state.PlannedWrite]:
    registry = spec["registry"]
    report = portable_adapters.readiness(
        registry,
        spec["runtime_map"],
        detected,
        checked_at,
    )
    descriptor = portable_adapters.invocation_descriptor(
        registry,
        spec["routing_decision"],
        report,
        spec["runtime_map"],
    )
    registry_id = registry["registry_id"]
    plans = [
        local_state.PlannedWrite(
            local_state.STATE_DIR,
            ("readiness", f"{registry_id}.json"),
            local_state.canonical_json(report),
        ),
        local_state.PlannedWrite(
            local_state.LOCAL_DIR,
            ("generated", f"{descriptor['profile_id']}.json"),
            local_state.canonical_json(descriptor),
        ),
    ]
    receipt = {
        "schema": "evolvehls.agentic.portable-setup-receipt",
        "schema_version": "1.0",
        "registry_id": registry_id,
        "registry_version": registry["version"],
        "registry_digest": hashlib.sha256(_canonical(registry)).hexdigest(),
        "runtime_map_id": spec["runtime_map"]["runtime_map_id"],
        "runtime_map_version": spec["runtime_map"]["version"],
        "checked_at": checked_at,
        "probe_authorization": spec["probe_authorization"],
        "probes_performed": False,
        "writes": len(plans),
    }
    plans.append(
        local_state.PlannedWrite(
            local_state.STATE_DIR,
            ("setup", f"{registry_id}.json"),
            local_state.canonical_json(receipt),
        )
    )
    return plans


def preview(
    repository: Path,
    spec: Mapping[str, Any],
    detected: list[Mapping[str, Any]] | None = None,
    *,
    clock: Callable[[], datetime] | None = None,
) -> dict[str, Any]:
    validate_spec(spec)
    observed = detected if detected is not None else adapters.detect_all()
    checked_at = _checked_at(clock)
    return {
        "registry_id": spec["registry"]["registry_id"],
        "checked_at": checked_at,
        "dry_run": True,
        "probes": spec["probe_authorization"],
        "probes_performed": False,
        "writes": local_state.preview(repository, _documents(spec, observed, checked_at)),
    }


def apply(
    repository: Path,
    spec: Mapping[str, Any],
    *,
    detected: list[Mapping[str, Any]] | None = None,
    dry_run: bool = False,
    replace: bool = False,
    clock: Callable[[], datetime] | None = None,
) -> dict[str, Any]:
    validate_spec(spec)
    observed = detected if detected is not None else adapters.detect_all()
    checked_at = _checked_at(clock)
    if dry_run:
        return preview(repository, spec, observed, clock=lambda: contracts.parse_timestamp(checked_at))
    written = local_state.commit(repository, _documents(spec, observed, checked_at), replace=replace)
    return {
        "registry_id": spec["registry"]["registry_id"],
        "checked_at": checked_at,
        "dry_run": False,
        "changed": [str(path.relative_to(repository.resolve())) for path in written],
        "idempotent": not written,
    }


def config_preview(
    repository: Path,
    spec: Mapping[str, Any],
    detected: list[Mapping[str, Any]] | None = None,
    *,
    clock: Callable[[], datetime] | None = None,
) -> list[dict[str, object]]:
    validate_spec(spec)
    observed = detected if detected is not None else adapters.detect_all()
    checked_at = _checked_at(clock)
    plans = [item for item in _documents(spec, observed, checked_at) if item.parts[0] == "generated"]
    return local_state.preview(repository, plans)


def config_generate(
    repository: Path,
    spec: Mapping[str, Any],
    *,
    detected: list[Mapping[str, Any]] | None = None,
    dry_run: bool = False,
    replace: bool = False,
    clock: Callable[[], datetime] | None = None,
) -> list[str]:
    validate_spec(spec)
    observed = detected if detected is not None else adapters.detect_all()
    checked_at = _checked_at(clock)
    plans = [item for item in _documents(spec, observed, checked_at) if item.parts[0] == "generated"]
    paths = local_state.commit(repository, plans, replace=replace, dry_run=dry_run)
    return [str(path.relative_to(repository.resolve())) for path in paths]