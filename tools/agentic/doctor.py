#!/usr/bin/env python3
"""Report portable agent client and project configuration capability safely."""

from __future__ import annotations
import argparse,json,re,shutil,subprocess,sys
from pathlib import Path

CLIENTS={"codex":("codex",[".codex/config.toml"]),"claude-code":("claude",[".claude/settings.json","CLAUDE.md"]),"cline":("cline",[".cline",".clinerules"])}

def version(exe,run=subprocess.run):
    try: result=run([exe,"--version"],capture_output=True,text=True,timeout=5,check=False,shell=False)
    except (OSError,subprocess.SubprocessError): return None,"version command failed"
    line=(result.stdout or result.stderr).strip().splitlines()
    if result.returncode or not line: return None,"version unavailable"
    found=re.search(r"\d+(?:\.\d+)+(?:[-+][A-Za-z0-9.-]+)?",line[0])
    return (found.group(0) if found else "present (unparsed)"),None

def config_status(root,relative):
    path=root/relative
    if not path.exists(): return {"path":relative,"status":"absent"}
    if path.is_dir(): return {"path":relative,"status":"present"}
    try:
        text=path.read_text(encoding="utf-8")
        if path.suffix==".json": json.loads(text)
        elif path.suffix==".toml":
            import tomllib; tomllib.loads(text)
    except (OSError,ValueError): return {"path":relative,"status":"parse-error"}
    return {"path":relative,"status":"valid"}

def diagnose(root,which=shutil.which,run=subprocess.run):
    clients=[]
    for name,(command,configs) in CLIENTS.items():
        exe=which(command); parsed,error=version(exe,run) if exe else (None,None)
        item={"client":name,"available":exe is not None,"version":parsed,"configuration":[config_status(root,x) for x in configs]}
        if error:item["diagnostic"]=error
        clients.append(item)
    return {"schema":"evolvehls.agentic.doctor","schema_version":"1.0","clients":clients}

def main(argv=None):
    parser=argparse.ArgumentParser(description=__doc__); parser.add_argument("--root",type=Path,default=Path.cwd()); parser.add_argument("--json",action="store_true"); args=parser.parse_args(argv)
    report=diagnose(args.root.resolve())
    if args.json: print(json.dumps(report,indent=2,sort_keys=True))
    else:
        for client in report["clients"]:
            print(f"{client['client']}: {client['version'] or ('available' if client['available'] else 'unavailable')}")
            for config in client["configuration"]: print(f"  {config['path']}: {config['status']}")
            if client.get("diagnostic"): print(f"  diagnostic: {client['diagnostic']}")
    return 0
if __name__=="__main__":sys.exit(main())
