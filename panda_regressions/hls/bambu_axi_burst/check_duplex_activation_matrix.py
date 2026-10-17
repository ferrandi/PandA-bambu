#!/usr/bin/env python3
"""Fail-closed frontend and backend-boundary checks for multi-root duplex cases."""
from __future__ import annotations

import argparse
import json
import re
import sys
import xml.etree.ElementTree as ET
from pathlib import Path

from check_copy_handoff import call_names, parse_blocks, records, refs
from check_burst_results import _parameterized_instances
from check_duplex_effect_analysis_artifacts import (
    AXI_SUFFIXES, READ_AXI_INPUTS, verify_private_duplex_wiring)

BURST_CALLS = {
    "configure_read": "configure_read",
    "burst_read": "burst_read",
    "configure_write": "configure_write",
    "store_write": "store_write",
    "drain_write": "drain_write",
}
EXPECTED_ROOTS = {
    **{case: ("matrix_candidate", "matrix_peer") for case in range(1, 11)},
    13: ("matrix_candidate", "matrix_peer"), 14: ("matrix_candidate", "matrix_peer"),
    15: ("matrix_candidate", "matrix_peer"),
    16: ("matrix_helper", "matrix_candidate", "matrix_peer"),
    17: ("matrix_candidate", "unrelated_architectureless"),
}
SINGLE_ROOTS = {11: ("matrix_candidate",), 12: ("matrix_candidate",)}
REVERSED = {2, 4, 6, 8, 10, 14}
DATAFLOW_ANALYSIS_ORDER = {
    9: ("matrix_dataflow_task", "matrix_candidate", "matrix_peer"),
    10: ("matrix_dataflow_task", "matrix_peer", "matrix_candidate"),
    15: ("matrix_other_dataflow_task", "matrix_candidate", "matrix_peer"),
}
DATAFLOW_TASKS = {9: "matrix_dataflow_task", 10: "matrix_dataflow_task",
                  15: "matrix_other_dataflow_task"}
POSITIVE_ROOTS = {
    1: {"matrix_candidate", "matrix_peer"}, 2: {"matrix_candidate", "matrix_peer"},
    15: {"matrix_candidate"}, 17: {"matrix_candidate"},
}
VETO_REASON = {
    3: "canonical contiguous loop recognition failed", 4: "canonical contiguous loop recognition failed",
    5: "bundle does not explicitly request both read and write bursts",
    6: "bundle does not explicitly request both read and write bursts",
    7: "original effects are not fully classified ordinary accesses",
    8: "original effects are not fully classified ordinary accesses",
    9: "DATAFLOW/forwarded bundle ownership is unsupported",
    10: "DATAFLOW/forwarded bundle ownership is unsupported",
    11: "DATAFLOW/forwarded bundle ownership is unsupported",
}
VETO_REASON_FIELD = {
    5: "activation_reason", 6: "activation_reason",
    7: "activation_reason", 8: "activation_reason",
}
INCOMPATIBLE = {13, 14}
INCOMPATIBLE_PROFILE_DIAGNOSTIC = (
    "Incompatible duplex m_axi profiles or latency for bundle 'shared' across root functions; "
    "use distinct bundle names.")
ALL_CASES = set(EXPECTED_ROOTS) | set(SINGLE_ROOTS) | INCOMPATIBLE
LEGACY_ACCESS_DIRECTIONS = {
    3: {"matrix_candidate": {"read", "write"}, "matrix_peer": {"read", "write"}},
    4: {"matrix_candidate": {"read", "write"}, "matrix_peer": {"read", "write"}},
    5: {"matrix_candidate": {"read", "write"}, "matrix_peer": {"write"}},
    6: {"matrix_candidate": {"read", "write"}, "matrix_peer": {"write"}},
    7: {"matrix_candidate": {"read", "write"}},
    8: {"matrix_candidate": {"read", "write"}},
    9: {"matrix_candidate": {"read", "write"}, "matrix_dataflow_task": {"read", "write"}},
    10: {"matrix_candidate": {"read", "write"}, "matrix_dataflow_task": {"read", "write"}},
    15: {"matrix_other_dataflow_task": {"read", "write"}},
    11: {"matrix_candidate": {"read", "write"}},
    12: {"matrix_candidate": {"read", "write"}},
    16: {"matrix_helper": {"read", "write"}, "matrix_peer": {"read", "write"}},
}


def need(condition: bool, message: str) -> None:
    if not condition:
        raise ValueError(message)


def activation_markers(combined: str) -> list[tuple[str, str]]:
    return re.findall(
        r'^\s*(?:---)?Activated same-base pointwise duplex m_axi bundle (\S+) for root (\S+)\s*$',
        combined, re.MULTILINE)


def require_raw(path: Path, label: str) -> dict[str, tuple[str, str]]:
    need(path.is_file() and path.stat().st_size > 0, f"{label} raw IR is missing or empty")
    return records(path.read_text(encoding="utf-8", errors="replace"))


def function_nodes(nodes: dict[str, tuple[str, str]]) -> dict[str, list[str]]:
    identifiers = {}
    for node, (kind, attrs) in nodes.items():
        match = re.search(r'\bstrg:\s*"([^"]+)"', attrs)
        if kind == "identifier_node" and match:
            identifiers[node] = match.group(1)
    functions: dict[str, list[str]] = {}
    for node, (kind, attrs) in nodes.items():
        if kind != "function_val_node":
            continue
        for name_ref in refs(attrs, "name"):
            symbol = identifiers.get(name_ref)
            if symbol:
                functions.setdefault(symbol, []).append(node)
    return functions


def burst_call_label(callee: str) -> str | None:
    for label, operation in BURST_CALLS.items():
        if re.fullmatch(rf"[^\s]+_bambu_artificial_ParmMgr_{re.escape(operation)}", callee):
            return label
    return None


def root_calls(path: Path, root: str) -> list[str]:
    nodes = require_raw(path, path.name)
    funcs = function_nodes(nodes)
    need(root in funcs, f"{path.name} lacks selected root function {root}")
    need(len(funcs[root]) == 1, f"{path.name} has ambiguous function bodies for selected root {root}")
    fn = funcs[root][0]
    bodies = refs(nodes[fn][1], "body")
    need(len(bodies) == 1 and bodies[0] in nodes and
         nodes[bodies[0]][0] == "statement_list_node",
         f"{path.name} root {root} has no unique statement-list body")
    names = call_names(nodes)
    statements = [node for node, (kind, attrs) in nodes.items()
                  if kind.endswith("stmt") and refs(attrs, "parent") == [fn]]
    return [names.get(statement, "") for statement in statements if names.get(statement)]


