#!/usr/bin/env python3
"""Provider profile validation, runtime resolution, and redaction."""
from __future__ import annotations
import json,os,re,subprocess
from pathlib import Path
from typing import Any,Mapping
PROTOCOLS={"openai-responses","anthropic-messages","openai-chat-completions"};STATUSES={"supported","unsupported","unknown","probe-failed"};ENV=re.compile(r"^[A-Z][A-Z0-9_]*$");SENSITIVE={"authorization","x-api-key","token","api_key","key","secret","endpoint","url","model"}
class ProfileError(ValueError):pass
def require(ok,message):
    if not ok:raise ProfileError(message)
def is_number(v):return isinstance(v,(int,float)) and not isinstance(v,bool)
def load_profile(path:Path):
    try:value=json.loads(path.read_text(encoding="utf-8"))
    except (OSError,UnicodeDecodeError,json.JSONDecodeError) as error:raise ProfileError(f"cannot parse profile: {type(error).__name__}") from None
    validate_profile(value);return value
def validate_profile(p:Any):
    required={"profile_id","schema_version","protocols","authentication","endpoint","models","capabilities","context","usage_reporting","retry","timeout"}
    require(isinstance(p,dict) and set(p)==required,"profile fields do not match schema")
    require(isinstance(p["profile_id"],str) and re.fullmatch(r"[a-z0-9][a-z0-9.-]*",p["profile_id"]) is not None,"invalid profile_id");require(p["schema_version"]=="1.0","unsupported schema version")
    protocols=p["protocols"];require(isinstance(protocols,dict) and protocols and set(protocols)<=PROTOCOLS and all(isinstance(v,str) and v in STATUSES for v in protocols.values()),"invalid protocols")
    auth=p["authentication"];require(isinstance(auth,dict) and auth.get("class") in {"environment","command","none"},"invalid authentication");kind=auth["class"]
    expected={"environment":{"class","token_env","headers"},"command":{"class","token_helper","headers"},"none":{"class"}}[kind];require(set(auth)==expected,"authentication fields do not match class")
    if kind=="environment":require(isinstance(auth["token_env"],str) and ENV.fullmatch(auth["token_env"]) is not None,"invalid token environment")
    if kind=="command":require(isinstance(auth["token_helper"],list) and auth["token_helper"] and all(isinstance(x,str) and x for x in auth["token_helper"]),"invalid token helper")
    if kind!="none":
        headers=auth["headers"];require(isinstance(headers,dict) and headers and set(headers)<=PROTOCOLS,"invalid authentication headers")
        for spec in headers.values():require(isinstance(spec,dict) and set(spec)=={"name","prefix"} and isinstance(spec["name"],str) and re.fullmatch(r"[A-Za-z0-9-]+",spec["name"]) and isinstance(spec["prefix"],str),"invalid authentication header")
    endpoint=p["endpoint"];require(isinstance(endpoint,dict) and set(endpoint)=={"env"} and isinstance(endpoint["env"],str) and ENV.fullmatch(endpoint["env"]) is not None,"invalid endpoint source")
    models=p["models"];require(isinstance(models,dict) and models,"models must be non-empty")
    for alias,spec in models.items():
        require(isinstance(alias,str) and re.fullmatch(r"[a-z][a-z0-9-]*",alias) is not None,"invalid model alias");require(isinstance(spec,dict) and set(spec)=={"source","confidential"} and isinstance(spec["confidential"],bool),"invalid model declaration")
        source=spec["source"];require(isinstance(source,dict) and set(source)=={"env"} and isinstance(source["env"],str) and ENV.fullmatch(source["env"]) is not None,"invalid model source")
    caps=p["capabilities"];require(isinstance(caps,dict) and set(caps)=={"streaming","tool_calling","structured_output"} and all(isinstance(v,str) and v in STATUSES for v in caps.values()),"invalid capabilities")
    ctx=p["context"];modalities=ctx.get("input_modalities") if isinstance(ctx,dict) else None;window=ctx.get("window_tokens") if isinstance(ctx,dict) else None
    require(isinstance(ctx,dict) and set(ctx)=={"window_tokens","input_modalities"} and (window is None or isinstance(window,int) and not isinstance(window,bool) and window>0) and isinstance(modalities,list) and all(isinstance(x,str) and x for x in modalities) and len(modalities)==len(set(modalities)),"invalid context")
    require(isinstance(p["usage_reporting"],str) and p["usage_reporting"] in STATUSES,"invalid usage reporting")
    retry=p["retry"];require(isinstance(retry,dict) and set(retry)=={"max_attempts","backoff_seconds"} and isinstance(retry["max_attempts"],int) and not isinstance(retry["max_attempts"],bool) and 1<=retry["max_attempts"]<=5 and is_number(retry["backoff_seconds"]) and retry["backoff_seconds"]>=0,"invalid retry")
    timeout=p["timeout"];require(isinstance(timeout,dict) and set(timeout)=={"seconds"} and is_number(timeout["seconds"]) and 0<timeout["seconds"]<=300,"invalid timeout")
def resolve_runtime(p:Mapping[str,Any],role:str,env:Mapping[str,str]|None=None,run=subprocess.run):
    values=os.environ if env is None else env;endpoint=values.get(p["endpoint"]["env"]);require(bool(endpoint),f"required endpoint variable {p['endpoint']['env']} is unset");require(role in p["models"],f"unknown model role alias: {role}")
    model_env=p["models"][role]["source"]["env"];model=values.get(model_env);require(bool(model),f"required model variable {model_env} is unset");auth=p["authentication"];token=None
    if auth["class"]=="environment":token=values.get(auth["token_env"]);require(bool(token),f"required token variable {auth['token_env']} is unset")
    elif auth["class"]=="command":
        try:result=run(auth["token_helper"],capture_output=True,text=True,timeout=10,check=True,shell=False)
        except (OSError,subprocess.SubprocessError):raise ProfileError("token helper failed") from None
        token=result.stdout.strip();require(bool(token),"token helper returned no token")
    return endpoint.rstrip("/"),token,model
def auth_headers(p,protocol,token):
    if not token:return {}
    require(protocol in p["authentication"]["headers"],f"no authentication header declared for {protocol}");spec=p["authentication"]["headers"][protocol];return {spec["name"]:spec["prefix"]+token}
def redact(value:Any,secrets=()):
    secrets=tuple(x for x in secrets if x)
    if isinstance(value,dict):return {k:("[REDACTED]" if k.lower() in SENSITIVE else redact(v,secrets)) for k,v in value.items()}
    if isinstance(value,list):return [redact(v,secrets) for v in value]
    if isinstance(value,str):
        for secret in secrets:value=value.replace(secret,"[REDACTED]")
    return value
