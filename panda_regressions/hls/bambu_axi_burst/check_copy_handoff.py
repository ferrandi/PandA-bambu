#!/usr/bin/env python3
"""Fail-closed compiler/RTL witness for the distinct-buffer copy handoff."""
from __future__ import annotations

import json
import re
import sys
import xml.etree.ElementTree as ET
from pathlib import Path

OPS = {
    "CONFIGURE_READ": "input_bambu_artificial_ParmMgr_configure_read",
    "BURST_READ": "input_bambu_artificial_ParmMgr_burst_read",
    "CONFIGURE_WRITE": "output_bambu_artificial_ParmMgr_configure_write",
    "STORE_WRITE": "output_bambu_artificial_ParmMgr_store_write",
    "DRAIN_WRITE": "output_bambu_artificial_ParmMgr_drain_write",
}


def need(condition: bool, message: str) -> None:
    if not condition:
        raise ValueError(message)


def artifacts(root: Path) -> dict[str, Path]:
    pt = root / "panda-temp"
    required = {
        "before_interface": pt / "before_Frontend::InterfaceInfer.raw",
        "after_interface": pt / "after_Frontend::InterfaceInfer.raw",
        "after_virtual_phi": None,
        "dfg": root / "HLS_output/dot/copy_distinct_buffers/OP_DFG.dot",
        "adg": root / "HLS_output/dot/copy_distinct_buffers/OP_ADG.dot",
        "odg": root / "HLS_output/dot/copy_distinct_buffers/OP_ODG.dot",
        "feedback": root / "HLS_output/dot/copy_distinct_buffers/OP_FFLSAODG.dot",
        "schedule_dot": root / "HLS_output/dot/copy_distinct_buffers/HLS_scheduling.dot",
        "rtl": root / "copy_distinct_buffers.v",
    }
    raw = sorted(pt.glob("after_Frontend::BuildVirtualPhi::copy_distinct_buffers*.raw"))
    need(len(raw) == 1, f"expected exactly one post-BuildVirtualPhi raw IR; found {len(raw)}")
    required["after_virtual_phi"] = raw[0]
    xmls = sorted(root.glob("*copy_distinct_buffers*scheduling.xml"))
    need(len(xmls) == 1, f"expected one scheduling XML; found {len(xmls)}")
    required["schedule_xml"] = xmls[0]
    for label, path in required.items():
        need(path is not None and path.is_file() and path.stat().st_size > 0,
             f"required artifact missing/empty: {label} ({path})")
    return required


def records(text: str) -> dict[str, tuple[str, str]]:
    out = {}
    for line in text.splitlines():
        m = re.match(r"^(@\d+)\s+(\w+)\s*(.*)$", line)
        if m:
            need(m.group(1) not in out, f"duplicate raw IR node {m.group(1)}")
            out[m.group(1)] = (m.group(2), m.group(3))
    need(bool(out), "raw IR has no parseable nodes")
    return out


def refs(attrs: str, field: str) -> list[str]:
    return re.findall(rf"\b{re.escape(field)}:\s*(@\d+)", attrs)


def call_names(nodes: dict[str, tuple[str, str]]) -> dict[str, str]:
    ids = {node: re.search(r'\bstrg:\s*"([^"]+)"', attrs).group(1)
           for node, (kind, attrs) in nodes.items() if kind == "identifier_node"
           and re.search(r'\bstrg:\s*"([^"]+)"', attrs)}
    fns = {node: ids.get(refs(attrs, "name")[0], "") for node, (kind, attrs) in nodes.items()
           if kind == "function_val_node" and refs(attrs, "name")}
    names = {}
    for node, (kind, attrs) in nodes.items():
        if not kind.endswith("stmt"):
            continue
        fn_refs = refs(attrs, "fn")
        for op in refs(attrs, "op"):
            if nodes.get(op, ("", ""))[0] == "call_node":
                fn_refs += refs(nodes[op][1], "fn")
        callee = []
        for fr in fn_refs:
            fr_kind, fr_attrs = nodes.get(fr, ("", ""))
            targets = refs(fr_attrs, "op") if fr_kind == "addr_node" else [fr]
            callee.extend(fns.get(t, "") for t in targets)
        names[node] = next((n for n in callee if n), "")
    return names