def root_call_records(path: Path, root: str) -> tuple[dict[str, tuple[str, dict[str, str], list[str]]], dict[str, tuple[int, str, list[str], list[str], list[str]]]]:
    nodes = require_raw(path, path.name)
    funcs = function_nodes(nodes)
    need(root in funcs and len(funcs[root]) == 1, f"{path.name} lacks one unambiguous function {root}")
    fn = funcs[root][0]
    body_refs = refs(nodes[fn][1], "body")
    need(len(body_refs) == 1 and body_refs[0] in nodes and nodes[body_refs[0]][0] == "statement_list_node",
         f"{path.name} root {root} has no unique statement-list body")
    blocks = parse_blocks(nodes[body_refs[0]][1])
    names = call_names(nodes)
    calls: dict[str, tuple[str, dict[str, str], list[str]]] = {}
    for statement, callee in names.items():
        if not callee or refs(nodes[statement][1], "parent") != [fn]:
            continue
        label = burst_call_label(callee)
        if label is None:
            continue
        need(label not in calls, f"{path.name} root {root} duplicates {label}")
        attrs = nodes[statement][1]
        args = refs(attrs, "arg")
        if not args:
            pending = refs(attrs, "op")
            visited = set()
            while pending and not args:
                current = pending.pop()
                if current in visited or current not in nodes:
                    continue
                visited.add(current)
                args = refs(nodes[current][1], "arg")
                pending.extend(refs(nodes[current][1], "op"))
        bb = re.search(r"\bbb_index:\s*(\d+)", attrs)
        need(bb is not None, f"{path.name} root {root} {label} has no basic-block identity")
        block = int(bb.group(1))
        need(block in blocks, f"{path.name} root {root} {label} references absent block {block}")
        calls[label] = (statement, nodes[statement], args)
    return calls, blocks


def constant_value(nodes: dict[str, tuple[str, str]], node_ref: str, label: str) -> int:
    need(node_ref in nodes and nodes[node_ref][0] == "constant_int_val_node",
         f"{label} is not an integer constant")
    match = re.search(r"\bvalue:\s*(-?\d+)", nodes[node_ref][1])
    need(match is not None, f"{label} constant has no numeric value")
    return int(match.group(1))


def normalized_ssa_expression(nodes: dict[str, tuple[str, str]], node_ref: str,
                             visiting: frozenset[str] = frozenset()) -> tuple[object, ...]:
    if node_ref not in nodes:
        return ("missing", node_ref)
    if node_ref in visiting:
        return ("cycle", node_ref)
    kind, attrs = nodes[node_ref]
    next_visiting = visiting | {node_ref}
    if kind == "ssa_node":
        definitions = refs(attrs, "def_stmt")
        if len(definitions) == 1 and definitions[0] in nodes:
            statement_kind, statement_attrs = nodes[definitions[0]]
            operations = refs(statement_attrs, "op")
            if statement_kind.endswith("stmt") and len(operations) >= 2 and operations[0] == node_ref:
                return normalized_ssa_expression(nodes, operations[-1], next_visiting)
        return ("ssa", node_ref)
    if kind == "constant_int_val_node":
        value = re.search(r"\bvalue:\s*(-?\d+)", attrs)
        return (kind, value.group(1) if value else None)
    operands = refs(attrs, "op")
    if operands:
        return (kind, tuple(normalized_ssa_expression(nodes, operand, next_visiting)
                            for operand in operands))
    return (kind, node_ref)


def verify_burst_call_arguments(path: Path, root: str) -> None:
    nodes = require_raw(path, path.name)
    calls, blocks = root_call_records(path, root)
    need(set(calls) == set(BURST_CALLS), f"{path.name} root {root} lacks one or more required burst calls")
    for label, (statement, _node, args) in calls.items():
        need(len(args) == 5, f"{path.name} root {root} {label} must have five ABI arguments")
        opcode = {"configure_read": 2, "burst_read": 0, "configure_write": 2,
                  "store_write": 1, "drain_write": 3}[label]
        need(constant_value(nodes, args[0], f"{label} opcode") == opcode,
             f"{path.name} root {root} {label} has the wrong opcode")
        need(constant_value(nodes, args[1], f"{label} bitwidth") == 32,
             f"{path.name} root {root} {label} has the wrong bitwidth")
        if label in {"configure_read", "configure_write", "burst_read", "drain_write"}:
            need(constant_value(nodes, args[2], f"{label} payload") == 0,
                 f"{path.name} root {root} {label} has a nonzero control payload")
        if label in {"burst_read", "store_write", "drain_write"}:
            need(constant_value(nodes, args[4], f"{label} reserved/count field") == 0,
                 f"{path.name} root {root} {label} has a nonzero reserved/count field")
        if label in {"configure_read", "configure_write"}:
            need(args[3] in nodes and nodes[args[3]][0] == "ssa_node" and
                 args[4] in nodes and nodes[args[4]][0] == "ssa_node",
                 f"{path.name} root {root} {label} lacks SSA base/count arguments")
        if label in {"burst_read", "store_write"}:
            need(args[3] in nodes and nodes[args[3]][0] == "ssa_node",
                 f"{path.name} root {root} {label} lacks an SSA access address")
    read_config = calls["configure_read"][2]
    write_config = calls["configure_write"][2]
    write_drain = calls["drain_write"][2]
    need(read_config[3] == write_config[3] == write_drain[3],
         f"{path.name} root {root} CONFIGURE/DRAIN base arguments differ")
    need(normalized_ssa_expression(nodes, read_config[4]) ==
         normalized_ssa_expression(nodes, write_config[4]),
         f"{path.name} root {root} read/write CONFIGURE count expressions differ")
    read_config_block = int(re.search(r"\bbb_index:\s*(\d+)", nodes[calls["configure_read"][0]][1]).group(1))
    write_config_block = int(re.search(r"\bbb_index:\s*(\d+)", nodes[calls["configure_write"][0]][1]).group(1))
    read_block = int(re.search(r"\bbb_index:\s*(\d+)", nodes[calls["burst_read"][0]][1]).group(1))
    store_block = int(re.search(r"\bbb_index:\s*(\d+)", nodes[calls["store_write"][0]][1]).group(1))
    drain_block = int(re.search(r"\bbb_index:\s*(\d+)", nodes[calls["drain_write"][0]][1]).group(1))
    need(blocks[read_config_block]["loop_id"] == blocks[write_config_block]["loop_id"] == 0,
         f"{path.name} root {root} CONFIGURE calls are not in loop preheaders")
    need(blocks[read_block]["loop_id"] != 0 and
         blocks[read_block]["loop_id"] == blocks[store_block]["loop_id"],
         f"{path.name} root {root} LOAD/STORE calls are not in the same loop")
    need(blocks[drain_block]["loop_id"] == 0,
         f"{path.name} root {root} DRAIN is not outside the loop at its exit")


