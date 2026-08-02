from __future__ import annotations

import json
import sys
import unittest
from datetime import datetime, timezone
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "tools" / "agentic"))

import provider_discovery
import provider_network


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

    def test_no_inference_path_is_a_candidate(self):
        self.assertEqual(provider_discovery.candidate_paths("https://gateway.example.invalid/v1"), ["/v1/models"])
        self.assertEqual(provider_discovery.candidate_paths("https://gateway.example.invalid"), ["/models", "/v1/models"])


if __name__ == "__main__":
    unittest.main()