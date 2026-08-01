#!/usr/bin/env python3
"""Version-aware, bounded portable client-adapter detection.

Detection deliberately never reads native session storage, browser state, token
caches, or global client configuration. It only uses executable presence,
bounded version output, and optional explicitly approved project-local files.
"""
from __future__ import annotations

import re
import shutil
import subprocess
from collections.abc import Callable, Mapping
from typing import Any

from redaction import diagnostic, safe_diagnostics

CAPABILITIES = {
    "native_account_execution",
    "configured_api_execution",
    "custom_provider_settings",
    "custom_base_url",
    "environment_credential_reference",
    "token_helper_reference",
    "model_selection",
    "separate_plan_act_models",
    "structured_output",
    "tool_execution",
    "noninteractive_mode",
    "working_directory",
    "resume_session",
    "cancellation",
    "timeout",
    "machine_readable_result",
}
READINESS = {
    "available",
    "authenticated-or-ready",
    "configuration-required",
    "unsupported-version",
    "execution-path-unsupported",
    "unavailable",
    "unknown",
}
PROTOCOLS = {"openai-responses", "openai-chat-completions", "anthropic-messages"}

# Templates are fictionalized capability contracts, not assertions about any
# installed client. Unknown versions fail closed and need confirmation.
TEMPLATES: dict[str, dict[str, Any]] = {
    "claude-code": {
        "command": "claude",
        "execution_family": "claude-code",
        "paths": {
            "native-account": {
                "invocation_class": "native-account-cli",
                "access_classes": ["native-account-client"],
                "funding_classes": ["subscription"],
                "auth_modes": ["native-session"],
                "protocols": ["anthropic-messages"],
                "capabilities": ["native_account_execution", "model_selection", "tool_execution", "working_directory", "cancellation", "timeout"],
            },
            "configured-api": {
                "invocation_class": "configured-api-cli",
                "access_classes": ["api-gateway"],
                "funding_classes": ["project", "organization", "personal-api"],
                "auth_modes": ["environment-token", "token-helper"],
                "protocols": ["anthropic-messages"],
                "capabilities": ["configured_api_execution", "custom_provider_settings", "custom_base_url", "environment_credential_reference", "model_selection", "tool_execution", "working_directory", "cancellation", "timeout"],
            },
        },
        "versions": {"1": {"native-account", "configured-api"}},
    },
    "codex": {
        "command": "codex",
        "execution_family": "codex",
        "paths": {
            "native-account": {
                "invocation_class": "native-account-cli",
                "access_classes": ["native-account-client"],
                "funding_classes": ["subscription"],
                "auth_modes": ["native-session"],
                "protocols": ["openai-responses"],
                "capabilities": ["native_account_execution", "model_selection", "tool_execution", "working_directory", "resume_session", "cancellation", "timeout"],
            },
            "configured-api": {
                "invocation_class": "configured-api-cli",
                "access_classes": ["api-gateway"],
                "funding_classes": ["project", "organization", "personal-api"],
                "auth_modes": ["environment-token", "token-helper"],
                "protocols": ["openai-responses", "openai-chat-completions"],
                "capabilities": ["configured_api_execution", "custom_provider_settings", "custom_base_url", "environment_credential_reference", "model_selection", "tool_execution", "working_directory", "resume_session", "cancellation", "timeout"],
            },
        },
        "versions": {"1": {"native-account", "configured-api"}},
    },
    "cline": {
        "command": "cline",
        "execution_family": "cline",
        "paths": {
            "configured-api": {
                "invocation_class": "configured-api-client",
                "access_classes": ["api-gateway"],
                "funding_classes": ["project", "organization", "personal-api"],
                "auth_modes": ["environment-token", "token-helper"],
                "protocols": ["openai-responses", "openai-chat-completions", "anthropic-messages"],
                "capabilities": ["configured_api_execution", "custom_base_url", "environment_credential_reference", "model_selection", "separate_plan_act_models", "tool_execution", "working_directory"],
            },
            "local-compatible": {
                "invocation_class": "configured-api-client",
                "access_classes": ["local-server"],
                "funding_classes": ["local"],
                "auth_modes": ["none", "environment-token"],
                "protocols": ["openai-chat-completions", "anthropic-messages"],
                "capabilities": ["configured_api_execution", "custom_base_url", "model_selection", "separate_plan_act_models", "tool_execution", "working_directory"],
            },
        },
        "versions": {"1": {"configured-api", "local-compatible"}},
    },
    "direct-http": {
        "command": None,
        "execution_family": "direct-http",
        "paths": {
            "openai-compatible": {
                "invocation_class": "http-api-client",
                "access_classes": ["api-gateway", "local-server"],
                "funding_classes": ["project", "organization", "personal-api", "local"],
                "auth_modes": ["environment-token", "token-helper", "none"],
                "protocols": ["openai-responses", "openai-chat-completions"],
                "capabilities": ["configured_api_execution", "custom_base_url", "environment_credential_reference", "token_helper_reference", "model_selection", "timeout", "cancellation"],
            },
            "anthropic-compatible": {
                "invocation_class": "http-api-client",
                "access_classes": ["api-gateway", "local-server"],
                "funding_classes": ["project", "organization", "personal-api", "local"],
                "auth_modes": ["environment-token", "token-helper", "none"],
                "protocols": ["anthropic-messages"],
                "capabilities": ["configured_api_execution", "custom_base_url", "environment_credential_reference", "token_helper_reference", "model_selection", "timeout", "cancellation"],
            },
        },
        "versions": {"builtin": {"openai-compatible", "anthropic-compatible"}},
    },
}


