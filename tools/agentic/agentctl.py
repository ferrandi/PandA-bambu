#!/usr/bin/env python3
"""Bounded portable-agent inspection CLI; it never launches tasks."""
from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

import catalog
import contracts
import provider
import routing

ROOT = Path(__file__).resolve().parents[2]
LOCAL = ROOT / ".agentic-local"
STATE = ROOT / "agentic-state"


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
    args = parser.parse_args(argv)
    try:
        if args.command == "profiles":
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
    except (catalog.CatalogError, contracts.ContractError, provider.ProfileError, routing.RoutingError, ValueError) as error:
        print(f"agentctl: {error}", file=sys.stderr)
        return 3
    return 0


if __name__ == "__main__":
    raise SystemExit(main())