def verify_drain_exit(path: Path, proof_log: str, root: str) -> None:
    nodes = require_raw(path, path.name)
    funcs = function_nodes(nodes)
    need(root in funcs and len(funcs[root]) == 1, f"{path.name} lacks one function {root} for DRAIN proof")
    fn = funcs[root][0]
    owner_args = set(refs(nodes[fn][1], "arg"))
    owner_ids = {int(node.lstrip("@")) for node in owner_args if node.lstrip("@").isdigit()}
    proof_records = []
    for line in proof_log.splitlines():
        line = line.lstrip()
        if not line.startswith("AXI_BURST_DUPLEX_PROOF "):
            continue
        try:
            record = json.loads(line[len("AXI_BURST_DUPLEX_PROOF "):])
        except (ValueError, TypeError):
            continue
        if record.get("owner_argument_index") in owner_ids:
            proof_records.append(record)
    need(len(proof_records) == 1,
         f"proof log must have one candidate-level record for root {root}; found {len(proof_records)}")
    drain_calls, _blocks = root_call_records(path, root)
    drain_block_match = re.search(r"\bbb_index:\s*(\d+)", nodes[drain_calls["drain_write"][0]][1])
    need(drain_block_match is not None, f"{path.name} root {root} DRAIN has no basic-block identity")
    need(proof_records[0].get("write_exit_bb") == int(drain_block_match.group(1)),
         f"{path.name} root {root} DRAIN is not in the proof-recorded write exit block")


def verify_call_contract(path: Path, roots: tuple[str, ...], positive_roots: set[str],
                         stage: str) -> None:
    for root in roots:
        calls = root_calls(path, root)
        counts = {label: sum(burst_call_label(callee) == label for callee in calls)
                  for label in BURST_CALLS}
        if root in positive_roots:
            need(counts == {label: 1 for label in BURST_CALLS},
                 f"{stage} root {root} must have exactly the five directional burst calls; got {counts}")
            if stage == "post-InterfaceInfer":
                verify_burst_call_arguments(path, root)
        else:
            need(not any(counts.values()), f"{stage} vetoed root {root} contains burst calls: {counts}")


def verify_legacy_access_contract(path: Path, access_directions: dict[str, set[str]]) -> None:
    for root, expected_directions in access_directions.items():
        calls = root_calls(path, root)
        observed = set()
        for name in calls:
            if re.search(r"(?:^|_)bambu_artificial_ParmMgr_Read$", name):
                observed.add("read")
            elif re.search(r"(?:^|_)bambu_artificial_ParmMgr_Write$", name):
                observed.add("write")
        need(expected_directions == observed,
             f"post-InterfaceInfer root {root} lacks legacy direction witness: "
             f"expected {sorted(expected_directions)}, observed {sorted(observed)}")


def verify_no_legacy_access_contract(path: Path, roots: set[str]) -> None:
    for root in roots:
        calls = root_calls(path, root)
        unexpected = [name for name in calls
                      if re.search(r"(?:^|_)bambu_artificial_ParmMgr_(?:Read|Write)$", name)]
        need(not unexpected,
             f"post-InterfaceInfer unused root {root} unexpectedly has legacy memory calls: {unexpected}")


def verify_unused_peer_veto(combined: str, expected_reason: str) -> None:
    candidate_records = re.findall(
        r'^\s*AXI_BURST_PREANALYSIS .*same_base_duplex_candidate=1 .*'
        r'production_activated=(\d) activation_reason="([^"]*)"\s*$',
        combined, re.MULTILINE)
    need(len(candidate_records) == 1 and candidate_records[0] == ("0", expected_reason),
         "same-base candidate was not vetoed application-wide for the expected unused-peer reason")
    need(not activation_markers(combined),
         "unused same-bundle peer did not veto production activation")
    proofs = []
    for line in combined.splitlines():
        line = line.lstrip()
        if not line.startswith("AXI_BURST_DUPLEX_PROOF "):
            continue
        try:
            proofs.append(json.loads(line[len("AXI_BURST_DUPLEX_PROOF "):]))
        except (ValueError, TypeError):
            continue
    need(any(record.get("semantic_proof") is True and
             record.get("semantic_reason") == "same-base pointwise read-before-write candidate"
             for record in proofs),
         "unused-peer veto fixture lacks its candidate-level positive semantic proof")
    need(any(record.get("semantic_proof") is False and
             record.get("semantic_reason") == expected_reason for record in proofs),
         "unused-peer veto fixture lacks its unused contribution proof record")


def pointer_argument_ids(path: Path, roots: tuple[str, ...]) -> set[int]:
    nodes = require_raw(path, path.name)
    functions = function_nodes(nodes)
    owner_ids: set[int] = set()
    for root in roots:
        need(root in functions and len(functions[root]) == 1,
             f"{path.name} lacks one selected owner function {root}")
        for argument in refs(nodes[functions[root][0]][1], "arg"):
            identifier = argument.lstrip("@")
            if not identifier.isdigit() or argument not in nodes:
                continue
            kind, attrs = nodes[argument]
            if kind != "argument_val_node":
                continue
            if any(type_ref in nodes and nodes[type_ref][0] == "pointer_ty_node"
                   for type_ref in refs(attrs, "type")):
                owner_ids.add(int(identifier))
    return owner_ids


def verify_preanalysis_veto(combined: str, expected_reason: str, reason_field: str,
                            expected_owners: set[int]) -> None:
    need(reason_field in {"same_base_duplex_reason", "activation_reason"},
         f"unsupported preanalysis veto field {reason_field}")
    pattern = re.compile(
        r'^\s*AXI_BURST_PREANALYSIS bundle=(\S+) owner_argument_index=(\d+) '
        r'.*same_base_duplex_candidate=(\d) same_base_duplex_reason="([^"]*)" '
        r'production_activated=(\d) activation_reason="([^"]*)"\s*$', re.MULTILINE)
    traces = list(pattern.finditer(combined))
    need(traces, "preanalysis veto record is missing")
    matched = []
    for trace in traces:
        bundle, owner_index, candidate, semantic_reason, activated, activation_reason = trace.groups()
        selected_reason = semantic_reason if reason_field == "same_base_duplex_reason" else activation_reason
        if (bundle == "shared" and int(owner_index) in expected_owners and
                selected_reason == expected_reason):
            matched.append((candidate, activated))
    need(matched and all(activated == "0" for _candidate, activated in matched),
         f"no shared-bundle preanalysis record for the selected pointer owner carries veto reason: {expected_reason}")


