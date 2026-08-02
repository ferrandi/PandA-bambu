#!/usr/bin/env python3
"""Portable PAF-02/PAF-03A contract loading and strict semantic validation."""
from __future__ import annotations

import json
import re
from datetime import datetime
from pathlib import Path
from typing import Any, Mapping

ROOT = Path(__file__).resolve().parents[2]
SCHEMA_DIR = ROOT / "agentic" / "schemas"
ACCESS = {"api-gateway", "native-account-client", "local-server", "native-local-client"}
FUNDING = {"project", "organization", "subscription", "personal-api", "local"}
AUTH = {"native-session", "environment-token", "token-helper", "none"}
PROTOCOLS = {"openai-responses", "anthropic-messages", "openai-chat-completions"}
CONFIDENCES = {"declared", "inferred", "observed", "historically-validated", "unknown"}
STATUSES = {"supported", "unsupported", "unknown", "probe-failed"}


class ContractError(ValueError):
    """Raised when a portable document violates its contract."""


def _require(condition: bool, message: str) -> None:
    if not condition:
        raise ContractError(message)


def _id(value: Any, field: str) -> None:
    _require(isinstance(value, str) and value, f"{field} must be a non-empty string")


def _fields(value: Any, required: set[str], name: str) -> None:
    _require(isinstance(value, Mapping) and set(value) == required, f"{name} fields do not match schema")


def _list(value: Any, field: str, allowed: set[str] | None = None, nonempty: bool = False) -> None:
    _require(isinstance(value, list) and (bool(value) or not nonempty) and all(isinstance(item, str) and item for item in value), f"invalid {field}")
    _require(len(value) == len(set(value)), f"{field} must not contain duplicates")
    if allowed is not None:
        _require(set(value) <= allowed, f"invalid {field}")


def load_json(path: Path) -> dict[str, Any]:
    try:
        _require(path.is_file() and not path.is_symlink(), "contract path must be a regular file")
        value = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, UnicodeDecodeError, json.JSONDecodeError) as error:
        raise ContractError(f"cannot parse contract document: {type(error).__name__}") from None
    _require(isinstance(value, dict), "contract document must be an object")
    return value


def load_schema(name: str) -> dict[str, Any]:
    return load_json(SCHEMA_DIR / f"{name}.schema.json")


def parse_timestamp(value: Any, field: str = "timestamp") -> datetime:
    _id(value, field)
    try:
        parsed = datetime.fromisoformat(value)
    except ValueError:
        raise ContractError(f"{field} must be an ISO 8601 timestamp") from None
    _require(parsed.tzinfo is not None and parsed.utcoffset() is not None, f"{field} must include a timezone")
    return parsed


def validate_role(value: Mapping[str, Any]) -> None:
    required = {"schema", "schema_version", "role_id", "version", "mandatory_capabilities", "preferred_capabilities", "default_objective", "permitted_modes", "minimum_confidence"}
    _fields(value, required, "role")
    _require(value["schema"] == "evolvehls.agentic.role" and value["schema_version"] == "1.0", "unsupported role schema")
    _id(value["role_id"], "role_id"); _id(value["version"], "version")
    _list(value["mandatory_capabilities"], "mandatory_capabilities"); _list(value["preferred_capabilities"], "preferred_capabilities")
    _require(set(value["mandatory_capabilities"]).isdisjoint(value["preferred_capabilities"]), "capabilities cannot be both mandatory and preferred")
    _list(value["permitted_modes"], "permitted_modes", {"development", "evaluation", "ablation"}, True)
    _require(value["minimum_confidence"] in CONFIDENCES, "invalid minimum_confidence")


def validate_task(value: Mapping[str, Any]) -> None:
    required = {"schema", "schema_version", "task_id", "version", "role", "objective", "inputs", "constraints", "expected_outputs", "validation_requirements", "budgets", "reproducibility"}
    _fields(value, required, "task")
    _require(value["schema"] == "evolvehls.agentic.task" and value["schema_version"] == "1.0", "unsupported task schema")
    for key in ("task_id", "version", "role", "objective"): _id(value[key], key)
    for key in ("inputs", "constraints", "expected_outputs", "validation_requirements"): _list(value[key], key)
    _require(isinstance(value["budgets"], dict), "budgets must be an object")
    _fields(value["reproducibility"], {"context_hash", "base_revision"}, "reproducibility")


