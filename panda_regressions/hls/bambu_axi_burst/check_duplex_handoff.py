#!/usr/bin/env python3
"""Fail-closed witness for same-bundle duplex compiler handoff artifacts."""
from __future__ import annotations

import argparse
import json
import re
import sys
import xml.etree.ElementTree as ET
from collections import deque
from pathlib import Path

from check_duplex_effect_analysis_artifacts import (
    CheckError, VALUE_OPERAND_ARITY, VALUE_TERMINALS, check_production_duplex, fields,
    function_graph, need, operation_refs, parse_proof_records, verify_success_record,
)

OPS = {
    "CONFIGURE_READ": "configure_read",
    "BURST_READ": "burst_read",
    "CONFIGURE_WRITE": "configure_write",
    "STORE_WRITE": "store_write",
    "DRAIN_WRITE": "drain_write",
}
READ_OPS = {"CONFIGURE_READ", "BURST_READ"}
WRITE_OPS = set(OPS) - READ_OPS


def read_artifact(path: Path, label: str) -> str:
    need(path.is_file() and not path.is_symlink() and path.stat().st_size > 0,
         f"required nonempty {label} missing: {path}")
    return path.read_text(encoding="utf-8", errors="strict")


def artifacts(root: Path, top: str) -> dict[str, Path]:
    pt = root / "panda-temp"
    before = pt / "before_Frontend::InterfaceInfer.raw"
    after = pt / "after_Frontend::InterfaceInfer.raw"
    virtual = sorted(path for path in pt.glob("after_Frontend::BuildVirtualPhi::*.raw")
                     if re.fullmatch(rf"after_Frontend::BuildVirtualPhi::{re.escape(top)}\(\d+\)\[\d+\]\.raw",
                                     path.name))
    need(len(virtual) == 1, f"expected exactly one post-BuildVirtualPhi raw for {top}; found {len(virtual)}")
    schedules = sorted(root.glob(f"*{top}*scheduling.xml"))
    need(len(schedules) == 1, f"expected exactly one scheduling XML for {top}; found {len(schedules)}")
    dot_root = root / "HLS_output/dot" / top
    stderr_candidates = [root / name for name in ("stderr.log", "bambu.stderr.log")
                         if (root / name).is_file()]
    stdout_candidates = [root / name for name in ("stdout.log", "bambu.stdout.log")
                         if (root / name).is_file()]
    need(len(stderr_candidates) == 1, f"expected exactly one compiler stderr log; found {len(stderr_candidates)}")
    need(len(stdout_candidates) <= 1, f"ambiguous compiler stdout logs: {stdout_candidates}")
    paths = {
        "before": before,
        "after": after,
        "virtual": virtual[0],
        "schedule_xml": schedules[0],
        "schedule_dot": dot_root / "HLS_scheduling.dot",
        "dfg": dot_root / "OP_DFG.dot",
        "adg": dot_root / "OP_ADG.dot",
        "rtl": root / f"{top}.v",
        "architecture": pt / "architecture.xml",
        "stderr": stderr_candidates[0],
        "stdout": stdout_candidates[0] if stdout_candidates else root / "stdout.log",
    }
    for label, path in paths.items():
        if label == "stdout" and not path.exists():
            continue
        read_artifact(path, label)
    return paths


def function_names(nodes: dict[int, tuple[str, str]]) -> dict[int, str]:
    identifiers = {index: match.group(1) for index, (kind, attrs) in nodes.items()
                   if kind == "identifier_node" and
                   (match := re.search(r'\bstrg: "([^\"]+)"', attrs))}
    return {index: identifiers.get(int(fields(node).get("name", ["-1"])[0]), "")
            for index, node in nodes.items() if node[0] == "function_val_node" and
            fields(node).get("name")}


def call_name(nodes: dict[int, tuple[str, str]], funcs: dict[int, str],
              statement: tuple[str, str]) -> str:
    fn_ids = fields(statement).get("fn", [])
    for operand in fields(statement).get("op", []):
        operand_node = nodes.get(int(operand), ("", ""))
        if operand_node[0] == "call_node":
            fn_ids.extend(fields(operand_node).get("fn", []))
    if not fn_ids:
        return ""
    fn_node = nodes.get(int(fn_ids[0]), ("", ""))
    targets = fields(fn_node).get("op", []) if fn_node[0] == "addr_node" else [fn_ids[0]]
    names = [funcs.get(int(target), "") for target in targets]
    return next((name for name in names if name), "")