def parse_blocks(attrs: str) -> dict[int, dict[str, object]]:
    starts = list(re.finditer(r"\bbloc:\s*(\d+)\s+loop_id:\s*(\d+)", attrs))
    need(bool(starts), "copy CFG has no basic blocks")
    result = {}
    for i, m in enumerate(starts):
        end = starts[i + 1].start() if i + 1 < len(starts) else len(attrs)
        bid, loop = map(int, m.groups())
        section = attrs[m.end():end]
        result[bid] = {"loop_id": loop, "entry": "pred: ENTRY" in section,
                       "succ": [int(x) for x in re.findall(r"\bsucc:\s*(\d+)", section)]}
    return result


def extract_ir(path: Path, require_calls: bool = True) -> dict[str, object]:
    nodes = records(path.read_text())
    names = call_names(nodes)
    fns = [n for n, (kind, attrs) in nodes.items() if kind == "function_val_node"
           and any("copy_distinct_buffers" == re.search(r'\bstrg:\s*"([^"]+)"', nodes.get(i, ("", ""))[1]).group(1)
                   for i in refs(attrs, "name") if re.search(r'\bstrg:\s*"([^"]+)"', nodes.get(i, ("", ""))[1]))]
    need(len(fns) == 1, f"expected one copy_distinct_buffers function in {path.name}; found {len(fns)}")
    fn = fns[0]
    body_refs = refs(nodes[fn][1], "body")
    need(len(body_refs) == 1 and nodes.get(body_refs[0], ("", ""))[0] == "statement_list_node",
         "copy function missing statement-list CFG")
    stmts = {n: (k, a) for n, (k, a) in nodes.items()
             if k.endswith("stmt") and refs(a, "parent") == [fn]}
    ops = {}
    for label, callee in OPS.items() if require_calls else ():
        found = [n for n in stmts if names.get(n) == callee]
        need(len(found) == 1, f"{path.name}: expected one {label}; found {len(found)}")
        ops[label] = found[0]
    bb = parse_blocks(nodes[body_refs[0]][1])
    for label, node in list(ops.items()):
        m = re.search(r"\bbb_index:\s*(\d+)", stmts[node][1])
        need(m, f"{label} missing BB identity")
        ops[label] = {"node": node, "bb": int(m.group(1)), "attrs": stmts[node][1],
                      "args": refs(stmts[node][1], "arg"),
                      "vuse": refs(stmts[node][1], "vuse"), "vdef": refs(stmts[node][1], "vdef")}
    return {"nodes": nodes, "stmts": stmts, "ops": ops, "blocks": bb}


def relevant_call_counts(ir: dict[str, object]) -> dict[str, int]:
    nodes = ir["nodes"]
    names = call_names(nodes)
    stmts = ir["stmts"]
    return {label: sum(names.get(node) == callee for node in stmts)
            for label, callee in OPS.items()}


def scheduling(path: Path) -> dict[str, dict[str, str]]:
    root = ET.parse(path).getroot()
    rows = {}
    for elem in root.iter("scheduling_constraints"):
        name = elem.attrib.get("vertex_name", "")
        if name:
            rows[name.rsplit("_", 1)[-1]] = dict(elem.attrib)
    return rows


def dot_edges(path: Path) -> list[dict[str, str]]:
    edges = []
    for line in path.read_text().splitlines():
        m = re.match(r"\s*(\d+)\s*->\s*(\d+)(?:\s*\[(.*)\])?\s*;", line)
        if m:
            edges.append({"from": m.group(1), "to": m.group(2), "attributes": m.group(3) or ""})
    return edges


def dot_graph(path: Path) -> dict[str, object]:
    text = path.read_text()
    need(re.search(r"\bdigraph\s+\w*\s*\{", text), f"invalid DOT graph: {path.name}")
    declarations = re.findall(r"^\s*([\w]+)\s*\[([^\n]*)", text, re.M)
    nodes = {node for node, _ in declarations}
    labels = {node: label_match.group(1) for node, attrs in declarations
              if (label_match := re.search(r'\blabel="([^"]*)"', attrs))}
    edges = [{"from": source, "to": target}
             for source, target in re.findall(r"^\s*([\w]+)\s*->\s*([\w]+)", text, re.M)]
    need(nodes and edges, f"DOT graph has no nodes or edges: {path.name}")
    need(all(edge["from"] in nodes and edge["to"] in nodes for edge in edges),
         f"DOT graph has edges with missing nodes: {path.name}")
    return {"nodes": nodes, "edges": edges, "labels": labels}


