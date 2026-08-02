from __future__ import annotations

import copy
import io
import json
import os
import stat
import subprocess
import sys
import tempfile
import unittest
from contextlib import redirect_stderr
from datetime import datetime, timedelta, timezone
from pathlib import Path
from unittest.mock import Mock, patch

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "tools" / "agentic"))

import adapters
import agentctl
import contracts
import local_state
import portable_adapters
import redaction
import routing
import setup

FIXTURES = ROOT / "agentic" / "fixtures"
REGISTRY_PATH = FIXTURES / "profiles" / "portable-fixture-registry.json"
RUNTIME_MAP_PATH = FIXTURES / "profiles" / "portable-adapter-registry.json"


class RedactionTests(unittest.TestCase):
    def test_free_form_secrets_headers_endpoints_and_identities_are_removed(self):
        unsafe = "Authorization: Bearer sk-secretvalue123456 https://private.invalid/x?token=abc user@example.invalid"
        value = redaction.normalize({"diagnostic": unsafe, "cookie": "session=opaque"})
        rendered = json.dumps(value)
        self.assertNotIn("private.invalid", rendered)
        self.assertNotIn("user@example.invalid", rendered)
        self.assertNotIn("sk-secretvalue", rendered)
        self.assertTrue(redaction.is_safe(value))

    def test_diagnostics_are_classified_not_trusted(self):
        self.assertEqual(redaction.safe_diagnostics(["raw provider timeout body", "unknown input"]), ["timeout", "unknown"])

    def test_percent_encoded_credentials_are_redacted_at_bounded_decode_depth(self):
        encoded_values = (
            "Authorization%3A%20Bearer%20abcdef123456",
            "token%3Dsecret-value",
            "AUTHORIZATION%3a%20bEaReR%20abcdef123456",
            "Authorization%253A%2520Bearer%2520abcdef123456",
        )
        for value in encoded_values:
            self.assertFalse(redaction.is_safe(value))
            self.assertEqual(redaction.redact_text(value), redaction.REDACTED)
            self.assertTrue(redaction.is_safe(redaction.redact_text(value)))

    def test_benign_encoded_text_and_nested_encoded_credentials_are_handled_safely(self):
        benign = "status%20message%3A%20ready"
        self.assertEqual(redaction.redact_text(benign), benign)
        self.assertTrue(redaction.is_safe(benign))
        nested = {"outer": ["safe", {"diagnostic": "token%3Dsecret-value"}]}
        normalized = redaction.normalize(nested)
        self.assertEqual(normalized["outer"][1]["diagnostic"], redaction.REDACTED)
        self.assertTrue(redaction.is_safe(normalized))


class DetectionTests(unittest.TestCase):
    def run(self, version: str) -> Mock:  # type: ignore[override]
        return Mock(return_value=subprocess.CompletedProcess([], 0, version, ""))

    def test_codex_native_and_api_are_independent(self):
        result = adapters.detect("codex", which=lambda _: "/fixture/codex", run=self.run("codex 1.2.3"))
        states = {item["execution_path_id"]: item["state"] for item in result["paths"]}
        self.assertEqual(states["native-account"], "unknown")
        self.assertEqual(states["configured-api"], "configuration-required")

    def test_unavailable_malformed_and_newer_versions_fail_closed(self):
        unavailable = adapters.detect("codex", which=lambda _: None)
        self.assertTrue(all(item["state"] == "unavailable" for item in unavailable["paths"]))
        malformed = adapters.detect("codex", which=lambda _: "/fixture/codex", run=self.run("unparseable"))
        newer = adapters.detect("codex", which=lambda _: "/fixture/codex", run=self.run("codex 2.0.0"))
        self.assertTrue(all(item["state"] == "unknown" for item in malformed["paths"]))
        self.assertTrue(all(item["state"] == "unsupported-version" for item in newer["paths"]))