def selected_ops(raw: str, top: str, *, expected: bool) -> tuple[dict[int, tuple[str, str]], int,
                                                                    dict[int, tuple[int, str, list[int], list[str], list[str]]],
                                                                    set[int], dict[str, int]]:
    nodes, function, blocks, reachable = function_graph(raw, top)
    funcs = function_names(nodes)
    matches: dict[str, list[int]] = {label: [] for label in OPS}
    for block_data in blocks.values():
        for stmt_id in block_data[2]:
            statement = nodes.get(stmt_id)
            if statement and statement[0] in {"call_stmt", "assign_stmt", "gimple_assign"}:
                name = call_name(nodes, funcs, statement)
                for label, suffix in OPS.items():
                    if name.endswith("ParmMgr_" + suffix):
                        matches[label].append(stmt_id)
    counts = {label: len(ids) for label, ids in matches.items()}
    if expected:
        need(all(count == 1 for count in counts.values()),
             f"selected function {top} must contain exactly one of each duplex op: {counts}")
    else:
        need(not any(counts.values()),
             f"pre-InterfaceInfer function {top} unexpectedly contains duplex ops: {counts}")
    return nodes, function, blocks, reachable, {label: ids[0] for label, ids in matches.items() if ids}


def path_exists(blocks: dict[int, tuple[int, str, list[int], list[str], list[str]]],
                source: int, target: int) -> list[int]:
    pending = deque([(source, [source])])
    visited: set[int] = set()
    while pending:
        current, path = pending.popleft()
        if current == target:
            return path
        if current in visited:
            continue
        visited.add(current)
        for successor in blocks[current][4]:
            if successor == "EXIT":
                continue
            next_block = int(successor)
            need(next_block in blocks, f"CFG block {current} references absent successor {next_block}")
            pending.append((next_block, path + [next_block]))
    return []


def schedule_records(path: Path, top: str) -> dict[int, dict[str, str]]:
    root = ET.parse(path).getroot()
    records: dict[int, dict[str, str]] = {}
    for element in root.iter("scheduling_constraints"):
        name = element.get("vertex_name", "")
        match = re.fullmatch(rf"{re.escape(top)}_(\d+)_(\d+)", name)
        if not match:
            continue
        function_id, operation_id = map(int, match.groups())
        need(operation_id not in records, f"duplicate scheduling XML operation ID {operation_id}")
        records[operation_id] = {**element.attrib, "function_id": str(function_id)}
    need(records, f"scheduling XML has no operation records for top {top}")
    return records


def dot_nodes_edges(path: Path) -> tuple[dict[int, str], list[tuple[int, int, str]]]:
    text = read_artifact(path, "DOT graph")
    nodes: dict[int, str] = {}
    edges: list[tuple[int, int, str]] = []
    for line in text.splitlines():
        node = re.match(r'^\s*(\d+)\s*\[.*?label="(.*)"\];\s*$', line)
        edge = re.match(r'^\s*(\d+)\s*->\s*(\d+)\s*\[(.*?)\];\s*$', line)
        if node:
            ident = int(node.group(1))
            need(ident not in nodes, f"{path.name}: duplicate DOT node {ident}")
            nodes[ident] = node.group(2).replace(r"\n", "\n").replace(r"\l", "\n")
        elif edge:
            edges.append((int(edge.group(1)), int(edge.group(2)), edge.group(3)))
    need(nodes and edges, f"{path.name}: missing DOT operation nodes or edges")
    return nodes, edges


def verify_dot_mapping(path: Path, top: str, function_id: int,
                       op_ids: dict[str, int]) -> dict[str, dict[str, object]]:
    nodes, _edges = dot_nodes_edges(path)
    mapped: dict[str, dict[str, object]] = {}
    for label, op_id in op_ids.items():
        hits = [(dot_id, text) for dot_id, text in nodes.items()
                if re.search(rf"\b{re.escape(top)}_{function_id}_{op_id}\s+-\s+", text)]
        need(len(hits) == 1, f"{path.name} must map {label} operation ID {op_id} exactly once")
        mapped[label] = {"dot_node": hits[0][0], "label": hits[0][1]}
    return mapped