def validate_result(value: Mapping[str, Any]) -> None:
    required = {"schema", "schema_version", "task_id", "task_version", "outcome", "execution_plan", "artifacts", "validation", "diagnostics"}
    _fields(value, required, "result")
    _require(value["schema"] == "evolvehls.agentic.result" and value["schema_version"] == "1.0", "unsupported result schema")
    _id(value["task_id"], "task_id")
    _id(value["task_version"], "task_version")
    _id(value["execution_plan"], "execution_plan")
    _require(value["outcome"] in {"succeeded", "failed", "blocked", "cancelled"}, "invalid result outcome")
    _require(
        isinstance(value["validation"], list)
        and all(
            isinstance(item, Mapping)
            and set(item) == {"name", "status", "evidence"}
            and isinstance(item["name"], str)
            and item["name"]
            and item["status"] in {"passed", "failed", "not-run"}
            and (item["evidence"] is None or isinstance(item["evidence"], str))
            and (item["status"] != "passed" or isinstance(item["evidence"], str) and item["evidence"])
            for item in value["validation"]
        ),
        "invalid validation records",
    )


def validate_overlay(value: Mapping[str, Any]) -> None:
    _fields(value, {"schema", "schema_version", "profile_id", "rules"}, "overlay")
    _require(value["schema"] == "evolvehls.agentic.policy-overlay" and value["schema_version"] == "1.0", "unsupported overlay schema")
    _require(isinstance(value["rules"], list), "overlay rules must be a list")
    seen: set[str] = set()
    for rule in value["rules"]:
        _fields(rule, {"model_id", "eligible", "rejection_reasons", "execution_units", "capabilities", "metadata"}, "overlay rule")
        _id(rule["model_id"], "rule.model_id")
        _require(rule["model_id"] not in seen, "overlay rules must target a model at most once")
        seen.add(rule["model_id"])
        _require(isinstance(rule["eligible"], bool) and isinstance(rule["execution_units"], list) and isinstance(rule["capabilities"], dict) and isinstance(rule["metadata"], dict), "invalid overlay rule")


def validate_selection(value: Mapping[str, Any]) -> None:
    _fields(value, {"schema", "schema_version", "selection_id", "created_at", "catalog_snapshot", "role_id", "role_version", "execution_plan"}, "selection")
    _require(value["schema"] == "evolvehls.agentic.selection" and value["schema_version"] == "1.0", "unsupported selection schema")
    parse_timestamp(value["created_at"], "created_at")
    _require(isinstance(value["execution_plan"], dict), "execution_plan must be an object")


def validate_adapter(value: Mapping[str, Any]) -> None:
    _fields(value, {"schema", "schema_version", "adapter_id", "invocation_class", "execution_family", "access_classes", "protocols"}, "adapter")
    _require(value["schema"] == "evolvehls.agentic.client-adapter" and value["schema_version"] == "1.0", "unsupported adapter schema")
    _id(value["adapter_id"], "adapter_id"); _id(value["execution_family"], "execution_family")
    _require(value["invocation_class"] in {"native-account-cli", "http-api-client", "native-local-cli"}, "invalid invocation class")
    _list(value["access_classes"], "access_classes", ACCESS, True); _list(value["protocols"], "protocols", PROTOCOLS, True)