class CanonicalRuntimeTests(unittest.TestCase):
    def setUp(self):
        self.registry = contracts.load_contract(REGISTRY_PATH, "registry")
        self.runtime_map = contracts.load_json(RUNTIME_MAP_PATH)
        self.policy = contracts.load_contract(FIXTURES / "policies" / "local-first.json", "policy")
        self.readiness = contracts.load_contract(FIXTURES / "readiness" / "portable-fixture-readiness.json", "readiness")
        self.task = contracts.load_contract(FIXTURES / "tasks" / "fixture-task.json", "task")
        self.role = contracts.load_contract(ROOT / "agentic" / "roles" / "implementer.yaml", "role")
        self.decision = routing.resolve(self.task, self.role, self.registry, self.policy, self.readiness, "development")

    def test_schema_runtime_map_and_routed_descriptor_interoperate(self):
        self.assertEqual(
            contracts.load_schema("client-runtime-map")["$schema"],
            "https://json-schema.org/draft/2020-12/schema",
        )
        portable_adapters.validate_runtime_map(self.runtime_map, self.registry)
        descriptor = portable_adapters.invocation_descriptor(
            self.registry,
            self.decision,
            self.readiness,
            self.runtime_map,
        )
        self.assertEqual(descriptor["profile_id"], self.decision["selected"]["profile_id"])
        self.assertEqual(descriptor["adapter_id"], self.decision["selected"]["adapter_id"])
        self.assertEqual(descriptor["runtime_entry_id"], "local-server-direct-http")
        self.assertEqual(descriptor["execution_path"], "openai-compatible")
        self.assertEqual(descriptor["routing_provenance"]["registry_id"], self.registry["registry_id"])
        self.assertNotIn("portable_profile_id", descriptor)

    def test_runtime_map_unknown_wrong_and_duplicate_references_fail_closed(self):
        bad = copy.deepcopy(self.runtime_map)
        bad["entries"][0]["profile_id"] = "missing"
        with self.assertRaises(portable_adapters.PortableAdapterError):
            portable_adapters.validate_runtime_map(bad, self.registry)
        bad = copy.deepcopy(self.runtime_map)
        bad["entries"][0]["adapter_id"] = "codex-like"
        with self.assertRaises(portable_adapters.PortableAdapterError):
            portable_adapters.validate_runtime_map(bad, self.registry)
        bad = copy.deepcopy(self.runtime_map)
        bad["entries"].append(copy.deepcopy(bad["entries"][0]))
        bad["entries"][-1]["runtime_entry_id"] = "another-local-server-path"
        with self.assertRaises(portable_adapters.PortableAdapterError):
            portable_adapters.validate_runtime_map(bad, self.registry)

    def test_runtime_map_public_identifiers_must_match_documented_pattern(self):
        for field, value in (
            ("runtime_map_id", "   "),
            ("runtime_entry_id", "bad/entry"),
            ("profile_id", "profile id"),
            ("adapter_id", "adapter_id"),
        ):
            bad = copy.deepcopy(self.runtime_map)
            target = bad if field == "runtime_map_id" else bad["entries"][0]
            target[field] = value
            with self.subTest(field=field), self.assertRaises(portable_adapters.PortableAdapterError):
                portable_adapters.validate_runtime_map(bad, self.registry)

    def test_decision_and_readiness_mismatches_fail_closed(self):
        bad_decision = copy.deepcopy(self.decision)
        bad_decision["registry_version"] = "other"
        with self.assertRaises(portable_adapters.PortableAdapterError):
            portable_adapters.invocation_descriptor(self.registry, bad_decision, self.readiness, self.runtime_map)
        bad_readiness = copy.deepcopy(self.readiness)
        bad_readiness["registry_id"] = "other"
        with self.assertRaises(portable_adapters.PortableAdapterError):
            portable_adapters.invocation_descriptor(self.registry, self.decision, bad_readiness, self.runtime_map)
        bad_readiness = copy.deepcopy(self.readiness)
        bad_readiness["profiles"] = [item for item in bad_readiness["profiles"] if item["ref"] != "local-server"]
        with self.assertRaises(portable_adapters.PortableAdapterError):
            portable_adapters.invocation_descriptor(self.registry, self.decision, bad_readiness, self.runtime_map)

    def test_stale_readiness_requires_an_explicit_policy(self):
        with self.assertRaises(portable_adapters.PortableAdapterError):
            portable_adapters.invocation_descriptor(
                self.registry,
                self.decision,
                self.readiness,
                self.runtime_map,
                now=datetime(2026, 1, 2, tzinfo=timezone.utc),
                max_readiness_age=timedelta(minutes=1),
            )


