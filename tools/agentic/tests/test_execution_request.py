#!/usr/bin/env python3
"""PAF-05A1b fixture route and immutable request tests; no execution occurs."""
from __future__ import annotations

import copy
import sys
import unittest
from pathlib import Path
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[3]
TOOLS = ROOT / "tools" / "agentic"
FIXTURES = ROOT / "agentic" / "fixtures"
sys.path.insert(0, str(TOOLS))

import adapters  # noqa: E402
import contracts  # noqa: E402
import execution_contracts  # noqa: E402
import execution_request  # noqa: E402
import local_state  # noqa: E402
import portable_adapters  # noqa: E402
import routing  # noqa: E402

BASE = "5d26a318537e1838405d80b952009317c270b8b0"


class FixtureExecutionRequestTests(unittest.TestCase):
    def setUp(self):
        self.task = contracts.load_contract(
            FIXTURES / "tasks" / "fixture-execution-task.json", "task"
        )
        self.role = contracts.load_contract(
            ROOT / "agentic" / "roles" / "implementer.yaml", "role"
        )
        self.registry = contracts.load_contract(
            FIXTURES / "profiles" / "fixture-execution-registry.json", "registry"
        )
        self.policy = contracts.load_contract(
            FIXTURES / "policies" / "fixture-execution-policy.json", "policy"
        )
        self.readiness = contracts.load_contract(
            FIXTURES / "readiness" / "fixture-execution-readiness.json",
            "readiness",
        )
        contracts.validate_readiness(self.readiness, self.registry)
        self.runtime_map = contracts.load_json(
            FIXTURES / "profiles" / "fixture-execution-runtime-map.json"
        )
        portable_adapters.validate_runtime_map(
            self.runtime_map,
            self.registry,
            templates=execution_request.fixture_templates(),
        )

    def decision(self, mode="development"):
        return routing.resolve(
            self.task,
            self.role,
            self.registry,
            self.policy,
            self.readiness,
            mode=mode,
            required_profile_id=execution_request.FIXTURE_PROFILE_ID,
        )

    def descriptor(self, decision=None):
        return portable_adapters.invocation_descriptor(
            self.registry,
            decision or self.decision(),
            self.readiness,
            self.runtime_map,
            templates=execution_request.fixture_templates(),
        )

    def build(self, **kwargs):
        options = {
            "execution_id": "fixture-run",
            "base_commit": BASE,
            "created_at": "2026-01-01T00:00:00+00:00",
        }
        options.update(kwargs)
        return execution_request.build_fixture_execution_request(
            self.task,
            self.role,
            self.registry,
            self.policy,
            self.readiness,
            self.runtime_map,
            **options,
        )

    def test_fixture_documents_validate_and_exact_commit(self):
        contracts.validate_task(self.task)
        contracts.validate_role(self.role)
        contracts.validate_registry(self.registry)
        contracts.validate_policy(self.policy)
        contracts.validate_readiness(self.readiness, self.registry)
        portable_adapters.validate_runtime_map(
            self.runtime_map,
            self.registry,
            templates=execution_request.fixture_templates(),
        )
        self.assertRegex(self.task["reproducibility"]["base_revision"], r"^[0-9a-f]{40}$")
        self.assertEqual(self.task["reproducibility"]["base_revision"], BASE)

    def test_canonical_routing_is_exact_deterministic_and_has_no_fallback(self):
        development = self.decision()
        evaluation = self.decision("evaluation")
        for decision in (development, evaluation):
            contracts.validate_decision(decision)
            self.assertEqual(decision["selected"]["profile_id"], "fixture-local-profile")
            self.assertEqual(decision["selected"]["adapter_id"], "fixture-local-adapter")
            self.assertFalse(decision["fallback_authorized"])
            self.assertEqual(decision["rejected"], [])
        self.assertEqual(
            {item["profile_id"] for item in self.registry["profiles"]},
            {"fixture-local-profile"},
        )

    def test_existing_descriptor_path_and_placeholders(self):
        decision = self.decision()
        with patch(
            "portable_adapters.invocation_descriptor",
            wraps=portable_adapters.invocation_descriptor,
        ) as builder:
            descriptor = self.descriptor(decision)
        builder.assert_called_once()
        self.assertEqual(descriptor["profile_id"], "fixture-local-profile")
        self.assertEqual(descriptor["adapter_id"], "fixture-local-adapter")
        self.assertEqual(descriptor["runtime_map_id"], "fixture-execution-runtime-map")
        self.assertEqual(descriptor["runtime_entry_id"], "fixture-local-runtime-entry")
        self.assertEqual(descriptor["invocation_class"], "native-local-cli")
        self.assertEqual(descriptor["execution_family"], "fixture-local")
        self.assertEqual(descriptor["routing_provenance"], {
            key: decision[key]
            for key in (
                "schema_version", "resolver_version", "registry_id",
                "registry_version", "policy_id", "policy_version", "mode",
                "task_id", "role_id",
            )
        })
        self.assertEqual(descriptor["input_handoff"], "PAF-05-defined")
        self.assertEqual(descriptor["working_directory_behavior"], "PAF-05-defined")
        self.assertEqual(descriptor["result_collection"], "PAF-05-defined")
        text = repr(descriptor).lower()
        for prohibited in ("endpoint", "credential", "command", "api_key"):
            self.assertNotIn(prohibited, text)

    def test_request_is_deterministic_and_digest_bound(self):
        first = self.build()
        second = self.build()
        self.assertEqual(first, second)
        execution_contracts.validate_execution_request(first)
        self.assertEqual(
            first["task_digest"],
            local_state.canonical_digest(
                self.task, domain="evolvehls.agentic.task"
            ),
        )
        self.assertEqual(
            first["role_digest"],
            local_state.canonical_digest(
                self.role, domain="evolvehls.agentic.role"
            ),
        )
        descriptor = self.descriptor()
        self.assertEqual(
            first["invocation_descriptor_digest"],
            local_state.canonical_digest(
                descriptor, domain="evolvehls.agentic.invocation-descriptor"
            ),
        )
        execution_contracts.validate_execution_request_context(
            first, self.task, self.role, self.decision(), descriptor, self.runtime_map
        )

    def test_builder_rejects_base_role_symbolic_and_limits(self):
        cases = [
            {"base_commit": "HEAD"},
            {"base_commit": "a" * 40},
            {"wall_clock_timeout_seconds": 0},
            {"stdout_limit_bytes": 0},
            {"total_output_limit_bytes": 1},
            {"total_output_limit_bytes": 131073},
        ]
        for options in cases:
            with self.subTest(options=options):
                with self.assertRaises(execution_request.ExecutionRequestError):
                    self.build(**options)
        bad_task = copy.deepcopy(self.task)
        bad_task["role"] = "reviewer"
        with self.assertRaises(execution_request.ExecutionRequestError):
            execution_request.build_fixture_execution_request(
                bad_task, self.role, self.registry, self.policy, self.readiness,
                self.runtime_map, execution_id="fixture-run", base_commit=BASE,
                created_at="2026-01-01T00:00:00+00:00",
            )

    def test_context_rejects_route_runtime_descriptor_and_provenance_mismatch(self):
        request = self.build()
        decision = self.decision()
        descriptor = self.descriptor(decision)
        mutations = []
        for field, attempted in (
            ("profile_id", "other-profile"),
            ("adapter_id", "other-adapter"),
            ("runtime_map_id", "other-map"),
            ("runtime_entry_id", "other-entry"),
            ("invocation_descriptor_digest", "b" * 64),
        ):
            changed = copy.deepcopy(request)
            changed[field] = attempted
            mutations.append((changed, decision, descriptor, self.runtime_map))
        changed = copy.deepcopy(request)
        changed["routing_provenance"]["policy_id"] = "other-policy"
        mutations.append((changed, decision, descriptor, self.runtime_map))
        changed_map = copy.deepcopy(self.runtime_map)
        changed_map["version"] = "2.0"
        mutations.append((request, decision, descriptor, changed_map))
        changed_decision = copy.deepcopy(decision)
        changed_decision["fallback_authorized"] = True
        mutations.append((request, changed_decision, descriptor, self.runtime_map))
        for values in mutations:
            with self.assertRaises(execution_contracts.ExecutionContractError):
                execution_contracts.validate_execution_request_context(
                    values[0], self.task, self.role, values[1], values[2], values[3]
                )

    def test_injected_template_shape_and_canonical_agreement(self):
        cases = ["not-a-mapping", {"fixture-local": None}]
        value = execution_request.fixture_templates()
        del value["fixture-local"]["paths"]
        cases.append(value)
        value = execution_request.fixture_templates()
        value["fixture-local"]["paths"] = "not-a-mapping"
        cases.append(value)
        for field in (
            "access_classes",
            "funding_classes",
            "auth_modes",
            "protocols",
            "invocation_class",
        ):
            value = execution_request.fixture_templates()
            del value["fixture-local"]["paths"]["fixture"][field]
            cases.append(value)
        for field in (
            "access_classes",
            "funding_classes",
            "auth_modes",
            "protocols",
        ):
            value = execution_request.fixture_templates()
            value["fixture-local"]["paths"]["fixture"][field] = "not-a-list"
            cases.append(value)
        value = execution_request.fixture_templates()
        value["fixture-local"]["execution_family"] = "other-family"
        cases.append(value)
        value = execution_request.fixture_templates()
        value["fixture-local"]["paths"]["fixture"]["invocation_class"] = (
            "http-api-client"
        )
        cases.append(value)
        for templates in cases:
            with self.assertRaises(portable_adapters.PortableAdapterError):
                portable_adapters.validate_runtime_map(
                    self.runtime_map, self.registry, templates=templates
                )

    def test_template_factory_resists_external_mutation(self):
        mutated = execution_request.fixture_templates()
        mutated["fixture-local"]["paths"]["fixture"]["invocation_class"] = (
            "http-api-client"
        )
        fresh = execution_request.fixture_templates()
        self.assertEqual(
            fresh["fixture-local"]["paths"]["fixture"]["invocation_class"],
            "native-local-cli",
        )
        request = self.build()
        descriptor = self.descriptor()
        self.assertEqual(descriptor["invocation_class"], "native-local-cli")
        self.assertEqual(
            request["invocation_descriptor_digest"],
            local_state.canonical_digest(
                descriptor, domain="evolvehls.agentic.invocation-descriptor"
            ),
        )

    def test_context_is_route_agnostic_and_builder_is_fixture_specific(self):
        request = self.build()
        decision = self.decision()
        descriptor = self.descriptor(decision)
        execution_contracts.validate_execution_request_context(
            request, self.task, self.role, decision, descriptor, self.runtime_map
        )

        alternate_request = copy.deepcopy(request)
        alternate_decision = copy.deepcopy(decision)
        alternate_descriptor = copy.deepcopy(descriptor)
        alternate_runtime_map = copy.deepcopy(self.runtime_map)
        for value in (
            alternate_request,
            alternate_decision["selected"],
            alternate_descriptor,
            alternate_runtime_map["entries"][0],
        ):
            value["profile_id"] = "alternate-profile"
            value["adapter_id"] = "alternate-adapter"
        alternate_request["invocation_descriptor_digest"] = (
            local_state.canonical_digest(
                alternate_descriptor,
                domain="evolvehls.agentic.invocation-descriptor",
            )
        )
        execution_contracts.validate_execution_request_context(
            alternate_request,
            self.task,
            self.role,
            alternate_decision,
            alternate_descriptor,
            alternate_runtime_map,
        )

        changed_decision = copy.deepcopy(decision)
        changed_decision["selected"]["profile_id"] = "other-profile"
        with self.assertRaises(execution_contracts.ExecutionContractError):
            execution_contracts.validate_execution_request_context(
                request,
                self.task,
                self.role,
                changed_decision,
                descriptor,
                self.runtime_map,
            )
        non_fixture_registry = copy.deepcopy(self.registry)
        non_fixture_registry["registry_id"] = "other-registry"
        with self.assertRaises(execution_request.ExecutionRequestError):
            execution_request.build_fixture_execution_request(
                self.task,
                self.role,
                non_fixture_registry,
                self.policy,
                self.readiness,
                self.runtime_map,
                execution_id="fixture-run",
                base_commit=BASE,
                created_at="2026-01-01T00:00:00+00:00",
            )

    def test_builder_defensively_copies_caller_inputs_and_output(self):
        original_task = copy.deepcopy(self.task)
        original_runtime_map = copy.deepcopy(self.runtime_map)
        request = self.build()
        request["routing_provenance"]["policy_id"] = "mutated-policy"
        self.assertEqual(self.task, original_task)
        self.assertEqual(self.runtime_map, original_runtime_map)
        self.assertEqual(self.policy["policy_id"], "fixture-execution-policy")
        with self.assertRaises(execution_contracts.ExecutionContractError):
            execution_contracts.validate_execution_request_context(
                request,
                self.task,
                self.role,
                self.decision(),
                self.descriptor(),
                self.runtime_map,
            )

    def test_production_behavior_with_explicit_none_templates(self):
        production = contracts.load_contract(
            FIXTURES / "profiles" / "portable-fixture-registry.json", "registry"
        )
        runtime_map = contracts.load_json(
            FIXTURES / "profiles" / "portable-adapter-registry.json"
        )
        portable_adapters.validate_runtime_map(runtime_map, production)
        portable_adapters.validate_runtime_map(
            runtime_map, production, templates=None
        )

    def test_fixture_metadata_is_absent_from_production_defaults(self):
        self.assertNotIn("fixture-local", adapters.TEMPLATES)
        detected = adapters.detect_all(
            which=lambda _: None,
        )
        self.assertNotIn("fixture-local", {item["adapter_id"] for item in detected})
        production = contracts.load_contract(
            FIXTURES / "profiles" / "portable-fixture-registry.json", "registry"
        )
        self.assertNotIn(
            "fixture-local-profile",
            {item["profile_id"] for item in production["profiles"]},
        )
        with self.assertRaises(portable_adapters.PortableAdapterError):
            portable_adapters.validate_runtime_map(self.runtime_map, self.registry)


if __name__ == "__main__":
    unittest.main()