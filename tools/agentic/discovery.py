#!/usr/bin/env python3
"""Generic, injected model discovery interfaces for provider-core."""
from __future__ import annotations
import json,re,urllib.error,urllib.request
from dataclasses import dataclass
from typing import Any,Callable,Mapping
from provider import ProfileError
MAX_RESPONSE_BYTES=1048576

@dataclass(frozen=True)
class DiscoveryResponse:
    status:int
    headers:Mapping[str,str]
    body:bytes

@dataclass(frozen=True)
class DiscoveryResult:
    adapter:str
    models:tuple[dict[str,Any],...]
    attempted:tuple[str,...]
    requires_model_id:bool
    diagnostics:tuple[str,...]

class _NoRedirect(urllib.request.HTTPRedirectHandler):
    def redirect_request(self,request,fp,code,message,headers,new_url):return None
def urllib_get(url:str,headers:Mapping[str,str],timeout:float)->DiscoveryResponse:
    request=urllib.request.Request(url,headers=dict(headers),method="GET")
    try:
        with urllib.request.build_opener(_NoRedirect).open(request,timeout=timeout) as response:return DiscoveryResponse(response.status,dict(response.headers),response.read(MAX_RESPONSE_BYTES+1))
    except urllib.error.HTTPError as error:return DiscoveryResponse(error.code,dict(error.headers),error.read(MAX_RESPONSE_BYTES+1))

def _items(document:Any)->list[dict[str,Any]]|None:
    if isinstance(document,list):values=document
    elif isinstance(document,dict) and isinstance(document.get("data"),list):values=document["data"]
    elif isinstance(document,dict) and isinstance(document.get("models"),list):values=document["models"]
    else:return None
    return [item for item in values if isinstance(item,dict) and isinstance(item.get("id"),str) and item["id"]]

def discover(endpoint:str,headers:Mapping[str,str],methods:list[Mapping[str,Any]],timeout:float,transport:Callable[...,DiscoveryResponse]=urllib_get,imported:Mapping[str,Any]|None=None)->DiscoveryResult:
    attempted=[];diagnostics=[]
    for method in methods:
        kind=method.get("kind")
        if kind=="imported-catalog":
            attempted.append(kind);items=_items(imported)
        elif kind in {"openai-models","model-info"}:
            path=method.get("path");attempted.append(kind)
            if not isinstance(path,str) or re.fullmatch(r"/[^/].*",path) is None or "://" in path:raise ProfileError("invalid discovery path")
            try:response=transport(endpoint.rstrip("/")+path,headers,timeout)
            except (OSError,urllib.error.URLError,TimeoutError):diagnostics.append(f"{kind}: transport-failure");continue
            if response.status in {401,403}:diagnostics.append(f"{kind}: authentication-failure");continue
            if not 200<=response.status<300:diagnostics.append(f"{kind}: unavailable");continue
            try:items=None if len(response.body)>MAX_RESPONSE_BYTES else _items(json.loads(response.body.decode("utf-8")))
            except (UnicodeDecodeError,json.JSONDecodeError):items=None
        else:raise ProfileError("unknown discovery method")
        if items is not None:
            normalized=tuple({"model_id":item["id"],"display_name":item.get("name"),"metadata":{key:value for key,value in item.items() if key not in {"id","name"}},"capabilities":{},"eligible":False,"rejection_reasons":["awaiting local policy and capability evaluation"],"execution_units":[]} for item in items)
            return DiscoveryResult(kind,normalized,tuple(attempted),False,tuple(diagnostics))
        diagnostics.append(f"{kind}: malformed-response")
    return DiscoveryResult("manual-model-id",(),tuple(attempted),True,tuple(diagnostics))
