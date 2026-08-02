#!/usr/bin/env python3
"""Bounded portable-agent inspection and provider-onboarding CLI; never launches tasks."""
from __future__ import annotations

import argparse
import json
import os
import sys
from pathlib import Path

import adapters
import catalog
import contracts
import portable_adapters
import provider
import provider_discovery
import provider_guided
import provider_onboarding
import routing
import setup

ROOT = Path(__file__).resolve().parents[2]
LOCAL = ROOT / ".agentic-local"
STATE = ROOT / "agentic-state"
EXIT_CODES = {
    "validation": 3,
    "parse": 9,
    "persistence": 10,
    "collision": 11,
    "reference": 12,
    "not-implemented": 13,
}


def _document(path: str, kind: str) -> dict:
    return contracts.load_contract(Path(path), kind)


def _catalog(path: str) -> dict:
    value = contracts.load_json(Path(path))
    required = {"schema", "schema_version", "profile_id", "snapshot_id", "created_at", "models"}
    if set(value) != required or value["schema"] != "evolvehls.agentic.catalog":
        raise catalog.CatalogError("invalid catalog document")
    return value


def _print(value: object) -> None:
    print(json.dumps(value, sort_keys=True, indent=2))


def _provider_command(args: argparse.Namespace) -> int:
    if args.provider_command == "add":
        result = provider_guided.run(
            args.root,
            io=provider_guided.StdioWizardIO(),
            replace=args.replace,
        )
        _print({"status": result.status, "preview": result.preview, "applied": result.applied})
    elif args.provider_command == "discover":
        config = provider_onboarding.load_provider(args.root, args.provider_id)
        if config["endpoint"]["protocol"] != "openai-compatible":
            raise provider_onboarding.ProviderOnboardingError(
                "validation",
                "network model discovery currently supports OpenAI-compatible endpoints only",
            )
        evidence = provider_discovery.discover(
            config["provider_id"],
            config["endpoint"]["origin"],
            authentication=config["authentication"],
            env=os.environ,
            allow_private=args.allow_private_network,
        )
        assigned_models = {item["model"] for item in config.get("role_assignments", [])}
        visible_models = {item["model_id"] for item in evidence["models"]}
        result = {
            "evidence": evidence,
            "missing_assigned_models": sorted(assigned_models - visible_models) if evidence["status"] == "succeeded" else [],
            "persisted": False,
        }
        if args.apply:
            io = provider_guided.StdioWizardIO()
            if not io.isatty():
                raise provider_onboarding.ProviderOnboardingError(
                    "validation",
                    "provider discover --apply requires an interactive TTY confirmation",
                )
            if io.read("Persist refreshed discovery evidence without changing assignments? [yes/no] ").strip().lower() != "yes":
                result["status"] = "cancelled"
            else:
                spec = provider_onboarding.refresh_spec(config, evidence)
                result["applied"] = provider_onboarding.apply(args.root, spec, replace=True)
                result["persisted"] = True
        _print(result)
    elif args.provider_command in {"apply", "preview"}:
        spec = provider_onboarding.load_spec(Path(args.spec))
        _print(provider_onboarding.apply(args.root, spec, dry_run=args.provider_command == "preview", replace=args.replace))
    elif args.provider_command == "validate":
        registry, runtime_map, provenance = provider_onboarding.effective_documents(args.root)
        _print({"valid": True, "registry_id": registry["registry_id"], "runtime_map_id": runtime_map["runtime_map_id"], "provenance": provenance})
    elif args.provider_command == "show":
        _print(provider_onboarding.load_provider(args.root, args.provider_id))
    elif args.provider_command == "list":
        _print(provider_onboarding.list_providers(args.root))
    else:
        _print(provider_onboarding.remove(args.root, args.provider_id, force=args.force, dry_run=args.dry_run))
    return 0


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(prog="agentctl")
    commands = parser.add_subparsers(dest="command", required=True)
    catalog_command = commands.add_parser("catalog")
    catalog_sub = catalog_command.add_subparsers(dest="catalog_command", required=True)
    sync = catalog_sub.add_parser("sync")
    sync.add_argument("--profile", required=True)
    sync.add_argument("--imported-catalog", required=True)
    sync.add_argument("--overlay")
    sync.add_argument("--output-dir", default=str(LOCAL / "catalogs"))
    models = commands.add_parser("models")
    model_sub = models.add_subparsers(dest="models_command", required=True)
    query = model_sub.add_parser("query")
    query.add_argument("--catalog", required=True)
    query.add_argument("--role", required=True)
    select = model_sub.add_parser("select")
    select.add_argument("--catalog", required=True)
    select.add_argument("--role", required=True)
    select.add_argument("--objective")
    select.add_argument("--mode", default="development")
    select.add_argument("--output-dir", default=str(STATE / "selections"))
    explain = model_sub.add_parser("explain")
    explain.add_argument("--latest", action="store_true")
    explain.add_argument("--selection")
    explain.add_argument("--selection-dir", default=str(STATE / "selections"))
    profiles = commands.add_parser("profiles")
    profiles_sub = profiles.add_subparsers(dest="profiles_command", required=True)
    validate = profiles_sub.add_parser("validate")
    validate.add_argument("--registry", required=True)
    listing = profiles_sub.add_parser("list")
    listing.add_argument("--registry", required=True)
    show = profiles_sub.add_parser("show")
    show.add_argument("--registry", required=True)
    show.add_argument("--profile", required=True)
    readiness = commands.add_parser("readiness")
    readiness_sub = readiness.add_subparsers(dest="readiness_command", required=True)
    readiness_show = readiness_sub.add_parser("show")
    readiness_show.add_argument("--report", required=True)
    readiness_show.add_argument("--registry", required=True)
    route = commands.add_parser("routing")
    route_sub = route.add_subparsers(dest="routing_command", required=True)
    route_explain = route_sub.add_parser("explain")
    route_explain.add_argument("--registry", required=True)
    route_explain.add_argument("--policy", required=True)
    route_explain.add_argument("--readiness", required=True)
    route_explain.add_argument("--task", required=True)
    route_explain.add_argument("--role", required=True)
    route_explain.add_argument("--mode", default="development")
    route_explain.add_argument("--prior")
    portable_setup = commands.add_parser("setup")
    portable_setup.add_argument("--spec", required=True)
    portable_setup.add_argument("--root", type=Path, default=ROOT)
    portable_setup.add_argument("--dry-run", action="store_true")
    portable_setup.add_argument("--replace", action="store_true")
    portable_setup.add_argument("--json", action="store_true")
    portable_doctor = commands.add_parser("doctor")
    portable_doctor.add_argument("--json", action="store_true")
    adapters_parser = commands.add_parser("adapters")
    portable_adapters_sub = adapters_parser.add_subparsers(dest="portable_adapter_command", required=True)
    for name in ("detect", "list", "show"):
        item = portable_adapters_sub.add_parser(name)
        item.add_argument("--json", action="store_true")
        item.add_argument("--dry-run", action="store_true")
        if name == "show":
            item.add_argument("--adapter", required=True)
    portable_config = commands.add_parser("config")
    portable_config_sub = portable_config.add_subparsers(dest="portable_config_command", required=True)
    for name in ("preview", "generate", "validate"):
        item = portable_config_sub.add_parser(name)
        item.add_argument("--spec", required=True)
        item.add_argument("--root", type=Path, default=ROOT)
        item.add_argument("--json", action="store_true")
        item.add_argument("--dry-run", action="store_true")
        if name == "generate":
            item.add_argument("--replace", action="store_true")

    providers = commands.add_parser("provider")
    provider_sub = providers.add_subparsers(dest="provider_command", required=True)
    add = provider_sub.add_parser("add", help="interactive non-secret provider onboarding")
    add.add_argument("--root", type=Path, default=ROOT)
    add.add_argument("--replace", action="store_true")
    for name in ("apply", "preview"):
        item = provider_sub.add_parser(name)
        item.add_argument("--spec", required=True)
        item.add_argument("--root", type=Path, default=ROOT)
        item.add_argument("--replace", action="store_true")
    discover = provider_sub.add_parser("discover", help="inspect OpenAI-compatible model listing; does not persist changes")
    discover.add_argument("provider_id")
    discover.add_argument("--root", type=Path, default=ROOT)
    discover.add_argument("--allow-private-network", action="store_true")
    discover.add_argument("--apply", action="store_true", help="persist refreshed evidence after interactive confirmation")
    validate = provider_sub.add_parser("validate")
    validate.add_argument("--root", type=Path, default=ROOT)
    show = provider_sub.add_parser("show")
    show.add_argument("provider_id")
    show.add_argument("--root", type=Path, default=ROOT)
    provider_list = provider_sub.add_parser("list")
    provider_list.add_argument("--root", type=Path, default=ROOT)
    remove = provider_sub.add_parser("remove")
    remove.add_argument("provider_id")
    remove.add_argument("--root", type=Path, default=ROOT)
    remove.add_argument("--force", action="store_true")
    remove.add_argument("--dry-run", action="store_true")

    args = parser.parse_args(argv)
    try:
        if args.command == "provider":
            return _provider_command(args)
        if args.command == "setup":
            spec = contracts.load_json(Path(args.spec))
            _print(setup.apply(args.root, spec, dry_run=args.dry_run, replace=args.replace))
        elif args.command == "doctor":
            _print({"adapters": adapters.detect_all(), "task_execution": "not-implemented"})
        elif args.command == "adapters":
            if args.portable_adapter_command == "detect":
                _print(adapters.detect_all())
            elif args.portable_adapter_command == "list":
                _print([adapters.adapter_descriptor(name) for name in sorted(adapters.TEMPLATES)])
            else:
                _print(adapters.adapter_descriptor(args.adapter))
        elif args.command == "config":
            spec = contracts.load_json(Path(args.spec))
            if args.portable_config_command == "preview":
                _print(setup.config_preview(args.root, spec))
            elif args.portable_config_command == "generate":
                _print(setup.config_generate(args.root, spec, dry_run=args.dry_run, replace=args.replace))
            else:
                setup.validate_spec(spec)
                _print({"registry_id": spec["registry"]["registry_id"], "registry_version": spec["registry"]["version"], "runtime_map_id": spec["runtime_map"]["runtime_map_id"], "valid": True})
        elif args.command == "profiles":
            registry = _document(args.registry, "registry")
            if args.profiles_command == "validate":
                _print({"registry_id": registry["registry_id"], "valid": True})
            elif args.profiles_command == "list":
                _print([{"profile_id": item["profile_id"], "adapter_id": item["adapter_id"], "access_class": item["access_class"], "funding_class": item["funding_class"]} for item in sorted(registry["profiles"], key=lambda item: item["profile_id"])])
            else:
                matches = [item for item in registry["profiles"] if item["profile_id"] == args.profile]
                if len(matches) != 1:
                    raise contracts.ContractError("unknown profile identifier")
                _print(matches[0])
        elif args.command == "readiness":
            registry = _document(args.registry, "registry")
            report = _document(args.report, "readiness")
            contracts.validate_readiness(report, registry)
            _print(report)
        elif args.command == "routing":
            registry = _document(args.registry, "registry")
            policy = _document(args.policy, "policy")
            readiness_report = _document(args.readiness, "readiness")
            contracts.validate_readiness(readiness_report, registry)
            prior = _document(args.prior, "decision") if args.prior else None
            decision = routing.resolve(_document(args.task, "task"), _document(args.role, "role"), registry, policy, readiness_report, args.mode, prior)
            contracts.validate_decision(decision)
            _print(decision)
        elif args.command == "catalog" and args.catalog_command == "sync":
            profile = provider.load_profile(Path(args.profile))
            imported = contracts.load_json(Path(args.imported_catalog))
            overlay = _document(args.overlay, "overlay") if args.overlay else None
            result = catalog.synchronize(profile, "", {}, imported=imported, overlay=overlay)
            path = catalog.persist_catalog(result, Path(args.output_dir))
            _print({"catalog": str(path), "snapshot_id": result["snapshot_id"], "requires_model_id": result["discovery"]["requires_model_id"]})
        elif args.command == "models" and args.models_command == "query":
            _print(catalog.query(_catalog(args.catalog), _document(args.role, "role")))
        elif args.command == "models" and args.models_command == "select":
            source = _catalog(args.catalog)
            role = _document(args.role, "role")
            plan = catalog.select(source, role, args.objective, args.mode)
            path = catalog.persist_selection(plan, source, role, Path(args.output_dir))
            _print({"selection": str(path), "selected": plan["selected"], "explanation": plan["explanation"]})
        else:
            selection = catalog.latest_selection(Path(args.selection_dir)) if args.latest else _document(args.selection, "selection")
            _print(selection["execution_plan"]["explanation"])
    except KeyboardInterrupt:
        print("agentctl: cancelled; no changes made", file=sys.stderr)
        return 130
    except provider_guided.GuidedOnboardingError as error:
        print(f"agentctl: {error}", file=sys.stderr)
        return EXIT_CODES["validation"]
    except provider_onboarding.ProviderOnboardingError as error:
        print(f"agentctl: {error}", file=sys.stderr)
        return EXIT_CODES[error.category]
    except RecursionError:
        print("agentctl: malformed response", file=sys.stderr)
        return EXIT_CODES["validation"]
    except (catalog.CatalogError, contracts.ContractError, provider.ProfileError, routing.RoutingError, portable_adapters.PortableAdapterError, setup.SetupError, ValueError) as error:
        print(f"agentctl: {error}", file=sys.stderr)
        return 3
    return 0


if __name__ == "__main__":
    raise SystemExit(main())