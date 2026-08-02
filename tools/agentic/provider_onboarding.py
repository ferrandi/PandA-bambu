#!/usr/bin/env python3
"""Declarative, non-secret PAF-04A provider configuration and overlays."""
from __future__ import annotations

import hashlib
import json
import re
from datetime import datetime, timezone
from pathlib import Path
from typing import Any, Callable, Mapping
from urllib.parse import urlsplit, urlunsplit

import contracts
import local_state
import provider_overlays

ROOT = Path(__file__).resolve().parents[2]
SPEC_SCHEMA = "evolvehls.agentic.provider-onboarding-spec"
CONFIG_SCHEMA = "evolvehls.agentic.provider-configuration"
EVIDENCE_SCHEMA = "evolvehls.agentic.provider-discovery-evidence"
RECEIPT_SCHEMA = "evolvehls.agentic.provider-onboarding-receipt"
PROFILE_OVERLAY_SCHEMA = "evolvehls.agentic.provider-profile-overlay"
RUNTIME_OVERLAY_SCHEMA = "evolvehls.agentic.provider-runtime-overlay"
BUILTIN_REGISTRY = ROOT / "agentic" / "fixtures" / "profiles" / "portable-fixture-registry.json"
BUILTIN_RUNTIME_MAP = ROOT / "agentic" / "fixtures" / "profiles" / "portable-adapter-registry.json"


class ProviderOnboardingError(ValueError):
    """Classified provider configuration failure."""

    def __init__(self, category: str, message: str):
        super().__init__(message)
        self.category = category


def _now(clock: Callable[[], datetime] | None = None) -> str:
    instant = (clock or (lambda: datetime.now(timezone.utc)))()
    if instant.tzinfo is None or instant.utcoffset() is None:
        raise ProviderOnboardingError("validation", "clock must return a timezone-aware timestamp")
    return instant.astimezone(timezone.utc).isoformat()


def _canonical(value: Mapping[str, Any]) -> bytes:
    return local_state.canonical_json(value)


def _digest(value: Mapping[str, Any]) -> str:
    return hashlib.sha256(_canonical(value)).hexdigest()


def normalize_origin(value: object) -> str:
    if not isinstance(value, str) or not value.strip():
        raise ProviderOnboardingError("validation", "endpoint origin is required")
    try:
        parsed = urlsplit(value.strip())
    except ValueError:
        raise ProviderOnboardingError("validation", "endpoint origin is malformed") from None
    if parsed.scheme not in {"https", "http"} or not parsed.hostname:
        raise ProviderOnboardingError("validation", "endpoint origin must be HTTPS or loopback HTTP")
    if parsed.username is not None or parsed.password is not None or parsed.query or parsed.fragment:
        raise ProviderOnboardingError("validation", "endpoint origin must not contain credentials, query, or fragment")
    host = parsed.hostname.lower().rstrip(".")
    loopback = host == "localhost" or host == "::1" or host.startswith("127.")
    if parsed.scheme == "http" and not loopback:
        raise ProviderOnboardingError("validation", "remote endpoints must use HTTPS")
    try:
        port = parsed.port
    except ValueError:
        raise ProviderOnboardingError("validation", "endpoint origin has an invalid port") from None
    authority = f"[{host}]" if ":" in host else host
    if port is not None:
        authority = f"{authority}:{port}"
    return urlunsplit((parsed.scheme, authority, parsed.path.rstrip("/"), "", ""))


def _safe_yaml(text: str) -> object:
    try:
        import yaml
    except ImportError:
        raise ProviderOnboardingError("parse", "YAML support requires the repository validation Python environment") from None

    class DuplicateKeyLoader(yaml.SafeLoader):
        pass

    def construct_mapping(loader, node, deep=False):
        mapping: dict[object, object] = {}
        for key_node, value_node in node.value:
            key = loader.construct_object(key_node, deep=deep)
            if key in mapping:
                raise yaml.constructor.ConstructorError("while constructing a mapping", node.start_mark, f"duplicate key: {key}", key_node.start_mark)
            mapping[key] = loader.construct_object(value_node, deep=deep)
        return mapping

    DuplicateKeyLoader.add_constructor(yaml.resolver.BaseResolver.DEFAULT_MAPPING_TAG, construct_mapping)
    try:
        return yaml.load(text, Loader=DuplicateKeyLoader)
    except yaml.YAMLError as error:
        raise ProviderOnboardingError("parse", f"malformed YAML provider specification: {type(error).__name__}") from None


