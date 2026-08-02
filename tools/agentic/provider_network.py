#!/usr/bin/env python3
"""Bounded, DNS-pinned, bodyless provider model-discovery transport."""
from __future__ import annotations

import http.client
import ipaddress
import socket
import ssl
import time
from dataclasses import dataclass
from typing import Callable, Mapping
from urllib.parse import urljoin, urlsplit, urlunsplit


MAX_RESPONSE_BYTES = 512 * 1024
# A request may perform its initial exchange and at most one accepted redirect.
MAX_REDIRECTS = 1
MAX_EXCHANGES = 1 + MAX_REDIRECTS
MAX_ADDRESSES = 16
CONNECT_TIMEOUT_SECONDS = 3
READ_TIMEOUT_SECONDS = 5
TOTAL_TIMEOUT_SECONDS = 10
_ALLOWED_HEADERS = {"accept", "authorization"}


class NetworkPolicyError(ValueError):
    """A locally enforced endpoint or request-policy failure."""


class NetworkTransportError(ValueError):
    """A redacted classified socket or HTTP failure."""

    def __init__(self, classification: str):
        super().__init__(classification)
        self.classification = classification


@dataclass
class ExchangeBudget:
    """Explicit request budget shared by all exchanges in one discovery operation."""

    maximum: int
    exchanges: int = 0

    def consume(self) -> None:
        if self.exchanges >= self.maximum:
            raise NetworkTransportError("exchange-limit")
        self.exchanges += 1


@dataclass(frozen=True)
class DiscoveryRequest:
    url: str
    headers: Mapping[str, str]
    method: str = "GET"
    body: bytes | None = None

    def __post_init__(self) -> None:
        if self.method != "GET" or self.body is not None:
            raise NetworkPolicyError("discovery requests must be bodyless GET requests")
        if any(key.lower() not in _ALLOWED_HEADERS for key in self.headers):
            raise NetworkPolicyError("discovery request contains an unapproved header")


@dataclass(frozen=True)
class DiscoveryResponse:
    status: int
    headers: Mapping[str, str]
    body: bytes
    location: str | None = None

    def __post_init__(self) -> None:
        if len(self.body) > MAX_RESPONSE_BYTES:
            raise NetworkPolicyError("discovery response exceeds the size limit")


Resolver = Callable[[str, int], list[str]]


def system_resolver(host: str, port: int) -> list[str]:
    try:
        values = socket.getaddrinfo(host, port, type=socket.SOCK_STREAM)
    except socket.gaierror as error:
        raise NetworkTransportError("network") from error
    return sorted({item[4][0] for item in values})


def normalize_endpoint(value: str) -> str:
    try:
        parsed = urlsplit(value)
        port = parsed.port
    except ValueError as error:
        raise NetworkPolicyError("endpoint has an invalid port") from error
    if parsed.scheme not in {"https", "http"} or not parsed.hostname:
        raise NetworkPolicyError("endpoint must use HTTP(S) with a host")
    if parsed.username is not None or parsed.password is not None or parsed.query or parsed.fragment:
        raise NetworkPolicyError("endpoint must not contain credentials, query, or fragment")
    host = parsed.hostname.lower().rstrip(".")
    authority = f"[{host}]" if ":" in host else host
    if port is not None:
        authority += f":{port}"
    return urlunsplit((parsed.scheme, authority, parsed.path.rstrip("/"), "", ""))


def _address_value(address: str) -> ipaddress.IPv4Address | ipaddress.IPv6Address:
    value = ipaddress.ip_address(address)
    if isinstance(value, ipaddress.IPv6Address) and value.ipv4_mapped is not None:
        return value.ipv4_mapped
    return value


def _allowed_address(address: str, allow_private: bool) -> bool:
    value = _address_value(address)
    if value.is_loopback:
        return True
    if value.is_unspecified or value.is_multicast or value.is_link_local or value.is_reserved:
        return False
    if value.is_private:
        return allow_private
    return value.is_global


def validated_addresses(endpoint: str, resolver: Resolver, *, allow_private: bool = False) -> tuple[str, list[str]]:
    normalized = normalize_endpoint(endpoint)
    parsed = urlsplit(normalized)
    port = parsed.port or (443 if parsed.scheme == "https" else 80)
    addresses = resolver(parsed.hostname or "", port)
    if not addresses or len(addresses) > MAX_ADDRESSES:
        raise NetworkPolicyError("endpoint must resolve to between one and sixteen addresses")
    try:
        permitted = [_allowed_address(address, allow_private) for address in addresses]
    except ValueError as error:
        raise NetworkPolicyError("resolver returned an invalid address") from error
    if not all(permitted):
        raise NetworkPolicyError("endpoint resolves to a prohibited address")
    if parsed.scheme == "http" and not all(_address_value(address).is_loopback for address in addresses):
        raise NetworkPolicyError("remote HTTP endpoints are prohibited")
    return normalized, addresses


