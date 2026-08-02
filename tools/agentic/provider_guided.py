#!/usr/bin/env python3
"""Testable manual PAF-04B provider onboarding; never performs network I/O."""
from __future__ import annotations

import re
import sys
from dataclasses import dataclass
from datetime import datetime, timezone
from pathlib import Path
from typing import Callable, Mapping, Protocol

import provider_onboarding


ROLES = ("planning", "implementation", "review")
EXECUTION_PROTOCOLS = {
    "openai-compatible": ("openai-chat-completions", "openai-responses"),
    "anthropic-compatible": ("anthropic-messages",),
}


class WizardIO(Protocol):
    def read(self, prompt: str) -> str: ...
    def write(self, message: str) -> None: ...
    def isatty(self) -> bool: ...


class GuidedOnboardingError(ValueError):
    """A non-persistent guided-onboarding failure."""


class StdioWizardIO:
    def read(self, prompt: str) -> str:
        return input(prompt)

    def write(self, message: str) -> None:
        print(message)

    def isatty(self) -> bool:
        return sys.stdin.isatty() and sys.stdout.isatty()


@dataclass(frozen=True)
class GuidedResult:
    status: str
    spec: dict
    preview: dict | None
    applied: dict | None


def provider_id(display_name: str) -> str:
    value = re.sub(r"[^a-z0-9]+", "-", display_name.lower()).strip("-.")
    if not value:
        raise GuidedOnboardingError("provider name must contain a letter or number")
    return value[:190].rstrip("-.")


def manual_evidence(provider_id_value: str, endpoint: str, protocol: str, clock: Callable[[], datetime]) -> dict:
    instant = clock()
    if instant.tzinfo is None or instant.utcoffset() is None:
        raise GuidedOnboardingError("clock must return a timezone-aware timestamp")
    return {
        "schema": provider_onboarding.EVIDENCE_SCHEMA,
        "schema_version": "1.1",
        "provider_id": provider_id_value,
        "endpoint_origin": endpoint,
        "checked_at": instant.astimezone(timezone.utc).isoformat(),
        "method": "manual",
        "request_path": None,
        "status": "manual",
        "listing_protocol": {
            "value": protocol,
            "origin": "user-confirmed",
            "confidence": "declared",
        },
        "authentication": "not-requested",
        "models": [],
        "truncated": False,
        "failure": None,
        "diagnostics": ["Model listing was not performed; no inference was executed."],
    }


def recommend(models: list[str]) -> tuple[dict[str, str], list[str]]:
    selected = sorted(models)[0]
    assignments = {role: selected for role in ROLES}
    return assignments, [
        "No verified capability distinction is available in manual onboarding.",
        f"All workflow roles are recommended to use {selected}.",
    ]


def _answer(io: WizardIO, prompt: str) -> str:
    try:
        value = io.read(prompt).strip()
    except StopIteration as error:
        raise EOFError from error
    if value.lower() == "cancel":
        raise KeyboardInterrupt
    return value


def _choice(io: WizardIO, prompt: str, accepted: tuple[str, ...]) -> str:
    while True:
        value = _answer(io, prompt).lower()
        if value in accepted:
            return value
        io.write("Enter one of: " + ", ".join(accepted) + ", or cancel.")


def _models(io: WizardIO) -> list[str]:
    while True:
        value = _answer(io, "Model ID (or comma-separated model IDs): ")
        models = sorted({item.strip() for item in value.split(",") if item.strip()})
        if models:
            return models
        io.write("Enter at least one model ID.")


def _assignments(io: WizardIO, models: list[str], defaults: Mapping[str, str]) -> list[dict[str, str]]:
    mode = _choice(io, "Accept recommendations or customize? [accept/customize] ", ("accept", "customize"))
    if mode == "accept":
        selected = dict(defaults)
    else:
        selected = {}
        for role in ROLES:
            while True:
                model = _answer(io, f"{role.title()} model [{defaults[role]}]: ") or defaults[role]
                if model in models:
                    selected[role] = model
                    break
                io.write("Choose one of: " + ", ".join(models))
    return [{"role_id": role, "model": selected[role]} for role in ROLES]


def run(
    repository: Path,
    *,
    io: WizardIO,
    clock: Callable[[], datetime] | None = None,
    replace: bool = False,
) -> GuidedResult:
    if not io.isatty():
        raise GuidedOnboardingError("provider add requires an interactive TTY; use provider preview/apply for automation")
    now = clock or (lambda: datetime.now(timezone.utc))
    try:
        display_name = _answer(io, "Provider name: ")
        if not display_name:
            raise GuidedOnboardingError("provider name is required")
        derived_id = provider_id(display_name)
        chosen_id = _answer(io, f"Provider ID [{derived_id}]: ") or derived_id
        endpoint = provider_onboarding.normalize_origin(_answer(io, "Endpoint: "))
        auth_mode = _choice(io, "Authentication [none/environment]: ", ("none", "environment"))
        authentication = {"mode": "none"}
        if auth_mode == "environment":
            env_var = _answer(io, "Environment variable: ")
            authentication = {"mode": "environment-token", "env_var": env_var}
        endpoint_protocol = _choice(io, "Endpoint protocol [openai/anthropic]: ", ("openai", "anthropic"))
        protocol = "openai-compatible" if endpoint_protocol == "openai" else "anthropic-compatible"
        choices = EXECUTION_PROTOCOLS[protocol]
        execution_protocol = _choice(io, "Execution protocol [" + "/".join(choices) + "]: ", choices)
        io.write("Manual model entry selected. No endpoint discovery or model execution will occur.")
        models = _models(io)
        defaults, reasons = recommend(models)
        io.write("Recommended assignments:")
        for reason in reasons:
            io.write("  " + reason)
        role_assignments = _assignments(io, models, defaults)
        spec = {
            "schema": provider_onboarding.SPEC_SCHEMA,
            "schema_version": "1.1",
            "provider_id": chosen_id,
            "display_name": display_name,
            "endpoint": {"origin": endpoint, "protocol": protocol},
            "authentication": authentication,
            "models": models,
            "role_assignments": role_assignments,
            "execution_protocol": execution_protocol,
            "discovery_evidence": manual_evidence(chosen_id, endpoint, protocol, now),
        }
        preview = provider_onboarding.apply(repository, spec, replace=replace, dry_run=True, clock=now)
        io.write("Preview generated through the PAF-04A transaction path.")
        if _choice(io, "Persist this provider? [yes/no] ", ("yes", "no")) == "no":
            return GuidedResult("cancelled", spec, preview, None)
        applied = provider_onboarding.apply(repository, spec, replace=replace, dry_run=False, clock=now)
        return GuidedResult("applied", spec, preview, applied)
    except EOFError as error:
        raise GuidedOnboardingError("input ended; no changes made") from error
    except KeyboardInterrupt:
        return GuidedResult("cancelled", {}, None, None)