def verify_no_duplex_backend_rtl(output: Path) -> None:
    top = output / "top.v"
    if not top.exists() and not top.is_symlink():
        return
    need(not top.is_symlink() and top.is_file() and top.stat().st_size > 0,
         "vetoed multi-root top.v must be a nonempty regular file when emitted")
    code = re.sub(r"(?s)/\*.*?\*/|//[^\n]*", "", top.read_text(errors="replace"))
    modules = [match.group(3) for match in re.finditer(
        r"(?ms)^\s*module\s+([A-Za-z_$][\w$]*)\s*\((.*?)\);(.*?)^\s*endmodule\b", code)]
    need(modules, "vetoed multi-root top.v contains no parseable modules")
    aggregator_instances = [instance for body in modules
                            for module_type, instance, _ports in named_instances(body)
                            if re.search(r"_duplex_aggregator(?:_modgen)?$", module_type)]
    need(not aggregator_instances,
         "preanalysis-vetoed multi-root top.v unexpectedly instantiates a duplex aggregator")


def verify_architecture(path: Path, roots: tuple[str, ...], case: int) -> None:
    root = ET.parse(path).getroot()
    for symbol in roots:
        funcs = [node for node in root.findall(".//function") if node.get("symbol") == symbol]
        need(len(funcs) == 1, f"architecture XML must contain exactly one root {symbol}")
        if case == 17 and symbol == "unrelated_architectureless":
            bundles = funcs[0].findall("./bundles/bundle")
            bundle_modes = {bundle.get("name"): bundle.get("mode") for bundle in bundles}
            need(bundle_modes == {"data": "ptrdefault", "n": "default"},
                 "unrelated control must retain only its data=ptrdefault and n=default bundles")
            profile_fields = {"max_read_burst_length", "max_write_burst_length",
                              "num_read_outstanding", "read_fifo_depth", "write_burst_outstanding"}
            need(not any(bundle.get("mode") == "m_axi" for bundle in bundles) and
                 not any(profile_fields.intersection(bundle.attrib) for bundle in bundles),
                 "unrelated control acquired an m_axi bundle or read/write burst profile")
            continue
        if case == 16 and symbol == "matrix_candidate":
            shared = [bundle for bundle in funcs[0].findall("./bundles/bundle")
                      if bundle.get("name") == "shared"]
            profile_fields = {"max_read_burst_length", "max_write_burst_length",
                              "num_read_outstanding", "read_fifo_depth", "write_burst_outstanding"}
            need(len(shared) == 1 and shared[0].get("mode") == "ptrdefault" and
                 not profile_fields.intersection(shared[0].attrib),
                 "called wrapper must retain only its non-burst shared pointer bundle")
            continue
        bundles = funcs[0].findall("./bundles/bundle")
        if case == 11:
            selected = [bundle for bundle in bundles if bundle.get("name") == "DF_bambu_shared"]
        elif case in {9, 10} and symbol == "matrix_peer":
            selected = [bundle for bundle in bundles if bundle.get("name") == "peer_input"]
        elif case == 15 and symbol == "matrix_peer":
            selected = [bundle for bundle in bundles if bundle.get("name") == "other"]
        else:
            selected = [bundle for bundle in bundles if bundle.get("name") == "shared"]
        need(len(selected) == 1 and selected[0].get("mode") == "m_axi",
             f"root {symbol} lacks the intended explicit m_axi bundle architecture")
        if (case not in {5, 6, 15} or symbol != "matrix_peer") and not (
                case in {9, 10} and symbol == "matrix_peer"):
            need(selected[0].get("max_read_burst_length") is not None and
                 selected[0].get("max_write_burst_length") is not None,
                 f"root {symbol} does not preserve both directional burst requests")
        if case in {13, 14}:
            expected_read = ("16" if symbol == "matrix_candidate" else "8") if case == 13 else \
                ("8" if symbol == "matrix_candidate" else "16")
            need(selected[0].get("max_read_burst_length") == expected_read,
                 f"incompatible root {symbol} has wrong read profile; expected {expected_read}")
    task = DATAFLOW_TASKS.get(case)
    if task is not None:
        task_functions = [node for node in root.findall(".//function") if node.get("symbol") == task]
        need(len(task_functions) == 1 and task_functions[0].get("dataflow_module") == "1",
             f"architecture XML lacks one DATAFLOW module root {task}")
        task_bundles = [bundle for bundle in task_functions[0].findall("./bundles/bundle")
                        if bundle.get("name") == ("other" if case == 15 else "shared")]
        need(len(task_bundles) == 1 and task_bundles[0].get("mode") == "m_axi" and
             task_bundles[0].get("max_read_burst_length") is not None and
             task_bundles[0].get("max_write_burst_length") is not None,
             f"DATAFLOW task {task} lost its expected bidirectional m_axi profile")
    if case in {9, 10, 15}:
        peer_functions = [node for node in root.findall(".//function") if node.get("symbol") == "matrix_peer"]
        need(len(peer_functions) == 1 and peer_functions[0].get("dataflow_top") == "1",
             "architecture XML lacks the DATAFLOW peer top root")


def verify_incompatible_profiles(path: Path, roots: tuple[str, ...]) -> None:
    root = ET.parse(path).getroot()
    profile_fields = ("max_read_burst_length", "max_write_burst_length", "latency",
                      "num_read_outstanding", "read_fifo_depth", "write_burst_outstanding")
    profiles = []
    for symbol in roots:
        functions = [node for node in root.findall(".//function") if node.get("symbol") == symbol]
        need(len(functions) == 1, f"architecture XML must contain exactly one root {symbol}")
        bundles = [bundle for bundle in functions[0].findall("./bundles/bundle")
                   if bundle.get("name") == "shared"]
        need(len(bundles) == 1 and bundles[0].get("mode") == "m_axi",
             f"incompatible root {symbol} lacks one explicit shared m_axi profile")
        need(all(bundles[0].get(field) is not None for field in
                 ("max_read_burst_length", "max_write_burst_length")),
             f"incompatible root {symbol} lacks an explicit directional burst profile")
        profiles.append(tuple(bundles[0].get(field) for field in profile_fields))
    need(len(profiles) == 2 and profiles[0] != profiles[1],
         "incompatible root bundle profiles are not distinct")


