#!/usr/bin/env python3
"""Bounded OpenAI-compatible model-list discovery; never sends inference."""
from __future__ import annotations

import json
from datetime import datetime, timezone
from typing import Callable, Mapping
from urllib.parse import urlsplit

import provider_network
import provider_onboarding


MAX_MODELS = 256
MAX_JSON_DEPTH = 16
# The operation budget covers all candidate paths and their redirect exchanges.
MAX_DISCOVERY_EXCHANGES = provider_network.MAX_EXCHANGES


class DiscoveryError(ValueError):
    def __init__(self, classification: str):
        super().__init__(classification)
        self.classification = classification


def _depth(value: object, level: int = 0) -> int:
    if level > MAX_JSON_DEPTH:
        raise DiscoveryError("malformed-response")
    if isinstance(value, dict):
        return max([level] + [_depth(item, level + 1) for item in value.values()])
    if isinstance(value, list):
        return max([level] + [_depth(item, level + 1) for item in value])
    return level


def _scan_nesting(text: str) -> None:
    """Iteratively reject malformed or excessively nested JSON before decoding."""
    depth = 0
    in_string = False
    escaped = False
    for character in text:
        if in_string:
            if escaped:
                escaped = False
            elif character == "\\":
                escaped = True
            elif character == '"':
                in_string = False
            continue
        if character == '"':
            in_string = True
        elif character in "{[":
            depth += 1
            if depth > MAX_JSON_DEPTH:
                raise DiscoveryError("malformed-response")
        elif character in "}]":
            depth -= 1
            if depth < 0:
                raise DiscoveryError("malformed-response")
    if in_string or escaped or depth != 0:
        raise DiscoveryError("malformed-response")


def _document(body: bytes) -> object:
    try:
        text = body.decode("utf-8")
        _scan_nesting(text)
        value = json.loads(text, parse_constant=lambda _: (_ for _ in ()).throw(ValueError()))
    except (RecursionError, UnicodeDecodeError, ValueError, json.JSONDecodeError) as error:
        raise DiscoveryError("malformed-response") from error
    _depth(value)
    return value


def _models(document: object) -> tuple[list[dict], bool]:
    if not isinstance(document, dict) or not isinstance(document.get("data"), list):
        raise DiscoveryError("malformed-response")
    raw = document["data"]
    normalized: dict[str, dict] = {}
    for item in raw:
        if not isinstance(item, dict) or not isinstance(item.get("id"), str) or not item["id"] or len(item["id"]) > 512:
            raise DiscoveryError("malformed-response")
        model_id = item["id"]
        context = item.get("context_window")
        if context is not None and (not isinstance(context, int) or isinstance(context, bool) or context <= 0):
            context = None
        value = {
            "model_id": model_id,
            "origin": "endpoint-reported",
            "confidence": "observed",
            "context_window": context,
        }
        previous = normalized.get(model_id)
        if previous is not None and previous != value:
            raise DiscoveryError("malformed-response")
        normalized[model_id] = value
    ordered = [normalized[key] for key in sorted(normalized)]
    return ordered[:MAX_MODELS], len(ordered) > MAX_MODELS


def candidate_paths(endpoint: str) -> list[str]:
    path = urlsplit(endpoint).path.rstrip("/")
    candidates = [f"{path}/models" if path else "/models"]
    if path == "":
        candidates.append("/v1/models")
    return list(dict.fromkeys(candidates))


def discover(
    provider_id: str,
    endpoint: str,
    *,
    authentication: Mapping[str, str],
    env: Mapping[str, str],
    fetch: Callable[[str, Mapping[str, str]], provider_network.DiscoveryResponse] | None = None,
    clock: Callable[[], datetime] | None = None,
    allow_private: bool = False,
) -> dict:
    """Discover only OpenAI-compatible model-list evidence.

    The environment value is read only to construct a transient Authorization
    header and never enters the returned document. One operation-wide exchange
    budget spans all candidate paths and every transport redirect exchange.
    """
    now = (clock or (lambda: datetime.now(timezone.utc)))()
    if now.tzinfo is None or now.utcoffset() is None:
        raise DiscoveryError("malformed-response")
    headers: dict[str, str] = {"Accept": "application/json"}
    authentication_observation = "not-required"
    if authentication["mode"] == "environment-token":
        token = env.get(authentication["env_var"])
        if not token:
            return _evidence(provider_id, endpoint, now, "failed", None, "missing", [], False, "authentication")
        headers["Authorization"] = "Bearer " + token
        authentication_observation = "accepted"
    paths = candidate_paths(endpoint)
    budget = provider_network.ExchangeBudget(MAX_DISCOVERY_EXCHANGES)
    last_failure = "unsupported"
    for path in paths:
        try:
            if fetch is not None:
                budget.consume()
                response = fetch(path, headers)
            else:
                response = provider_network.discovery_get(
                    endpoint,
                    path,
                    headers=headers,
                    allow_private=allow_private,
                    budget=budget,
                )
        except provider_network.NetworkPolicyError:
            last_failure = "endpoint-policy"
            break
        except provider_network.NetworkTransportError as error:
            last_failure = error.classification
            if error.classification == "exchange-limit":
                break
            continue
        if response.status in {401, 403}:
            return _evidence(provider_id, endpoint, now, "failed", path, "rejected", [], False, "authentication")
        if not 200 <= response.status < 300:
            last_failure = "unsupported"
            continue
        content_type = response.headers.get("content-type", "").split(";", 1)[0].lower()
        if content_type not in {"application/json", "application/problem+json"} and not content_type.endswith("+json"):
            last_failure = "malformed-response"
            continue
        try:
            models, truncated = _models(_document(response.body))
        except DiscoveryError as error:
            last_failure = error.classification
            continue
        return _evidence(provider_id, endpoint, now, "succeeded", path, authentication_observation, models, truncated, None)
    return _evidence(provider_id, endpoint, now, "unsupported" if last_failure == "unsupported" else "failed", None, authentication_observation, [], False, last_failure)


def _evidence(
    provider_id: str,
    endpoint: str,
    now: datetime,
    status: str,
    request_path: str | None,
    authentication: str,
    models: list[dict],
    truncated: bool,
    failure: str | None,
) -> dict:
    return {
        "schema": provider_onboarding.EVIDENCE_SCHEMA,
        "schema_version": "1.1",
        "provider_id": provider_id,
        "endpoint_origin": endpoint,
        "checked_at": now.astimezone(timezone.utc).isoformat(),
        "method": "openai-model-list",
        "request_path": request_path,
        "status": status,
        "listing_protocol": {
            "value": "openai-compatible",
            "origin": "protocol-derived",
            "confidence": "declared",
        },
        "authentication": authentication,
        "models": models,
        "truncated": truncated,
        "failure": failure,
        "diagnostics": [],
    }