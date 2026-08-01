#!/usr/bin/env python3
"""Safely probe explicitly selected provider protocol surfaces."""
from __future__ import annotations
import argparse,json,os,socket,sys,urllib.error,urllib.request
from dataclasses import dataclass
from pathlib import Path
from typing import Callable,Mapping
from provider import PROTOCOLS,ProfileError,auth_headers,load_profile,redact,resolve_runtime
EXIT_OK,EXIT_CAPABILITY_FAILURE,EXIT_CONFIGURATION,EXIT_TRANSPORT=0,2,3,4
PATHS={"openai-responses":"/v1/responses","anthropic-messages":"/v1/messages","openai-chat-completions":"/v1/chat/completions"}
@dataclass
class TransportResponse:status:int;headers:Mapping[str,str];body:bytes
def urllib_transport(url,headers,body,timeout):
    request=urllib.request.Request(url,data=body,headers=dict(headers),method="POST")
    try:
        with urllib.request.urlopen(request,timeout=timeout) as response:return TransportResponse(response.status,dict(response.headers),response.read())
    except urllib.error.HTTPError as error:return TransportResponse(error.code,dict(error.headers),error.read())
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
def request(transport,url,headers,body,timeout):
    try:return transport(url,headers,body,timeout),None
    except (TimeoutError,socket.timeout):return None,"timeout"
    except (OSError,urllib.error.URLError):return None,"transport-failure"
def probe(profile,protocols,role,dry_run,env,transport:Callable[...,TransportResponse]=urllib_transport):
    if len(protocols)!=len(set(protocols)):raise ProfileError("duplicate requested protocol")
    if set(protocols)-set(profile["protocols"]):raise ProfileError("requested protocol is not declared by this profile")
    if dry_run:return {"profile_id":profile["profile_id"],"dry_run":True,"protocols":[{"protocol":p,"status":"not-run"} for p in protocols]}
    endpoint,token,model=resolve_runtime(profile,role,env);results=[]
    for protocol in protocols:
        if profile["protocols"][protocol]=="unsupported":
            results.append({"protocol":protocol,"status":"unsupported-protocol","authentication":"unknown","capabilities":{"streaming":"unsupported","tool_calling":"unsupported","structured_output":"unsupported"},"timeout_cancellation":"not-run","usage_metadata":"not-reported"});continue
        headers={"Content-Type":"application/json",**auth_headers(profile,protocol,token)}
        if protocol=="anthropic-messages":headers["anthropic-version"]="2023-06-01"
        response,failure=request(transport,endpoint+PATHS[protocol],headers,json.dumps(payload(protocol,model,"compatibility")).encode(),float(profile["timeout"]["seconds"]))
        if failure:status,value=failure,None
        else:status,value=base_status(protocol,response)
        capabilities={"streaming":"unknown","tool_calling":"unknown","structured_output":"unknown"};timing={"timeout":"observed" if status=="timeout" else "not-observed","cancellation":"not-probed"};probe_failure=None
        if status=="success":
            for operation,key in (("streaming","streaming"),("tool_calling","tool_calling"),("structured_output","structured_output")):
                if protocol=="anthropic-messages" and operation=="structured_output":continue
                cap_response,cap_failure=request(transport,endpoint+PATHS[protocol],headers,json.dumps(payload(protocol,model,operation)).encode(),float(profile["timeout"]["seconds"]))
                if cap_failure:capabilities[key]="probe-failed";timing["timeout"]="observed" if cap_failure=="timeout" else timing["timeout"];probe_failure=cap_failure
                else:
                    if operation=="streaming" and 200<=cap_response.status<300:
                        capabilities[key]=observed(protocol,operation,cap_response,None)
                    else:
                        cap_status,cap_value=base_status(protocol,cap_response)
                        capabilities[key]=observed(protocol,operation,cap_response,cap_value) if cap_status=="success" else ("unsupported" if cap_status=="protocol-error" else "probe-failed")
        results.append({"protocol":protocol,"status":status,"authentication":"accepted" if status=="success" else ("rejected" if status=="authentication-failure" else "unknown"),"capabilities":capabilities,"probe_failure":probe_failure,"timeout_cancellation":timing,"usage_metadata":"reported" if value and isinstance(value.get("usage"),dict) else "not-reported"})
    return redact({"profile_id":profile["profile_id"],"dry_run":False,"protocols":results},[endpoint,token or "",model])
def exit_code(report):
    statuses={x["status"] for x in report["protocols"]}
    failures={x.get("probe_failure") for x in report["protocols"]}
    if statuses&{"transport-failure","timeout"} or failures&{"transport-failure","timeout"}:return EXIT_TRANSPORT
    if statuses<={"success","not-run"} and failures<={None}:return EXIT_OK
    return EXIT_CAPABILITY_FAILURE
def main(argv=None):
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument("profile",type=Path);parser.add_argument("--protocol",action="append",choices=sorted(PROTOCOLS),required=True);parser.add_argument("--role",default="implementation");parser.add_argument("--dry-run",action="store_true");parser.add_argument("--json",action="store_true");args=parser.parse_args(argv)
    try:report=probe(load_profile(args.profile),args.protocol,args.role,args.dry_run,os.environ)
    except ProfileError as error:print(f"configuration error: {error}",file=sys.stderr);return EXIT_CONFIGURATION
    if args.json:print(json.dumps(report,indent=2,sort_keys=True))
    else:
        print(f"profile: {report['profile_id']}"+(" (dry run)" if report["dry_run"] else ""))
        for result in report["protocols"]:print(f"{result['protocol']}: {result['status']}")
    return exit_code(report)
if __name__=="__main__":sys.exit(main())
