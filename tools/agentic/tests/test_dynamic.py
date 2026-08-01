from __future__ import annotations

import copy
import json
import sys
import tempfile
import unittest
from datetime import datetime, timedelta, timezone
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "tools" / "agentic"))

import catalog
import contracts
import discovery
import probe_cache
import resolver


class ContractTests(unittest.TestCase):
    def test_new_schemas_are_json(self):
        for name in (
            "catalog",
            "probe-record",
            "execution-plan",
            "role",
            "task",
            "result",
            "policy-overlay",
            "selection",
        ):
            value = json.loads((ROOT / f"agentic/schemas/{name}.schema.json").read_text())
            self.assertEqual(value["$schema"], "https://json-schema.org/draft/2020-12/schema")

    def test_required_local_paths_are_ignored(self):
        text = (ROOT / ".gitignore").read_text()
        self.assertIn("/.agentic-local/", text)
        self.assertIn("/agentic-state/", text)

    def test_redirects_are_disabled(self):
        self.assertIsNone(discovery._NoRedirect().redirect_request(None, None, 302, "redirect", {}, "https://other.invalid"))


class DiscoveryTests(unittest.TestCase):
    def test_generic_listing_normalizes_without_name_patterns(self):
        calls = []

        def transport(url, headers, timeout):
            calls.append((url, headers, timeout))
            return discovery.DiscoveryResponse(200, {}, b'{"data":[{"id":"opaque-a","name":"Fictional A","context":42}]}')

        result = discovery.discover("https://fixture.invalid", {"Authorization": "secret"}, [{"kind": "openai-models", "path": "/v1/models"}], 5, transport)
        self.assertEqual(result.models[0]["model_id"], "opaque-a")
        self.assertFalse(result.requires_model_id)
        self.assertEqual(calls[0][0], "https://fixture.invalid/v1/models")
        self.assertFalse(result.models[0]["eligible"])
        self.assertEqual(result.models[0]["execution_units"], [])

    def test_listing_falls_back_to_manual_model(self):
        def transport(*args):
            return discovery.DiscoveryResponse(404, {}, b"{}")

        result = discovery.discover("https://fixture.invalid", {}, [{"kind": "openai-models", "path": "/v1/models"}, {"kind": "model-info", "path": "/v1/model/info"}], 5, transport)
        self.assertTrue(result.requires_model_id)
        self.assertEqual(result.models, ())

    def test_imported_catalog(self):
        result = discovery.discover("", {}, [{"kind": "imported-catalog"}], 5, imported={"models": [{"id": "fixture-model"}]})
        self.assertEqual(result.adapter, "imported-catalog")

    def test_discovery_rejects_ambiguous_paths(self):
        for path in ("/", "//other"):
            with self.subTest(path=path), self.assertRaises(discovery.ProfileError):
                discovery.discover("https://fixture.invalid", {}, [{"kind": "openai-models", "path": path}], 5, lambda *args: None)

    def test_discovery_rejects_duplicate_model_identifiers(self):
        with self.assertRaises(discovery.ProfileError):
            discovery.discover("", {}, [{"kind": "imported-catalog"}], 5, imported={"models": [{"id": "duplicate"}, {"id": "duplicate"}]})