def validate_profile(value: Mapping[str, Any]) -> None:
    required = {"profile_id", "schema_version", "adapter_id", "access_class", "funding_class", "auth_mode", "binding", "model", "protocol", "capabilities", "privacy", "cost", "resources", "availability", "priority"}
    _fields(value, required, "profile")
    _require(value["schema_version"] == "1.0", "unsupported profile schema")
    _id(value["profile_id"], "profile_id"); _id(value["adapter_id"], "adapter_id")
    _require(value["access_class"] in ACCESS and value["funding_class"] in FUNDING and value["auth_mode"] in AUTH and value["protocol"] in PROTOCOLS, "invalid profile class")
    _fields(value["binding"], {"kind", "ref"}, "binding"); _require(value["binding"]["kind"] in {"provider-profile", "native-session", "local-runtime"}, "invalid binding kind"); _id(value["binding"]["ref"], "binding.ref")
    _fields(value["model"], {"kind", "ref"}, "model"); _require(value["model"]["kind"] in {"selector", "pinned"}, "invalid model kind"); _id(value["model"]["ref"], "model.ref")
    _require(isinstance(value["capabilities"], dict), "capabilities must be an object")
    for name, evidence in value["capabilities"].items():
        _id(name, "capability name"); _require(isinstance(evidence, Mapping) and evidence.get("status") in STATUSES and evidence.get("confidence") in CONFIDENCES and isinstance(evidence.get("provenance"), list) and evidence["provenance"], "invalid capability evidence")
    _fields(value["privacy"], {"data_classes"}, "privacy"); _list(value["privacy"]["data_classes"], "data_classes", {"public", "internal", "restricted"}, True)
    _fields(value["cost"], {"tier"}, "cost"); _require(isinstance(value["cost"]["tier"], int) and 0 <= value["cost"]["tier"] <= 9, "invalid cost tier")
    _fields(value["resources"], {"requires"}, "resources"); _list(value["resources"]["requires"], "resource requirements")
    _fields(value["availability"], {"required"}, "availability"); _require(isinstance(value["availability"]["required"], bool) and isinstance(value["priority"], int) and value["priority"] >= 0, "invalid profile availability or priority")
    _require(not (value["access_class"] == "native-account-client") or value["auth_mode"] == "native-session", "native account clients require native-session auth")
    _require(not (value["access_class"] in {"local-server", "native-local-client"}) or value["funding_class"] == "local", "local access requires local funding")


def validate_registry(value: Mapping[str, Any]) -> None:
    _fields(value, {"schema", "schema_version", "registry_id", "version", "adapters", "profiles"}, "registry")
    _require(value["schema"] == "evolvehls.agentic.profile-registry" and value["schema_version"] == "1.0", "unsupported registry schema")
    _id(value["registry_id"], "registry_id"); _id(value["version"], "version"); _require(isinstance(value["adapters"], list) and isinstance(value["profiles"], list), "registry entries must be lists")
    for adapter in value["adapters"]: validate_adapter(adapter)
    ids = [item["adapter_id"] for item in value["adapters"]]; _require(len(ids) == len(set(ids)), "duplicate adapter identifiers")
    for profile in value["profiles"]: validate_profile(profile)
    ids = [item["profile_id"] for item in value["profiles"]]; _require(len(ids) == len(set(ids)), "duplicate profile identifiers")
    adapters = {item["adapter_id"]: item for item in value["adapters"]}
    for profile in value["profiles"]:
        _require(profile["adapter_id"] in adapters, "profile references unknown adapter")


def validate_policy(value: Mapping[str, Any]) -> None:
    required = {"schema", "schema_version", "policy_id", "version", "allowed_access_classes", "allowed_funding_classes", "allowed_auth_modes", "allowed_data_classes", "max_cost_tier", "available_resources", "preferences", "fallbacks", "independent_review"}
    _fields(value, required, "routing policy")
    _require(value["schema"] == "evolvehls.agentic.routing-policy" and value["schema_version"] == "1.0", "unsupported routing policy schema")
    _id(value["policy_id"], "policy_id"); _id(value["version"], "version")
    _list(value["allowed_access_classes"], "allowed_access_classes", ACCESS, True); _list(value["allowed_funding_classes"], "allowed_funding_classes", FUNDING, True); _list(value["allowed_auth_modes"], "allowed_auth_modes", AUTH, True)
    _list(value["allowed_data_classes"], "allowed_data_classes", {"public", "internal", "restricted"}, True); _list(value["available_resources"], "available_resources")
    _require(isinstance(value["max_cost_tier"], int) and 0 <= value["max_cost_tier"] <= 9 and isinstance(value["preferences"], list) and value["preferences"], "invalid routing policy")
    preference_ids = []
    for item in value["preferences"]:
        _fields(item, {"preference_id", "access_classes", "funding_classes"}, "preference"); _id(item["preference_id"], "preference_id")
        _list(item["access_classes"], "preference access", ACCESS, True); _list(item["funding_classes"], "preference funding", FUNDING, True); preference_ids.append(item["preference_id"])
    _require(len(preference_ids) == len(set(preference_ids)), "duplicate preference identifiers")
    _require(isinstance(value["fallbacks"], list), "fallbacks must be a list")
    for item in value["fallbacks"]:
        _fields(item, {"from", "to", "allow_funding_transition"}, "fallback"); _require(item["from"] in preference_ids and item["to"] in preference_ids and item["from"] != item["to"] and isinstance(item["allow_funding_transition"], bool), "invalid fallback")
    _fields(value["independent_review"], {"required", "different_adapter", "different_execution_family"}, "independent review")
    _require(all(isinstance(item, bool) for item in value["independent_review"].values()), "invalid independent review policy")