def named_instances(body: str) -> list[tuple[str, str, dict[str, str]]]:
    instances = []
    pattern = re.compile(
        r"(?m)^\s*([A-Za-z_$][\w$]*)\s*(?:#\s*\((?:[^()]|\([^()]*\))*\)\s*)?"
        r"([A-Za-z_$][\w$]*)\s*\(")
    for match in pattern.finditer(body):
        open_index = match.end() - 1
        depth = 1
        close_index = open_index + 1
        while close_index < len(body) and depth:
            if body[close_index] == "(":
                depth += 1
            elif body[close_index] == ")":
                depth -= 1
            close_index += 1
        if depth:
            continue
        arguments = body[open_index + 1:close_index - 1]
        ports = {}
        for port_match in re.finditer(r"\.([A-Za-z_$][\w$]*)\s*\(", arguments):
            port_open = port_match.end() - 1
            port_depth = 1
            port_close = port_open + 1
            while port_close < len(arguments) and port_depth:
                if arguments[port_close] == "(":
                    port_depth += 1
                elif arguments[port_close] == ")":
                    port_depth -= 1
                port_close += 1
            if not port_depth:
                ports[port_match.group(1)] = arguments[port_open + 1:port_close - 1].strip()
        instances.append((match.group(1), match.group(2), ports))
    return instances


def verify_backend_rtl(path: Path, roots: tuple[str, ...], positive_roots: set[str]) -> None:
    need(path.is_file() and path.stat().st_size > 0, "multi-root backend guard did not leave top.v")
    code = re.sub(r"(?s)/\*.*?\*/|//[^\n]*", "", path.read_text(errors="replace"))
    modules = {}
    for match in re.finditer(r"(?ms)^\s*module\s+([A-Za-z_$][\w$]*)\s*\((.*?)\);(.*?)^\s*endmodule\b", code):
        modules[match.group(1)] = match.group(2) + "\n" + match.group(3)
    for root in positive_roots:
        module_name = next((candidate for candidate in (f"datapath_{root}", root)
                            if candidate in modules), None)
        need(module_name is not None, f"top.v has no generated module for activated root {root}")
        body = modules[module_name]
        all_instances = named_instances(body)
        aggregator_instances = [instance for instance in all_instances
                                if re.search(r"_duplex_aggregator(?:_modgen)?$", instance[0])]
        need(len(aggregator_instances) == 1,
             f"root {root} must instantiate exactly one duplex aggregator")
        aggregator_type, _aggregator_instance, aggregator_args = aggregator_instances[0]
        aggregator_modules = [name for name in modules
                              if name == aggregator_type or name.endswith("_" + aggregator_type)]
        need(len(aggregator_modules) == 1,
             f"root {root} aggregator module definition is missing or ambiguous")
        aggregator_body = modules[aggregator_modules[0]]
        for suffix in AXI_SUFFIXES:
            direction = "input" if suffix in READ_AXI_INPUTS else "output"
            need(re.search(rf"(?m)^\s*{direction}\s+(?:\[[^]]+\]\s+)?"
                           rf"p_m_axi_shared_{suffix}\s*;?\s*$", aggregator_body),
                 f"root {root} aggregator interface has wrong direction for {suffix}")
        endpoint_instances = {}
        for role in ("read_endpoint", "write_endpoint"):
            instances = [instance for instance in all_instances
                         if re.search(rf"_duplex_{role}(?:_modgen|_\d+_i\d+)?$", instance[0])]
            need(len(instances) == 1, f"root {root} must instantiate exactly one duplex {role} instance")
            endpoint_instances[role] = (instances[0][0], instances[0][2])
        for role in ("read_endpoint", "write_endpoint"):
            prefix = "read" if role == "read_endpoint" else "write"
            need(role in endpoint_instances,
                 f"root {root} lacks the {role} endpoint")
        operation_mappings = {
            "read_endpoint": ("configure_read", "burst_read"),
            "write_endpoint": ("configure_write", "store_write", "drain_write"),
        }
        for role, operations in operation_mappings.items():
            for operation in operations:
                pattern = (rf"\bfuselector_[A-Za-z0-9_$]*_duplex_{role}"
                           rf"(?:_\d+_i\d+)?(?:_[A-Za-z0-9_$]+)*_{operation}\b")
                need(re.search(pattern, body) is not None,
                     f"root {root} lacks duplex {role} operation mapping {operation}")
        for suffix in AXI_SUFFIXES:
            direction = "input" if suffix in READ_AXI_INPUTS else "output"
            need(re.search(rf"(?m)^\s*{direction}\s+(?:\[[^]]+\]\s+)?p_m_axi_shared_{suffix}\s*;?\s*$",
                           body), f"root {root} physical M_AXI_{suffix.upper()} direction is wrong")
        connections = aggregator_args
        for suffix in AXI_SUFFIXES:
            pin = f"p_m_axi_shared_{suffix}"
            need(connections.get(pin) == pin,
                 f"root {root} aggregator does not connect physical pin {suffix} to its bundle net")
        need(not re.search(r"(?m)^\s*(?:input|output)\b[^;\n]*\bduplex_", body),
             f"root {root} leaks private duplex endpoint ports")
        read_cores = list(_parameterized_instances(aggregator_body, "MinimalAXI4MasterPipelined"))
        write_cores = list(_parameterized_instances(aggregator_body, "MinimalAXI4MasterPipelinedWrite"))
        need(len(read_cores) == len(write_cores) == 1,
             f"root {root} must instantiate one independently configured read/write core pair")
        for core in (read_cores[0], write_cores[0]):
            need(core.get("B_MAX") == "16" and core.get("MAX_OUTSTANDING") == "1" and
                 core.get("FIFO_DEPTH") is not None,
                 f"root {root} burst core lost the selected profile parameters")
        need(read_cores[0].get("BITSIZE_m_axi_araddr") == "32" and
             write_cores[0].get("BITSIZE_m_axi_awaddr") == "32",
             f"root {root} burst core address profile is not 32-bit")
        verify_private_duplex_wiring(
            code, body,
            {"read_endpoint": endpoint_instances["read_endpoint"],
             "write_endpoint": endpoint_instances["write_endpoint"]},
            aggregator_type, aggregator_args,
            int(read_cores[0]["BITSIZE_m_axi_araddr"]),
            int(read_cores[0].get("BITSIZE_m_axi_rdata", "32")))
        need(not re.search(r"\b(?:MinimalAXI4AdapterSingleBeat|IOB_cache_axi)\b", body),
             f"activated root {root} RTL coexists with a legacy adapter")