def load_spec(path: Path) -> dict[str, Any]:
    suffix = path.suffix.lower()
    if suffix not in {".json", ".yaml", ".yml"}:
        raise ProviderOnboardingError("parse", "unsupported provider specification extension")
    try:
        if path.is_symlink() or not path.is_file():
            raise OSError()
        text = path.read_text(encoding="utf-8")
        value = json.loads(text) if suffix == ".json" else _safe_yaml(text)
    except ProviderOnboardingError:
        raise
    except (OSError, UnicodeDecodeError, json.JSONDecodeError) as error:
        kind = "JSON" if suffix == ".json" else "YAML"
        raise ProviderOnboardingError("parse", f"malformed {kind} provider specification: {type(error).__name__}") from None
    if not isinstance(value, dict):
        raise ProviderOnboardingError("parse", "provider specification top level must be a mapping")
    try:
        contracts.validate_provider_onboarding_spec(value)
    except contracts.ContractError as error:
        raise ProviderOnboardingError("validation", str(error)) from None
    return value


def _slug(value: str, maximum: int = 40) -> str:
    normalized = re.sub(r"[^a-z0-9]+", "-", value.lower()).strip("-.")
    return (normalized or "model")[:maximum].rstrip("-.")


def _identity(prefix: str, *material: str) -> str:
    rendered = "\x1f".join(material)
    suffix = hashlib.sha256(rendered.encode("utf-8")).hexdigest()[:12]
    result = f"{prefix}-{suffix}"
    try:
        return local_state.validate_identifier(result, "derived identifier")
    except local_state.LocalStateError:
        raise ProviderOnboardingError("validation", "derived identifier exceeds local-state bounds") from None


def derive_ids(spec: Mapping[str, Any]) -> dict[str, str]:
    try:
        contracts.validate_provider_onboarding_spec(spec)
    except contracts.ContractError as error:
        raise ProviderOnboardingError("validation", str(error)) from None
    provider_id = spec["provider_id"]
    endpoint = spec["endpoint"]["origin"]
    protocol = spec["endpoint"]["protocol"]
    model = spec["model"]
    auth = spec["authentication"]
    auth_ref = auth.get("env_var", "none")
    material = (provider_id, endpoint, protocol, model, auth["mode"], auth_ref)
    readable = _slug(model)
    return {
        "provider_id": provider_id,
        "profile_id": _identity(f"provider-{provider_id[:80]}-{readable}", *material),
        "runtime_entry_id": _identity(f"runtime-{provider_id[:80]}-{readable}", *material, "direct-http"),
        "profile_overlay_id": _identity(f"provider-profile-overlay-{provider_id[:80]}", *material),
        "runtime_overlay_id": _identity(f"provider-runtime-overlay-{provider_id[:80]}", *material),
    }


def _builtin() -> tuple[dict[str, Any], dict[str, Any]]:
    try:
        registry = contracts.load_contract(BUILTIN_REGISTRY, "registry")
        runtime = contracts.load_json(BUILTIN_RUNTIME_MAP)
    except contracts.ContractError as error:
        raise ProviderOnboardingError("validation", f"built-in canonical documents are invalid: {error}") from None
    return registry, runtime