def validate_readiness(value: Mapping[str, Any], registry: Mapping[str, Any] | None = None) -> None:
    _fields(value, {"schema", "schema_version", "registry_id", "checked_at", "redacted", "adapters", "profiles", "resources", "diagnostics"}, "readiness report")
    _require(value["schema"] == "evolvehls.agentic.readiness-report" and value["schema_version"] == "1.0" and value["redacted"] is True, "invalid readiness report")
    _id(value["registry_id"], "registry_id"); parse_timestamp(value["checked_at"], "checked_at"); _list(value["resources"], "resources"); _list(value["diagnostics"], "diagnostics")
    for key in ("adapters", "profiles"):
        _require(isinstance(value[key], list), f"{key} must be a list")
        refs = []
        for item in value[key]:
            _fields(item, {"ref", "status"}, "readiness item"); _id(item["ref"], "readiness ref"); _require(item["status"] in {"ready", "unavailable", "unknown"}, "invalid readiness status"); refs.append(item["ref"])
        _require(len(refs) == len(set(refs)), f"duplicate readiness {key}")
    if registry is not None:
        _require(value["registry_id"] == registry["registry_id"], "readiness registry mismatch")
        _require(set(item["ref"] for item in value["adapters"]) <= {item["adapter_id"] for item in registry["adapters"]}, "readiness references unknown adapter")
        _require(set(item["ref"] for item in value["profiles"]) <= {item["profile_id"] for item in registry["profiles"]}, "readiness references unknown profile")


_PROVIDER_IDENTIFIER = re.compile(r"^[a-z][a-z0-9.-]*$")
_ENVIRONMENT_REFERENCE = re.compile(r"^[A-Z][A-Z0-9_]{0,127}$")
_PROTOCOL_CONFIGURATION = {
    "openai-compatible": ("openai-chat-completions", "openai-compatible"),
    "anthropic-compatible": ("anthropic-messages", "anthropic-compatible"),
}
_PROVIDER_SECRET_FIELDS = {
    "api_key", "authorization", "credential", "password", "secret", "token",
}


def _provider_id(value: Any, field: str) -> None:
    _require(
        isinstance(value, str)
        and len(value) <= 190
        and _PROVIDER_IDENTIFIER.fullmatch(value) is not None,
        f"invalid {field}",
    )


def _provider_string(value: Any, field: str, *, maximum: int = 512) -> None:
    _require(isinstance(value, str) and value and len(value) <= maximum, f"invalid {field}")


def _no_secret_fields(value: Any) -> None:
    if isinstance(value, Mapping):
        for key, item in value.items():
            _require(str(key).lower().replace("-", "_") not in _PROVIDER_SECRET_FIELDS, "provider documents must not contain secret values")
            _no_secret_fields(item)
    elif isinstance(value, list):
        for item in value:
            _no_secret_fields(item)


def _provider_authentication(value: Any) -> None:
    _require(isinstance(value, Mapping), "invalid provider authentication")
    mode = value.get("mode")
    if mode == "none":
        _fields(value, {"mode"}, "provider authentication")
    elif mode == "environment-token":
        _fields(value, {"mode", "env_var"}, "provider authentication")
        _require(isinstance(value["env_var"], str) and _ENVIRONMENT_REFERENCE.fullmatch(value["env_var"]) is not None, "invalid environment-variable reference")
    else:
        raise ContractError("invalid provider authentication mode")


def _provider_endpoint(value: Any) -> None:
    _fields(value, {"origin", "protocol"}, "provider endpoint")
    _provider_string(value["origin"], "endpoint origin")
    _require(value["protocol"] in _PROTOCOL_CONFIGURATION, "unsupported provider protocol")