def parse_version(text: str) -> str | None:
    match = re.search(r"\b(\d+)(?:\.\d+)+(?:[-+][A-Za-z0-9.-]+)?\b", text[:4096])
    return match.group(0) if match else None


def adapter_descriptor(adapter_id: str) -> dict[str, Any]:
    template = TEMPLATES[adapter_id]
    return {
        "adapter_id": adapter_id,
        "execution_family": template["execution_family"],
        "command": template["command"],
        "execution_paths": [
            {
                "path_id": path_id,
                **spec,
            }
            for path_id, spec in sorted(template["paths"].items())
        ],
    }


def detect(
    adapter_id: str,
    *,
    which: Callable[[str], str | None] = shutil.which,
    run: Callable[..., subprocess.CompletedProcess[str]] = subprocess.run,
) -> dict[str, Any]:
    """Return per-path evidence without examining any authentication material."""
    template = TEMPLATES[adapter_id]
    command = template["command"]
    if command is None:
        version = "builtin"
        exposed = template["versions"][version]
        executable = True
        diagnostics: list[str] = []
    else:
        executable_path = which(command)
        executable = executable_path is not None
        version = None
        diagnostics = []
        if executable:
            try:
                result = run([executable_path, "--version"], capture_output=True, text=True, timeout=5, check=False, shell=False)
                version = parse_version((result.stdout or result.stderr or "")[:4096]) if result.returncode == 0 else None
            except (OSError, subprocess.SubprocessError) as error:
                diagnostics.append(diagnostic(error))
        exposed = template["versions"].get(version.split(".", 1)[0] if version else "", set())
    paths = []
    for path_id, spec in sorted(template["paths"].items()):
        if not executable:
            state = "unavailable"
        elif path_id not in exposed:
            state = "unsupported-version" if version else "unknown"
        elif path_id == "native-account":
            # Authentication is owned by the client. Presence/version prove no
            # session readiness, and PAF intentionally does not inspect it.
            state = "unknown"
        else:
            state = "configuration-required"
        paths.append(
            {
                "adapter_id": adapter_id,
                "execution_path_id": path_id,
                "adapter_version": version,
                "state": state,
                "capabilities": sorted(spec["capabilities"] if path_id in exposed else []),
                "capability_evidence": {
                    "source": "version-capability-map" if version else "bounded-version-command",
                    "version": version,
                },
            }
        )
    return {
        "adapter_id": adapter_id,
        "available": executable,
        "adapter_version": version,
        "execution_family": template["execution_family"],
        "paths": paths,
        "diagnostics": safe_diagnostics(diagnostics),
    }


def detect_all(**kwargs: Any) -> list[dict[str, Any]]:
    return [detect(adapter_id, **kwargs) for adapter_id in sorted(TEMPLATES)]