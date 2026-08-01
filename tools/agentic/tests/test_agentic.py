from __future__ import annotations
import copy,io,json,subprocess,sys,tempfile,unittest
from contextlib import redirect_stderr
from pathlib import Path
from unittest.mock import Mock,patch
ROOT=Path(__file__).resolve().parents[3];TOOL=ROOT/"tools"/"agentic";sys.path.insert(0,str(TOOL))
import provider,doctor,probe_gateway as probe
WORK=ROOT/"agentic"/"providers"/"work.example.yaml";LOCAL=ROOT/"agentic"/"providers"/"local.example.yaml"
class ProviderTests(unittest.TestCase):
 def test_schema_and_examples_are_valid(self):
  self.assertEqual(json.loads((ROOT/"agentic/schemas/provider.schema.json").read_text())["$schema"],"https://json-schema.org/draft/2020-12/schema");provider.load_profile(WORK);provider.load_profile(LOCAL)
 def test_invalid_and_adversarial_profiles(self):
  original=provider.load_profile(WORK)
  changes=[("protocols",["bad"]),("capabilities",{"streaming":[],"tool_calling":"unknown","structured_output":"unknown"}),("context",{"window_tokens":True,"input_modalities":["text"]}),("context",{"window_tokens":1,"input_modalities":["text","text"]}),("context",{"window_tokens":1,"input_modalities":[[]]}),("timeout",{"seconds":True})]
  for key,value in changes:
   bad=copy.deepcopy(original);bad[key]=value
   with self.subTest(key=key,value=value),self.assertRaises(provider.ProfileError):provider.validate_profile(bad)
 def test_auth_class_fields_and_redaction(self):
  bad=provider.load_profile(WORK);bad["authentication"].pop("token_env")
  with self.assertRaises(provider.ProfileError):provider.validate_profile(bad)
  rendered=json.dumps(provider.redact({"Authorization":"Bearer secret","message":"https://private secret","x-api-key":"secret"},["secret","https://private"]))
  self.assertNotIn("secret",rendered);self.assertNotIn("private",rendered)
class DoctorTests(unittest.TestCase):
 def test_missing_clients_are_nonfatal(self):self.assertTrue(all(not x["available"] for x in doctor.diagnose(ROOT,which=lambda _:None)["clients"]))
 def test_version_parsing_with_mocked_subprocess(self):
  run=Mock(return_value=subprocess.CompletedProcess([],0,"codex-cli 1.2.3\n",""));self.assertEqual(doctor.version("/bin/codex",run)[0],"1.2.3");run.assert_called_once()
 def test_parse_error_is_redacted(self):
  with tempfile.TemporaryDirectory() as directory:
   root=Path(directory);(root/".codex").mkdir();(root/".codex/config.toml").write_text('token="secret"\nbad=[');rendered=json.dumps(doctor.diagnose(root,which=lambda _:None));self.assertIn("parse-error",rendered);self.assertNotIn("secret",rendered)
class FixtureTransport:
 def __init__(self):self.calls=[]
 def __call__(self,url,headers,raw,timeout):
  body=json.loads(raw);self.calls.append((url,headers,body,timeout))
  protocol="anthropic-messages" if url.endswith("/messages") else ("openai-chat-completions" if url.endswith("/chat/completions") else "openai-responses")
  if body.get("stream"):return probe.TransportResponse(200,{"Content-Type":"text/event-stream"},b"data: {}")
  if protocol=="openai-responses":
   if body.get("tools"):doc={"id":"r","output":[{"type":"function_call"}]}
   elif body.get("text"):doc={"id":"r","output":[{"type":"message","content":[{"type":"output_text","text":"{\"ok\":true}"}]}]}
   else:doc={"id":"r","output":[],"usage":{"input_tokens":1}}
  elif protocol=="anthropic-messages":
   doc={"id":"a","content":[{"type":"tool_use"}] if body.get("tools") else [{"type":"text","text":"ok"}],"usage":{"input_tokens":1}}
  else:
   message={"content":"{\"ok\":true}"};message.update({"tool_calls":[{}]} if body.get("tools") else {});doc={"id":"c","choices":[{"message":message}],"usage":{"prompt_tokens":1}}
  return probe.TransportResponse(200,{"Content-Type":"application/json"},json.dumps(doc).encode())
