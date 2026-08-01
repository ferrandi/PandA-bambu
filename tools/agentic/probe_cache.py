#!/usr/bin/env python3
"""Fail-closed local TTL cache for redacted capability observations."""
from __future__ import annotations
import hashlib,json,tempfile
from datetime import datetime,timedelta,timezone
from pathlib import Path
from typing import Any
PROTOCOLS={"openai-responses","anthropic-messages","openai-chat-completions","embeddings"}
CAPABILITIES={"basic_text","streaming","tool_calling","structured_output","context_limit","usage_reporting","embeddings"}
OBSERVATIONS={"supported","unsupported"}
def _redacted_model_ref(model_ref):return "sha256:"+hashlib.sha256(model_ref.encode()).hexdigest()
def _key(profile_id,model_ref,protocol,capability):
    return hashlib.sha256("\0".join((profile_id,model_ref,protocol,capability)).encode()).hexdigest()
def _aware(value):
    parsed=datetime.fromisoformat(value)
    if parsed.tzinfo is None or parsed.utcoffset() is None:raise ValueError("timestamp must be timezone-aware")
    return parsed
def store_observation(directory:Path,profile_id:str,model_ref:str,protocol:str,capability:str,status:str,ttl_seconds:int,now:datetime|None=None)->Path:
    if not all(isinstance(item,str) and item for item in (profile_id,model_ref)):raise ValueError("invalid cache identity")
    if protocol not in PROTOCOLS or capability not in CAPABILITIES or status not in OBSERVATIONS:raise ValueError("invalid observation")
    if not isinstance(ttl_seconds,int) or isinstance(ttl_seconds,bool) or ttl_seconds<1:raise ValueError("invalid TTL")
    instant=now or datetime.now(timezone.utc)
    if instant.tzinfo is None or instant.utcoffset() is None:raise ValueError("now must be timezone-aware")
    record={"schema":"evolvehls.agentic.probe-record","schema_version":"1.0","profile_id":profile_id,"model_ref":_redacted_model_ref(model_ref),"protocol":protocol,"capability":capability,"status":status,"confidence":"observed","observed_at":instant.isoformat(),"expires_at":(instant+timedelta(seconds=ttl_seconds)).isoformat(),"redacted":True}
    directory.mkdir(parents=True,exist_ok=True);path=directory/(_key(profile_id,model_ref,protocol,capability)+".json")
    with tempfile.NamedTemporaryFile("w",encoding="utf-8",dir=directory,delete=False) as output:json.dump(record,output,indent=2,sort_keys=True);output.write("\n");temporary=Path(output.name)
    temporary.replace(path);return path
def store_success(directory:Path,profile_id:str,model_ref:str,protocol:str,capability:str,ttl_seconds:int,now:datetime|None=None)->Path:
    return store_observation(directory,profile_id,model_ref,protocol,capability,"supported",ttl_seconds,now)
def load_fresh(directory:Path,profile_id:str,model_ref:str,protocol:str,capability:str,now:datetime|None=None,max_ttl_seconds:int=86400)->dict[str,Any]|None:
    if protocol not in PROTOCOLS or capability not in CAPABILITIES:return None
    path=directory/(_key(profile_id,model_ref,protocol,capability)+".json")
    try:
        record=json.loads(path.read_text(encoding="utf-8"));observed=_aware(record["observed_at"]);expires=_aware(record["expires_at"]);instant=now or datetime.now(timezone.utc)
        expected={"schema":"evolvehls.agentic.probe-record","schema_version":"1.0","profile_id":profile_id,"model_ref":_redacted_model_ref(model_ref),"protocol":protocol,"capability":capability,"confidence":"observed","redacted":True}
        valid=isinstance(record,dict) and set(record)==set(expected)|{"status","observed_at","expires_at"} and all(record.get(key)==value for key,value in expected.items()) and record.get("status") in OBSERVATIONS and instant.tzinfo is not None and observed<=instant<expires and timedelta(0)<expires-observed<=timedelta(seconds=max_ttl_seconds)
    except (OSError,UnicodeDecodeError,json.JSONDecodeError,KeyError,TypeError,ValueError,OverflowError):return None
    return record if valid else None