def virtual_edges(nodes: dict[int, tuple[str, str]], function_id: int,
                  blocks: dict[int, tuple[int, str, list[int], list[str], list[str]]],
                  op_ids: dict[str, int]) -> list[dict[str, object]]:
    names = {node_id: label for label, node_id in op_ids.items()}
    defs: dict[int, int] = {}
    uses: dict[int, list[int]] = {}
    for bb, (_loop, _attrs, statements, _preds, _succs) in blocks.items():
        for statement_id in statements:
            statement = nodes[statement_id]
            for value in fields(statement).get("vdef", []):
                value_id = int(value)
                need(value_id not in defs, f"virtual SSA @{value_id} has multiple defining statements")
                defs[value_id] = statement_id
            for value in fields(statement).get("vuse", []):
                uses.setdefault(int(value), []).append(statement_id)
    result = []
    for value_id, consumers in uses.items():
        source = defs.get(value_id)
        for target in consumers:
            source_name = names.get(source, f"statement:@{source}") if source is not None else "ENTRY"
            target_name = names.get(target, f"statement:@{target}")
            if source != target:
                result.append({"from": source_name, "to": target_name,
                               "value": f"@{value_id}", "source_statement": source,
                               "target_statement": target})
    need(result, "selected function has no virtual memory dependence chains")
    return result


def verify_load_store(raw: str, top: str, kind: str, stderr: str) -> dict[str, object]:
    records = parse_proof_records(stderr)
    need(len(records) == 1, "expected exactly one structured semantic proof record")
    record = records[0]
    need(record.get("bundle") == "shared" and record.get("semantic_proof") is True,
         "semantic proof record does not describe activated shared-bundle proof")
    verify_success_record(record, raw, top,
                          require_load_store_data_dependency=(kind == "copy"))
    return {"proof_kind": record.get("proof_kind"),
            "load_statement_index": record.get("load_statement_index"),
            "store_statement_index": record.get("store_statement_index"),
            "dependency_required": kind == "copy",
            "verified": True}


def verify_emitted_value_dependency(nodes: dict[int, tuple[str, str]], op_ids: dict[str, int],
                                    kind: str) -> dict[str, object]:
    load_id, store_id = op_ids["BURST_READ"], op_ids["STORE_WRITE"]
    load_values = [index for index, (node_kind, attrs) in nodes.items()
                   if node_kind == "ssa_node" and " virtual" not in attrs and
                   fields(nodes[index]).get("def_stmt") == [str(load_id)]]
    need(len(load_values) == 1, f"BURST_READ must define exactly one payload SSA value; found {load_values}")
    store_args = [int(value) for value in fields(nodes[store_id]).get("arg", [])]
    need(len(store_args) >= 3, "STORE_WRITE has no recognizable payload argument")
    payload = store_args[2]
    depends, complete = emitted_operand_dependency_result(nodes, payload, load_values[0])
    need(complete, "STORE_WRITE payload dependency traversal is incomplete")
    if kind == "copy":
        need(depends, "emitted STORE_WRITE payload no longer depends on BURST_READ")
    else:
        need(not depends, "independent case emitted a STORE_WRITE payload dependency on BURST_READ")
    return {"load_value": load_values[0], "store_payload": payload,
            "depends": depends, "traversal_complete": complete}


