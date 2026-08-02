from __future__ import annotations

import copy
import io
import json
import sys
import tempfile
import unittest
from contextlib import redirect_stderr
from datetime import datetime, timedelta, timezone
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "tools" / "agentic"))

import agentctl
import contracts
import portable_adapters
import provider_onboarding
import provider_overlays
import routing


class ProviderOnboardingTests(unittest.TestCase):
    def setUp(self):
        self.now = datetime(2026, 2, 3, 4, 5, 6, tzinfo=timezone.utc)
        self.spec = {
            "schema": provider_onboarding.SPEC_SCHEMA,
            "schema_version": "1.0",
            "provider_id": "fixture-gateway",
            "endpoint": {
                "origin": "https://gateway.example.invalid/v1",
                "protocol": "openai-compatible",
            },
            "authentication": {"mode": "environment-token", "env_var": "FIXTURE_PROVIDER_TOKEN"},
            "model": "coding/model:v1",
            "roles": ["implementation"],
        }

    def _write(self, root: Path, name: str, value: str) -> Path:
        path = root / name
        path.write_text(value, encoding="utf-8")
        return path

    def _json(self) -> str:
        return json.dumps(self.spec)

    def test_json_yaml_loading_and_parse_failures(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            json_path = self._write(root, "provider.json", self._json())
            self.assertEqual(provider_onboarding.load_spec(json_path), self.spec)
            yaml_path = self._write(
                root,
                "provider.yaml",
                "\n".join(
                    [
                        "schema: evolvehls.agentic.provider-onboarding-spec",
                        "schema_version: '1.0'",
                        "provider_id: fixture-gateway",
                        "endpoint:",
                        "  origin: https://gateway.example.invalid/v1",
                        "  protocol: openai-compatible",
                        "authentication:",
                        "  mode: environment-token",
                        "  env_var: FIXTURE_PROVIDER_TOKEN",
                        "model: coding/model:v1",
                        "roles:",
                        "  - implementation",
                    ]
                ),
            )
            try:
                self.assertEqual(provider_onboarding.load_spec(yaml_path), self.spec)
            except provider_onboarding.ProviderOnboardingError as error:
                self.assertIn("YAML support", str(error))
            for name, content in (("bad.json", "{"), ("bad.yaml", "endpoint: ["), ("sequence.yaml", "- item")):
                with self.subTest(name=name):
                    with self.assertRaises(provider_onboarding.ProviderOnboardingError):
                        provider_onboarding.load_spec(self._write(root, name, content))
            duplicate = self._write(root, "duplicate.yaml", "schema: x\nschema: y\n")
            try:
                with self.assertRaises(provider_onboarding.ProviderOnboardingError):
                    provider_onboarding.load_spec(duplicate)
            except AssertionError:
                self.skipTest("PyYAML is unavailable in the current interpreter")
            with self.assertRaises(provider_onboarding.ProviderOnboardingError):
                provider_onboarding.load_spec(self._write(root, "provider.txt", self._json()))

    def test_contract_validators_reject_unknowns_secrets_and_bad_evidence(self):
        contracts.validate_provider_onboarding_spec(self.spec)
        bad = copy.deepcopy(self.spec)
        bad["secret"] = "not-allowed"
        with self.assertRaises(contracts.ContractError):
            contracts.validate_provider_onboarding_spec(bad)
        evidence = {
            "schema": provider_onboarding.EVIDENCE_SCHEMA,
            "schema_version": "1.0",
            "provider_id": "fixture-gateway",
            "checked_at": self.now.isoformat(),
            "method": "not-requested",
            "status": "not-performed",
            "diagnostics": [],
        }
        contracts.validate_provider_discovery_evidence(evidence)
        evidence["status"] = "manual"
        with self.assertRaises(contracts.ContractError):
            contracts.validate_provider_discovery_evidence(evidence)

    def test_capability_evidence_is_structurally_strict(self):
        registry, runtime_map = provider_onboarding._builtin()
        contracts.validate_registry(registry)
        for profile in registry["profiles"]:
            contracts.validate_profile(profile)
        config = provider_onboarding.configuration(self.spec, clock=lambda: self.now)
        profile_overlay, _ = provider_onboarding.overlays(config, registry, runtime_map)
        contracts.validate_provider_profile_overlay(profile_overlay, registry)
        for field in (
            "apiKey",
            "access_token",
            "refresh_token",
            "client_secret",
            "private_key",
            "headers",
            "metadata",
            "arbitrary",
        ):
            with self.subTest(field=field):
                malformed = copy.deepcopy(profile_overlay)
                malformed["profile"]["capabilities"]["basic_text"][field] = {"not": "allowed"}
                with self.assertRaisesRegex(contracts.ContractError, "capability evidence fields"):
                    contracts.validate_provider_profile_overlay(malformed, registry)

    def test_deterministic_ids_are_safe_and_material(self):
        first = provider_onboarding.derive_ids(self.spec)
        self.assertEqual(first, provider_onboarding.derive_ids(copy.deepcopy(self.spec)))
        self.assertNotIn("FIXTURE_PROVIDER_TOKEN", json.dumps(first))
        changed = copy.deepcopy(self.spec)
        changed["model"] = "coding:model/v1"
        second = provider_onboarding.derive_ids(changed)
        self.assertNotEqual(first["profile_id"], second["profile_id"])
        self.assertRegex(first["profile_id"], r"^[a-z][a-z0-9.-]*$")
        for field in ("origin", "protocol"):
            changed = copy.deepcopy(self.spec)
            changed["endpoint"][field] = "https://other.invalid/v1" if field == "origin" else "anthropic-compatible"
            self.assertNotEqual(first["profile_id"], provider_onboarding.derive_ids(changed)["profile_id"])

    def test_apply_idempotency_replacement_and_rollback(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            first = provider_onboarding.apply(root, self.spec, clock=lambda: self.now)
            self.assertFalse(first["idempotent"])
            self.assertTrue(provider_onboarding.apply(root, self.spec, clock=lambda: self.now)["idempotent"])
            changed = copy.deepcopy(self.spec)
            changed["endpoint"]["origin"] = "https://other.example.invalid/v1"
            with self.assertRaisesRegex(provider_onboarding.ProviderOnboardingError, "material provider"):
                provider_onboarding.apply(root, changed, clock=lambda: self.now)
            original = (root / ".agentic-local/providers/fixture-gateway.json").read_bytes()
            for failure in range(1, 5):
                with self.subTest(failure=failure):
                    with self.assertRaises(provider_onboarding.ProviderOnboardingError):
                        provider_onboarding.apply(root, changed, replace=True, clock=lambda: self.now, fail_after=failure)
                    self.assertEqual((root / ".agentic-local/providers/fixture-gateway.json").read_bytes(), original)
            result = provider_onboarding.apply(root, changed, replace=True, clock=lambda: self.now)
            self.assertFalse(result["idempotent"])
            self.assertTrue((root / ".agentic-local/backups/fixture-gateway.json.bak").exists())

    def test_composition_collisions_and_canonical_routing(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            provider_onboarding.apply(root, self.spec, clock=lambda: self.now)
            registry, runtime_map, provenance = provider_onboarding.effective_documents(root)
            self.assertEqual(registry["registry_id"], "portable-fixture-registry")
            self.assertEqual(runtime_map["runtime_map_id"], "portable-fixture-runtime-map")
            self.assertEqual(len(provenance["profile_overlays"]), 1)
            config = provider_onboarding.load_provider(root, "fixture-gateway")
            self.assertIn(config["canonical"]["profile_id"], {item["profile_id"] for item in registry["profiles"]})
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
            role = {"schema": "evolvehls.agentic.role", "schema_version": "1.0", "role_id": "implementation", "version": "1", "mandatory_capabilities": ["basic_text"], "preferred_capabilities": [], "default_objective": "delivery", "permitted_modes": ["development"], "minimum_confidence": "declared"}
            task = {"schema": "evolvehls.agentic.task", "schema_version": "1.0", "task_id": "fixture", "version": "1", "role": "implementation", "objective": "delivery", "inputs": [], "constraints": [], "expected_outputs": [], "validation_requirements": [], "budgets": {}, "reproducibility": {"context_hash": "x", "base_revision": "y"}}
            policy = {"schema": "evolvehls.agentic.routing-policy", "schema_version": "1.0", "policy_id": "fixture", "version": "1", "allowed_access_classes": ["api-gateway"], "allowed_funding_classes": ["personal-api"], "allowed_auth_modes": ["environment-token"], "allowed_data_classes": ["public"], "max_cost_tier": 1, "available_resources": [], "preferences": [{"preference_id": "gateway", "access_classes": ["api-gateway"], "funding_classes": ["personal-api"]}], "fallbacks": [], "independent_review": {"required": False, "different_adapter": False, "different_execution_family": False}}
            decision = routing.resolve(task, role, registry, policy, readiness)
            descriptor = portable_adapters.invocation_descriptor(
                registry, decision, readiness, runtime_map, composition_provenance=provenance
            )
            self.assertEqual(descriptor["profile_id"], config["canonical"]["profile_id"])
            self.assertEqual(descriptor["adapter_id"], "generic-http")
            self.assertEqual(descriptor["runtime_entry_id"], config["canonical"]["runtime_entry_id"])
            self.assertEqual(descriptor["composition_provenance"]["registry"]["source"], "builtin")
            self.assertEqual(len(descriptor["composition_provenance"]["profile_overlays"]), 1)
            self.assertNotIn("TOKEN", json.dumps((registry, runtime_map, descriptor)))
            profile_overlay, runtime_overlay = provider_onboarding.overlays(config, *provider_onboarding._builtin())
            conflict = copy.deepcopy(profile_overlay)
            conflict["profile"]["model"]["ref"] = "other"
            with self.assertRaises(provider_overlays.OverlayError):
                provider_overlays.compose(*provider_onboarding._builtin(), [profile_overlay, conflict], [runtime_overlay])

    def test_removal_preview_references_and_rollback(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            provider_onboarding.apply(root, self.spec, clock=lambda: self.now)
            preview = provider_onboarding.remove(root, "fixture-gateway", dry_run=True)
            self.assertFalse(preview["removed"])
            self.assertTrue((root / ".agentic-local/providers/fixture-gateway.json").exists())
            profile = provider_onboarding.load_provider(root, "fixture-gateway")["canonical"]["profile_id"]
            state = root / "agentic-state/readiness"
            state.mkdir(parents=True)
            (state / "active.json").write_text(profile, encoding="utf-8")
            with self.assertRaises(provider_onboarding.ProviderOnboardingError):
                provider_onboarding.remove(root, "fixture-gateway")
            (state / "active.json").unlink()
            for failure in range(1, 5):
                with self.subTest(failure=failure):
                    with self.assertRaises(provider_onboarding.ProviderOnboardingError):
                        provider_onboarding.remove(root, "fixture-gateway", fail_after=failure)
                    self.assertTrue((root / ".agentic-local/providers/fixture-gateway.json").exists())
            self.assertTrue(provider_onboarding.remove(root, "fixture-gateway")["removed"])
            self.assertTrue(provider_onboarding.remove(root, "fixture-gateway")["already_removed"])

    def test_cli_preview_failures_and_non_tty_add(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            spec_path = self._write(root, "provider.json", self._json())
            self.assertEqual(agentctl.main(["provider", "preview", "--spec", str(spec_path), "--root", str(root)]), 0)
            self.assertFalse((root / ".agentic-local").exists())
            stderr = io.StringIO()
            with redirect_stderr(stderr):
                self.assertEqual(agentctl.main(["provider", "add", "--root", str(root)]), 3)
            self.assertIn("interactive TTY", stderr.getvalue())
            self.assertFalse((root / ".agentic-local").exists())


if __name__ == "__main__":
    unittest.main()