def cfg_path(blocks: dict[int, dict[str, object]], source: int, target: int) -> list[int]:
    need(source in blocks and target in blocks, "operation BB is absent from copy CFG")
    for bid, block in blocks.items():
        need(all(succ in blocks for succ in block["succ"]), f"CFG block {bid} references an absent successor")
    pending = [(source, [source])]
    visited = set()
    while pending:
        bid, path = pending.pop(0)
        if bid == target:
            return path
        if bid in visited:
            continue
        visited.add(bid)
        pending.extend((succ, path + [succ]) for succ in blocks[bid]["succ"] if succ not in visited)
    return []


def verilog_module(text: str, name: str) -> str:
    m = re.search(rf"\bmodule\s+{re.escape(name)}\b(.*?)\bendmodule", text, re.S)
    need(m, f"RTL missing module {name}")
    return m.group(1)


def balanced(text: str, start: int) -> str:
    opening = text.find("(", start)
    need(opening >= 0, "malformed Verilog instance")
    depth = 0
    for i in range(opening, len(text)):
        if text[i] == "(": depth += 1
        elif text[i] == ")":
            depth -= 1
            if depth == 0: return text[opening + 1:i]
    raise ValueError("unterminated Verilog instance")


def balanced_end(text: str, opening: int) -> int:
    depth = 0
    for i in range(opening, len(text)):
        if text[i] == "(": depth += 1
        elif text[i] == ")":
            depth -= 1
            if depth == 0: return i
    raise ValueError("unterminated Verilog instance")