def emitted_operand_dependency_result(nodes: dict[int, tuple[str, str]], root: int,
                                      target: int, limit: int = 4096) -> tuple[bool, bool]:
    pending, visited = [root], set()
    arity = {**VALUE_OPERAND_ARITY, "and_node": 2, "concat_bit_node": 3,
             "shr_node": 2}
    while pending:
        if len(visited) >= limit:
            return False, False
        index = pending.pop()
        if index in visited:
            continue
        if index not in nodes:
            return False, False
        if index == target:
            return True, True
        visited.add(index)
        kind, _attrs = nodes[index]
        node_fields = fields(nodes[index])
        if kind == "ssa_node":
            definitions = node_fields.get("def_stmt", [])
            if definitions:
                if len(definitions) != 1 or int(definitions[0]) not in nodes:
                    return False, False
                statement_index = int(definitions[0])
                statement = nodes[statement_index]
                if statement[0] in {"assign_stmt", "gimple_assign"}:
                    operands = operation_refs(statement)
                    if len(operands) != 2 or operands[0] != index:
                        return False, False
                    pending.append(operands[1])
                elif statement[0] == "phi_stmt":
                    results = fields(statement).get("res", [])
                    definitions = [int(value) for value in fields(statement).get("def", [])]
                    if len(results) != 1 or int(results[0]) != index or not definitions or \
                            any(value not in nodes for value in definitions):
                        return False, False
                    pending.extend(definitions)
                else:
                    return False, False
            elif len(node_fields.get("var", [])) == 1:
                variable = int(node_fields["var"][0])
                if variable not in nodes or nodes[variable][0] != "argument_val_node":
                    return False, False
            else:
                return False, False
        elif kind in VALUE_TERMINALS:
            continue
        elif kind in arity:
            operands = operation_refs(nodes[index])
            if len(operands) != arity[kind]:
                return False, False
            pending.extend(operands)
        else:
            return False, False
    return False, True


