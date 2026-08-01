#!/usr/bin/env python3
"""Safely probe explicitly selected provider protocol surfaces."""
from __future__ import annotations
import argparse,json,os,socket,sys,urllib.error,urllib.request
from dataclasses import dataclass
from pathlib import Path
from typing import Callable,Mapping
from probe_cache import store_observation
from provider import PROTOCOLS,ProfileError,auth_headers,load_profile,redact,resolve_runtime
EXIT_OK,EXIT_CAPABILITY_FAILURE,EXIT_CONFIGURATION,EXIT_TRANSPORT=0,2,3,4
PATHS={"openai-responses":"/v1/responses","anthropic-messages":"/v1/messages","openai-chat-completions":"/v1/chat/completions"}
MAX_RESPONSE_BYTES=1048576
PROBE_CAPABILITIES={"basic_text","streaming","tool_calling","structured_output","context_limit","usage_reporting","embeddings"}
@dataclass
class TransportResponse:status:int;headers:Mapping[str,str];body:bytes
class _NoRedirect(urllib.request.HTTPRedirectHandler):
    def redirect_request(self,request,fp,code,message,headers,new_url):return None
def urllib_transport(url,headers,body,timeout):
    request=urllib.request.Request(url,data=body,headers=dict(headers),method="POST")
    try:
        with urllib.request.build_opener(_NoRedirect).open(request,timeout=timeout) as response:return TransportResponse(response.status,dict(response.headers),response.read(MAX_RESPONSE_BYTES+1))
    except urllib.error.HTTPError as error:return TransportResponse(error.code,dict(error.headers),error.read(MAX_RESPONSE_BYTES+1))
def payload(protocol,model,operation):
    prompt="Return JSON with ok=true. This is a synthetic capability check; use no external data.";base={"model":model}
    if protocol=="openai-responses":
        base.update({"input":prompt,"max_output_tokens":32})
        if operation=="streaming":base["stream"]=True
        elif operation=="tool_calling":base.update({"tools":[{"type":"function","name":"capability_echo","description":"Return a synthetic value","parameters":{"type":"object","properties":{"value":{"type":"string"}}}}],"tool_choice":{"type":"function","name":"capability_echo"}})
        elif operation=="structured_output":base["text"]={"format":{"type":"json_schema","name":"probe","strict":True,"schema":{"type":"object","properties":{"ok":{"type":"boolean"}},"required":["ok"],"additionalProperties":False}}}
    elif protocol=="anthropic-messages":
        base.update({"max_tokens":32,"messages":[{"role":"user","content":prompt}]})
        if operation=="streaming":base["stream"]=True
        elif operation=="tool_calling":base.update({"tools":[{"name":"capability_echo","description":"Return a synthetic value","input_schema":{"type":"object","properties":{"value":{"type":"string"}}}}],"tool_choice":{"type":"tool","name":"capability_echo"}})
    else:
        base.update({"max_tokens":32,"messages":[{"role":"user","content":prompt}]})
        if operation=="streaming":base["stream"]=True
        elif operation=="tool_calling":base.update({"tools":[{"type":"function","function":{"name":"capability_echo","description":"Return a synthetic value","parameters":{"type":"object","properties":{"value":{"type":"string"}}}}}],"tool_choice":{"type":"function","function":{"name":"capability_echo"}}})
        elif operation=="structured_output":base["response_format"]={"type":"json_object"}
    return base
def document(response):
    if len(response.body)>MAX_RESPONSE_BYTES:return None
    try:value=json.loads(response.body.decode())
    except (UnicodeDecodeError,json.JSONDecodeError):return None
    return value if isinstance(value,dict) else None
def base_status(protocol,response):
    if response.status in {401,403}:return "authentication-failure",None
    if response.status in {404,405,501}:return "unsupported-protocol",None
    if not 200<=response.status<300:return "protocol-error",None
    value=document(response)
    if value is None or value.get("error") is not None:return "malformed-response",None
    if protocol=="openai-responses":valid=isinstance(value.get("id"),str) and isinstance(value.get("output"),list) and all(isinstance(x,dict) and ("content" not in x or isinstance(x["content"],list)) for x in value["output"])
    elif protocol=="anthropic-messages":valid=isinstance(value.get("content"),list)
    else:valid=isinstance(value.get("choices"),list) and bool(value["choices"]) and all(isinstance(x,dict) and isinstance(x.get("message"),dict) for x in value["choices"])
    return ("success",value) if valid else ("malformed-response",None)
