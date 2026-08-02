from __future__ import annotations

import io
import json
import sys
import tempfile
import unittest
from contextlib import redirect_stderr
from datetime import datetime, timezone
from pathlib import Path
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "tools" / "agentic"))

import agentctl
import provider_discovery
import provider_network
import provider_onboarding


class ProviderDiscoveryTests(unittest.TestCase):
    def setUp(self):
        self.now = lambda: datetime(2026, 2, 3, 4, 5, 6, tzinfo=timezone.utc)
        self.auth = {"mode": "environment-token", "env_var": "FIXTURE_SECRET"}
        self.secret = "sentinel-secret-value"

    def test_openai_listing_is_normalized_and_secret_is_not_returned(self):
        calls = []

        def fetch(path, headers):
            calls.append((path, dict(headers)))
            return provider_network.DiscoveryResponse(
                200,
                {"content-type": "application/json"},
                b'{"data":[{"id":"model-b"},{"id":"model-a","context_window":8192}]}',
            )

        evidence = provider_discovery.discover(
            "fixture",
            "https://gateway.example.invalid/v1",
            authentication=self.auth,
            env={"FIXTURE_SECRET": self.secret},
            fetch=fetch,
            clock=self.now,
        )
        self.assertEqual(calls[0][0], "/v1/models")
        self.assertEqual(calls[0][1]["Authorization"], "Bearer " + self.secret)
        self.assertEqual([item["model_id"] for item in evidence["models"]], ["model-a", "model-b"])
        self.assertEqual(evidence["models"][0]["context_window"], 8192)
        self.assertEqual(evidence["status"], "succeeded")
        self.assertNotIn(self.secret, json.dumps(evidence))

    def test_missing_or_rejected_authentication_is_classified_without_model_output(self):
        missing = provider_discovery.discover(
            "fixture",
            "https://gateway.example.invalid/v1",
            authentication=self.auth,
            env={},
            fetch=lambda path, headers: self.fail("fetch must not run"),
            clock=self.now,
        )
        self.assertEqual((missing["status"], missing["authentication"], missing["failure"]), ("failed", "missing", "authentication"))

        rejected = provider_discovery.discover(
            "fixture",
            "https://gateway.example.invalid/v1",
            authentication=self.auth,
            env={"FIXTURE_SECRET": self.secret},
            fetch=lambda path, headers: provider_network.DiscoveryResponse(401, {}, b""),
            clock=self.now,
        )
        self.assertEqual((rejected["status"], rejected["authentication"], rejected["failure"]), ("failed", "rejected", "authentication"))
        self.assertNotIn(self.secret, json.dumps(rejected))

    def test_malformed_wrong_content_type_and_excessive_models_are_bounded(self):
        malformed = provider_discovery.discover(
            "fixture",
            "https://gateway.example.invalid/v1",
            authentication={"mode": "none"},
            env={},
            fetch=lambda path, headers: provider_network.DiscoveryResponse(200, {"content-type": "application/json"}, b'{"models":[]}'),
            clock=self.now,
        )
        self.assertEqual(malformed["failure"], "malformed-response")

        wrong_type = provider_discovery.discover(
            "fixture",
            "https://gateway.example.invalid/v1",
            authentication={"mode": "none"},
            env={},
            fetch=lambda path, headers: provider_network.DiscoveryResponse(200, {"content-type": "text/plain"}, b'{"data":[]}'),
            clock=self.now,
        )
        self.assertEqual(wrong_type["failure"], "malformed-response")

        values = [{"id": f"model-{index:03d}"} for index in range(300)]
        body = json.dumps({"data": values}).encode()
        bounded = provider_discovery.discover(
            "fixture",
            "https://gateway.example.invalid/v1",
            authentication={"mode": "none"},
            env={},
            fetch=lambda path, headers: provider_network.DiscoveryResponse(200, {"content-type": "application/json"}, body),
            clock=self.now,
        )
        self.assertEqual(len(bounded["models"]), provider_discovery.MAX_MODELS)
        self.assertTrue(bounded["truncated"])

    def test_preparse_nesting_limit_and_string_escaping(self):
        exact = ("[" * provider_discovery.MAX_JSON_DEPTH + "0" + "]" * provider_discovery.MAX_JSON_DEPTH).encode()
        self.assertEqual(provider_discovery._document(exact), [[[[[[[[[[[[[[[[0]]]]]]]]]]]]]]]])

        quoted = b'{"data":[{"id":"brackets [] {} and escaped quote \\" plus backslash \\\\"}]}'
        self.assertEqual(provider_discovery._document(quoted)["data"][0]["id"], 'brackets [] {} and escaped quote " plus backslash \\')

        for body in (
            ("[" * (provider_discovery.MAX_JSON_DEPTH + 1) + "0" + "]" * (provider_discovery.MAX_JSON_DEPTH + 1)).encode(),
            ("[" * 5000 + "0" + "]" * 5000).encode(),
            b'{"data":[}',
        ):
            with self.subTest(length=len(body)):
                with self.assertRaisesRegex(provider_discovery.DiscoveryError, "^malformed-response$"):
                    provider_discovery._document(body)

    def test_preparse_large_nesting_is_classified_without_decoder_recursion(self):
        body = ("[" * 5000 + "0" + "]" * 5000).encode()
        self.assertLess(len(body), provider_network.MAX_RESPONSE_BYTES)
        with patch.object(provider_discovery.json, "loads", side_effect=lambda *args, **kwargs: self.fail("json.loads must not run")):
            with self.assertRaisesRegex(provider_discovery.DiscoveryError, "^malformed-response$"):
                provider_discovery._document(body)

    def test_candidate_path_attempts_share_operation_exchange_budget(self):
        calls = []

        def fetch(path, headers):
            calls.append((path, dict(headers)))
            return provider_network.DiscoveryResponse(404, {}, b"")

        evidence = provider_discovery.discover(
            "fixture",
            "https://gateway.example.invalid",
            authentication=self.auth,
            env={"FIXTURE_SECRET": self.secret},
            fetch=fetch,
            clock=self.now,
        )
        self.assertEqual(calls, [("/models", {"Accept": "application/json", "Authorization": "Bearer " + self.secret}), ("/v1/models", {"Accept": "application/json", "Authorization": "Bearer " + self.secret})])
        self.assertEqual(evidence["failure"], "unsupported")
        self.assertNotIn(self.secret, json.dumps(evidence))

    def test_candidate_path_budget_exhaustion_does_not_emit_a_later_request(self):
        calls = []
        original = provider_discovery.MAX_DISCOVERY_EXCHANGES
        provider_discovery.MAX_DISCOVERY_EXCHANGES = 1
        try:
            def fetch(path, headers):
                calls.append((path, dict(headers)))
                return provider_network.DiscoveryResponse(404, {}, b"")

            evidence = provider_discovery.discover(
                "fixture",
                "https://gateway.example.invalid",
                authentication=self.auth,
                env={"FIXTURE_SECRET": self.secret},
                fetch=fetch,
                clock=self.now,
            )
        finally:
            provider_discovery.MAX_DISCOVERY_EXCHANGES = original
        self.assertEqual([item[0] for item in calls], ["/models"])
        self.assertEqual((evidence["status"], evidence["failure"]), ("failed", "exchange-limit"))
        self.assertNotIn(self.secret, json.dumps(evidence))

    def test_provider_discover_recursion_is_classified_without_traceback(self):
        spec = {
            "schema": provider_onboarding.SPEC_SCHEMA,
            "schema_version": "1.0",
            "provider_id": "fixture-cli",
            "endpoint": {"origin": "https://gateway.example.invalid/v1", "protocol": "openai-compatible"},
            "authentication": {"mode": "none"},
            "model": "model-a",
            "roles": [],
        }
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            provider_onboarding.apply(root, spec, clock=self.now)
            stderr = io.StringIO()
            with patch.object(provider_discovery, "discover", side_effect=RecursionError()):
                with redirect_stderr(stderr):
                    result = agentctl.main(["provider", "discover", "fixture-cli", "--root", str(root)])
        self.assertEqual(result, agentctl.EXIT_CODES["validation"])
        self.assertEqual(stderr.getvalue(), "agentctl: malformed response\n")
        self.assertNotIn("Traceback", stderr.getvalue())

    def test_no_inference_path_is_a_candidate(self):
        self.assertEqual(provider_discovery.candidate_paths("https://gateway.example.invalid/v1"), ["/v1/models"])
        self.assertEqual(provider_discovery.candidate_paths("https://gateway.example.invalid"), ["/models", "/v1/models"])


if __name__ == "__main__":
    unittest.main()