def validate(root: Path, top: str, profile: str, kind: str) -> dict[str, object]:
    need(kind in {"copy", "independent"}, f"unknown case kind {kind!r}; expected copy|independent")
    need(profile.strip() and profile == profile.strip(), "profile must be a nonempty token")
    files = artifacts(root, top)
    before_text = read_artifact(files["before"], "pre-InterfaceInfer raw IR")
    after_text = read_artifact(files["after"], "post-InterfaceInfer raw IR")
    virtual_text = read_artifact(files["virtual"], "post-BuildVirtualPhi raw IR")
    _before_nodes, _before_fn, _before_blocks, _before_reach, before_ops = selected_ops(
        before_text, top, expected=False)
    _after_nodes, _after_fn, _after_blocks, _after_reach, after_ops = selected_ops(
        after_text, top, expected=True)
    nodes, function_id, blocks, reachable, op_ids = selected_ops(virtual_text, top, expected=True)
    need(before_ops == {}, "pre-InterfaceInfer raw IR contains duplex operations")
    need(after_ops == op_ids,
         f"post-InterfaceInfer and post-BuildVirtualPhi operation IDs disagree: {after_ops} vs {op_ids}")

    sched = schedule_records(files["schedule_xml"], top)
    op_records: dict[str, dict[str, object]] = {}
    for label, statement_id in op_ids.items():
        stmt = nodes[statement_id]
        need(statement_id in reachable, f"{label} is not reachable from selected function {top}")
        schedule = sched.get(statement_id)
        need(schedule is not None, f"scheduling XML does not contain raw operation ID {statement_id}")
        need(schedule["function_id"] == str(function_id),
             f"scheduling XML maps {label} to a different function ID")
        need("cstep" in schedule and "fu_name" in schedule and "fu_index" in schedule,
             f"scheduling XML record for {label} is missing cstep/FU fields")
        bb = int(re.search(r"\bbb_index: (\d+)", stmt[1]).group(1))
        op_records[label] = {"operation_id": statement_id, "function_id": function_id,
                             "bb": bb, "cstep": int(schedule["cstep"]),
                             "fu_name": schedule.get("fu_name", ""),
                             "fu_index": schedule.get("fu_index", "")}
    need(all(op_records[label]["fu_name"] and
             re.fullmatch(r"\d+", op_records[label]["fu_index"])
             for label in OPS), "scheduling record has invalid function-unit mapping")
    read_fu = {(op_records[label]["fu_name"], op_records[label]["fu_index"])
               for label in READ_OPS}
    write_fu = {(op_records[label]["fu_name"], op_records[label]["fu_index"])
                for label in WRITE_OPS}
    need(len(read_fu) == 1 and len(write_fu) == 1 and read_fu.isdisjoint(write_fu),
         "read/write operations do not use distinct coherent function-unit instances")
    read_endpoint = next(iter(read_fu))[0]
    write_endpoint = next(iter(write_fu))[0]
    need("duplex_read_endpoint" in read_endpoint and "duplex_write_endpoint" not in read_endpoint,
         "READ operations must map to the duplex read endpoint")
    need("duplex_write_endpoint" in write_endpoint and "duplex_read_endpoint" not in write_endpoint,
         "WRITE operations must map to the duplex write endpoint")

    dot_mapping = {name: verify_dot_mapping(files[name], top, function_id,
                                            {label: item["operation_id"] for label, item in op_records.items()})
                   for name in ("dfg", "adg", "schedule_dot")}
    chains = virtual_edges(nodes, function_id, blocks, op_ids)
    chain_pairs = {(edge["from"], edge["to"]) for edge in chains}
    need(("CONFIGURE_READ", "BURST_READ") in chain_pairs,
         "broken virtual handoff: CONFIGURE_READ does not reach BURST_READ")
    need(("CONFIGURE_WRITE", "STORE_WRITE") in chain_pairs,
         "broken virtual handoff: CONFIGURE_WRITE does not reach STORE_WRITE")

    def ordered_path(source: str, target: str) -> list[int]:
        first, last = op_records[source], op_records[target]
        path = path_exists(blocks, first["bb"], last["bb"])
        need(path, f"no normal CFG path from {source} to {target}")
        need(first["cstep"] < last["cstep"], f"{source} is not scheduled before {target}")
        return path

    read_path = ordered_path("CONFIGURE_READ", "BURST_READ")
    write_path = ordered_path("CONFIGURE_WRITE", "STORE_WRITE")
    store_drain_path = ordered_path("STORE_WRITE", "DRAIN_WRITE")
    if kind == "copy":
        need(op_records["BURST_READ"]["cstep"] < op_records["STORE_WRITE"]["cstep"],
             "copy case must schedule BURST_READ before STORE_WRITE")
    dependence = verify_load_store(before_text, top, kind,
                                   read_artifact(files["stderr"], "compiler stderr"))
    emitted_dependency = verify_emitted_value_dependency(nodes, op_ids, kind)

    arch = ET.parse(files["architecture"]).getroot()
    funcs = [elem for elem in arch.findall(".//function") if elem.get("name") == top]
    need(len(funcs) == 1, f"architecture XML must contain exactly one function named {top}")
    bundles = [node for node in funcs[0].findall("./bundles/bundle") if node.get("name") == "shared"]
    need(len(bundles) == 1, "architecture XML must retain exactly one shared bundle")
    rtl = read_artifact(files["rtl"], "generated RTL")
    stdout = files["stdout"].read_text(encoding="utf-8", errors="strict") if files["stdout"].exists() else ""
    stderr = read_artifact(files["stderr"], "compiler stderr")
    check_production_duplex(
        rtl, stdout + "\n" + stderr, top, bundles[0].attrib,
        distinct_base_override=(dependence["proof_kind"] == "distinct_base_restricted_pointwise"))

    cross_arcs = [edge for edge in chains
                  if (edge["from"] in READ_OPS and edge["to"] in WRITE_OPS) or
                  (edge["from"] in WRITE_OPS and edge["to"] in READ_OPS)]
    return {
        "top": top, "profile": profile, "case_kind": kind,
        "artifacts": {label: str(path) for label, path in files.items()},
        "operations": op_records,
        "virtual_chains": chains,
        "cross_direction_virtual_arcs": cross_arcs,
        "schedule_dot_mapping": dot_mapping,
        "paths": {"configure_read_to_burst_read": read_path,
                  "configure_write_to_store_write": write_path,
                  "store_write_to_drain_write": store_drain_path},
        "load_store_value_proof": dependence,
        "emitted_load_store_dependency": emitted_dependency,
        "production_structure_verified": True,
    }


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("artifact_dir", type=Path)
    parser.add_argument("--top", required=True)
    parser.add_argument("--profile", required=True)
    parser.add_argument("--kind", choices=("copy", "independent"), required=True)
    args = parser.parse_args(argv)
    try:
        report = validate(args.artifact_dir, args.top, args.profile, args.kind)
        output = args.artifact_dir / "duplex_handoff_report.json"
        output.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")
        print(f"DUPLEX_HANDOFF_CHECK_PASS TOP={args.top} PROFILE={args.profile} KIND={args.kind} report={output}")
        return 0
    except (OSError, ValueError, ET.ParseError) as exc:
        print(f"DUPLEX_HANDOFF_CHECK_FAIL: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