def observed(protocol,operation,response,value):
    if len(response.body)>MAX_RESPONSE_BYTES:return "probe-failed"
    if operation=="streaming":
        content_type=next((v for k,v in response.headers.items() if k.lower()=="content-type"),"")
        return "supported" if "text/event-stream" in content_type or response.body.lstrip().startswith(b"data:") else "unsupported"
    if value is None:return "probe-failed"
    if operation=="tool_calling":
        if protocol=="openai-responses":ok=any(x.get("type")=="function_call" for x in value["output"] if isinstance(x,dict))
        elif protocol=="anthropic-messages":ok=any(x.get("type")=="tool_use" for x in value["content"] if isinstance(x,dict))
        else:ok=any(bool(x["message"].get("tool_calls")) for x in value["choices"])
        return "supported" if ok else "unsupported"
    if protocol=="anthropic-messages":return "unknown"
    if protocol=="openai-responses":
        texts=[part.get("text") for item in value["output"] if isinstance(item,dict) for part in item.get("content",[]) if isinstance(part,dict) and isinstance(part.get("text"),str)]
        text=texts[0] if texts else None
    else:text=value["choices"][0]["message"].get("content")
    try:parsed=json.loads(text) if isinstance(text,str) else None
    except json.JSONDecodeError:parsed=None
    return "supported" if isinstance(parsed,dict) else "unsupported"
def has_basic_text(protocol,value):
    if not isinstance(value,dict):return False
    if protocol=="openai-responses":return any(isinstance(part,dict) and isinstance(part.get("text"),str) and bool(part["text"]) for item in value.get("output",[]) if isinstance(item,dict) for part in item.get("content",[]) if isinstance(item.get("content",[]),list))
    if protocol=="anthropic-messages":return any(isinstance(item,dict) and item.get("type")=="text" and isinstance(item.get("text"),str) and bool(item["text"]) for item in value.get("content",[]))
    return any(isinstance(item,dict) and isinstance(item.get("message"),dict) and isinstance(item["message"].get("content"),str) and bool(item["message"]["content"]) for item in value.get("choices",[]))
def request(transport,url,headers,body,timeout):
    try:return transport(url,headers,body,timeout),None
    except (TimeoutError,socket.timeout):return None,"timeout"
    except (OSError,urllib.error.URLError):return None,"transport-failure"