class ProbeTests(unittest.TestCase):
 def setUp(self):
  self.profile=provider.load_profile(WORK);self.env={"EVOLVEHLS_WORK_API_BASE_URL":"https://private.invalid","EVOLVEHLS_WORK_API_TOKEN":"top-secret","EVOLVEHLS_WORK_MODEL":"confidential-model"}
 def fixed(self,status=200,body=b'{"id":"r","output":[]}'):return lambda *args:probe.TransportResponse(status,{},body)
 def test_dry_run_reads_no_runtime_and_makes_no_request(self):
  transport=Mock();report=probe.probe(self.profile,["openai-responses"],"implementation",True,{},transport);self.assertEqual(report["protocols"][0]["status"],"not-run");transport.assert_not_called()
 def test_all_protocol_shapes_headers_and_capabilities(self):
  transport=FixtureTransport();report=probe.probe(self.profile,list(provider.PROTOCOLS),"implementation",False,self.env,transport)
  self.assertTrue(all(x["status"]=="success" for x in report["protocols"]))
  for result in report["protocols"]:
   expected={"streaming":"supported","tool_calling":"supported","structured_output":"unknown" if result["protocol"]=="anthropic-messages" else "supported"};self.assertEqual(result["capabilities"],expected)
  anthropic=next(x for x in transport.calls if x[0].endswith("/messages"));self.assertEqual(anthropic[1]["x-api-key"],"top-secret");self.assertNotIn("Authorization",anthropic[1])
  responses_tool=next(x for x in transport.calls if x[0].endswith("/responses") and x[2].get("tools"));self.assertIn("parameters",responses_tool[2]["tools"][0]);self.assertNotIn("input_schema",responses_tool[2]["tools"][0])
  rendered=json.dumps(report);self.assertNotIn("private.invalid",rendered);self.assertNotIn("top-secret",rendered);self.assertNotIn("confidential-model",rendered)
 def test_failure_classifications(self):
  cases=[(401,b'{"error":"bad"}',"authentication-failure"),(404,b"{}","unsupported-protocol"),(400,b'{"error":"shape"}',"protocol-error"),(200,b"{}","malformed-response"),(200,b"not json","malformed-response")]
  for status,body,expected in cases:
   with self.subTest(expected=expected):self.assertEqual(probe.probe(self.profile,["openai-responses"],"implementation",False,self.env,self.fixed(status,body))["protocols"][0]["status"],expected)
 def test_timeout(self):
  report=probe.probe(self.profile,["openai-responses"],"implementation",False,self.env,lambda *args:(_ for _ in ()).throw(TimeoutError()));self.assertEqual(report["protocols"][0]["status"],"timeout");self.assertEqual(probe.exit_code(report),probe.EXIT_TRANSPORT)
 def test_cli_configuration_error_has_no_secret_or_traceback(self):
  with tempfile.TemporaryDirectory() as directory:
   path=Path(directory)/"bad.yaml";path.write_text('{"protocols": []}');stderr=io.StringIO()
   with redirect_stderr(stderr):code=probe.main([str(path),"--protocol","openai-responses"])
   self.assertEqual(code,probe.EXIT_CONFIGURATION);self.assertNotIn("Traceback",stderr.getvalue());self.assertNotIn("secret",stderr.getvalue())
 def test_raw_shapes_utf8_and_capability_exit(self):
  nested=probe.TransportResponse(200,{},b"{\"id\":\"r\",\"output\":[{\"content\":null}]}");self.assertEqual(probe.base_status("openai-responses",nested)[0],"malformed-response")
  valid=probe.TransportResponse(200,{},b"{\"id\":\"r\",\"error\":null,\"output\":[]}");self.assertEqual(probe.base_status("openai-responses",valid)[0],"success")
  malformed=probe.TransportResponse(200,{},b"{\"choices\":\"wrong\"}");self.assertEqual(probe.base_status("openai-chat-completions",malformed)[0],"malformed-response")
  with tempfile.TemporaryDirectory() as directory:
   path=Path(directory)/"bad.yaml";path.write_bytes(b"\xff")
   with self.assertRaises(provider.ProfileError):provider.load_profile(path)
  calls=[0]
  def partial(*args):
   calls[0]+=1
   if calls[0]>1:raise TimeoutError()
   return valid
  report=probe.probe(self.profile,["openai-responses"],"implementation",False,self.env,partial);self.assertEqual(probe.exit_code(report),probe.EXIT_TRANSPORT)
if __name__=="__main__":unittest.main()