def verify_backend_guard(combined: str, return_code: int) -> None:
    need(return_code > 0 and return_code < 128,
         "multi-root frontend gate must stop at the backend single-top guard without crashing")
    need("Expected single top function name (top_symbols.size() == 1)" in combined,
         "multi-root failed before the exact BackendWrapper single-top assertion")


def verify_root_order(combined: str, roots: tuple[str, ...], case: int) -> None:
    expected = tuple(reversed(roots)) if case in REVERSED else roots
    seen = re.findall(r"Analyzing function ([^\s]+)", combined)
    need(tuple(seen) == expected,
         f"frontend root set/order mismatch: expected {expected}, observed {tuple(seen)}")


def verify_completed_root_order(combined: str, roots: tuple[str, ...]) -> None:
    analyzing = tuple(re.findall(r"Analyzing function ([^\s]+)", combined))
    analyzed = tuple(re.findall(r"Analyzed function ([^\s]+)", combined))
    need(analyzing == roots and analyzed == roots,
         f"InterfaceInfer did not complete roots in order: expected {roots}, "
         f"analyzing={analyzing}, analyzed={analyzed}")
    guard = re.search(r"Expected single top function name \(top_symbols\.size\(\) == 1\)", combined)
    need(guard is not None and combined.rfind(f"Analyzed function {roots[-1]}") < guard.start(),
         "BackendWrapper single-top guard occurred before all case 16 roots completed InterfaceInfer")


def verify_dataflow_analysis_order(combined: str, case: int) -> None:
    expected = DATAFLOW_ANALYSIS_ORDER[case]
    analyzing = tuple(re.findall(r"Analyzing function ([^\s]+)", combined))
    analyzed = tuple(re.findall(r"Analyzed function ([^\s]+)", combined))
    need(analyzing == expected,
         f"DATAFLOW frontend root set/order mismatch: expected {expected}, observed {analyzing}")
    need(analyzed == expected,
         f"InterfaceInfer did not complete the expected DATAFLOW roots: expected {expected}, observed {analyzed}")


def verify_dataflow_task_call(path: Path, peer: str, task: str) -> None:
    nodes = require_raw(path, path.name)
    funcs = function_nodes(nodes)
    need(peer in funcs and len(funcs[peer]) == 1, f"{path.name} lacks one DATAFLOW peer root {peer}")
    need(task in funcs and len(funcs[task]) == 1, f"{path.name} lacks one DATAFLOW task root {task}")
    peer_function, task_function = funcs[peer][0], funcs[task][0]
    peer_args = refs(nodes[peer_function][1], "arg")
    task_args = refs(nodes[task_function][1], "arg")
    need(peer_args and task_args, f"{path.name} DATAFLOW call lacks pointer parameters")
    names = call_names(nodes)
    task_calls = [statement for statement, callee in names.items()
                  if callee == task and refs(nodes[statement][1], "parent") == [peer_function]]
    need(len(task_calls) == 1,
         f"{path.name} must preserve exactly one {peer}-to-{task} call")
    call_args = refs(nodes[task_calls[0]][1], "arg")
    need(len(call_args) == len(task_args) and call_args[0] in nodes and
         nodes[call_args[0]][0] == "ssa_node",
         f"{path.name} {peer}-to-{task} call has no direct forwarded pointer argument")
    forwarded_var = refs(nodes[call_args[0]][1], "var")
    need(forwarded_var == [peer_args[0]],
         f"{path.name} {peer}-to-{task} call does not forward the peer pointer unchanged")
    peer_type = refs(nodes[peer_args[0]][1], "type")
    task_type = refs(nodes[task_args[0]][1], "type")
    call_type = refs(nodes[call_args[0]][1], "type")
    need(peer_type and peer_type == task_type == call_type,
         f"{path.name} {peer}-to-{task} forwarded pointer type differs from the task parameter")


def verify_called_pointer_forwarding(path: Path, caller: str, callee: str) -> None:
    nodes = require_raw(path, path.name)
    funcs = function_nodes(nodes)
    need(caller in funcs and len(funcs[caller]) == 1,
         f"{path.name} lacks one caller function {caller}")
    need(callee in funcs and len(funcs[callee]) == 1,
         f"{path.name} lacks one callee function {callee}")
    caller_fn, callee_fn = funcs[caller][0], funcs[callee][0]
    caller_args, callee_args = refs(nodes[caller_fn][1], "arg"), refs(nodes[callee_fn][1], "arg")
    need(caller_args and callee_args, f"{path.name} {caller}-to-{callee} call lacks pointer parameters")
    names = call_names(nodes)
    calls = [statement for statement, name in names.items()
             if name == callee and refs(nodes[statement][1], "parent") == [caller_fn]]
    need(len(calls) == 1, f"{path.name} must preserve exactly one {caller}-to-{callee} call")
    call_args = refs(nodes[calls[0]][1], "arg")
    need(len(call_args) == len(callee_args) and call_args[0] in nodes and
         nodes[call_args[0]][0] == "ssa_node",
         f"{path.name} {caller}-to-{callee} call lacks a direct forwarded pointer")
    need(refs(nodes[call_args[0]][1], "var") == [caller_args[0]],
         f"{path.name} {caller}-to-{callee} call does not forward the pointer unchanged")
    caller_type = refs(nodes[caller_args[0]][1], "type")
    callee_type = refs(nodes[callee_args[0]][1], "type")
    call_type = refs(nodes[call_args[0]][1], "type")
    need(caller_type and caller_type == callee_type == call_type,
         f"{path.name} {caller}-to-{callee} forwarded pointer type differs from the callee parameter")


def verify_dataflow_task_memory(path: Path, task: str) -> None:
    nodes = require_raw(path, path.name)
    funcs = function_nodes(nodes)
    need(task in funcs and len(funcs[task]) == 1, f"{path.name} lacks one DATAFLOW task {task}")
    task_function = funcs[task][0]
    reads = []
    writes = []
    for statement, (kind, attrs) in nodes.items():
        if not kind.endswith("stmt") or refs(attrs, "parent") != [task_function]:
            continue
        memory_ops = [operand for operand in refs(attrs, "op")
                      if operand in nodes and nodes[operand][0] == "mem_access_node" and
                      "ordinary_memory_access" in nodes[operand][1]]
        if not memory_ops:
            continue
        if refs(attrs, "vuse"):
            reads.append(statement)
        if refs(attrs, "vdef"):
            writes.append(statement)
    need(len(reads) == 1 and len(writes) == 1,
         f"{path.name} DATAFLOW task must preserve one ordinary LOAD and one STORE; "
         f"observed reads={len(reads)} writes={len(writes)}")