def probe(profile,protocols,role,dry_run,env,transport:Callable[...,TransportResponse]=urllib_transport,capabilities=None,model_override=None,cache_dir=None):
    selected=set(capabilities or {"basic_text"});require_unknown=selected-PROBE_CAPABILITIES
    if require_unknown:raise ProfileError("unknown requested capability")
    if len(protocols)!=len(set(protocols)):raise ProfileError("duplicate requested protocol")
    if set(protocols)-set(profile["protocols"]):raise ProfileError("requested protocol is not declared by this profile")
    if dry_run:return {"profile_id":profile["profile_id"],"dry_run":True,"protocols":[{"protocol":p,"status":"not-run"} for p in protocols]}
    endpoint,token,model=resolve_runtime(profile,role,env,model_override=model_override);results=[]
    for protocol in protocols:
        if profile["protocols"][protocol]=="unsupported":
            results.append({"protocol":protocol,"status":"unsupported-protocol","authentication":"unknown","capabilities":{"basic_text":"unsupported","streaming":"unsupported","tool_calling":"unsupported","structured_output":"unsupported","context_limit":"unknown","usage_reporting":"unknown","embeddings":"unknown"},"probe_failure":None,"timeout_cancellation":{"timeout":"not-observed","cancellation":"not-probed"},"usage_metadata":"not-reported"});continue
        headers={"Content-Type":"application/json",**auth_headers(profile,protocol,token)}
        if protocol=="anthropic-messages":headers["anthropic-version"]="2023-06-01"
        response,failure=request(transport,endpoint+PATHS[protocol],headers,json.dumps(payload(protocol,model,"compatibility")).encode(),float(profile["timeout"]["seconds"]))
        if failure:status,value=failure,None
        else:status,value=base_status(protocol,response)
        capabilities={"basic_text":"supported" if status=="success" and has_basic_text(protocol,value) else ("unsupported" if status=="success" else "probe-failed"),"streaming":"unknown","tool_calling":"unknown","structured_output":"unknown","context_limit":"unknown","usage_reporting":"supported" if status=="success" and value and isinstance(value.get("usage"),dict) else "unknown","embeddings":"unknown"};timing={"timeout":"observed" if status=="timeout" else "not-observed","cancellation":"not-probed"};probe_failure=None
        if status=="success":
            for operation,key in (("streaming","streaming"),("tool_calling","tool_calling"),("structured_output","structured_output")):
                if key not in selected:continue
                if protocol=="anthropic-messages" and operation=="structured_output":continue
                cap_response,cap_failure=request(transport,endpoint+PATHS[protocol],headers,json.dumps(payload(protocol,model,operation)).encode(),float(profile["timeout"]["seconds"]))
                if cap_failure:capabilities[key]="probe-failed";timing["timeout"]="observed" if cap_failure=="timeout" else timing["timeout"];probe_failure=cap_failure
                else:
                    if operation=="streaming" and 200<=cap_response.status<300:
                        capabilities[key]=observed(protocol,operation,cap_response,None)
                    else:
                        cap_status,cap_value=base_status(protocol,cap_response)
                        capabilities[key]=observed(protocol,operation,cap_response,cap_value) if cap_status=="success" else ("unsupported" if cap_status=="protocol-error" else "probe-failed")
        result={"protocol":protocol,"status":status,"authentication":"accepted" if status=="success" else ("rejected" if status=="authentication-failure" else "unknown"),"capabilities":capabilities,"probe_failure":probe_failure,"timeout_cancellation":timing,"usage_metadata":"reported" if value and isinstance(value.get("usage"),dict) else "not-reported"}
        results.append(result)
        if cache_dir is not None and status=="success":
            try:
                for capability in selected:
                    if capabilities.get(capability) in {"supported","unsupported"}:store_observation(cache_dir,profile["profile_id"],model,protocol,capability,capabilities[capability],profile["discovery"]["probe_ttl_seconds"])
            except (OSError,ValueError):raise ProfileError("cannot write redacted probe cache") from None
    return redact({"profile_id":profile["profile_id"],"dry_run":False,"protocols":results},[endpoint,token or "",model])
def exit_code(report):
    statuses={x["status"] for x in report["protocols"]}
    failures={x.get("probe_failure") for x in report["protocols"]}
    capability_statuses={status for item in report["protocols"] for status in item.get("capabilities",{}).values()}
    if statuses&{"transport-failure","timeout"} or failures&{"transport-failure","timeout"}:return EXIT_TRANSPORT
    if capability_statuses&{"unsupported","probe-failed"}:return EXIT_CAPABILITY_FAILURE
    if statuses<={"success","not-run"} and failures<={None}:return EXIT_OK
    return EXIT_CAPABILITY_FAILURE
def main(argv=None):
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument("profile",type=Path);parser.add_argument("--protocol",action="append",choices=sorted(PROTOCOLS),required=True);parser.add_argument("--role",default="implementation");parser.add_argument("--model-env");parser.add_argument("--dry-run",action="store_true");parser.add_argument("--capability",action="append",choices=sorted(PROBE_CAPABILITIES));parser.add_argument("--cache-dir",type=Path,default=Path("agentic-state/probes"));parser.add_argument("--json",action="store_true");args=parser.parse_args(argv)
    try:report=probe(load_profile(args.profile),args.protocol,args.role,args.dry_run,os.environ,capabilities=args.capability or ["basic_text"],model_override=os.environ.get(args.model_env) if args.model_env else None,cache_dir=args.cache_dir)
    except ProfileError as error:print(f"configuration error: {error}",file=sys.stderr);return EXIT_CONFIGURATION
    if args.json:print(json.dumps(report,indent=2,sort_keys=True))
    else:
        print(f"profile: {report['profile_id']}"+(" (dry run)" if report["dry_run"] else ""))
        for result in report["protocols"]:print(f"{result['protocol']}: {result['status']}")
    return exit_code(report)
if __name__=="__main__":sys.exit(main())
