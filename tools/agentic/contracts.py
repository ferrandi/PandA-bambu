#!/usr/bin/env python3
"""Portable PAF-02 contract loading and strict semantic validation."""
from __future__ import annotations

import json
from pathlib import Path
from typing import Any, Mapping

ROOT = Path(__file__).resolve().parents[2]
SCHEMA_DIR = ROOT / "agentic" / "schemas"
CONTRACT_DIR = ROOT / "agentic"
CAPABILITY_STATUSES = {"supported", "unsupported", "unknown", "probe-failed"}
CONFIDENCES = {"declared", "inferred", "observed", "historically-validated", "unknown"}
MODES = {"development", "evaluation", "ablation"}


class ContractError(ValueError):
    """Raised when a portable PAF-02 document violates its contract."""


def _require(condition: bool, message: str) -> None:
    if not condition:
        raise ContractError(message)


def load_json(path: Path) -> dict[str, Any]:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, UnicodeDecodeError, json.JSONDecodeError) as error:
        raise ContractError(f"cannot parse contract document: {type(error).__name__}") from None
    _require(isinstance(value, dict), "contract document must be an object")
    return value


def load_schema(name: str) -> dict[str, Any]:
    return load_json(SCHEMA_DIR / f"{name}.schema.json")


def _id(value: Any, field: str) -> None:
    _require(isinstance(value, str) and value, f"{field} must be a non-empty string")


def _string_list(value: Any, field: str, unique: bool = False) -> None:
    _require(isinstance(value, list) and all(isinstance(item, str) and item for item in value), f"{field} must be a list of non-empty strings")
    if unique:
        _require(len(value) == len(set(value)), f"{field} must not contain duplicates")


def _capability_requirements(value: Any, field: str) -> None:
    _string_list(value, field, unique=True)


def validate_role(value: Mapping[str, Any]) -> None:
    required = {"schema", "schema_version", "role_id", "version", "mandatory_capabilities", "preferred_capabilities", "default_objective", "permitted_modes", "minimum_confidence"}
    _require(set(value) == required, "role fields do not match schema")
    _require(value["schema"] == "evolvehls.agentic.role" and value["schema_version"] == "1.0", "unsupported role schema")
    _id(value["role_id"], "role_id")
    _id(value["version"], "version")
    _capability_requirements(value["mandatory_capabilities"], "mandatory_capabilities")
    _capability_requirements(value["preferred_capabilities"], "preferred_capabilities")
    _require(set(value["mandatory_capabilities"]).isdisjoint(value["preferred_capabilities"]), "capabilities cannot be both mandatory and preferred")
    _id(value["default_objective"], "default_objective")
    _require(isinstance(value["permitted_modes"], list) and value["permitted_modes"] and set(value["permitted_modes"]) <= MODES and len(value["permitted_modes"]) == len(set(value["permitted_modes"])), "invalid permitted_modes")
    _require(value["minimum_confidence"] in CONFIDENCES, "invalid minimum_confidence")


def validate_task(value: Mapping[str, Any]) -> None:
    required = {"schema", "schema_version", "task_id", "version", "role", "objective", "inputs", "constraints", "expected_outputs", "validation_requirements", "budgets", "reproducibility"}
    _require(set(value) == required, "task fields do not match schema")
    _require(value["schema"] == "evolvehls.agentic.task" and value["schema_version"] == "1.0", "unsupported task schema")
    for key in ("task_id", "version", "role", "objective"):
        _id(value[key], key)
    for key in ("inputs", "constraints", "expected_outputs", "validation_requirements"):
        _string_list(value[key], key, unique=True)
    _require(isinstance(value["budgets"], dict), "budgets must be an object")
    _require(isinstance(value["reproducibility"], dict) and set(value["reproducibility"]) == {"context_hash", "base_revision"}, "invalid reproducibility")
    for key in ("context_hash", "base_revision"):
        _id(value["reproducibility"][key], f"reproducibility.{key}")