class LocalStateTests(unittest.TestCase):
    def test_safe_writes_are_atomic_restrictive_idempotent_and_refuse_overwrite(self):
        plan = local_state.PlannedWrite(local_state.LOCAL_DIR, ("generated", "fixture.json"), b"{}")
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            first = local_state.commit(root, [plan])
            self.assertEqual(stat.S_IMODE(first[0].stat().st_mode), 0o600)
            self.assertEqual(local_state.commit(root, [plan]), [])
            changed = local_state.PlannedWrite(local_state.LOCAL_DIR, ("generated", "fixture.json"), b'{"changed":true}')
            with self.assertRaises(local_state.LocalStateError):
                local_state.commit(root, [changed])
            local_state.commit(root, [changed], replace=True)
            self.assertTrue((root / ".agentic-local" / "backups" / "fixture.json.bak").exists())

    def test_traversal_and_symlink_are_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            with self.assertRaises(local_state.LocalStateError):
                local_state.safe_path(root, local_state.LOCAL_DIR, "../escape.json")
            target = root / "outside"
            target.write_text("x")
            link = root / ".agentic-local" / "generated"
            link.parent.mkdir()
            os.symlink(target, link)
            with self.assertRaises(local_state.LocalStateError):
                local_state.commit(root, [local_state.PlannedWrite(local_state.LOCAL_DIR, ("generated", "fixture.json"), b"{}")])

    def test_identifier_length_reserves_json_and_backup_suffixes(self):
        maximum = "a" * local_state.MAX_IDENTIFIER_LENGTH
        self.assertEqual(local_state.validate_identifier(maximum, "identifier"), maximum)
        self.assertEqual(
            local_state.validate_identifier(f"{maximum}.json", "filename"),
            f"{maximum}.json",
        )
        with self.assertRaises(local_state.LocalStateError):
            local_state.validate_identifier("a" * (local_state.MAX_IDENTIFIER_LENGTH + 1), "identifier")
        maximum_json = "a" * (local_state.MAX_JSON_NAME_LENGTH - len(".json")) + ".json"
        self.assertEqual(local_state.validate_identifier(maximum_json, "filename"), maximum_json)
        with self.assertRaises(local_state.LocalStateError):
            local_state.validate_identifier(
                f"{'a' * (local_state.MAX_JSON_NAME_LENGTH - len('.json') + 1)}.json",
                "filename",
            )
        self.assertLessEqual(
            len(f"{maximum_json}.bak"),
            local_state.MAX_FILENAME_LENGTH,
        )