def validate_provider_onboarding_spec(value: Mapping[str, Any]) -> None:
    required = {"schema", "schema_version", "provider_id", "endpoint", "authentication", "model", "roles"}
    _fields(value, required, "provider onboarding specification")
    _require(value["schema"] == "evolvehls.agentic.provider-onboarding-spec" and value["schema_version"] == "1.0", "unsupported provider onboarding specification schema")
    _provider_id(value["provider_id"], "provider_id")
    _provider_endpoint(value["endpoint"])
    _provider_authentication(value["authentication"])
    _provider_string(value["model"], "model")
    _list(value["roles"], "roles")
    _no_secret_fields(value)


def validate_provider_discovery_evidence(value: Mapping[str, Any]) -> None:
    required = {"schema", "schema_version", "provider_id", "checked_at", "method", "status", "diagnostics"}
    _fields(value, required, "provider discovery evidence")
    _require(value["schema"] == "evolvehls.agentic.provider-discovery-evidence" and value["schema_version"] == "1.0", "unsupported provider discovery evidence schema")
    _provider_id(value["provider_id"], "provider_id")
    parse_timestamp(value["checked_at"], "checked_at")
    _require(value["method"] in {"not-requested", "manual"}, "unsupported discovery evidence method")
    _require(value["status"] in {"not-performed", "manual"}, "invalid discovery evidence status")
    _list(value["diagnostics"], "diagnostics")
    _require((value["method"], value["status"]) in {("not-requested", "not-performed"), ("manual", "manual")}, "inconsistent discovery evidence")
    _no_secret_fields(value)


def validate_provider_configuration(value: Mapping[str, Any]) -> None:
    required = {"schema", "schema_version", "provider_id", "endpoint", "authentication", "model", "canonical", "configured_at"}
    _fields(value, required, "provider configuration")
    _require(value["schema"] == "evolvehls.agentic.provider-configuration" and value["schema_version"] == "1.0", "unsupported provider configuration schema")
    _provider_id(value["provider_id"], "provider_id")
    _provider_endpoint(value["endpoint"])
    _provider_authentication(value["authentication"])
    _provider_string(value["model"], "model")
    _fields(value["canonical"], {"profile_id", "adapter_id", "runtime_entry_id", "execution_path"}, "provider canonical references")
    for field in value["canonical"]:
        _provider_id(value["canonical"][field], f"canonical.{field}")
    _require(value["canonical"]["adapter_id"] == "generic-http", "provider configuration must reference the canonical generic-http adapter")
    parse_timestamp(value["configured_at"], "configured_at")
    _no_secret_fields(value)


def validate_provider_onboarding_receipt(value: Mapping[str, Any]) -> None:
    required = {"schema", "schema_version", "provider_id", "configured_at", "configuration_digest", "profile_overlay_id", "runtime_overlay_id", "action"}
    _fields(value, required, "provider onboarding receipt")
    _require(value["schema"] == "evolvehls.agentic.provider-onboarding-receipt" and value["schema_version"] == "1.0", "unsupported provider onboarding receipt schema")
    _provider_id(value["provider_id"], "provider_id")
    parse_timestamp(value["configured_at"], "configured_at")
    _require(isinstance(value["configuration_digest"], str) and re.fullmatch(r"[0-9a-f]{64}", value["configuration_digest"]) is not None, "invalid configuration digest")
    _provider_id(value["profile_overlay_id"], "profile_overlay_id")
    _provider_id(value["runtime_overlay_id"], "runtime_overlay_id")
    _require(value["action"] in {"applied", "replaced", "removed"}, "invalid provider receipt action")
    _no_secret_fields(value)


