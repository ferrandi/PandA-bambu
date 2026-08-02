#!/usr/bin/env python3
"""Centralized redaction and diagnostic normalization for portable agent state.

No caller may treat a ``redacted`` marker as proof that arbitrary text is safe.
This module removes credential-shaped values and converts untrusted diagnostics
into a bounded, useful vocabulary before records are persisted or displayed.
"""
from __future__ import annotations

import hashlib
import re
from collections.abc import Mapping
from urllib.parse import unquote
from typing import Any

REDACTED = "[REDACTED]"
SECRET_KEY_NAMES = {
    "authorization",
    "cookie",
    "cookies",
    "set-cookie",
    "x-api-key",
    "api-key",
    "api_key",
    "token",
    "access_token",
    "refresh_token",
    "session",
    "session_id",
    "secret",
    "password",
    "credential",
}
PRIVATE_VALUE_NAMES = {
    "endpoint",
    "url",
    "base_url",
    "base-url",
    "model",
    "model_id",
    "account",
    "account_id",
    "email",
    "organization",
    "organization_id",
}
SAFE_DIAGNOSTICS = {
    "authentication-failed",
    "configuration-invalid",
    "protocol-unsupported",
    "transport-failed",
    "timeout",
    "client-unavailable",
    "execution-path-unavailable",
    "version-unsupported",
    "configuration-required",
    "unknown",
}
_KEY_SHAPED = re.compile(
    r"(?ix)"
    r"\b(?:sk|rk|pk|key|token|secret)[_-]?[a-z0-9]{12,}\b"
    r"|\b(?:eyJ[a-z0-9_-]{10,}\.[a-z0-9_-]{10,}\.[a-z0-9_-]{10,})\b"
)
_BEARER = re.compile(r"(?i)\b(?:bearer|basic)\s+[a-z0-9._~+/=-]{6,}")
_HEADER = re.compile(r"(?im)^\s*(?:authorization|x-api-key|cookie|set-cookie)\s*:\s*[^\r\n]+$")
_URL = re.compile(r"(?i)\bhttps?://[^\s'\"<>]+")
_EMAIL = re.compile(r"(?i)\b[a-z0-9._%+-]+@[a-z0-9.-]+\.[a-z]{2,}\b")
_SESSION = re.compile(r"(?i)\b(?:session|oauth|cookie)[=_:-][^\s,;]+")
_ENV_VALUE = re.compile(r"(?m)\b[A-Z][A-Z0-9_]{2,}=(?:[^\s]+)")
_SECRET_ASSIGNMENT = re.compile(
    r"(?i)\b(?:api[_-]?key|token|secret|password|credential)\s*=\s*[^\s,;]+"
)
_ENCODED_INSPECTION_DEPTH = 2


def _decoded_variants(value: str) -> tuple[str, ...]:
    """Return a bounded decode chain for inspection without normalizing output."""
    variants = [value]
    decoded = value
    for _ in range(_ENCODED_INSPECTION_DEPTH):
        candidate = unquote(decoded)
        if candidate == decoded:
            break
        variants.append(candidate)
        decoded = candidate
    return tuple(variants)


def _contains_sensitive_text(value: str) -> bool:
    return any(
        pattern.search(value)
        for pattern in (
            _KEY_SHAPED,
            _BEARER,
            _HEADER,
            _URL,
            _EMAIL,
            _SESSION,
            _ENV_VALUE,
            _SECRET_ASSIGNMENT,
        )
    )


def digest(value: str) -> str:
    """Return a non-reversible reference suitable for confidential identifiers."""
    return "sha256:" + hashlib.sha256(value.encode("utf-8")).hexdigest()


def redact_text(value: str, secrets: tuple[str, ...] = ()) -> str:
    """Remove free-form credentials, endpoints, identities, and supplied values."""
    if "%" in value and any(_contains_sensitive_text(item) for item in _decoded_variants(value)):
        return REDACTED
    for secret in sorted((item for item in secrets if item), key=len, reverse=True):
        value = value.replace(secret, REDACTED)
    value = _HEADER.sub(REDACTED, value)
    value = _BEARER.sub(REDACTED, value)
    value = _KEY_SHAPED.sub(REDACTED, value)
    value = _SESSION.sub(REDACTED, value)
    value = _ENV_VALUE.sub(lambda match: match.group(0).split("=", 1)[0] + "=" + REDACTED, value)
    value = _URL.sub(REDACTED, value)
    value = _EMAIL.sub(REDACTED, value)
    return value


def normalize(value: Any, secrets: tuple[str, ...] = (), confidential_models: set[str] | None = None) -> Any:
    """Recursively remove unsafe values while retaining approved references."""
    confidential_models = confidential_models or set()
    if isinstance(value, Mapping):
        output: dict[str, Any] = {}
        for key, item in value.items():
            name = str(key).lower().replace("-", "_")
            if name in SECRET_KEY_NAMES:
                output[str(key)] = REDACTED
            elif name in PRIVATE_VALUE_NAMES:
                if name in {"model", "model_id"} and isinstance(item, str) and item in confidential_models:
                    output[str(key)] = digest(item)
                else:
                    output[str(key)] = REDACTED
            else:
                output[str(key)] = normalize(item, secrets, confidential_models)
        return output
    if isinstance(value, list):
        return [normalize(item, secrets, confidential_models) for item in value]
    if isinstance(value, tuple):
        return [normalize(item, secrets, confidential_models) for item in value]
    if isinstance(value, str):
        return redact_text(value, secrets)
    return value


def diagnostic(value: object) -> str:
    """Classify untrusted diagnostic text without persisting that text."""
    text = str(value).lower()
    if "timeout" in text:
        return "timeout"
    if "auth" in text or "401" in text or "403" in text:
        return "authentication-failed"
    if "protocol" in text or "404" in text or "405" in text or "501" in text:
        return "protocol-unsupported"
    if "version" in text:
        return "version-unsupported"
    if "config" in text or "parse" in text:
        return "configuration-invalid"
    if "not found" in text or "unavailable" in text or "executable" in text:
        return "client-unavailable"
    if "transport" in text or "network" in text or "connection" in text:
        return "transport-failed"
    return "unknown"


def safe_diagnostics(values: object) -> list[str]:
    """Return deterministic unique classified diagnostics only."""
    if not isinstance(values, list):
        return ["unknown"]
    return sorted({value if value in SAFE_DIAGNOSTICS else diagnostic(value) for value in values})


def is_safe(value: Any) -> bool:
    """Reject values which still include recognisable sensitive free-form content."""
    rendered = str(value)
    return not any(_contains_sensitive_text(item) for item in _decoded_variants(rendered))