class SetupTests(unittest.TestCase):
    def setUp(self):
        self.registry = contracts.load_contract(REGISTRY_PATH, "registry")
        self.runtime_map = contracts.load_json(RUNTIME_MAP_PATH)
        policy = contracts.load_contract(FIXTURES / "policies" / "local-first.json", "policy")
        readiness = contracts.load_contract(FIXTURES / "readiness" / "portable-fixture-readiness.json", "readiness")
        task = contracts.load_contract(FIXTURES / "tasks" / "fixture-task.json", "task")
        role = contracts.load_contract(ROOT / "agentic" / "roles" / "implementer.yaml", "role")
        self.spec = {
            "schema": setup.SETUP_SCHEMA,
            "schema_version": "1.0",
            "registry": self.registry,
            "runtime_map": self.runtime_map,
            "routing_decision": routing.resolve(task, role, self.registry, policy, readiness),
            "probe_authorization": "not-requested",
        }
        self.detected = [adapters.detect("direct-http")]
        self.instant = datetime(2026, 2, 3, 4, 5, 6, tzinfo=timezone.utc)

    def test_injected_timestamp_is_preserved_in_readiness_and_receipt(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            result = setup.apply(root, self.spec, detected=self.detected, clock=lambda: self.instant)
            self.assertEqual(result["checked_at"], self.instant.isoformat())
            report_path = root / "agentic-state" / "readiness" / "portable-fixture-registry.json"
            receipt_path = root / "agentic-state" / "setup" / "portable-fixture-registry.json"
            report = json.loads(report_path.read_text())
            receipt = json.loads(receipt_path.read_text())
            contracts.validate_readiness(report, self.registry)
            self.assertEqual(report["checked_at"], self.instant.isoformat())
            self.assertEqual(receipt["checked_at"], self.instant.isoformat())
            self.assertFalse(receipt["probes_performed"])

    def test_default_timestamp_is_aware_current_and_portable_readiness_cannot_be_persisted(self):
        before = datetime.now(timezone.utc)
        report = portable_adapters.readiness(self.registry, self.runtime_map, self.detected)
        after = datetime.now(timezone.utc)
        observed = contracts.parse_timestamp(report["checked_at"])
        self.assertLessEqual(before, observed)
        self.assertLessEqual(observed, after)
        self.assertEqual(report["schema"], "evolvehls.agentic.readiness-report")
        portable = {"schema": "evolvehls.agentic.portable-readiness-report"}
        with tempfile.TemporaryDirectory() as directory:
            target = Path(directory) / "agentic-state" / "readiness" / "portable-fixture-registry.json"
            target.parent.mkdir(parents=True)
            target.write_text(json.dumps(portable))
            with self.assertRaises(contracts.ContractError):
                contracts.load_contract(target, "readiness")


class CliTests(unittest.TestCase):
    def test_portable_adapter_error_is_classified_without_traceback_or_shadow_attribute_error(self):
        with tempfile.TemporaryDirectory() as directory:
            spec_path = Path(directory) / "valid-json.json"
            spec_path.write_text("{}")
            stderr = io.StringIO()
            with patch.object(
                setup,
                "validate_spec",
                side_effect=portable_adapters.PortableAdapterError("unsafe runtime metadata"),
            ), redirect_stderr(stderr):
                result = agentctl.main(["config", "validate", "--spec", str(spec_path)])
        diagnostic = stderr.getvalue()
        self.assertEqual(result, 3)
        self.assertIn("agentctl: unsafe runtime metadata", diagnostic)
        self.assertNotIn("Traceback", diagnostic)
        self.assertNotIn("AttributeError", diagnostic)

    def test_invalid_setup_is_classified_without_traceback_or_shadow_attribute_error(self):
        with tempfile.TemporaryDirectory() as directory:
            spec_path = Path(directory) / "invalid.json"
            spec_path.write_text('{"schema":"invalid"}')
            stderr = io.StringIO()
            with redirect_stderr(stderr):
                result = agentctl.main(["config", "validate", "--spec", str(spec_path)])
        diagnostic = stderr.getvalue()
        self.assertEqual(result, 3)
        self.assertIn("agentctl:", diagnostic)
        self.assertNotIn("Traceback", diagnostic)
        self.assertNotIn("AttributeError", diagnostic)

    def test_overlong_local_state_identifier_is_classified_without_traceback(self):
        with tempfile.TemporaryDirectory() as directory:
            spec_path = Path(directory) / "valid-json.json"
            spec_path.write_text("{}")
            stderr = io.StringIO()
            with patch.object(
                setup,
                "validate_spec",
                side_effect=local_state.LocalStateError("invalid path identifier"),
            ), redirect_stderr(stderr):
                result = agentctl.main(["config", "validate", "--spec", str(spec_path)])
        diagnostic = stderr.getvalue()
        self.assertEqual(result, 3)
        self.assertIn("agentctl: invalid path identifier", diagnostic)
        self.assertNotIn("Traceback", diagnostic)


if __name__ == "__main__":
    unittest.main()