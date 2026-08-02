#!/usr/bin/env python3
"""Deterministic, non-executing PAF-03A execution-profile routing."""
from __future__ import annotations

import copy
from collections.abc import Mapping
from typing import Any

ACCESS_CLASSES = {"api-gateway", "native-account-client", "local-server", "native-local-client"}
FUNDING_CLASSES = {"project", "organization", "subscription", "personal-api", "local"}
AUTH_MODES = {"native-session", "environment-token", "token-helper", "none"}
PROTOCOLS = {"openai-responses", "anthropic-messages", "openai-chat-completions"}
CONFIDENCE_ORDER = {"unknown": 0, "declared": 1, "inferred": 2, "observed": 3, "historically-validated": 4}
RESOLVER_VERSION = "1.0"


class RoutingError(ValueError):
    """Raised when no policy-compliant execution profile can be resolved."""

    def __init__(self, message: str, rejected: list[dict[str, Any]]):
        super().__init__(message)
        self.rejected = rejected


def _ready(items: list[Mapping[str, Any]], ref: str) -> bool:
    return any(item.get("ref") == ref and item.get("status") == "ready" for item in items)


def _capability_reasons(profile: Mapping[str, Any], role: Mapping[str, Any]) -> list[str]:
    reasons: list[str] = []
    minimum = CONFIDENCE_ORDER[role["minimum_confidence"]]
    capabilities = profile["capabilities"]
    for name in sorted(role["mandatory_capabilities"]):
        evidence = capabilities.get(name)
        if not isinstance(evidence, Mapping) or evidence.get("status") != "supported":
            reasons.append(f"mandatory capability unavailable: {name}")
        elif CONFIDENCE_ORDER.get(evidence.get("confidence"), -1) < minimum:
            reasons.append(f"mandatory capability confidence below {role['minimum_confidence']}: {name}")
    return reasons


def _preference(profile: Mapping[str, Any], policy: Mapping[str, Any]) -> tuple[int, str] | None:
    for index, item in enumerate(policy["preferences"]):
        if profile["access_class"] in item["access_classes"] and profile["funding_class"] in item["funding_classes"]:
            return index, item["preference_id"]
    return None


def _fallback_allowed(policy: Mapping[str, Any], preferences: list[tuple[int, str]]) -> bool:
    if not preferences or preferences[0][0] == 0:
        return False
    previous = policy["preferences"][0]["preference_id"]
    selected = preferences[0][1]
    return any(item["from"] == previous and item["to"] == selected for item in policy["fallbacks"])