def validate_result(value: Mapping[str, Any]) -> None:
    required = {"schema", "schema_version", "task_id", "task_version", "outcome", "execution_plan", "artifacts", "validation", "diagnostics"}
    _require(set(value) == required, "result fields do not match schema")
    _require(value["schema"] == "evolvehls.agentic.result" and value["schema_version"] == "1.0", "unsupported result schema")
    for key in ("task_id", "task_version"):
        _id(value[key], key)
    _require(value["outcome"] in {"succeeded", "failed", "blocked", "cancelled"}, "invalid result outcome")
    _id(value["execution_plan"], "execution_plan")
    _require(isinstance(value["artifacts"], list) and all(isinstance(item, dict) and set(item) == {"path", "digest"} and all(isinstance(item[key], str) and item[key] for key in ("path", "digest")) for item in value["artifacts"]), "invalid artifacts")
    _require(isinstance(value["validation"], list) and all(isinstance(item, dict) and set(item) == {"name", "status", "evidence"} and isinstance(item["name"], str) and item["name"] and item["status"] in {"passed", "failed", "not-run"} and (item["evidence"] is None or isinstance(item["evidence"], str)) for item in value["validation"]), "invalid validation records")
    _string_list(value["diagnostics"], "diagnostics")


def validate_overlay(value: Mapping[str, Any]) -> None:
    required = {"schema", "schema_version", "profile_id", "rules"}
    _require(set(value) == required, "overlay fields do not match schema")
    _require(value["schema"] == "evolvehls.agentic.policy-overlay" and value["schema_version"] == "1.0", "unsupported overlay schema")
    _id(value["profile_id"], "profile_id")
    _require(isinstance(value["rules"], list), "overlay rules must be a list")
    seen: set[str] = set()
    for rule in value["rules"]:
        _require(isinstance(rule, dict) and set(rule) == {"model_id", "eligible", "rejection_reasons", "execution_units", "capabilities", "metadata"}, "invalid overlay rule")
        _id(rule["model_id"], "rule.model_id")
        _require(rule["model_id"] not in seen, "overlay rules must target a model at most once")
        seen.add(rule["model_id"])
        _require(isinstance(rule["eligible"], bool), "rule.eligible must be boolean")
        _string_list(rule["rejection_reasons"], "rule.rejection_reasons", unique=True)
        _require(isinstance(rule["execution_units"], list), "rule.execution_units must be a list")
        _require(isinstance(rule["capabilities"], dict), "rule.capabilities must be an object")
        for name, evidence in rule["capabilities"].items():
            _id(name, "capability name")
            _require(isinstance(evidence, dict) and set(evidence) == {"status", "confidence", "provenance"}, "invalid overlay capability evidence")
            _require(evidence["status"] in CAPABILITY_STATUSES and evidence["confidence"] in CONFIDENCES and isinstance(evidence["provenance"], list) and evidence["provenance"], "invalid overlay capability status")
        _require(isinstance(rule["metadata"], dict), "rule.metadata must be an object")


def validate_selection(value: Mapping[str, Any]) -> None:
    required = {"schema", "schema_version", "selection_id", "created_at", "catalog_snapshot", "role_id", "role_version", "execution_plan"}
    _require(set(value) == required, "selection fields do not match schema")
    _require(value["schema"] == "evolvehls.agentic.selection" and value["schema_version"] == "1.0", "unsupported selection schema")
    for key in ("selection_id", "created_at", "catalog_snapshot", "role_id", "role_version"):
        _id(value[key], key)
    _require(isinstance(value["execution_plan"], dict), "execution_plan must be an object")


def load_contract(path: Path, kind: str) -> dict[str, Any]:
    value = load_json(path)
    validators = {"role": validate_role, "task": validate_task, "result": validate_result, "overlay": validate_overlay, "selection": validate_selection}
    try:
        validators[kind](value)
    except KeyError:
        raise ContractError("unknown contract kind") from None
    return value