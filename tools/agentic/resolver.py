#!/usr/bin/env python3
"""Versioned deterministic resolver operating directly on normalized catalog models."""
from __future__ import annotations
import math
from collections.abc import Mapping
from typing import Any
RESOLVER_VERSION="1.0"
OBJECTIVES={"lowest-cost-valid","fastest-valid","maximum-quality","balanced-quality-cost","maximum-agentic-reliability","independent-review","research-pinned"}
UNIT_FIELDS={"client","provider","model","protocol","effort"};PIN_FIELDS={"catalog_snapshot","client_version","task_version","context_hash","base_revision","budgets"}
class ResolutionError(ValueError):
    def __init__(self,message:str,rejected:list[dict[str,Any]]):super().__init__(message);self.rejected=rejected
def _number(metadata,key):
    value=metadata.get(key)
    return float(value) if isinstance(value,(int,float)) and not isinstance(value,bool) and math.isfinite(value) else None
def _required_metrics(objective):
    return {"cost"} if objective=="lowest-cost-valid" else {"latency"} if objective=="fastest-valid" else {"quality"} if objective=="maximum-quality" else {"agentic_reliability"} if objective in {"maximum-agentic-reliability","independent-review"} else {"cost","latency","quality"} if objective=="balanced-quality-cost" else set()
def _score(metadata,objective):
    values={key:(_number(metadata,key) or 0) for key in ("cost","latency","quality","agentic_reliability")};cost,latency,quality,reliability=(values[key] for key in ("cost","latency","quality","agentic_reliability"))
    if objective=="lowest-cost-valid":return (cost,latency,-quality)
    if objective=="fastest-valid":return (latency,cost,-quality)
    if objective=="maximum-quality":return (-quality,cost,latency)
    if objective in {"maximum-agentic-reliability","independent-review"}:return (-reliability,-quality,cost)
    if objective=="research-pinned":return (0,)
    return (cost+latency-quality-reliability,cost,latency)
def _valid_unit(value):
    return isinstance(value,Mapping) and set(value)==UNIT_FIELDS and value.get("client") in {"codex","claude-code","cline"} and value.get("protocol") in {"openai-responses","anthropic-messages","openai-chat-completions"} and all(isinstance(value.get(key),str) and value[key] for key in UNIT_FIELDS)
def _valid_pins(value):
    return isinstance(value,Mapping) and set(value)==PIN_FIELDS and all(isinstance(value[key],str) and value[key] for key in PIN_FIELDS-{"budgets"}) and isinstance(value["budgets"],Mapping)
def _rejected_identity(model_id,unit):
    return {"client":unit.get("client") if isinstance(unit,Mapping) and isinstance(unit.get("client"),str) else None,"provider":unit.get("provider") if isinstance(unit,Mapping) and isinstance(unit.get("provider"),str) else None,"model":unit.get("model") if isinstance(unit,Mapping) and isinstance(unit.get("model"),str) else model_id,"protocol":unit.get("protocol") if isinstance(unit,Mapping) and isinstance(unit.get("protocol"),str) else None,"effort":unit.get("effort") if isinstance(unit,Mapping) and isinstance(unit.get("effort"),str) else None}
def resolve(models:list[Mapping[str,Any]],role:str,objective:str,mandatory:set[str],mode:str="development",override:Mapping[str,Any]|None=None,pins:Mapping[str,Any]|None=None,ablation_dimensions:list[str]|None=None)->dict[str,Any]:
    if not isinstance(role,str) or not role:raise ValueError("role must be non-empty")
    if objective not in OBJECTIVES:raise ValueError("unknown objective")
    if mode not in {"development","evaluation","ablation"}:raise ValueError("unknown mode")
    if pins is not None and not _valid_pins(pins):raise ValueError("pins do not match the execution-plan schema")
    if mode=="evaluation" and not _valid_pins(pins):raise ValueError("evaluation mode requires schema-valid complete pins")
    if mode=="ablation" and (not isinstance(ablation_dimensions,list) or not ablation_dimensions or not all(isinstance(item,str) and item for item in ablation_dimensions) or len(ablation_dimensions)!=len(set(ablation_dimensions))):raise ValueError("ablation mode requires unique non-empty declared dimensions")
    if mode!="ablation" and ablation_dimensions:raise ValueError("ablation dimensions require ablation mode")
    if override is not None and not _valid_unit(override):raise ValueError("explicit override must identify a complete execution unit")
    if objective=="research-pinned" and override is None:raise ValueError("research-pinned requires an explicit override")
    valid=[];rejected=[]
    for model in models:
        model_id=model.get("model_id");raw_reasons=model.get("rejection_reasons",[]);base_reasons=list(raw_reasons) if isinstance(raw_reasons,list) and all(isinstance(item,str) and item for item in raw_reasons) else ["invalid rejection reasons"]
        if not isinstance(model_id,str) or not model_id:base_reasons.append("invalid model identifier");model_id="unknown"
        if not model.get("eligible",False):base_reasons.append("local policy marks candidate ineligible")
        capabilities=model.get("capabilities",{})
        for capability in sorted(mandatory):
            evidence=capabilities.get(capability,{}) if isinstance(capabilities,Mapping) else {}
            if not isinstance(evidence,Mapping) or evidence.get("status")!="supported":base_reasons.append(f"mandatory capability unavailable: {capability}")
        metadata=model.get("metadata",{})
        if not isinstance(metadata,Mapping):base_reasons.append("invalid objective metadata");metadata={}
        for metric in sorted(_required_metrics(objective)):
            if _number(metadata,metric) is None:base_reasons.append(f"objective metadata unavailable: {metric}")
        units=model.get("execution_units");units=units if isinstance(units,list) and units else [None]
        for unit in units:
            reasons=list(base_reasons)
            if not _valid_unit(unit) or unit.get("model")!=model_id:reasons.append("model has no schema-valid compatible execution unit")
            if override is not None and (not _valid_unit(unit) or dict(unit)!=dict(override)):reasons.append("explicit override selects another execution unit")
            if reasons:rejected.append({"candidate":_rejected_identity(model_id,unit),"reasons":reasons})
            else:valid.append((metadata,dict(unit)))
    if not valid:
        details="; ".join(f"{item['candidate']['model']}: {', '.join(item['reasons'])}" for item in rejected);raise ResolutionError(f"no valid execution plan; {details}",rejected)
    ranked=sorted(valid,key=lambda pair:(_score(pair[0],objective),pair[1]["model"],pair[1]["client"],pair[1]["protocol"],pair[1]["effort"]))
    return {"schema":"evolvehls.agentic.execution-plan","schema_version":"1.0","resolver_version":RESOLVER_VERSION,"mode":mode,"objective":objective,"role":role,"selected":ranked[0][1],"fallback_chain":[] if mode!="development" else [unit for _,unit in ranked[1:]],"fallback_policy":"explicit-approval" if mode=="development" else "disabled","explanation":[f"selected deterministically from {len(valid)} valid execution unit(s)",f"objective: {objective}"],"rejected":rejected,"pins":dict(pins) if pins is not None else {"catalog_snapshot":None,"client_version":None,"task_version":None,"context_hash":None,"base_revision":None,"budgets":None},"ablation_dimensions":list(ablation_dimensions or [])}
