from __future__ import annotations

import copy
import json
import os
import stat
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path
from unittest.mock import Mock

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "tools" / "agentic"))

import adapters
import local_state
import portable_adapters
import redaction
import setup


REGISTRY_PATH = ROOT / "agentic" / "fixtures" / "profiles" / "portable-adapter-registry.json"
SPEC_PATH = ROOT / "agentic" / "fixtures" / "profiles" / "portable-adapter-setup.json"


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


class DetectionTests(unittest.TestCase):
    def run(self, version: str) -> Mock:  # type: ignore[override]
        return Mock(return_value=subprocess.CompletedProcess([], 0, version, ""))

    def test_codex_native_and_api_are_independent(self):
        result = adapters.detect("codex", which=lambda _: "/fixture/codex", run=self.run("codex 1.2.3"))
        states = {item["execution_path_id"]: item["state"] for item in result["paths"]}
        self.assertEqual(states["native-account"], "unknown")
        self.assertEqual(states["configured-api"], "configuration-required")

    def test_claude_native_and_api_are_independent(self):
        result = adapters.detect("claude-code", which=lambda _: "/fixture/claude", run=self.run("claude 1.2.3"))
        states = {item["execution_path_id"]: item["state"] for item in result["paths"]}
        self.assertEqual(states["native-account"], "unknown")
        self.assertEqual(states["configured-api"], "configuration-required")

    def test_unavailable_and_malformed_version_fail_closed(self):
        unavailable = adapters.detect("codex", which=lambda _: None)
        self.assertTrue(all(item["state"] == "unavailable" for item in unavailable["paths"]))
        malformed = adapters.detect("codex", which=lambda _: "/fixture/codex", run=self.run("unparseable"))
        self.assertTrue(all(item["state"] == "unknown" for item in malformed["paths"]))


class ContractTests(unittest.TestCase):
    def setUp(self):
        self.registry = json.loads(REGISTRY_PATH.read_text())

    def test_fixture_has_two_codex_paths_and_descriptor(self):
        portable_adapters.validate_registry(self.registry)
        detection = [
            {
                "adapter_id": "codex",
                "available": True,
                "paths": [
                    {"adapter_id": "codex", "execution_path_id": "native-account", "state": "authenticated-or-ready"},
                    {"adapter_id": "codex", "execution_path_id": "configured-api", "state": "configuration-required"},
                ],
                "diagnostics": [],
            },
            adapters.detect("direct-http"),
        ]
        report = portable_adapters.readiness(self.registry, detection, "2026-01-01T00:00:00+00:00")
        descriptor = portable_adapters.invocation_descriptor(self.registry, "codex-native-fixture", report)
        self.assertEqual(descriptor["execution_path_id"], "native-account")
        self.assertNotIn("native-session-reference", json.dumps(report))
        self.assertEqual(next(item["state"] for item in report["profiles"] if item["ref"] == "codex-native-fixture"), "authenticated-or-ready")

    def test_duplicate_path_protocol_and_access_class_are_rejected(self):
        for field, value in (("protocols", ["openai-responses", "openai-responses"]), ("access_classes", ["api-gateway", "api-gateway"])):
            bad = copy.deepcopy(self.registry)
            bad["adapters"][0]["execution_paths"][0][field] = value
            with self.subTest(field=field), self.assertRaises(portable_adapters.PortableAdapterError):
                portable_adapters.validate_registry(bad)


class LocalStateTests(unittest.TestCase):
    def test_safe_writes_are_atomic_restrictive_idempotent_and_refuse_overwrite(self):
        plan = local_state.PlannedWrite(local_state.LOCAL_DIR, ("generated", "fixture"), b"{}")
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            first = local_state.commit(root, [plan])
            self.assertEqual(stat.S_IMODE(first[0].stat().st_mode), 0o600)
            self.assertEqual(local_state.commit(root, [plan]), [])
            changed = local_state.PlannedWrite(local_state.LOCAL_DIR, ("generated", "fixture"), b'{"changed":true}')
            with self.assertRaises(local_state.LocalStateError):
                local_state.commit(root, [changed])
            local_state.commit(root, [changed], replace=True)
            self.assertTrue((root / ".agentic-local" / "backups" / "fixture.bak").exists())

    def test_traversal_and_symlink_are_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            with self.assertRaises(local_state.LocalStateError):
                local_state.safe_path(root, local_state.LOCAL_DIR, "../escape")
            target = root / "outside"
            target.write_text("x")
            link = root / ".agentic-local" / "generated"
            link.parent.mkdir()
            os.symlink(target, link)
            with self.assertRaises(local_state.LocalStateError):
                local_state.commit(root, [local_state.PlannedWrite(local_state.LOCAL_DIR, ("generated", "fixture"), b"{}")])


class SetupSmokeTests(unittest.TestCase):
    def setUp(self):
        self.spec = json.loads(SPEC_PATH.read_text())
        self.detected = [
            {
                "adapter_id": "codex",
                "available": True,
                "paths": [
                    {"adapter_id": "codex", "execution_path_id": "native-account", "state": "authenticated-or-ready"},
                    {"adapter_id": "codex", "execution_path_id": "configured-api", "state": "configuration-required"},
                ],
                "diagnostics": [],
            }
        ]

    def test_preview_dry_run_and_idempotent_setup(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            dry = setup.apply(root, self.spec, detected=self.detected, dry_run=True)
            self.assertTrue(dry["dry_run"])
            self.assertFalse((root / ".agentic-local").exists())
            first = setup.apply(root, self.spec, detected=self.detected)
            self.assertFalse(first["idempotent"])
            second = setup.apply(root, self.spec, detected=self.detected)
            self.assertTrue(second["idempotent"])
            generated = setup.config_generate(root, self.spec, detected=self.detected)
            self.assertEqual(generated, [])


if __name__ == "__main__":
    unittest.main()