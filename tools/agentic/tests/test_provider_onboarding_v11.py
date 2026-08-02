from __future__ import annotations

import copy
import sys
import tempfile
import unittest
from datetime import datetime, timezone
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "tools" / "agentic"))

import contracts
import provider_onboarding
import routing


class ProviderOnboardingV11Tests(unittest.TestCase):
    def setUp(self):
        self.now = datetime(2026, 2, 3, 4, 5, 6, tzinfo=timezone.utc)
        self.spec = {
            "schema": "evolvehls.agentic.provider-onboarding-spec",
            "schema_version": "1.1",
            "provider_id": "fixture-multi",
            "display_name": "Fixture Multi",
            "endpoint": {
                "origin": "https://gateway.example.invalid/v1",
                "protocol": "openai-compatible",
            },
            "authentication": {"mode": "none"},
            "models": ["model-a", "model-b"],
            "role_assignments": [
                {"role_id": "planning", "model": "model-a"},
                {"role_id": "implementation", "model": "model-b"},
                {"role_id": "review", "model": "model-a"},
            ],
            "execution_protocol": "openai-chat-completions",
            "discovery_evidence": {
                "schema": "evolvehls.agentic.provider-discovery-evidence",
                "schema_version": "1.1",
                "provider_id": "fixture-multi",
                "endpoint_origin": "https://gateway.example.invalid/v1",
                "checked_at": self.now.isoformat(),
                "method": "manual",
                "request_path": None,
                "status": "manual",
                "listing_protocol": {
                    "value": "openai-compatible",
                    "origin": "user-confirmed",
                    "confidence": "declared",
                },
                "authentication": "not-requested",
                "models": [],
                "truncated": False,
                "failure": None,
                "diagnostics": [],
            },
        }

    def test_v11_validates_and_generates_distinct_canonical_profiles(self):
        contracts.validate_provider_onboarding_spec(self.spec)
        config = provider_onboarding.configuration(self.spec, clock=lambda: self.now)
        contracts.validate_provider_configuration(config)
        self.assertEqual(config["canonical"]["role_profiles"]["planning"], config["canonical"]["role_profiles"]["review"])
        self.assertNotEqual(
            config["canonical"]["role_profiles"]["planning"],
            config["canonical"]["role_profiles"]["implementation"],
        )
        profiles, runtimes = provider_onboarding.overlays(config, *provider_onboarding._builtin())
        self.assertEqual(profiles["schema_version"], "1.1")
        self.assertEqual(runtimes["schema_version"], "1.1")
        self.assertEqual(len(profiles["profiles"]), 2)
        self.assertEqual(len(runtimes["entries"]), 2)

    def test_v11_apply_is_transactional_and_idempotent(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            first = provider_onboarding.apply(root, self.spec, clock=lambda: self.now)
            self.assertFalse(first["idempotent"])
            self.assertTrue(provider_onboarding.apply(root, self.spec, clock=lambda: self.now)["idempotent"])
            receipt = provider_onboarding._read(root, "agentic-state", "provider-setup", "fixture-multi.json")
            self.assertEqual(receipt["schema_version"], "1.1")
            self.assertEqual(receipt["model_count"], 2)
            registry, runtime_map, _ = provider_onboarding.effective_documents(root)
            profile_ids = set(provider_onboarding.load_provider(root, "fixture-multi")["canonical"]["role_profiles"].values())
            self.assertEqual(
                profile_ids,
                {
                    item["profile_id"]
                    for item in registry["profiles"]
                    if item["profile_id"] in profile_ids
                },
            )
            self.assertEqual(
                profile_ids,
                {
                    item["profile_id"]
                    for item in runtime_map["entries"]
                    if item["profile_id"] in profile_ids
                },
            )

    def test_refresh_spec_preserves_assignments_and_requires_explicit_replace(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            provider_onboarding.apply(root, self.spec, clock=lambda: self.now)
            config = provider_onboarding.load_provider(root, "fixture-multi")
            evidence = copy.deepcopy(config["discovery_evidence"])
            evidence["status"] = "succeeded"
            evidence["method"] = "openai-model-list"
            evidence["request_path"] = "/v1/models"
            evidence["authentication"] = "not-required"
            evidence["models"] = [
                {"model_id": "model-a", "origin": "endpoint-reported", "confidence": "observed", "context_window": None}
            ]
            refreshed = provider_onboarding.refresh_spec(config, evidence)
            self.assertEqual(
                {item["role_id"]: item["model"] for item in refreshed["role_assignments"]},
                {item["role_id"]: item["model"] for item in self.spec["role_assignments"]},
            )
            self.assertEqual(refreshed["models"], self.spec["models"])
            with self.assertRaises(provider_onboarding.ProviderOnboardingError):
                provider_onboarding.apply(root, refreshed, clock=lambda: self.now)
            provider_onboarding.apply(root, refreshed, replace=True, clock=lambda: self.now)
            persisted = provider_onboarding.load_provider(root, "fixture-multi")
            self.assertEqual(
                {item["role_id"]: item["model"] for item in persisted["role_assignments"]},
                {item["role_id"]: item["model"] for item in self.spec["role_assignments"]},
            )
            self.assertEqual(persisted["discovery_evidence"]["models"][0]["model_id"], "model-a")

    def test_required_profile_constraint_is_fail_closed(self):
        registry, _ = provider_onboarding._builtin()
        profile = next(item for item in registry["profiles"] if item["profile_id"] == "personal-api")
        readiness = {
            "schema": "evolvehls.agentic.readiness-report",
            "schema_version": "1.0",
            "registry_id": registry["registry_id"],
            "checked_at": self.now.isoformat(),
            "redacted": True,
            "adapters": [{"ref": item["adapter_id"], "status": "ready"} for item in registry["adapters"]],
            "profiles": [{"ref": item["profile_id"], "status": "ready"} for item in registry["profiles"]],
            "resources": [],
            "diagnostics": [],
        }
        role = {
            "schema": "evolvehls.agentic.role",
            "schema_version": "1.0",
            "role_id": "implementation",
            "version": "1",
            "mandatory_capabilities": ["basic_text"],
            "preferred_capabilities": [],
            "default_objective": "delivery",
            "permitted_modes": ["development"],
            "minimum_confidence": "declared",
        }
        task = {
            "schema": "evolvehls.agentic.task",
            "schema_version": "1.0",
            "task_id": "fixture",
            "version": "1",
            "role": "implementation",
            "objective": "delivery",
            "inputs": [],
            "constraints": [],
            "expected_outputs": [],
            "validation_requirements": [],
            "budgets": {},
            "reproducibility": {"context_hash": "x", "base_revision": "y"},
        }
        policy = {
            "schema": "evolvehls.agentic.routing-policy",
            "schema_version": "1.0",
            "policy_id": "fixture",
            "version": "1",
            "allowed_access_classes": ["api-gateway"],
            "allowed_funding_classes": ["personal-api"],
            "allowed_auth_modes": ["environment-token"],
            "allowed_data_classes": ["public"],
            "max_cost_tier": 1,
            "available_resources": [],
            "preferences": [{"preference_id": "gateway", "access_classes": ["api-gateway"], "funding_classes": ["personal-api"]}],
            "fallbacks": [],
            "independent_review": {"required": False, "different_adapter": False, "different_execution_family": False},
        }
        decision = routing.resolve(task, role, registry, policy, readiness, required_profile_id=profile["profile_id"])
        self.assertEqual(decision["selected"]["profile_id"], profile["profile_id"])
        with self.assertRaises(routing.RoutingError):
            routing.resolve(task, role, registry, policy, readiness, required_profile_id="missing-profile")


if __name__ == "__main__":
    unittest.main()