from __future__ import annotations

import copy
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "tools" / "agentic"))

import contracts
import routing


class RoutingTests(unittest.TestCase):
    def setUp(self):
        fixture = ROOT / "agentic" / "fixtures"
        self.registry = contracts.load_contract(fixture / "profiles" / "portable-fixture-registry.json", "registry")
        self.policy = contracts.load_contract(fixture / "policies" / "local-first.json", "policy")
        self.readiness = contracts.load_contract(fixture / "readiness" / "portable-fixture-readiness.json", "readiness")
        contracts.validate_readiness(self.readiness, self.registry)
        self.task = contracts.load_contract(fixture / "tasks" / "fixture-task.json", "task")
        self.role = contracts.load_contract(ROOT / "agentic" / "roles" / "implementer.yaml", "role")

    def test_all_classes_are_represented_and_fixtures_validate(self):
        self.assertEqual({item["access_class"] for item in self.registry["profiles"]}, routing.ACCESS_CLASSES)
        self.assertEqual({item["funding_class"] for item in self.registry["profiles"]}, routing.FUNDING_CLASSES)
        self.assertEqual({item["auth_mode"] for item in self.registry["profiles"]}, routing.AUTH_MODES)
        for name in ("client-adapter", "execution-profile", "profile-registry", "routing-policy", "readiness-report", "stage-routing-decision"):
            self.assertEqual(contracts.load_schema(name)["$schema"], "https://json-schema.org/draft/2020-12/schema")
        profile_schema = contracts.load_schema("execution-profile")
        self.assertEqual(set(profile_schema["properties"]["access_class"]["enum"]), routing.ACCESS_CLASSES)
        self.assertEqual(set(profile_schema["properties"]["funding_class"]["enum"]), routing.FUNDING_CLASSES)
        self.assertEqual(set(profile_schema["properties"]["auth_mode"]["enum"]), routing.AUTH_MODES)

    def test_local_first_is_deterministic_and_immutable(self):
        original = copy.deepcopy((self.task, self.role, self.registry, self.policy, self.readiness))
        first = routing.resolve(self.task, self.role, self.registry, self.policy, self.readiness)
        second = routing.resolve(self.task, self.role, self.registry, self.policy, self.readiness)
        self.assertEqual(first, second)
        self.assertEqual(first["selected"]["profile_id"], "local-server")
        self.assertFalse(first["fallback_authorized"])
        self.assertEqual((self.task, self.role, self.registry, self.policy, self.readiness), original)
        contracts.validate_decision(first)

    def test_authorized_project_fallback_and_evaluation_no_fallback(self):
        readiness = copy.deepcopy(self.readiness)
        for item in readiness["profiles"]:
            if item["ref"] in {"local-server", "native-local"}:
                item["status"] = "unavailable"
        plan = routing.resolve(self.task, self.role, self.registry, self.policy, readiness)
        self.assertEqual(plan["selected"]["profile_id"], "project-gateway")
        self.assertTrue(plan["fallback_authorized"])
        with self.assertRaises(routing.RoutingError):
            routing.resolve(self.task, self.role, self.registry, self.policy, readiness, mode="evaluation")

    def test_policy_rejects_personal_and_unauthorized_transition(self):
        self.assertTrue(any(item["profile_id"] == "personal-api" and "policy prohibits funding_class: personal-api" in item["reasons"] for item in routing.resolve(self.task, self.role, self.registry, self.policy, self.readiness)["rejected"]))
        policy = copy.deepcopy(self.policy)
        policy["fallbacks"] = []
        readiness = copy.deepcopy(self.readiness)
        for item in readiness["profiles"]:
            if item["ref"] in {"local-server", "native-local"}:
                item["status"] = "unavailable"
        with self.assertRaises(routing.RoutingError) as error:
            routing.resolve(self.task, self.role, self.registry, policy, readiness)
        self.assertIn("policy does not authorize fallback transition", str(error.exception.rejected))

    def test_independent_review_requires_different_adapter_or_family(self):
        policy = copy.deepcopy(self.policy)
        policy["independent_review"] = {"required": True, "different_adapter": True, "different_execution_family": True}
        policy["preferences"] = [{"preference_id": "review", "access_classes": ["native-account-client"], "funding_classes": ["subscription"]}]
        policy["fallbacks"] = []
        prior = routing.resolve(self.task, self.role, self.registry, self.policy, self.readiness)
        plan = routing.resolve(self.task, self.role, self.registry, policy, self.readiness, prior=prior)
        self.assertEqual(plan["selected"]["adapter_id"], "codex-like")
        self.assertTrue(any(item["profile_id"] == "local-server" for item in plan["rejected"]))

    def test_invalid_combinations_duplicates_unknowns_and_readiness_fail_closed(self):
        bad = copy.deepcopy(self.registry)
        bad["profiles"][0]["funding_class"] = "project"
        with self.assertRaises(contracts.ContractError):
            contracts.validate_registry(bad)
        bad = copy.deepcopy(self.registry)
        bad["profiles"][0]["adapter_id"] = "missing"
        with self.assertRaises(contracts.ContractError):
            contracts.validate_registry(bad)
        bad = copy.deepcopy(self.registry)
        bad["profiles"].append(copy.deepcopy(bad["profiles"][0]))
        with self.assertRaises(contracts.ContractError):
            contracts.validate_registry(bad)
        bad_readiness = copy.deepcopy(self.readiness)
        bad_readiness["redacted"] = False
        with self.assertRaises(contracts.ContractError):
            contracts.validate_readiness(bad_readiness, self.registry)


if __name__ == "__main__":
    unittest.main()