def validate_provider_profile_overlay(value: Mapping[str, Any], registry: Mapping[str, Any] | None = None) -> None:
    required = {"schema", "schema_version", "overlay_id", "provider_id", "registry_id", "registry_version", "configuration_digest", "profile"}
    _fields(value, required, "provider profile overlay")
    _require(value["schema"] == "evolvehls.agentic.provider-profile-overlay" and value["schema_version"] == "1.0", "unsupported provider profile overlay schema")
    for field in ("overlay_id", "provider_id", "registry_id"):
        _provider_id(value[field], field)
    _provider_string(value["registry_version"], "registry_version")
    _require(isinstance(value["configuration_digest"], str) and re.fullmatch(r"[0-9a-f]{64}", value["configuration_digest"]) is not None, "invalid configuration digest")
    validate_profile(value["profile"])
    _require(value["profile"]["adapter_id"] == "generic-http", "provider profile overlay must reference generic-http")
    _require(value["profile"]["binding"] == {"kind": "provider-profile", "ref": value["provider_id"]}, "provider profile overlay binding must reference its provider")
    if registry is not None:
        _require(value["registry_id"] == registry["registry_id"] and value["registry_version"] == registry["version"], "provider profile overlay targets a different canonical registry")
        _require(value["profile"]["adapter_id"] in {item["adapter_id"] for item in registry["adapters"]}, "provider profile overlay references unknown canonical adapter")
    _no_secret_fields(value)


def validate_provider_runtime_overlay(value: Mapping[str, Any], registry: Mapping[str, Any] | None = None, runtime_map: Mapping[str, Any] | None = None) -> None:
    required = {"schema", "schema_version", "overlay_id", "provider_id", "runtime_map_id", "runtime_map_version", "configuration_digest", "entry"}
    _fields(value, required, "provider runtime overlay")
    _require(value["schema"] == "evolvehls.agentic.provider-runtime-overlay" and value["schema_version"] == "1.0", "unsupported provider runtime overlay schema")
    for field in ("overlay_id", "provider_id", "runtime_map_id"):
        _provider_id(value[field], field)
    _provider_string(value["runtime_map_version"], "runtime_map_version")
    _require(isinstance(value["configuration_digest"], str) and re.fullmatch(r"[0-9a-f]{64}", value["configuration_digest"]) is not None, "invalid configuration digest")
    entry = value["entry"]
    _fields(entry, {"runtime_entry_id", "profile_id", "adapter_id", "client_template", "execution_path"}, "provider runtime overlay entry")
    for field in entry:
        _provider_id(entry[field], f"entry.{field}")
    _require(entry["adapter_id"] == "generic-http" and entry["client_template"] == "direct-http", "provider runtime overlay must reference canonical generic-http/direct-http semantics")
    if runtime_map is not None:
        _require(value["runtime_map_id"] == runtime_map["runtime_map_id"] and value["runtime_map_version"] == runtime_map["version"], "provider runtime overlay targets a different canonical runtime map")
    if registry is not None:
        adapters = {item["adapter_id"] for item in registry["adapters"]}
        _require(entry["adapter_id"] in adapters, "provider runtime overlay references unknown canonical adapter")
    _no_secret_fields(value)


def validate_decision(value: Mapping[str, Any]) -> None:
    required = {"schema", "schema_version", "resolver_version", "registry_id", "registry_version", "policy_id", "policy_version", "mode", "task_id", "role_id", "selected", "fallback_authorized", "explanation", "rejected"}
    _fields(value, required, "routing decision")
    _require(value["schema"] == "evolvehls.agentic.stage-routing-decision" and value["schema_version"] == "1.0" and value["mode"] in {"development", "evaluation", "ablation"}, "invalid routing decision")
    _require(isinstance(value["fallback_authorized"], bool) and isinstance(value["explanation"], list) and value["explanation"], "invalid decision explanation")
    _require(isinstance(value["selected"], Mapping) and {"profile_id", "adapter_id", "execution_family", "access_class", "funding_class", "auth_mode", "binding", "model", "protocol", "capabilities"} == set(value["selected"]), "invalid selected routing candidate")


def load_contract(path: Path, kind: str) -> dict[str, Any]:
    value = load_json(path)
    validators = {
        "role": validate_role, "task": validate_task, "result": validate_result,
        "overlay": validate_overlay, "selection": validate_selection,
        "adapter": validate_adapter, "profile": validate_profile, "registry": validate_registry,
        "policy": validate_policy, "readiness": validate_readiness, "decision": validate_decision,
        "provider-onboarding-spec": validate_provider_onboarding_spec,
        "provider-discovery-evidence": validate_provider_discovery_evidence,
        "provider-configuration": validate_provider_configuration,
        "provider-onboarding-receipt": validate_provider_onboarding_receipt,
    }
    try: validators[kind](value)
    except KeyError: raise ContractError("unknown contract kind") from None
    return value