class ResolverTests(unittest.TestCase):
    def candidates(self):
        cap = {"tool_calling": {"status": "supported"}}
        return [
            {"model_id": "a", "execution_units": [{"client": "codex", "provider": "fixture", "model": "a", "protocol": "openai-responses", "effort": "medium"}], "eligible": True, "rejection_reasons": [], "capabilities": cap, "metadata": {"cost": 2, "quality": 8, "agentic_reliability": 8}},
            {"model_id": "b", "execution_units": [{"client": "claude-code", "provider": "fixture", "model": "b", "protocol": "anthropic-messages", "effort": "high"}], "eligible": True, "rejection_reasons": [], "capabilities": cap, "metadata": {"cost": 1, "quality": 7, "agentic_reliability": 7}},
        ]

    def test_deterministic_selection_and_explanation(self):
        plan = resolver.resolve(self.candidates(), "implementer", "lowest-cost-valid", {"tool_calling"})
        self.assertEqual(plan["selected"]["model"], "b")
        self.assertTrue(plan["explanation"])
        self.assertEqual(len(plan["fallback_chain"]), 1)

    def test_mandatory_filter_and_override(self):
        values = self.candidates()
        values[0]["capabilities"] = {}
        plan = resolver.resolve(values, "reviewer", "independent-review", {"tool_calling"}, override=values[1]["execution_units"][0])
        self.assertEqual(plan["selected"]["model"], "b")
        self.assertTrue(any("mandatory capability" in reason for item in plan["rejected"] for reason in item["reasons"]))

    def test_evaluation_has_no_fallback(self):
        plan = resolver.resolve(self.candidates(), "implementer", "maximum-quality", {"tool_calling"}, mode="evaluation", pins={"catalog_snapshot": "s", "client_version": "v", "task_version": "t", "context_hash": "h", "base_revision": "b", "budgets": {}})
        self.assertEqual(plan["fallback_chain"], [])

    def test_rejections_are_preserved_and_modes_are_strict(self):
        values = self.candidates()
        values[0]["eligible"] = False
        values[1]["eligible"] = False
        with self.assertRaises(resolver.ResolutionError) as caught:
            resolver.resolve(values, "implementer", "lowest-cost-valid", {"tool_calling"})
        self.assertEqual(len(caught.exception.rejected), 2)
        self.assertIn("local policy", str(caught.exception))
        with self.assertRaises(ValueError):
            resolver.resolve(self.candidates(), "implementer", "research-pinned", set())
        with self.assertRaises(ValueError):
            resolver.resolve(self.candidates(), "implementer", "maximum-quality", set(), mode="ablation", ablation_dimensions=["effort", "effort"])
        with self.assertRaises(ValueError):
            resolver.resolve(self.candidates(), "implementer", "maximum-quality", set(), pins={"bad": 1})
        values = self.candidates()
        values[0]["metadata"]["cost"] = float("nan")
        plan = resolver.resolve(values, "implementer", "lowest-cost-valid", {"tool_calling"})
        self.assertEqual(plan["selected"]["model"], "b")