def check_wrapper(path: Path) -> dict[str, object]:
    text = path.read_text()
    inputs = verilog_module(text, "input_bambu_artificial_ParmMgr_modgen")
    outputs = verilog_module(text, "output_bambu_artificial_ParmMgr_modgen")
    top = verilog_module(text, "copy_distinct_buffers")
    datapath = verilog_module(text, "datapath_copy_distinct_buffers")
    insts = {}
    for mod, body, expected in (("MinimalAXI4MasterPipelined", inputs, "input"),
                                ("MinimalAXI4MasterPipelinedWrite", outputs, "output")):
        hits = list(re.finditer(rf"\b{mod}\s*#\s*\(", body))
        need(len(hits) == 1, f"{mod} count in {expected} interface scope is {len(hits)}")
        start = hits[0].start()
        param_open = body.find("(", start)
        param_end = balanced_end(body, param_open)
        params = body[param_open + 1:param_end]
        inst_match = re.match(r"\s*\w+\s*\(", body[param_end + 1:])
        need(inst_match, f"{mod} instance has no port connections")
        port_open = param_end + 1 + inst_match.end() - 1
        p = {k: v.strip() for k, v in re.findall(r"\.(\w+)\s*\(([^()]*(?:\([^()]*\)[^()]*)*)\)", params)}
        port_list = balanced(body, port_open)
        ports = {k: v.strip() for k, v in re.findall(r"\.(\w+)\s*\(([^()]*)\)", port_list)}
        for key, val in (("B_MAX", "16"), ("MAX_OUTSTANDING", "1"), ("FIFO_DEPTH", "32")):
            need(p.get(key) == val, f"{mod} same instance missing {key}={val}")
        direction = "ar" if expected == "input" else "aw"
        need(ports.get("start") == "start_port" and ports.get("done") == "burst_done",
             f"{mod} start/done cross-wire or missing")
        for width_key, width in ((f"BITSIZE_m_axi_{direction}id", "1"),):
            need(p.get(width_key) == width, f"{mod} {width_key} must be 1-bit reduced engine ID")
        if expected == "input":
            need(ports.get("m_axi_arready") == "p_m_axi_input_arready", "read engine ARREADY uses wrong bundle")
            need(ports.get("m_axi_rdata") == "p_m_axi_input_rdata", "read engine RDATA uses wrong bundle")
            need(re.search(r"\bassign\s+p_m_axi_input_arcache\s*=\s*0\s*;", body),
                 "read AR sideband is not zero")
            need(re.search(r"\bassign\s+p_m_axi_input_awcache\s*=\s*0\s*;", body),
                 "inactive read AW sideband is not zero")
            need(re.search(r"\bassign\s+p_m_axi_input_bready\s*=\s*0\s*;", body),
                 "read-only BREADY is not zero")
        else:
            need(p.get("BITSIZE_in5") == "32", "writer count input BITSIZE_in5 must be 32 bits")
            need(ports.get("m_axi_awaddr") == "burst_awaddr",
                 "write engine AWADDR is not connected to burst_awaddr")
            need(ports.get("m_axi_awready") == "p_m_axi_output_awready", "write engine AWREADY uses wrong bundle")
            need(ports.get("m_axi_bready") == "burst_bready" and
                 re.search(r"\bassign\s+p_m_axi_output_bready\s*=\s*burst_bready\s*;", body),
                 "BREADY is not driven by write engine")
            need(re.search(r"\bassign\s+p_m_axi_output_awcache\s*=\s*4'b0010\s*;", body),
                 "AWCACHE is not 0010")
            need(re.search(r"\bassign\s+p_m_axi_output_arcache\s*=\s*0\s*;", body),
                 "inactive AR sideband is not zero")
            need(re.search(r"\bassign\s+p_m_axi_output_awaddr\s*=\s*burst_awaddr\s*;", body),
                 "write AWADDR is not driven by write engine")
        insts[expected] = {"module": mod, "parameters": p, "ports": ports}
    need("MinimalAXI4MasterPipelinedWrite" not in inputs and "MinimalAXI4MasterPipelined #" not in outputs,
         "read/write engines appear in wrong interface scopes")
    need(re.search(r"\bassign\s+p_m_axi_input_arid\s*=\s*\{\s*\{\s*\(\s*BITSIZE_arid\s*-\s*1\s*\)\s*\{\s*1'b0\s*\}\s*\}\s*,\s*burst_arid\s*\}\s*;", inputs),
         "read ID is not reduced integrally")
    need(re.search(r"\bwire\s+burst_rid\s*=\s*\|\s*p_m_axi_input_rid\s*;", inputs),
         "read response ID reduction missing")
    need(re.search(r"\bwire\s+burst_bid\s*=\s*\|\s*p_m_axi_output_bid\s*;", outputs),
         "write response ID reduction missing")
    interface_ports = {}
    for key, module in (("read", "input_bambu_artificial_ParmMgr_modgen"),
                        ("write", "output_bambu_artificial_ParmMgr_modgen")):
        matches = list(re.finditer(rf"(?<!\w){module}(?!\w)\s*", datapath))
        need(len(matches) == 1, f"datapath must instantiate exactly one {key} interface")
        pos = matches[0].end()
        if datapath[pos:pos + 1] == "#":
            param_open = datapath.find("(", pos)
            pos = balanced_end(datapath, param_open) + 1
        instance = re.match(r"\s*\w+\s*\(", datapath[pos:])
        need(instance, f"datapath {key} interface has no instance port list")
        port_open = pos + instance.end() - 1
        plist = balanced(datapath, port_open)
        interface_ports[key] = {name: value.strip() for name, value in
                                re.findall(r"\.(\w+)\s*\(([^()]*)\)", plist)}
    for port in ("start_port", "done_port"):
        read_net = interface_ports["read"].get(port)
        write_net = interface_ports["write"].get(port)
        need(read_net and write_net and read_net != write_net,
             f"read/write {port} are missing or cross-connected at top-level")
    need(len(re.findall(r"\bMinimalAXI4MasterPipelined(?!Write)\s*#", text)) == 1,
         "unexpected read burst/legacy engine instances in generated RTL")
    need(len(re.findall(r"\bMinimalAXI4MasterPipelinedWrite\b", text)) == 1,
         "generated RTL must contain exactly one writer engine instance")
    need(len(re.findall(r"\binput\s+\[5:0\]\s+p_m_axi_input_rid\s*;", inputs)) == 1 and
         len(re.findall(r"\binput\s+\[5:0\]\s+p_m_axi_output_bid\s*;", outputs)) == 1,
         "external AXI response ID width is not 6 bits")
    need(re.search(r"output\s+\[3:0\]\s+p_m_axi_output_awcache\s*;", text) and
         re.search(r"output\s+p_m_axi_output_bready\s*;", text),
         "top-level write interface lacks required width/sideband signals")
    return insts