def same_origin(source: str, target: str) -> bool:
    left, right = urlsplit(source), urlsplit(target)
    return (
        left.scheme == right.scheme
        and left.hostname == right.hostname
        and (left.port or (443 if left.scheme == "https" else 80))
        == (right.port or (443 if right.scheme == "https" else 80))
    )


def _host_header(parsed) -> str:
    port = parsed.port
    default = 443 if parsed.scheme == "https" else 80
    host = parsed.hostname or ""
    return host if port is None or port == default else f"{host}:{port}"


def _pinned_response(
    request: DiscoveryRequest,
    address: str,
    *,
    context: ssl.SSLContext | None = None,
) -> DiscoveryResponse:
    parsed = urlsplit(request.url)
    port = parsed.port or (443 if parsed.scheme == "https" else 80)
    path = parsed.path or "/"
    started = time.monotonic()
    try:
        raw = socket.create_connection((address, port), timeout=CONNECT_TIMEOUT_SECONDS)
        raw.settimeout(READ_TIMEOUT_SECONDS)
        if parsed.scheme == "https":
            tls = (context or ssl.create_default_context()).wrap_socket(raw, server_hostname=parsed.hostname)
            connection = http.client.HTTPConnection(address, port, timeout=READ_TIMEOUT_SECONDS)
            connection.sock = tls
        else:
            connection = http.client.HTTPConnection(address, port, timeout=READ_TIMEOUT_SECONDS)
            connection.sock = raw
        headers = {"Host": _host_header(parsed), "Accept": "application/json", **dict(request.headers)}
        connection.request("GET", path, body=None, headers=headers)
        response = connection.getresponse()
        body = response.read(MAX_RESPONSE_BYTES + 1)
        connection.close()
    except socket.timeout as error:
        raise NetworkTransportError("timeout") from error
    except (OSError, ssl.SSLError, http.client.HTTPException) as error:
        raise NetworkTransportError("network") from error
    if time.monotonic() - started > TOTAL_TIMEOUT_SECONDS:
        raise NetworkTransportError("timeout")
    if len(body) > MAX_RESPONSE_BYTES:
        raise NetworkTransportError("response-too-large")
    headers = {key.lower(): value for key, value in response.getheaders()}
    return DiscoveryResponse(response.status, headers, body, headers.get("location"))


def _exchange(
    request: DiscoveryRequest,
    address: str,
    budget: ExchangeBudget,
    *,
    context: ssl.SSLContext | None = None,
) -> DiscoveryResponse:
    budget.consume()
    return _pinned_response(request, address, context=context)


def discovery_get(
    endpoint: str,
    path: str,
    *,
    headers: Mapping[str, str],
    resolver: Resolver = system_resolver,
    allow_private: bool = False,
    context: ssl.SSLContext | None = None,
    budget: ExchangeBudget | None = None,
) -> DiscoveryResponse:
    """Perform one bounded direct-socket, DNS-pinned discovery GET operation.

    The request consumes an exchange before every emitted HTTP request. At most
    ``MAX_REDIRECTS`` same-origin redirects and ``MAX_EXCHANGES`` exchanges are
    permitted; a supplied budget can additionally bound an entire higher-level
    discovery operation spanning multiple candidate paths.
    """
    if not path.startswith("/") or path.startswith("//") or "://" in path:
        raise NetworkPolicyError("invalid discovery request path")
    normalized, addresses = validated_addresses(endpoint, resolver, allow_private=allow_private)
    endpoint_parts = urlsplit(normalized)
    origin = urlunsplit((endpoint_parts.scheme, endpoint_parts.netloc, "", "", ""))
    request = DiscoveryRequest(origin + path, headers)
    address = addresses[0]
    redirects = 0
    request_budget = budget or ExchangeBudget(MAX_EXCHANGES)

    while True:
        response = _exchange(request, address, request_budget, context=context)
        if response.status not in {301, 302, 307, 308}:
            return response
        if redirects >= MAX_REDIRECTS:
            raise NetworkTransportError("redirect-limit")
        if response.location is None:
            raise NetworkTransportError("redirect")
        target = urljoin(request.url, response.location)
        normalized_target = normalize_endpoint(target)
        target_parts = urlsplit(normalized_target)
        target_origin = urlunsplit((target_parts.scheme, target_parts.netloc, "", "", ""))
        if urlsplit(normalized).scheme == "https" and urlsplit(target_origin).scheme != "https":
            raise NetworkPolicyError("HTTPS redirect downgrade is prohibited")
        if not same_origin(normalized, target_origin):
            raise NetworkPolicyError("cross-origin discovery redirect is prohibited")
        redirected_path = target_parts.path or "/"
        _, redirected_addresses = validated_addresses(target_origin, resolver, allow_private=allow_private)
        redirects += 1
        request = DiscoveryRequest(target_origin + redirected_path, headers)
        address = redirected_addresses[0]