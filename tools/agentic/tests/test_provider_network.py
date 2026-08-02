from __future__ import annotations

import sys
import unittest
from pathlib import Path
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "tools" / "agentic"))

import provider_network


class ProviderNetworkTests(unittest.TestCase):
    endpoint = "https://gateway.example.invalid/v1"
    resolver = staticmethod(lambda host, port: ["8.8.8.8"])

    def test_request_is_get_only_and_bodyless(self):
        request = provider_network.DiscoveryRequest(
            "https://gateway.example.invalid/v1/models",
            {"Accept": "application/json"},
        )
        self.assertEqual(request.method, "GET")
        self.assertIsNone(request.body)
        with self.assertRaises(provider_network.NetworkPolicyError):
            provider_network.DiscoveryRequest(
                "https://gateway.example.invalid/v1/models",
                {},
                method="POST",
            )
        with self.assertRaises(provider_network.NetworkPolicyError):
            provider_network.DiscoveryRequest(
                "https://gateway.example.invalid/v1/models",
                {},
                body=b"{}",
            )

    def test_endpoint_policy_rejects_credentials_http_and_prohibited_addresses(self):
        resolver = lambda host, port: ["8.8.8.8"]
        for endpoint in (
            "ftp://gateway.example.invalid",
            "https://user:pass@gateway.example.invalid/v1",
            "https://gateway.example.invalid/v1?token=value",
            "http://gateway.example.invalid/v1",
        ):
            with self.subTest(endpoint=endpoint), self.assertRaises(provider_network.NetworkPolicyError):
                provider_network.validated_addresses(endpoint, resolver)

        for address in ("169.254.169.254", "224.0.0.1", "0.0.0.0", "fe80::1"):
            with self.subTest(address=address), self.assertRaises(provider_network.NetworkPolicyError):
                provider_network.validated_addresses(
                    "https://gateway.example.invalid/v1",
                    lambda host, port, address=address: [address],
                )

    def test_loopback_http_and_explicit_private_https(self):
        for endpoint, address in (
            ("http://127.0.0.1:8080/v1", "127.0.0.1"),
            ("http://[::1]:8080/v1", "::1"),
        ):
            with self.subTest(endpoint=endpoint):
                self.assertEqual(
                    provider_network.validated_addresses(
                        endpoint,
                        lambda host, port, address=address: [address],
                    )[0],
                    endpoint,
                )
        with self.assertRaises(provider_network.NetworkPolicyError):
            provider_network.validated_addresses(
                "https://gateway.example.invalid/v1",
                lambda host, port: ["10.0.0.8"],
            )
        self.assertEqual(
            provider_network.validated_addresses(
                "https://gateway.example.invalid/v1",
                lambda host, port: ["10.0.0.8"],
                allow_private=True,
            )[1],
            ["10.0.0.8"],
        )

    def test_ipv4_mapped_ipv6_uses_ipv4_classification(self):
        self.assertTrue(provider_network._address_value("::ffff:127.0.0.1").is_loopback)
        self.assertTrue(provider_network._address_value("::ffff:10.0.0.1").is_private)
        self.assertTrue(provider_network._address_value("::ffff:8.8.8.8").is_global)
        self.assertTrue(
            provider_network.validated_addresses(
                "http://[::ffff:127.0.0.1]:8080/v1",
                lambda host, port: ["::ffff:127.0.0.1"],
            )[0].startswith("http://")
        )
        self.assertEqual(
            provider_network.validated_addresses(
                self.endpoint,
                lambda host, port: ["::ffff:10.0.0.1"],
                allow_private=True,
            )[1],
            ["::ffff:10.0.0.1"],
        )
        for address in ("::ffff:10.0.0.1", "::ffff:169.254.169.254"):
            with self.subTest(address=address), self.assertRaises(provider_network.NetworkPolicyError):
                provider_network.validated_addresses(
                    self.endpoint,
                    lambda host, port, address=address: [address],
                )
        with self.assertRaises(provider_network.NetworkPolicyError):
            provider_network.validated_addresses(
                self.endpoint,
                lambda host, port: ["::ffff:169.254.169.254"],
                allow_private=True,
            )

    def test_mixed_dns_answers_fail_closed_and_origin_rules_are_strict(self):
        with self.assertRaises(provider_network.NetworkPolicyError):
            provider_network.validated_addresses(
                self.endpoint,
                lambda host, port: ["8.8.8.8", "169.254.169.254"],
            )
        self.assertTrue(
            provider_network.same_origin(
                self.endpoint,
                "https://gateway.example.invalid/other",
            )
        )
        self.assertFalse(
            provider_network.same_origin(
                self.endpoint,
                "https://other.example.invalid/v1",
            )
        )
        self.assertFalse(
            provider_network.same_origin(
                self.endpoint,
                "http://gateway.example.invalid/v1",
            )
        )

    def test_response_limit_and_unapproved_headers_are_rejected(self):
        with self.assertRaises(provider_network.NetworkPolicyError):
            provider_network.DiscoveryResponse(200, {}, b"x" * (provider_network.MAX_RESPONSE_BYTES + 1))
        with self.assertRaises(provider_network.NetworkPolicyError):
            provider_network.DiscoveryRequest(
                "https://gateway.example.invalid/v1/models",
                {"X-Unsafe": "value"},
            )

    def test_no_redirect_emits_one_exchange(self):
        emitted = []

        def response(request, address, *, context=None):
            emitted.append((request.url, dict(request.headers)))
            return provider_network.DiscoveryResponse(200, {"content-type": "application/json"}, b'{"data":[]}')

        with patch.object(provider_network, "_pinned_response", side_effect=response):
            result = provider_network.discovery_get(self.endpoint, "/v1/models", headers={"Authorization": "Bearer sentinel"}, resolver=self.resolver)

        self.assertEqual(result.status, 200)
        self.assertEqual(len(emitted), 1)
        self.assertEqual(emitted[0][0], "https://gateway.example.invalid/v1/models")

    def test_one_permitted_redirect_emits_two_exchanges(self):
        emitted = []
        responses = [
            provider_network.DiscoveryResponse(302, {}, b"", "/v1/models-final"),
            provider_network.DiscoveryResponse(200, {"content-type": "application/json"}, b'{"data":[]}'),
        ]

        def response(request, address, *, context=None):
            emitted.append((request.url, dict(request.headers)))
            return responses.pop(0)

        with patch.object(provider_network, "_pinned_response", side_effect=response):
            result = provider_network.discovery_get(self.endpoint, "/v1/models", headers={"Authorization": "Bearer sentinel"}, resolver=self.resolver)

        self.assertEqual(result.status, 200)
        self.assertEqual([item[0] for item in emitted], [
            "https://gateway.example.invalid/v1/models",
            "https://gateway.example.invalid/v1/models-final",
        ])
        self.assertEqual(emitted[0][1]["Authorization"], emitted[1][1]["Authorization"])

    def test_second_redirect_and_loop_are_rejected_before_a_third_exchange(self):
        emitted = []
        responses = [
            provider_network.DiscoveryResponse(302, {}, b"", "/v1/models"),
            provider_network.DiscoveryResponse(302, {}, b"", "/v1/models"),
        ]

        def response(request, address, *, context=None):
            emitted.append((request.url, dict(request.headers)))
            return responses.pop(0)

        with patch.object(provider_network, "_pinned_response", side_effect=response):
            with self.assertRaisesRegex(provider_network.NetworkTransportError, "^redirect-limit$"):
                provider_network.discovery_get(self.endpoint, "/v1/models", headers={"Authorization": "Bearer sentinel"}, resolver=self.resolver)

        self.assertEqual(len(emitted), provider_network.MAX_EXCHANGES)
        self.assertTrue(all("sentinel" not in str(item[0]) for item in emitted))

    def test_exchange_budget_exhaustion_emits_no_request_after_limit(self):
        emitted = []
        budget = provider_network.ExchangeBudget(1)

        def response(request, address, *, context=None):
            emitted.append((request.url, dict(request.headers)))
            return provider_network.DiscoveryResponse(302, {}, b"", "/v1/models-final")

        with patch.object(provider_network, "_pinned_response", side_effect=response):
            with self.assertRaisesRegex(provider_network.NetworkTransportError, "^exchange-limit$"):
                provider_network.discovery_get(
                    self.endpoint,
                    "/v1/models",
                    headers={"Authorization": "Bearer sentinel-secret"},
                    resolver=self.resolver,
                    budget=budget,
                )

        self.assertEqual(len(emitted), 1)
        self.assertNotIn("sentinel-secret", "exchange-limit")


if __name__ == "__main__":
    unittest.main()