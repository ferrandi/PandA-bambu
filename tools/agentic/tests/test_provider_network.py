from __future__ import annotations

import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "tools" / "agentic"))

import provider_network


class ProviderNetworkTests(unittest.TestCase):
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

    def test_mixed_dns_answers_fail_closed_and_origin_rules_are_strict(self):
        with self.assertRaises(provider_network.NetworkPolicyError):
            provider_network.validated_addresses(
                "https://gateway.example.invalid/v1",
                lambda host, port: ["8.8.8.8", "169.254.169.254"],
            )
        self.assertTrue(
            provider_network.same_origin(
                "https://gateway.example.invalid/v1",
                "https://gateway.example.invalid/other",
            )
        )
        self.assertFalse(
            provider_network.same_origin(
                "https://gateway.example.invalid/v1",
                "https://other.example.invalid/v1",
            )
        )
        self.assertFalse(
            provider_network.same_origin(
                "https://gateway.example.invalid/v1",
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


if __name__ == "__main__":
    unittest.main()