def configuration(spec: Mapping[str, Any], *, clock: Callable[[], datetime] | None = None) -> dict[str, Any]:
    try:
        contracts.validate_provider_onboarding_spec(spec)
    except contracts.ContractError as error:
        raise ProviderOnboardingError("validation", str(error)) from None
    normalized_origin = normalize_origin(spec["endpoint"]["origin"])
    if normalized_origin != spec["endpoint"]["origin"]:
        raise ProviderOnboardingError("validation", "endpoint origin must be normalized")
    identities = derive_ids(spec)
    execution_path = "anthropic-compatible" if spec["endpoint"]["protocol"] == "anthropic-compatible" else "openai-compatible"
    result = {
        "schema": CONFIG_SCHEMA,
        "schema_version": "1.0",
        "provider_id": spec["provider_id"],
        "endpoint": dict(spec["endpoint"]),
        "authentication": dict(spec["authentication"]),
        "model": spec["model"],
        "canonical": {
            "profile_id": identities["profile_id"],
            "adapter_id": "generic-http",
            "runtime_entry_id": identities["runtime_entry_id"],
            "execution_path": execution_path,
        },
        "configured_at": _now(clock),
    }
    try:
        contracts.validate_provider_configuration(result)
    except contracts.ContractError as error:
        raise ProviderOnboardingError("validation", str(error)) from None
    return result


def overlays(config: Mapping[str, Any], registry: Mapping[str, Any], runtime_map: Mapping[str, Any]) -> tuple[dict[str, Any], dict[str, Any]]:
    try:
        contracts.validate_provider_configuration(config)
    except contracts.ContractError as error:
        raise ProviderOnboardingError("validation", str(error)) from None
    config_digest = _digest(config)
    identities = derive_ids({
        "schema": SPEC_SCHEMA, "schema_version": "1.0", "provider_id": config["provider_id"],
        "endpoint": config["endpoint"], "authentication": config["authentication"], "model": config["model"], "roles": [],
    })
    local = urlsplit(config["endpoint"]["origin"]).scheme == "http"
    protocol = "anthropic-messages" if config["endpoint"]["protocol"] == "anthropic-compatible" else "openai-chat-completions"
    profile = {
        "profile_id": config["canonical"]["profile_id"], "schema_version": "1.0", "adapter_id": "generic-http",
        "access_class": "local-server" if local else "api-gateway",
        "funding_class": "local" if local else "personal-api",
        "auth_mode": config["authentication"]["mode"],
        "binding": {"kind": "provider-profile", "ref": config["provider_id"]},
        "model": {"kind": "pinned", "ref": config["model"]}, "protocol": protocol,
        "capabilities": {"basic_text": {"status": "supported", "confidence": "declared", "provenance": ["provider-configuration"]}},
        "privacy": {"data_classes": ["public"]}, "cost": {"tier": 0 if local else 1},
        "resources": {"requires": []}, "availability": {"required": True}, "priority": 0,
    }
    profile_overlay = {
        "schema": PROFILE_OVERLAY_SCHEMA, "schema_version": "1.0", "overlay_id": identities["profile_overlay_id"],
        "provider_id": config["provider_id"], "registry_id": registry["registry_id"], "registry_version": registry["version"],
        "configuration_digest": config_digest, "profile": profile,
    }
    runtime_overlay = {
        "schema": RUNTIME_OVERLAY_SCHEMA, "schema_version": "1.0", "overlay_id": identities["runtime_overlay_id"],
        "provider_id": config["provider_id"], "runtime_map_id": runtime_map["runtime_map_id"], "runtime_map_version": runtime_map["version"],
        "configuration_digest": config_digest,
        "entry": {"runtime_entry_id": config["canonical"]["runtime_entry_id"], "profile_id": config["canonical"]["profile_id"], "adapter_id": "generic-http", "client_template": "direct-http", "execution_path": config["canonical"]["execution_path"]},
    }
    try:
        contracts.validate_provider_profile_overlay(profile_overlay, registry)
        contracts.validate_provider_runtime_overlay(runtime_overlay, registry, runtime_map)
    except contracts.ContractError as error:
        raise ProviderOnboardingError("validation", str(error)) from None
    return profile_overlay, runtime_overlay


def _path(repository: Path, group: str, *parts: str) -> Path:
    return local_state.safe_path(repository, group, *parts)