class PortableTaskContractTests(unittest.TestCase):
    def setUp(self):
        self.fixture = ROOT / "agentic" / "fixtures"
        self.role = contracts.load_contract(ROOT / "agentic" / "roles" / "implementer.yaml", "role")
        self.task = contracts.load_contract(self.fixture / "tasks" / "fixture-task.json", "task")
        self.result = contracts.load_contract(self.fixture / "results" / "fixture-result.json", "result")
        self.overlay = contracts.load_contract(self.fixture / "overlays" / "fixture-overlay.json", "overlay")

    def discovered(self):
        return [{"model_id": "fixture-model", "display_name": "Fixture", "metadata": {}, "capabilities": {}, "eligible": False, "rejection_reasons": ["awaiting local policy and capability evaluation"], "execution_units": []}]

    def source(self, models):
        return {"schema": "evolvehls.agentic.catalog", "schema_version": "1.0", "profile_id": "fixture-profile", "snapshot_id": "snapshot", "created_at": "2026-01-01T00:00:00+00:00", "models": models}

    def test_all_tracked_fixtures_pass_semantic_validation(self):
        for kind, directory, suffix in (("role", ROOT / "agentic" / "roles", "*.yaml"), ("task", self.fixture / "tasks", "*.json"), ("result", self.fixture / "results", "*.json"), ("overlay", self.fixture / "overlays", "*.json")):
            for path in directory.glob(suffix):
                with self.subTest(path=path):
                    contracts.load_contract(path, kind)

    def test_fixtures_link_without_execution(self):
        self.assertEqual(self.task["role"], self.role["role_id"])
        self.assertEqual(self.result["task_id"], self.task["task_id"])
        self.assertEqual(self.result["validation"][0]["status"], "not-run")
        self.assertIsNone(self.result["validation"][0]["evidence"])

    def test_overlay_schema_has_concrete_paf_structures(self):
        schema = contracts.load_schema("policy-overlay")
        definitions = schema["$defs"]
        self.assertEqual(definitions["execution_unit"]["required"], ["client", "provider", "model", "protocol", "effort"])
        self.assertEqual(definitions["capability"]["required"], ["status", "confidence", "provenance"])
        self.assertIn("supported", definitions["status"]["enum"])
        self.assertIn("observed", definitions["confidence"]["enum"])
        self.assertEqual(definitions["provenance"]["required"], ["source", "confidence", "observed_at", "expires_at"])

    def test_result_schema_and_validator_require_passed_evidence(self):
        schema = contracts.load_schema("result")
        condition = schema["properties"]["validation"]["items"]["allOf"][0]
        self.assertEqual(condition["if"]["properties"]["status"]["const"], "passed")
        self.assertEqual(condition["then"]["properties"]["evidence"]["minLength"], 1)
        bad = copy.deepcopy(self.result)
        bad["validation"][0] = {"name": "result-contract", "status": "passed", "evidence": ""}
        with self.assertRaises(contracts.ContractError):
            contracts.validate_result(bad)

    def test_overlay_query_and_selection_are_deterministic(self):
        models = catalog.apply_overlay(self.discovered(), "fixture-profile", self.overlay)
        source = self.source(models)
        queried = catalog.query(source, self.role)
        self.assertEqual(queried["candidates"][0]["mandatory_unavailable"], [])
        self.assertEqual(queried["candidates"][0]["probe_needed"], [])
        plan = catalog.select(source, self.role)
        self.assertEqual(plan["selected"]["model"], "fixture-model")
        self.assertIn("catalog snapshot: snapshot", plan["explanation"])
        with tempfile.TemporaryDirectory() as directory:
            first = catalog.persist_selection(plan, source, self.role, Path(directory), "2026-01-01T00:00:00+00:00")
            second = catalog.persist_selection(plan, source, self.role, Path(directory), "2026-01-01T00:00:00+00:00")
            self.assertEqual(first, second)
            self.assertEqual(catalog.latest_selection(Path(directory))["selection_id"], json.loads(first.read_text())["selection_id"])

    def test_role_confidence_gates_query_and_selection_without_mutating_catalog(self):
        low = copy.deepcopy(self.overlay)
        low["rules"][0]["model_id"] = "low"
        low["rules"][0]["execution_units"][0]["model"] = "low"
        low["rules"][0]["capabilities"]["tool_calling"]["confidence"] = "declared"
        enough = copy.deepcopy(self.overlay)
        enough["rules"][0]["model_id"] = "enough"
        enough["rules"][0]["execution_units"][0]["model"] = "enough"
        enough["rules"][0]["metadata"] = {"cost": 2, "latency": 2, "quality": 2, "agentic_reliability": 2}
        models = catalog.apply_overlay(
            [
                {"model_id": "low", "display_name": "Low", "metadata": {}, "capabilities": {}, "eligible": False, "rejection_reasons": [], "execution_units": []},
                {"model_id": "enough", "display_name": "Enough", "metadata": {}, "capabilities": {}, "eligible": False, "rejection_reasons": [], "execution_units": []},
            ],
            "fixture-profile",
            {**self.overlay, "rules": [low["rules"][0], enough["rules"][0]]},
        )
        source = self.source(models)
        queried = catalog.query(source, self.role)
        low_candidate = next(item for item in queried["candidates"] if item["model_id"] == "low")
        self.assertEqual(low_candidate["probe_needed"], ["tool_calling"])
        before = copy.deepcopy(source)
        plan = catalog.select(source, self.role)
        self.assertEqual(plan["selected"]["model"], "enough")
        self.assertEqual(source, before)
        self.assertTrue(any(item["candidate"]["model"] == "low" and "mandatory capability confidence below observed: tool_calling" in item["reasons"] for item in plan["rejected"]))

    def test_selection_fails_when_every_candidate_has_insufficient_confidence(self):
        low = copy.deepcopy(self.overlay)
        low["rules"][0]["capabilities"]["tool_calling"]["confidence"] = "declared"
        source = self.source(catalog.apply_overlay(self.discovered(), "fixture-profile", low))
        with self.assertRaises(resolver.ResolutionError) as caught:
            catalog.select(source, self.role)
        self.assertIn("mandatory capability confidence below observed: tool_calling", str(caught.exception))

    def test_selection_timestamp_validation_and_chronological_latest(self):
        selection = {"schema": "evolvehls.agentic.selection", "schema_version": "1.0", "selection_id": "id", "created_at": "2026-01-01T00:00:00+00:00", "catalog_snapshot": "snapshot", "role_id": "implementer", "role_version": "1.0", "execution_plan": {}}
        for created_at in ("not-a-timestamp", "2026-01-01T00:00:00"):
            with self.subTest(created_at=created_at):
                bad = {**selection, "created_at": created_at}
                with self.assertRaises(contracts.ContractError):
                    contracts.validate_selection(bad)
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            records = [
                {**selection, "selection_id": "a", "created_at": "2026-01-01T00:30:00+01:00"},
                {**selection, "selection_id": "b", "created_at": "2025-12-31T23:45:00+00:00"},
                {**selection, "selection_id": "c", "created_at": "2026-01-01T00:00:00+00:00"},
            ]
            for record in records:
                (root / f"{record['selection_id']}.json").write_text(json.dumps(record))
            (root / "corrupted.json").write_text(json.dumps({**selection, "selection_id": "broken", "created_at": "not-a-timestamp"}))
            self.assertEqual(catalog.latest_selection(root)["selection_id"], "c")

    def test_catalog_persistence_requires_generated_snapshot_identifier(self):
        source = self.source([])
        with tempfile.TemporaryDirectory() as directory:
            with self.assertRaises(catalog.CatalogError):
                catalog.persist_catalog({**source, "snapshot_id": "../escape"}, Path(directory))
            profile = {"profile_id": "fixture-profile", "discovery": {"methods": [{"kind": "imported-catalog"}]}, "timeout": {"seconds": 5}}
            generated = catalog.synchronize(profile, "", {}, imported={"models": [{"id": "fixture-model"}]})
            path = catalog.persist_catalog(generated, Path(directory))
            self.assertEqual(path.name, f"{generated['snapshot_id']}.json")
            self.assertRegex(generated["snapshot_id"], r"^[0-9a-f]{64}$")

    def test_overlay_fails_closed_for_unknown_or_invalid_model(self):
        bad = copy.deepcopy(self.overlay)
        bad["rules"][0]["model_id"] = "unknown"
        with self.assertRaises(catalog.CatalogError):
            catalog.apply_overlay(self.discovered(), "fixture-profile", bad)
        bad = copy.deepcopy(self.overlay)
        bad["rules"][0]["execution_units"][0]["model"] = "other"
        with self.assertRaises(catalog.CatalogError):
            catalog.apply_overlay(self.discovered(), "fixture-profile", bad)

    def test_contracts_reject_extra_fields_and_mismatched_results(self):
        bad = copy.deepcopy(self.task)
        bad["client"] = "forbidden"
        with self.assertRaises(contracts.ContractError):
            contracts.validate_task(bad)
        bad = copy.deepcopy(self.result)
        bad["task_version"] = ""
        with self.assertRaises(contracts.ContractError):
            contracts.validate_result(bad)