def resolve(
    task: Mapping[str, Any],
    role: Mapping[str, Any],
    registry: Mapping[str, Any],
    policy: Mapping[str, Any],
    readiness: Mapping[str, Any],
    mode: str = "development",
    prior: Mapping[str, Any] | None = None,
    required_profile_id: str | None = None,
) -> dict[str, Any]:
    """Resolve a profile only from validated declarative inputs; never execute it."""
    if mode not in {"development", "evaluation", "ablation"}:
        raise ValueError("unknown routing mode")
    adapters = {item["adapter_id"]: item for item in registry["adapters"]}
    rejected: list[dict[str, Any]] = []
    valid: list[tuple[int, int, str, str, dict[str, Any], str]] = []
    for raw in sorted(registry["profiles"], key=lambda value: value["profile_id"]):
        profile = copy.deepcopy(raw)
        reasons = _capability_reasons(profile, role)
        if required_profile_id is not None and profile["profile_id"] != required_profile_id:
            reasons.append("profile does not match required profile constraint")
        adapter = adapters.get(profile["adapter_id"])
        if adapter is None:
            reasons.append("unknown client adapter")
        else:
            if profile["access_class"] not in adapter["access_classes"]:
                reasons.append("adapter does not support access class")
            if profile["protocol"] not in adapter["protocols"]:
                reasons.append("adapter does not support protocol")
        for key, allowed in (
            ("access_class", policy["allowed_access_classes"]),
            ("funding_class", policy["allowed_funding_classes"]),
            ("auth_mode", policy["allowed_auth_modes"]),
        ):
            if profile[key] not in allowed:
                reasons.append(f"policy prohibits {key}: {profile[key]}")
        if not set(profile["privacy"]["data_classes"]) <= set(policy["allowed_data_classes"]):
            reasons.append("privacy data class is prohibited")
        if profile["cost"]["tier"] > policy["max_cost_tier"]:
            reasons.append("cost tier exceeds policy")
        if not set(profile["resources"]["requires"]) <= set(policy["available_resources"]):
            reasons.append("required resources are unavailable")
        if profile["availability"]["required"] and (
            not _ready(readiness["profiles"], profile["profile_id"])
            or not _ready(readiness["adapters"], profile["adapter_id"])
        ):
            reasons.append("required readiness is unavailable")
        preference = _preference(profile, policy)
        if preference is None:
            reasons.append("profile matches no policy preference")
        elif preference[0] > 0 and not any(
            item["from"] == policy["preferences"][0]["preference_id"]
            and item["to"] == preference[1]
            and item["allow_funding_transition"]
            for item in policy["fallbacks"]
        ):
            reasons.append("policy does not authorize fallback transition")
        if policy["independent_review"]["required"]:
            if prior is None:
                reasons.append("independent review requires prior execution evidence")
            elif policy["independent_review"]["different_adapter"] and prior["selected"]["adapter_id"] == profile["adapter_id"]:
                reasons.append("independent review requires a different adapter")
            elif policy["independent_review"]["different_execution_family"] and adapter and prior["selected"]["execution_family"] == adapter["execution_family"]:
                reasons.append("independent review requires a different execution family")
        if reasons:
            rejected.append({"profile_id": profile["profile_id"], "reasons": sorted(set(reasons))})
            continue
        assert adapter is not None and preference is not None
        selected = {
            "profile_id": profile["profile_id"],
            "adapter_id": profile["adapter_id"],
            "execution_family": adapter["execution_family"],
            "access_class": profile["access_class"],
            "funding_class": profile["funding_class"],
            "auth_mode": profile["auth_mode"],
            "binding": copy.deepcopy(profile["binding"]),
            "model": copy.deepcopy(profile["model"]),
            "protocol": profile["protocol"],
            "capabilities": {
                name: copy.deepcopy(profile["capabilities"][name])
                for name in sorted(role["mandatory_capabilities"])
            },
        }
        valid.append((preference[0], profile["priority"], profile["profile_id"], adapter["adapter_id"], selected, preference[1]))
    if not valid:
        if required_profile_id is not None:
            raise RoutingError(f"required profile is unavailable or policy-ineligible: {required_profile_id}", rejected)
        raise RoutingError("no policy-compliant execution profile", rejected)
    valid.sort(key=lambda value: value[:4])
    chosen = valid[0]
    fallback_authorized = mode == "development" and _fallback_allowed(policy, [(item[0], item[5]) for item in valid])
    if mode == "evaluation" and chosen[0] != 0:
        rejected.append({"profile_id": chosen[2], "reasons": ["evaluation mode disables fallback"]})
        raise RoutingError("evaluation mode cannot use a fallback profile", rejected)
    return {
        "schema": "evolvehls.agentic.stage-routing-decision",
        "schema_version": "1.0",
        "resolver_version": RESOLVER_VERSION,
        "registry_id": registry["registry_id"],
        "registry_version": registry["version"],
        "policy_id": policy["policy_id"],
        "policy_version": policy["version"],
        "mode": mode,
        "task_id": task["task_id"],
        "role_id": role["role_id"],
        "selected": chosen[4],
        "fallback_authorized": fallback_authorized,
        "explanation": [
            f"selected deterministically from {len(valid)} policy-compliant profile(s)",
            f"preference: {chosen[5]}",
            "fallback is explicitly authorized" if fallback_authorized else "fallback is disabled or not authorized",
        ],
        "rejected": sorted(rejected, key=lambda value: value["profile_id"]),
    }