def verify_legacy_bundle_contract(path: Path, expected: dict[str, dict[str, set[str]]]) -> None:
    for root, bundle_directions in expected.items():
        calls = root_calls(path, root)
        observed = set()
        for callee in calls:
            match = re.search(r"(?:^|_)([A-Za-z0-9_$]+)_bambu_artificial_ParmMgr_(Read|Write)$", callee)
            if match:
                observed.add((match.group(1), match.group(2).lower()))
        required = {(bundle, direction) for bundle, directions in bundle_directions.items()
                    for direction in directions}
        need(observed == required,
             f"post-InterfaceInfer root {root} has wrong legacy bundle accesses: "
             f"expected {sorted(required)}, observed {sorted(observed)}")


def verify_dataflow_candidate_trace(before: Path, combined: str, candidate: str,
                                    activated: bool, reason: str) -> None:
    nodes = require_raw(before, before.name)
    funcs = function_nodes(nodes)
    need(candidate in funcs and len(funcs[candidate]) == 1,
         f"{before.name} lacks one candidate function {candidate}")
    argument_ids = {int(ref.lstrip("@")) for ref in refs(nodes[funcs[candidate][0]][1], "arg")
                    if ref.lstrip("@").isdigit()}
    traces = []
    for line in combined.splitlines():
        if "AXI_BURST_PREANALYSIS " not in line:
            continue
        owner = re.search(r"\bowner_argument_index=(\d+)", line)
        same_base = re.search(r"\bsame_base_duplex_candidate=(\d)", line)
        production = re.search(r'\bproduction_activated=(\d) activation_reason="([^"]*)"', line)
        if owner and int(owner.group(1)) in argument_ids and same_base and production:
            traces.append((same_base.group(1), production.group(1), production.group(2)))
    expected_trace = ("1", "1" if activated else "0", reason)
    need(traces == [expected_trace],
         f"candidate {candidate} DATAFLOW activation trace mismatch: expected {expected_trace}, observed {traces}")
    proofs = []
    for line in combined.splitlines():
        line = line.strip()
        if not line.startswith("AXI_BURST_DUPLEX_PROOF "):
            continue
        try:
            record = json.loads(line[len("AXI_BURST_DUPLEX_PROOF "):])
        except (ValueError, TypeError):
            continue
        if record.get("owner_argument_index") in argument_ids:
            proofs.append(record)
    need(len(proofs) == 1 and proofs[0].get("semantic_proof") is True and
         proofs[0].get("semantic_reason") == "same-base pointwise read-before-write candidate" and
         proofs[0].get("profile_eligible") is True and
         proofs[0].get("profile_reason") == "read/write profile is eligible",
         f"candidate {candidate} lacks its unique positive semantic and profile proof record")


def verify_case16_called_contributions(before: Path, combined: str) -> None:
    nodes = require_raw(before, before.name)
    funcs = function_nodes(nodes)
    reason = "bundle contribution is not an independent entry root"
    for owner in ("matrix_helper", "matrix_peer"):
        need(owner in funcs and len(funcs[owner]) == 1,
             f"case 16 lacks one semantic/profile owner function {owner}")
        owner_args = refs(nodes[funcs[owner][0]][1], "arg")
        need(owner_args, f"case 16 owner {owner} has no pointer argument")
        pointer_owner = int(owner_args[0].lstrip("@"))
        owner_traces = []
        for line in combined.splitlines():
            if "AXI_BURST_PREANALYSIS " not in line:
                continue
            match = re.search(
                r'^\s*AXI_BURST_PREANALYSIS bundle=shared owner_argument_index=(\d+) '
                r'.*same_base_duplex_candidate=1 same_base_duplex_reason="same-base pointwise '
                r'read-before-write candidate" production_activated=0 activation_reason="([^"]*)"\s*$',
                line)
            if match and int(match.group(1)) == pointer_owner:
                owner_traces.append(match.group(2))
        need(owner_traces == [reason],
             f"case 16 {owner} contribution lacks the exact independent-entry-root veto")
        proof_records = []
        for line in combined.splitlines():
            line = line.strip()
            if not line.startswith("AXI_BURST_DUPLEX_PROOF "):
                continue
            try:
                record = json.loads(line[len("AXI_BURST_DUPLEX_PROOF "):])
            except (ValueError, TypeError):
                continue
            if record.get("owner_argument_index") == pointer_owner:
                proof_records.append(record)
        need(len(proof_records) == 1 and
             proof_records[0].get("semantic_proof") is True and
             proof_records[0].get("semantic_reason") ==
             "same-base pointwise read-before-write candidate" and
             proof_records[0].get("profile_eligible") is True and
             proof_records[0].get("profile_reason") == "read/write profile is eligible",
             f"case 16 {owner} lacks its positive semantic and compatible-profile proof")
    need(not activation_markers(combined),
         "case 16 called contribution unexpectedly activated a duplex bundle")


def verify_case16_profiles(path: Path) -> None:
    root = ET.parse(path).getroot()
    fields = ("max_read_burst_length", "max_write_burst_length", "latency",
              "num_read_outstanding", "read_fifo_depth", "write_burst_outstanding")
    profiles = []
    for symbol in ("matrix_helper", "matrix_peer"):
        functions = [node for node in root.findall(".//function") if node.get("symbol") == symbol]
        need(len(functions) == 1, f"case 16 architecture lacks one function {symbol}")
        bundles = [node for node in functions[0].findall("./bundles/bundle")
                   if node.get("name") == "shared"]
        need(len(bundles) == 1 and bundles[0].get("mode") == "m_axi" and
             all(bundles[0].get(field) is not None for field in fields),
             f"case 16 {symbol} lacks a complete explicit shared m_axi profile")
        profiles.append(tuple(bundles[0].get(field) for field in fields))
    need(profiles[0] == profiles[1],
         "case 16 helper and independent peer do not have compatible shared profiles")