def validate(root: Path) -> dict[str, object]:
    files = artifacts(root)
    pre = extract_ir(files["before_interface"], require_calls=False)
    post = extract_ir(files["after_interface"])
    pre_counts = relevant_call_counts(pre)
    need(not any(pre_counts.values()),
         f"before InterfaceInfer unexpectedly contains ParmMgr burst operations: {pre_counts}")
    post_counts = relevant_call_counts(post)
    need(all(count == 1 for count in post_counts.values()),
         f"after InterfaceInfer must contain one of each ParmMgr burst operation: {post_counts}")
    ir = extract_ir(files["after_virtual_phi"])
    sched = scheduling(files["schedule_xml"])
    expected = {}
    for label, op in ir["ops"].items():
        expected[label] = op["node"].lstrip("@")
        need(expected[label] in sched, f"schedule lacks {label} node {expected[label]}")
        op["schedule"] = {k: sched[expected[label]].get(k) for k in ("cstep", "fu_name", "fu_index")}
    blocks = ir["blocks"]
    cfg = ir["ops"]["CONFIGURE_WRITE"]
    drain = ir["ops"]["DRAIN_WRITE"]
    read_cfg = ir["ops"]["CONFIGURE_READ"]
    read_op = ir["ops"]["BURST_READ"]
    store = ir["ops"]["STORE_WRITE"]
    need(int(blocks[cfg["bb"]]["loop_id"]) == 0, "CONFIGURE_WRITE is inside a loop")
    need(int(blocks[read_cfg["bb"]]["loop_id"]) == 0, "CONFIGURE_READ is inside a loop")
    need(int(blocks[read_op["bb"]]["loop_id"]) != 0, "BURST_READ is not inside a loop")
    need(int(blocks[store["bb"]]["loop_id"]) != 0, "STORE_WRITE is not inside a loop")
    need(int(blocks[drain["bb"]]["loop_id"]) == 0, "DRAIN_WRITE is not at loop exit")
    need(int(cfg["schedule"]["cstep"]) < int(drain["schedule"]["cstep"]), "configure is not scheduled before drain")
    need(int(store["schedule"]["cstep"]) < int(drain["schedule"]["cstep"]),
         "STORE_WRITE is not scheduled before DRAIN_WRITE")
    need(int(ir["ops"]["CONFIGURE_READ"]["schedule"]["cstep"]) < int(ir["ops"]["BURST_READ"]["schedule"]["cstep"]),
         "read configure is not scheduled before burst read")
    need(int(cfg["schedule"]["cstep"]) < int(store["schedule"]["cstep"]),
         "write configure is not scheduled before store")
    need(int(read_op["schedule"]["cstep"]) < int(store["schedule"]["cstep"]),
         "burst read is not scheduled before store")
    # In this fixture the read return SSA is the value argument to STORE_WRITE; require it directly.
    load_node = ir["ops"]["BURST_READ"]["node"]
    load_results = {n for n, (kind, attrs) in ir["nodes"].items()
                    if kind == "ssa_node" and load_node in refs(attrs, "def_stmt")}
    store_uses = set(ir["ops"]["STORE_WRITE"]["args"])
    load_to_store_proved = bool(load_results & store_uses)
    need(load_to_store_proved, "LOAD-to-STORE scalar value dependency is missing")
    edges = []
    for dst, op in ir["ops"].items():
        for src, source_op in ir["ops"].items():
            common = set(op["vuse"]) & set(source_op["vdef"])
            if common:
                edges.append({"from": src, "to": dst, "kind": "virtual", "values": sorted(common)})
    scalar_edges = []
    for dst, op in ir["ops"].items():
        for source_label, source in ir["ops"].items():
            values = [n for n, (kind, attrs) in ir["nodes"].items()
                      if kind == "ssa_node" and refs(attrs, "def_stmt") == [source["node"]]
                      and n in op["args"]]
            if values:
                scalar_edges.append({"from": source_label, "to": dst, "kind": "data", "values": values})
    graph_edges = {label: dot_edges(files[label]) for label in ("dfg", "adg", "odg", "feedback")}
    graph_artifacts = {label: dot_graph(files[label])
                       for label in ("dfg", "adg", "odg", "feedback", "schedule_dot")}
    dfg_labels = "\n".join(graph_artifacts["dfg"]["labels"].values())
    for label, callee in OPS.items():
        node_id = ir["ops"][label]["node"].lstrip("@")
        need(re.search(rf"_{node_id}\s+-\s+{re.escape(callee)}\b", dfg_labels),
             f"DFG does not contain the post-InterfaceInfer {label} operation")
    virtual_edges = {(edge["from"], edge["to"]): edge for edge in edges}
    for source, target in (("CONFIGURE_READ", "BURST_READ"), ("CONFIGURE_WRITE", "STORE_WRITE")):
        need((source, target) in virtual_edges,
             f"missing post-InterfaceInfer virtual def/use edge {source}->{target}")
    store_drain_path = cfg_path(blocks, store["bb"], drain["bb"])
    store_precedes_drain_on_path = bool(store_drain_path) and int(store["schedule"]["cstep"]) < int(drain["schedule"]["cstep"])
    need(store_precedes_drain_on_path, "copy CFG has no ordered path from STORE_WRITE through DRAIN_WRITE")
    read_handoff = [virtual_edges[("CONFIGURE_READ", "BURST_READ")][key]
                    for key in ("from", "to")]
    write_handoff = [virtual_edges[("CONFIGURE_WRITE", "STORE_WRITE")][key]
                     for key in ("from", "to")] + ["DRAIN_WRITE"]
    def stmt_node(label: str, node: str, kind: str, attrs: str) -> dict[str, object]:
        bb_match = re.search(r"\bbb_index:\s*(\d+)", attrs)
        scalar_defs = [ssa for ssa, (ssa_kind, ssa_attrs) in ir["nodes"].items()
                       if ssa_kind == "ssa_node" and refs(ssa_attrs, "def_stmt") == [node]]
        scalar_uses = [ssa for ssa, (ssa_kind, ssa_attrs) in ir["nodes"].items()
                       if ssa_kind == "ssa_node" and node in refs(ssa_attrs, "use_stmt")]
        return {"label": label, "kind": kind, "bb": int(bb_match.group(1)) if bb_match else None,
                "scalar_defs": scalar_defs, "scalar_uses": scalar_uses,
                "virtual_uses": refs(attrs, "vuse"), "virtual_defs": refs(attrs, "vdef")}

    op_statement_ids = {op["node"] for op in ir["ops"].values()}
    nodes_report = [stmt_node("burst_call" if node in op_statement_ids else "scheduled_statement",
                              node, kind, attrs)
                    for node, (kind, attrs) in ir["stmts"].items()]
    report = {
        "fixture": "copy_distinct_buffers", "stages": {k: str(v) for k, v in files.items()},
        "calls": {label: {"node": op["node"], "bb": op["bb"], "cstep": op["schedule"]["cstep"],
                          "fu_name": op["schedule"]["fu_name"], "fu_index": op["schedule"]["fu_index"]}
                  for label, op in ir["ops"].items()},
        "load_to_store": {"loads": sorted(load_results), "store_scalar_uses": sorted(store_uses & load_results),
                          "proved": load_to_store_proved},
        "virtual_edges": edges,
        "scalar_edges": scalar_edges,
        "cross_direction_edges": [e for e in edges if ("READ" in e["from"] and "WRITE" in e["to"]) or
                                  ("WRITE" in e["from"] and "READ" in e["to"])] +
                                 [e for e in scalar_edges if ("READ" in e["from"] and "WRITE" in e["to"]) or
                                  ("WRITE" in e["from"] and "READ" in e["to"])],
        "graph_edges": {"data": graph_edges["dfg"], "dependence": graph_edges["adg"],
                        "order": graph_edges["odg"], "feedback": graph_edges["feedback"]},
        "operations": {label: {"bb": op["bb"], "scalar_uses": op["args"],
                                "virtual_uses": op["vuse"], "virtual_defs": op["vdef"]}
                       for label, op in ir["ops"].items()},
        "basic_block_statement_def_use": nodes_report,
        "direction_chains": {
            "read": {"operations": read_handoff,
                     "evidence": [virtual_edges[("CONFIGURE_READ", "BURST_READ")] ]},
            "write": {"operations": write_handoff,
                      "evidence": [virtual_edges[("CONFIGURE_WRITE", "STORE_WRITE")],
                                   {"kind": "cfg_path", "blocks": store_drain_path,
                                    "store_precedes_drain_on_path": store_precedes_drain_on_path}] }},
        "cfg": {str(bb): data for bb, data in blocks.items()},
        "graph_artifacts": {label: {"nodes": sorted(graph["nodes"]),
                                    "edge_count": len(graph["edges"])}
                            for label, graph in graph_artifacts.items()},
        "wrapper_instances": check_wrapper(files["rtl"]),
        "claims": ["fixture proves LOAD-to-STORE dependency", "does not prove universal cross-direction independence",
                   "does not prove same-bundle duplex operation"],
        "original_edges": "DOT/IR artifacts retained; dependency edge provenance is not classified beyond raw virtual edges",
    }
    return report


def main() -> int:
    if len(sys.argv) != 2:
        print(f"usage: {sys.argv[0]} <compiler-output-directory>", file=sys.stderr); return 2
    root = Path(sys.argv[1])
    try:
        report = validate(root)
        target = root / "copy_handoff_report.json"
        target.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n")
        print(f"COPY_HANDOFF_CHECK_PASS report={target}")
        return 0
    except (OSError, ValueError, ET.ParseError) as exc:
        print(f"COPY_HANDOFF_CHECK_FAIL: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