class CacheTests(unittest.TestCase):
    def test_success_cache_ttl(self):
        now = datetime(2026, 1, 1, tzinfo=timezone.utc)
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            path = probe_cache.store_success(root, "work", "model", "openai-responses", "tool_calling", 60, now)
            self.assertNotIn("token", path.read_text())
            self.assertIsNotNone(probe_cache.load_fresh(root, "work", "model", "openai-responses", "tool_calling", now + timedelta(seconds=59)))
            self.assertIsNone(probe_cache.load_fresh(root, "work", "model", "openai-responses", "tool_calling", now + timedelta(seconds=61)))

    def test_cache_rejects_forged_identity_naive_time_and_excess_ttl(self):
        now = datetime(2026, 1, 1, tzinfo=timezone.utc)
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            path = probe_cache.store_observation(root, "work", "model", "openai-responses", "streaming", "unsupported", 60, now)
            self.assertEqual(probe_cache.load_fresh(root, "work", "model", "openai-responses", "streaming", now, 60)["status"], "unsupported")
            record = json.loads(path.read_text())
            record["profile_id"] = "other"
            path.write_text(json.dumps(record))
            self.assertIsNone(probe_cache.load_fresh(root, "work", "model", "openai-responses", "streaming", now, 60))
            record["profile_id"] = "work"
            record["observed_at"] = "2026-01-01T00:00:00"
            path.write_text(json.dumps(record))
            self.assertIsNone(probe_cache.load_fresh(root, "work", "model", "openai-responses", "streaming", now, 60))
            record["observed_at"] = now.isoformat()
            record["expires_at"] = (now + timedelta(days=2)).isoformat()
            path.write_text(json.dumps(record))
            self.assertIsNone(probe_cache.load_fresh(root, "work", "model", "openai-responses", "streaming", now, 60))


if __name__ == "__main__":
    unittest.main()