def _read(repository: Path, group: str, *parts: str) -> dict[str, Any]:
    path = _path(repository, group, *parts)
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, UnicodeDecodeError, json.JSONDecodeError) as error:
        raise ProviderOnboardingError("persistence", f"provider state is unavailable: {type(error).__name__}") from None
    if not isinstance(value, dict):
        raise ProviderOnboardingError("persistence", "provider state is not an object")
    return value


def load_provider(repository: Path, provider_id: str) -> dict[str, Any]:
    value = _read(repository, local_state.LOCAL_DIR, "providers", f"{provider_id}.json")
    try:
        contracts.validate_provider_configuration(value)
    except contracts.ContractError as error:
        raise ProviderOnboardingError("validation", str(error)) from None
    return value


def _load_overlays(repository: Path) -> tuple[list[dict[str, Any]], list[dict[str, Any]]]:
    result: list[list[dict[str, Any]]] = [[], []]
    for index, (directory, validator) in enumerate((("profiles", contracts.validate_provider_profile_overlay), ("runtime", contracts.validate_provider_runtime_overlay))):
        root = _path(repository, local_state.LOCAL_DIR, "overlays", directory)
        if not root.exists():
            continue
        if root.is_symlink() or not root.is_dir():
            raise ProviderOnboardingError("persistence", "provider overlay directory is unsafe")
        for path in sorted(root.glob("*.json")):
            if path.is_symlink():
                raise ProviderOnboardingError("persistence", "provider overlay target is unsafe")
            try:
                value = json.loads(path.read_text(encoding="utf-8"))
                validator(value)
            except (OSError, UnicodeDecodeError, json.JSONDecodeError, contracts.ContractError) as error:
                raise ProviderOnboardingError("validation", f"invalid persisted provider overlay: {type(error).__name__}") from None
            result[index].append(value)
    return result[0], result[1]


def effective_documents(repository: Path) -> tuple[dict[str, Any], dict[str, Any], dict[str, Any]]:
    registry, runtime = _builtin()
    profiles, runtimes = _load_overlays(repository)
    try:
        return provider_overlays.compose(registry, runtime, profiles, runtimes)
    except provider_overlays.OverlayError as error:
        raise ProviderOnboardingError("validation", str(error)) from None


def _receipt(config: Mapping[str, Any], profile: Mapping[str, Any], runtime: Mapping[str, Any], action: str) -> dict[str, Any]:
    value = {
        "schema": RECEIPT_SCHEMA, "schema_version": "1.0", "provider_id": config["provider_id"],
        "configured_at": config["configured_at"], "configuration_digest": _digest(config),
        "profile_overlay_id": profile["overlay_id"], "runtime_overlay_id": runtime["overlay_id"], "action": action,
    }
    contracts.validate_provider_onboarding_receipt(value)
    return value


def _difference(old: Mapping[str, Any], new: Mapping[str, Any]) -> list[str]:
    ignored = {"configured_at"}
    return sorted(key for key in new if key not in ignored and old.get(key) != new.get(key))