def verify(case: int, output: Path, return_code: int) -> None:
    need(case in ALL_CASES, f"unknown matrix case {case}")
    stdout_path, stderr_path = output / "stdout.log", output / "stderr.log"
    need(stdout_path.is_file() and stderr_path.is_file(), "compiler logs are missing")
    stdout, stderr = stdout_path.read_text(errors="replace"), stderr_path.read_text(errors="replace")
    combined = stdout + "\n" + stderr
    roots = EXPECTED_ROOTS.get(case, SINGLE_ROOTS.get(case))
    need(roots is not None, "selected roots are not defined")
    if case in INCOMPATIBLE:
        need(return_code > 0 and return_code < 128, "incompatible profiles must fail as a usage error, not crash")
        need(combined.count(INCOMPATIBLE_PROFILE_DIAGNOSTIC) == 1,
             "exact incompatible-profile diagnostic is missing or duplicated")
        need(not activation_markers(combined),
             "incompatible profiles partially activated before rejection")
        need(not [path for path in output.glob("*.v") if path.name != "panda_libtech.v"],
             "incompatible profiles emitted top RTL")
        before = output / "panda-temp/before_Frontend::InterfaceInfer.raw"
        need(not before.is_symlink() and before.is_file() and before.stat().st_size > 0,
             "incompatible-profile pre-InterfaceInfer raw IR is missing or empty")
        verify_call_contract(before, roots, set(), "pre-InterfaceInfer incompatible profile")
        architecture = output / "panda-temp/architecture.xml"
        need(not architecture.is_symlink() and architecture.is_file() and architecture.stat().st_size > 0,
             "incompatible-profile architecture XML is missing or empty")
        verify_incompatible_profiles(architecture, roots)
        verify_architecture(architecture, roots, case)
        return

    if case in DATAFLOW_ANALYSIS_ORDER:
        verify_dataflow_analysis_order(combined, case)
    else:
        verify_root_order(combined, roots, case)
    if case == 16:
        verify_completed_root_order(combined, roots)

    before = output / "panda-temp/before_Frontend::InterfaceInfer.raw"
    after = output / "panda-temp/after_Frontend::InterfaceInfer.raw"
    require_raw(before, "pre-InterfaceInfer")
    require_raw(after, "post-InterfaceInfer")
    positive_roots = POSITIVE_ROOTS.get(case, set())
    verify_call_contract(before, roots, set(), "pre-InterfaceInfer")
    verify_call_contract(after, roots, positive_roots, "post-InterfaceInfer")
    task = DATAFLOW_TASKS.get(case)
    if task is not None:
        verify_call_contract(before, (task,), set(), "pre-InterfaceInfer DATAFLOW task")
        verify_call_contract(after, (task,), set(), "post-InterfaceInfer DATAFLOW task")
        peer = "matrix_peer"
        verify_dataflow_task_call(before, peer, task)
        verify_dataflow_task_call(after, peer, task)
        verify_dataflow_task_memory(before, task)
    verify_legacy_access_contract(after, LEGACY_ACCESS_DIRECTIONS.get(case, {}))
    if case in {9, 10, 15}:
        expected_legacy_bundles = (
            {"matrix_candidate": {"shared": {"read", "write"}},
             task: {"peer_input": {"read", "write"}}}
            if case in {9, 10} else
            {task: {"other": {"read", "write"}}})
        verify_legacy_bundle_contract(after, expected_legacy_bundles)
        verify_no_legacy_access_contract(after, {"matrix_peer"})
        if case in {9, 10}:
            verify_dataflow_candidate_trace(before, combined, "matrix_candidate", False, VETO_REASON[case])
            need(not activation_markers(combined),
                 "DATAFLOW/forwarded candidate unexpectedly activated a duplex bundle")
        else:
            verify_dataflow_candidate_trace(
                before, combined, "matrix_candidate", True,
                "frozen application-wide proof and profile accepted")
            need(activation_markers(combined) == [("shared", "matrix_candidate")],
                 "case 15 must activate only the independent candidate shared bundle")
    if case == 16:
        verify_called_pointer_forwarding(before, "matrix_candidate", "matrix_helper")
        verify_called_pointer_forwarding(after, "matrix_candidate", "matrix_helper")
        verify_case16_called_contributions(before, combined)
    if case in {7, 8}:
        verify_no_legacy_access_contract(after, {"matrix_peer"})
        verify_unused_peer_veto(combined, VETO_REASON[case])
    for root in positive_roots:
        verify_drain_exit(after, combined, root)
    if case in VETO_REASON and case not in {9, 10, 11, 12}:
        owners = pointer_argument_ids(before, roots)
        verify_preanalysis_veto(combined, VETO_REASON[case],
                                VETO_REASON_FIELD.get(case, "same_base_duplex_reason"), owners)
    architecture = output / "panda-temp/architecture.xml"
    need(architecture.is_file() and architecture.stat().st_size > 0, "multi-root architecture XML is missing")
    verify_architecture(architecture, roots, case)
    if case == 16:
        verify_case16_profiles(architecture)

    multi_root = len(roots) > 1
    if multi_root:
        verify_backend_guard(combined, return_code)
        if positive_roots:
            for root in positive_roots:
                need(activation_markers(combined).count(("shared", root)) == 1,
                     f"root {root} lacks exactly one production activation marker")
            verify_backend_rtl(output / "top.v", roots, positive_roots)
        else:
            verify_no_duplex_backend_rtl(output)
    else:
        need(return_code == 0, "single-root legacy control did not compile successfully")
        if case == 11:
            need(not activation_markers(combined),
                 "reserved DF_bambu_ bundle unexpectedly activated")
            rtl = output / "matrix_candidate.v"
            need(rtl.is_file() and re.search(r"\b(?:MinimalAXI4AdapterSingleBeat|IOB_cache_axi)\b",
                                             rtl.read_text(errors="replace")),
                 "reserved bundle did not retain legacy RTL")
        elif case == 12:
            need(not activation_markers(combined),
                 "feature-off control unexpectedly activated")
            rtl = output / "matrix_candidate.v"
            need(rtl.is_file() and re.search(r"\b(?:MinimalAXI4AdapterSingleBeat|IOB_cache_axi)\b",
                                             rtl.read_text(errors="replace")),
                 "feature-off control did not retain legacy RTL")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("case", type=int)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--return-code", required=True, type=int)
    args = parser.parse_args()
    try:
        verify(args.case, args.output, args.return_code)
    except (OSError, ValueError, ET.ParseError) as exc:
        print(f"DUPLEX ACTIVATION MATRIX CHECK FAILED: {exc}", file=sys.stderr)
        return 1
    print(f"DUPLEX ACTIVATION MATRIX CHECK PASS CASE={args.case}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
