from __future__ import annotations
import json,sys,tempfile,unittest
from datetime import datetime,timedelta,timezone
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3];sys.path.insert(0,str(ROOT/"tools"/"agentic"))
import discovery,probe_cache,resolver
class ContractTests(unittest.TestCase):
 def test_new_schemas_are_json(self):
  for name in ("catalog","probe-record","execution-plan"):
   value=json.loads((ROOT/f"agentic/schemas/{name}.schema.json").read_text());self.assertEqual(value["$schema"],"https://json-schema.org/draft/2020-12/schema")
 def test_required_local_paths_are_ignored(self):
  text=(ROOT/".gitignore").read_text();self.assertIn("/.agentic-local/",text);self.assertIn("/agentic-state/",text)
 def test_redirects_are_disabled(self):
  self.assertIsNone(discovery._NoRedirect().redirect_request(None,None,302,"redirect",{},"https://other.invalid"))
class DiscoveryTests(unittest.TestCase):
 def test_generic_listing_normalizes_without_name_patterns(self):
  calls=[]
  def transport(url,headers,timeout):
   calls.append((url,headers,timeout));return discovery.DiscoveryResponse(200,{},b'{"data":[{"id":"opaque-a","name":"Fictional A","context":42}]}')
  result=discovery.discover("https://fixture.invalid",{"Authorization":"secret"},[{"kind":"openai-models","path":"/v1/models"}],5,transport)
  self.assertEqual(result.models[0]["model_id"],"opaque-a");self.assertFalse(result.requires_model_id);self.assertEqual(calls[0][0],"https://fixture.invalid/v1/models");self.assertFalse(result.models[0]["eligible"]);self.assertEqual(result.models[0]["execution_units"],[])
 def test_listing_falls_back_to_manual_model(self):
  def transport(*args):return discovery.DiscoveryResponse(404,{},b"{}")
  result=discovery.discover("https://fixture.invalid",{},[{"kind":"openai-models","path":"/v1/models"},{"kind":"model-info","path":"/v1/model/info"}],5,transport)
  self.assertTrue(result.requires_model_id);self.assertEqual(result.models,())
 def test_imported_catalog(self):
  result=discovery.discover("",{},[{"kind":"imported-catalog"}],5,imported={"models":[{"id":"fixture-model"}]})
  self.assertEqual(result.adapter,"imported-catalog")
 def test_discovery_rejects_ambiguous_paths(self):
  for path in ("/","//other"):
   with self.subTest(path=path),self.assertRaises(discovery.ProfileError):discovery.discover("https://fixture.invalid",{},[{"kind":"openai-models","path":path}],5,lambda *args:None)
class ResolverTests(unittest.TestCase):
 def candidates(self):
  cap={"tool_calling":{"status":"supported"}}
  return [{"model_id":"a","execution_units":[{"client":"codex","provider":"fixture","model":"a","protocol":"openai-responses","effort":"medium"}],"eligible":True,"rejection_reasons":[],"capabilities":cap,"metadata":{"cost":2,"quality":8,"agentic_reliability":8}},{"model_id":"b","execution_units":[{"client":"claude-code","provider":"fixture","model":"b","protocol":"anthropic-messages","effort":"high"}],"eligible":True,"rejection_reasons":[],"capabilities":cap,"metadata":{"cost":1,"quality":7,"agentic_reliability":7}}]
 def test_deterministic_selection_and_explanation(self):
  plan=resolver.resolve(self.candidates(),"implementer","lowest-cost-valid",{"tool_calling"});self.assertEqual(plan["selected"]["model"],"b");self.assertTrue(plan["explanation"]);self.assertEqual(len(plan["fallback_chain"]),1)
 def test_mandatory_filter_and_override(self):
  values=self.candidates();values[0]["capabilities"]={}
  plan=resolver.resolve(values,"reviewer","independent-review",{"tool_calling"},override=values[1]["execution_units"][0]);self.assertEqual(plan["selected"]["model"],"b");self.assertTrue(any("mandatory capability" in reason for item in plan["rejected"] for reason in item["reasons"]))
 def test_evaluation_has_no_fallback(self):
  plan=resolver.resolve(self.candidates(),"implementer","maximum-quality",{"tool_calling"},mode="evaluation",pins={"catalog_snapshot":"s","client_version":"v","task_version":"t","context_hash":"h","base_revision":"b","budgets":{}});self.assertEqual(plan["fallback_chain"],[])
 def test_rejections_are_preserved_and_modes_are_strict(self):
  values=self.candidates();values[0]["eligible"]=False;values[1]["eligible"]=False
  with self.assertRaises(resolver.ResolutionError) as caught:resolver.resolve(values,"implementer","lowest-cost-valid",{"tool_calling"})
  self.assertEqual(len(caught.exception.rejected),2);self.assertIn("local policy",str(caught.exception))
  with self.assertRaises(ValueError):resolver.resolve(self.candidates(),"implementer","research-pinned",set())
  with self.assertRaises(ValueError):resolver.resolve(self.candidates(),"implementer","maximum-quality",set(),mode="ablation",ablation_dimensions=["effort","effort"])
  with self.assertRaises(ValueError):resolver.resolve(self.candidates(),"implementer","maximum-quality",set(),pins={"bad":1})
  values=self.candidates();values[0]["metadata"]["cost"]=float("nan")
  plan=resolver.resolve(values,"implementer","lowest-cost-valid",{"tool_calling"});self.assertEqual(plan["selected"]["model"],"b")
class CacheTests(unittest.TestCase):
 def test_success_cache_ttl(self):
  now=datetime(2026,1,1,tzinfo=timezone.utc)
  with tempfile.TemporaryDirectory() as directory:
   root=Path(directory);path=probe_cache.store_success(root,"work","model","openai-responses","tool_calling",60,now);rendered=path.read_text();self.assertNotIn("token",rendered)
   self.assertIsNotNone(probe_cache.load_fresh(root,"work","model","openai-responses","tool_calling",now+timedelta(seconds=59)))
   self.assertIsNone(probe_cache.load_fresh(root,"work","model","openai-responses","tool_calling",now+timedelta(seconds=61)))
 def test_cache_rejects_forged_identity_naive_time_and_excess_ttl(self):
  now=datetime(2026,1,1,tzinfo=timezone.utc)
  with tempfile.TemporaryDirectory() as directory:
   root=Path(directory);path=probe_cache.store_observation(root,"work","model","openai-responses","streaming","unsupported",60,now)
   self.assertEqual(probe_cache.load_fresh(root,"work","model","openai-responses","streaming",now,60)["status"],"unsupported")
   record=json.loads(path.read_text());record["profile_id"]="other";path.write_text(json.dumps(record));self.assertIsNone(probe_cache.load_fresh(root,"work","model","openai-responses","streaming",now,60))
   record["profile_id"]="work";record["observed_at"]="2026-01-01T00:00:00";path.write_text(json.dumps(record));self.assertIsNone(probe_cache.load_fresh(root,"work","model","openai-responses","streaming",now,60))
   record["observed_at"]=now.isoformat();record["expires_at"]=(now+timedelta(days=2)).isoformat();path.write_text(json.dumps(record));self.assertIsNone(probe_cache.load_fresh(root,"work","model","openai-responses","streaming",now,60))
if __name__=="__main__":unittest.main()