def apply(repository: Path, spec: Mapping[str, Any], *, replace: bool = False, dry_run: bool = False, clock: Callable[[], datetime] | None = None, fail_after: int | None = None) -> dict[str, Any]:
    config = configuration(spec, clock=clock)
    registry, runtime = _builtin()
    profile, runtime_overlay = overlays(config, registry, runtime)
    try:
        old = load_provider(repository, config["provider_id"])
    except ProviderOnboardingError as error:
        if error.category != "persistence":
            raise
        old = None
    if old is not None and _difference(old, config) == []:
        return {"provider_id": config["provider_id"], "dry_run": dry_run, "idempotent": True, "changed": []}
    if old is not None and not replace:
        raise ProviderOnboardingError("collision", "material provider configuration differs: " + ", ".join(_difference(old, config)))
    profiles, runtimes = _load_overlays(repository)
    profiles = [item for item in profiles if item["provider_id"] != config["provider_id"]] + [profile]
    runtimes = [item for item in runtimes if item["provider_id"] != config["provider_id"]] + [runtime_overlay]
    try:
        provider_overlays.compose(registry, runtime, profiles, runtimes)
    except provider_overlays.OverlayError as error:
        raise ProviderOnboardingError("collision", str(error)) from None
    receipt = _receipt(config, profile, runtime_overlay, "replaced" if old is not None else "applied")
    writes = [
        local_state.PlannedWrite(local_state.LOCAL_DIR, ("providers", f"{config['provider_id']}.json"), _canonical(config)),
        local_state.PlannedWrite(local_state.LOCAL_DIR, ("overlays", "profiles", f"{config['provider_id']}.json"), _canonical(profile)),
        local_state.PlannedWrite(local_state.LOCAL_DIR, ("overlays", "runtime", f"{config['provider_id']}.json"), _canonical(runtime_overlay)),
        local_state.PlannedWrite(local_state.STATE_DIR, ("provider-setup", f"{config['provider_id']}.json"), _canonical(receipt)),
    ]
    try:
        changed = local_state.transaction(repository, writes, replace=replace, dry_run=dry_run, fail_after=fail_after)
    except local_state.LocalStateError as error:
        raise ProviderOnboardingError("persistence", str(error)) from None
    return {"provider_id": config["provider_id"], "dry_run": dry_run, "idempotent": False, "changed": [str(path.relative_to(repository.resolve())) for path in changed]}


def list_providers(repository: Path) -> list[dict[str, Any]]:
    directory = _path(repository, local_state.LOCAL_DIR, "providers")
    if not directory.exists():
        return []
    if directory.is_symlink() or not directory.is_dir():
        raise ProviderOnboardingError("persistence", "provider directory is unsafe")
    result = []
    for path in sorted(directory.glob("*.json")):
        try:
            config = load_provider(repository, path.stem)
            result.append({"provider_id": config["provider_id"], "endpoint_origin": config["endpoint"]["origin"], "protocol": config["endpoint"]["protocol"], "model": config["model"]})
        except ProviderOnboardingError:
            result.append({"provider_id": path.stem, "status": "invalid"})
    return result


def remove(repository: Path, provider_id: str, *, force: bool = False, dry_run: bool = False, fail_after: int | None = None) -> dict[str, Any]:
    try:
        config = load_provider(repository, provider_id)
    except ProviderOnboardingError as error:
        if error.category == "persistence":
            return {"provider_id": provider_id, "removed": False, "already_removed": True, "changed": []}
        raise
    profile_id = config["canonical"]["profile_id"]
    references: list[str] = []
    readiness = _path(repository, local_state.STATE_DIR, "readiness")
    if readiness.exists():
        for path in readiness.glob("*.json"):
            if not path.is_symlink() and profile_id in path.read_text(encoding="utf-8", errors="replace"):
                references.append(str(path.relative_to(repository.resolve())))
    if references and not force:
        raise ProviderOnboardingError("reference", "active state references provider profile: " + ", ".join(sorted(references)))
    deletes = [
        local_state.PlannedDelete(local_state.LOCAL_DIR, ("providers", f"{provider_id}.json")),
        local_state.PlannedDelete(local_state.LOCAL_DIR, ("overlays", "profiles", f"{provider_id}.json")),
        local_state.PlannedDelete(local_state.LOCAL_DIR, ("overlays", "runtime", f"{provider_id}.json")),
        local_state.PlannedDelete(local_state.STATE_DIR, ("provider-setup", f"{provider_id}.json")),
    ]
    try:
        changed = local_state.transaction(repository, [], deletes, dry_run=dry_run, fail_after=fail_after)
    except local_state.LocalStateError as error:
        raise ProviderOnboardingError("persistence", str(error)) from None
    return {"provider_id": provider_id, "removed": not dry_run, "dry_run": dry_run, "references": references, "changed": [str(path.relative_to(repository.resolve())) for path in changed]}