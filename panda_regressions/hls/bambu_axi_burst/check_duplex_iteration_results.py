#!/usr/bin/env python3
"""Fail-closed mapping from a scheduled copy loop to generated controller states."""
from __future__ import annotations

import argparse
import hashlib
import html
import json
from pathlib import Path
import re
import sys
import xml.etree.ElementTree as ET

# Share the compiler-artifact parser with the native HLS regression family.
HLS_FAMILY = Path(__file__).resolve().parents[3] / "panda_regressions/hls/bambu_axi_burst"
sys.path.insert(0, str(HLS_FAMILY))
from check_duplex_effect_analysis_artifacts import CheckError, fields, function_graph, statement_block


def need(condition: bool, message: str) -> None:
    if not condition:
        raise CheckError(message)


def read_text(path: Path) -> str:
    need(path.is_file() and not path.is_symlink() and path.stat().st_size > 0,
         f"required nonempty artifact missing: {path}")
    return path.read_text(encoding="utf-8", errors="strict")


def module_body(rtl: str, name: str) -> str:
    modules = re.findall(rf"(?ms)^module\s+{re.escape(name)}\s*\((.*?)\);(.*?)^endmodule\b", rtl)
    need(len(modules) == 1, f"expected exactly one RTL module {name}, found {len(modules)}")
    return modules[0][1]


def instance_ports(body: str, instance: str) -> tuple[str, dict[str, str]]:
    matches = list(re.finditer(
        rf"(?ms)^\s*([A-Za-z_]\w*)\s*(?:#\s*\([^;]*?\)\s*)?{re.escape(instance)}\s*\(([^;]*?)\);",
        body))
    need(len(matches) == 1, f"expected one instance named {instance}, found {len(matches)}")
    ports: dict[str, str] = {}
    for port, signal in re.findall(r"\.(\w+)\s*\(\s*([^()]+?)\s*\)", matches[0].group(2)):
        need(port not in ports, f"duplicate .{port} port on instance {instance}")
        ports[port] = signal.strip()
    return matches[0].group(1), ports


def _dot_nodes(path: Path) -> tuple[dict[int, str], list[tuple[int, int, str]]]:
    text = read_text(path)
    nodes: dict[int, str] = {}
    edges: list[tuple[int, int, str]] = []
    for line in text.splitlines():
        node = re.match(r'^\s*(\d+)\s*\[.*?label="(.*)"\];\s*$', line)
        if node:
            ident = int(node.group(1))
            need(ident not in nodes, f"{path}: duplicate DOT node {ident}")
            nodes[ident] = html.unescape(node.group(2)).replace(r"\n", "\n").replace(r"\l", "\n")
            continue
        edge = re.match(r'^\s*(\d+)\s*->\s*(\d+)\s*\[.*?label="(.*?)".*\];\s*$', line)
        if edge:
            edges.append((int(edge.group(1)), int(edge.group(2)), html.unescape(edge.group(3))))
    need(nodes, f"{path}: no operation nodes parsed")
    return nodes, edges


def _definitions(nodes: dict[int, str]) -> dict[int, str]:
    return {ident: text.replace("\\n", "\n").replace("\\l", "\n") for ident, text in nodes.items()}


def _instruction_blocks(ir: str, top: str) -> dict[str, list[str]]:
    functions = re.findall(rf"(?ms)^define\b[^\n]*@{re.escape(top)}\([^\n]*\).*?^\}}", ir)
    need(len(functions) == 1, f"raw IR must contain exactly one definition of @{top}")
    body = functions[0]
    blocks: dict[str, list[str]] = {}
    current: str | None = None
    for line in body.splitlines():
        label = re.match(r"^\s*(\d+):(?:\s*;.*)?\s*$", line)
        if label:
            current = label.group(1)
            need(current not in blocks, f"raw IR has duplicate basic block {current}")
            blocks[current] = []
        elif current is not None and line.strip() and not line.lstrip().startswith(";"):
            blocks[current].append(line.strip())
    return blocks


def _canonical_ir_loop(ir: str, top: str) -> dict[str, str]:
    candidates: list[dict[str, str]] = []
    for block, lines in _instruction_blocks(ir, top).items():
        instructions = lines
        for line in instructions:
            phi = re.match(r"%([\w.]+)\s*=\s*phi\s+\w+\s+(.+)$", line)
            if not phi:
                continue
            phi_value, incoming = phi.groups()
            backedges = re.findall(r"\[\s*%([\w.]+)\s*,\s*%?(\d+)\s*\]", incoming)
            starts = re.findall(r"\[\s*(-?\d+)\s*,\s*%?(\d+)\s*\]", incoming)
            if (len(backedges) != 1 or backedges[0][1] != block or len(starts) != 1 or
                    starts[0][1] == block):
                continue
            increment, _ = backedges[0]
            inc_line = next((row for row in instructions if re.match(
                rf"%{re.escape(increment)}\s*=\s*add\b.*%{re.escape(phi_value)}\s*,\s*1\b", row)), None)
            if not inc_line:
                continue
            loads = [row for row in instructions if re.search(r"\bload\s+", row)]
            stores = [row for row in instructions if re.search(r"\bstore\s+", row)]
            if len(loads) != 1 or len(stores) != 1:
                continue
            load_result = re.match(r"%([\w.]+)\s*=\s*load\b.*?,\s*ptr\s+(%[\w.]+)|%([\w.]+)\s*=\s*load\b.*?,\s*\w+\*\s*(%[\w.]+)", loads[0])
            store = re.match(r"store\b.*?\s(%[\w.]+),\s*(?:(?:\w+\s*\*|ptr)\s+)?(%[\w.]+)", stores[0])
            if not load_result or not store:
                continue
            loaded = load_result.group(1) or load_result.group(3)
            load_ptr = load_result.group(2) or load_result.group(4)
            gep_defs = {}
            for row in instructions:
                gep = re.match(r"%([\w.]+)\s*=\s*getelementptr\b.*?,\s*i\d+\s+(%[\w.]+)\s*$", row)
                if gep:
                    gep_defs["%" + gep.group(1)] = gep.group(2)
            if load_ptr not in gep_defs or store.group(2) not in gep_defs:
                continue
            if store.group(1) != "%" + loaded:
                continue
            if gep_defs[load_ptr] != "%" + phi_value or gep_defs[store.group(2)] != "%" + phi_value:
                continue
            candidates.append({"ir_block": block, "phi": "%" + phi_value,
                               "increment": "%" + increment, "load": loads[0],
                               "store": stores[0], "load_pointer": load_ptr,
                               "store_pointer": store.group(2),
                               "initial_value": starts[0][0]})
    need(len(candidates) == 1,
         f"raw IR must identify exactly one canonical induction loop with one indexed LOAD/STORE; found {len(candidates)}")
    return candidates[0]


def _pandA_loop_identity(generated: Path, top: str, scheduled_bb: int,
                         operation_ids: dict[str, int]) -> int:
    raw_paths = sorted(path for path in (generated / "panda-temp").iterdir()
                       if re.fullmatch(rf"after_Frontend::BuildVirtualPhi::{re.escape(top)}\(\d+\)\[\d+\]\.raw",
                                       path.name))
    need(len(raw_paths) == 1,
         f"expected exactly one BuildVirtualPhi raw dump for {top}; found {len(raw_paths)}")
    raw_path = raw_paths[0]
    nodes, function, blocks, reachable = function_graph(read_text(raw_path), top)
    loop_blocks = [(bb, info) for bb, info in blocks.items() if info[0] != 0]
    need(len(loop_blocks) == 1,
         f"{raw_path}: expected exactly one canonical loop block; found {len(loop_blocks)}")
    loop_bb, (loop_id, attrs, statement_ids, _preds, _succs) = loop_blocks[0]
    need(loop_bb == scheduled_bb,
         f"PandA raw loop BB{loop_bb} does not match scheduled loop BB{scheduled_bb}")
    phi_ids = [int(value) for value in re.findall(r"\bphi: @(\d+)", attrs)]
    statements = set(statement_ids) | set(phi_ids)
    for role, operation_id in operation_ids.items():
        need(operation_id in statements,
             f"scheduled {role} operation {operation_id} is not a statement in PandA loop BB{loop_bb}")
        if role == "phi":
            phi_node = nodes.get(operation_id)
            need(phi_node is not None and phi_node[0] == "phi_stmt" and
                 str(function) in fields(phi_node).get("parent", []) and
                 re.search(rf"\bbb_index: {loop_bb}\b", phi_node[1]) is not None,
                 f"scheduled phi operation @{operation_id} is not in canonical PandA loop BB{loop_bb}")
            statement_bb, statement_loop = loop_bb, loop_id
        else:
            statement_bb, statement_loop, _ = statement_block(
                nodes, blocks, function, reachable, operation_id, f"scheduled {role} operation")
        need(statement_bb == loop_bb and statement_loop == loop_id,
             f"scheduled {role} operation {operation_id} is not in the canonical PandA loop")

    induction_candidates = []
    for phi_id in phi_ids:
        phi_node = nodes.get(phi_id)
        if not phi_node or phi_node[0] != "phi_stmt":
            continue
        phi_fields = fields(phi_node)
        results = phi_fields.get("res", [])
        definitions = phi_fields.get("def", [])
        edges = re.findall(r"\bedge: (\d+)", phi_node[1])
        if len(results) != 1 or len(definitions) != len(edges) or len(definitions) < 2:
            continue
        result_id = int(results[0])
        backedges = [(int(definition), int(edge)) for definition, edge in zip(definitions, edges)
                     if int(edge) == loop_bb]
        initial = [(int(definition), int(edge)) for definition, edge in zip(definitions, edges)
                   if int(edge) != loop_bb]
        if len(backedges) != 1 or len(initial) != 1:
            continue
        initial_node = nodes.get(initial[0][0])
        initial_value = re.search(r"\bvalue: (-?\d+)", initial_node[1]) if initial_node else None
        if not initial_node or initial_node[0] != "constant_int_val_node" or not initial_value or initial_value.group(1) != "0":
            continue
        backedge_id = backedges[0][0]
        backedge_node = nodes.get(backedge_id)
        if not backedge_node or backedge_node[0] != "ssa_node":
            continue
        increments = []
        for stmt_id in statement_ids:
            stmt_node = nodes.get(stmt_id)
            if not stmt_node or stmt_node[0] != "assign_stmt":
                continue
            operands = fields(stmt_node).get("op", [])
            if len(operands) != 2 or int(operands[0]) != backedge_id:
                continue
            add_node = nodes.get(int(operands[1]))
            if not add_node or add_node[0] != "add_node":
                continue
            add_operands = fields(add_node).get("op", [])
            if len(add_operands) != 2:
                continue
            if str(result_id) not in add_operands:
                continue
            constants = [nodes.get(int(value)) for value in add_operands]
            if not any(node and node[0] == "constant_int_val_node" and
                       re.search(r"\bvalue: 1\b", node[1]) for node in constants):
                continue
            if not any(int(value) == result_id for value in add_operands):
                continue
            if fields(backedge_node).get("def_stmt") != [str(stmt_id)]:
                continue
            increments.append(stmt_id)
        if len(increments) == 1:
            induction_candidates.append((phi_id, increments[0]))
    need(len(induction_candidates) == 1,
         f"PandA raw loop BB{loop_bb} must contain one zero-based phi with unit feedback; found {len(induction_candidates)}")
    phi_id, increment_id = induction_candidates[0]
    need(operation_ids.get("phi") == phi_id and operation_ids.get("increment") == increment_id,
         "scheduled phi/increment operation identities do not match the PandA raw loop")

    load_id = operation_ids["load"]
    load_node = nodes[load_id]
    need(load_node[0] == "assign_stmt", "scheduled LOAD identity is not a PandA assignment statement")
    load_ops = fields(load_node).get("op", [])
    load_call = next((nodes.get(int(value)) for value in load_ops
                      if nodes.get(int(value)) and nodes[int(value)][0] == "call_node"), None)
    need(load_call is not None, "scheduled LOAD identity does not resolve to a PandA call")
    load_name = _raw_function_name(nodes, fields(load_call).get("fn", []))
    need("burst_read" in load_name, "scheduled LOAD identity is not the burst-read call")

    store_id = operation_ids["store"]
    store_node = nodes[store_id]
    need(store_node[0] == "call_stmt", "scheduled STORE identity is not a PandA call statement")
    store_name = _raw_function_name(nodes, fields(store_node).get("fn", []))
    need("store_write" in store_name, "scheduled STORE identity is not the burst-write call")
    return function


def _raw_function_name(nodes: dict[int, tuple[str, str]], function_refs: list[str]) -> str:
    if len(function_refs) != 1:
        return ""
    address = nodes.get(int(function_refs[0]))
    if not address or address[0] != "addr_node":
        return ""
    targets = fields(address).get("op", [])
    if len(targets) != 1:
        return ""
    function = nodes.get(int(targets[0]))
    if not function or function[0] != "function_val_node":
        return ""
    names = fields(function).get("name", [])
    if len(names) != 1:
        return ""
    identifier = nodes.get(int(names[0]))
    if not identifier or identifier[0] != "identifier_node":
        return ""
    values = re.findall(r'\bstrg: "([^"]*)"', identifier[1])
    return values[0] if len(values) == 1 else ""


def _scheduled_loop(results_path: Path, top: str) -> tuple[str, int, int, int]:
    try:
        root = ET.parse(results_path).getroot()
    except ET.ParseError as exc:
        raise CheckError(f"invalid Bambu results XML: {exc}") from exc
    funcs = [node for node in root.iter() if node.tag.startswith("function_") and node.get("name") == top]
    need(len(funcs) == 1, f"results XML must contain exactly one function record for {top}")
    function = funcs[0]
    function_ii = int(function.get("function_ii", "-1"))
    loops = [node for node in function if re.fullmatch(r"loop_\d+", node.tag)]
    need(len(loops) == 1, f"expected exactly one scheduled loop for {top}; found {len(loops)}")
    loop = loops[0]
    loop_id = loop.tag
    bb = int(loop.get("bb", "-1"))
    ii = int(loop.get("ii", "-1"))
    need(bb >= 0 and ii > 0, f"{loop_id} must have a basic block and positive scheduled II")
    return loop_id, bb, ii, function_ii


def _state_map(fsm_path: Path, loop_bb: int) -> dict[str, set[tuple[int, int]]]:
    text = read_text(fsm_path)
    states: dict[str, set[tuple[int, int]]] = {}
    for state, label in re.findall(r"(?m)^(S_\d+)\s+\[label=<(.*?)>\];\s*$", text):
        bb_match = re.search(r"\|\s*BB(\d+)\s*$", label)
        if not bb_match or int(bb_match.group(1)) != loop_bb:
            continue
        ops = {(int(function_id), int(operation_id)) for function_id, operation_id in
               re.findall(r"kernel_(\d+)_(\d+)\s*\[", label)}
        need(state not in states, f"{fsm_path}: duplicate state {state}")
        states[state] = ops
    need(states, f"{fsm_path}: no FSM states mapped to loop BB{loop_bb}")
    return states


def _fu_nodes(nodes: dict[int, str], pattern: str) -> list[tuple[int, str]]:
    return [(ident, text) for ident, text in nodes.items() if re.search(pattern, text, re.I)]


def _rtl_instance_body(body: str, instance: str) -> tuple[str, dict[str, str]]:
    module, ports = instance_ports(body, instance)
    return module, ports


def _combinational_reaches(body: str, source: str, target: str) -> bool:
    instances = list(re.finditer(
        r"(?ms)^\s*([A-Za-z_]\w*)\s*(?:#\s*\([^;]*?\)\s*)?([A-Za-z_]\w*)\s*\(([^;]*?)\);", body))
    edges: dict[str, set[str]] = {}
    for match in instances:
        module, _, raw_ports = match.groups()
        ports = {name: signal.strip() for name, signal in
                 re.findall(r"\.(\w+)\s*\(\s*([^()]+?)\s*\)", raw_ports)}
        outputs = [signal for name, signal in ports.items() if name in {"out1", "duplex_address"}]
        inputs = [signal for name, signal in ports.items()
                  if re.fullmatch(r"in\d+", name) or name in {"sel", "sop", "ops"}]
        for input_net in inputs:
            for output_net in outputs:
                edges.setdefault(input_net, set()).add(output_net)
    pending = [source]
    visited = set()
    while pending:
        current = pending.pop()
        if current == target:
            return True
        if current in visited:
            continue
        visited.add(current)
        pending.extend(edges.get(current, ()))
    return False


def _rtl_binding(rtl: str, top: str, increment_id: int, shift_id: int,
                 src_gep_id: int, dst_gep_id: int, function_id: int) -> dict[str, str]:
    top_body = module_body(rtl, top)
    pbi_instances = list(re.finditer(r"(?m)^\s*(PBI_\w+)\s+(\w+)\s*\(", top_body))
    need(len(pbi_instances) == 1, f"{top} must instantiate exactly one PBI module")
    pbi_module, pbi_instance = pbi_instances[0].groups()
    pbi_body = module_body(rtl, pbi_module)
    controller_instances = list(re.finditer(r"(?m)^\s*(controller_\w+)\s+(\w+)\s*\(", pbi_body))
    need(len(controller_instances) == 1, f"{pbi_module} must instantiate exactly one controller")
    controller_module, controller_instance = controller_instances[0].groups()
    controller = module_body(rtl, controller_module)
    datapath_instances = list(re.finditer(r"(?m)^\s*(datapath_\w+)\s+(\w+)\s*\(", pbi_body))
    need(len(datapath_instances) == 1, f"{pbi_module} must instantiate exactly one datapath module")
    _datapath_module, datapath_instance = datapath_instances[0].groups()
    need(re.search(r"\breg\s*\[[^]]+\]\s*present_state\b", controller) is not None,
         f"{controller_module} lacks a present_state register")
    need(re.search(r"case\s*\(\s*present_state\s*\)", controller) is not None,
         f"{controller_module} does not dispatch on present_state")
    parameter_declarations = re.findall(r"\b(?:localparam|parameter)\b([^;]*);", controller)
    state_params = set()
    for declaration in parameter_declarations:
        state_params.update(re.findall(r"\b(S_\d+)\s*=", declaration))
    dispatch = re.search(r"\bcase\s*\(\s*present_state\s*\)(.*?)\bendcase\b", controller, re.S)
    need(dispatch is not None, f"{controller_module} has no complete present_state dispatch block")
    state_cases = set(re.findall(r"\b(S_\d+)\s*:", dispatch.group(1)))

    datapath = module_body(rtl, f"datapath_{top}")
    fu_instances = re.findall(r"\b(fu_kernel_\d+_\d+)\s*\(", datapath)
    increment_name = f"fu_kernel_{function_id}_{increment_id}" if f"fu_kernel_{function_id}_{increment_id}" in fu_instances else None
    shift_name = f"fu_kernel_{function_id}_{shift_id}" if f"fu_kernel_{function_id}_{shift_id}" in fu_instances else None
    src_gep_name = f"fu_kernel_{function_id}_{src_gep_id}" if f"fu_kernel_{function_id}_{src_gep_id}" in fu_instances else None
    dst_gep_name = f"fu_kernel_{function_id}_{dst_gep_id}" if f"fu_kernel_{function_id}_{dst_gep_id}" in fu_instances else None
    need(all((increment_name, shift_name, src_gep_name, dst_gep_name)),
         "datapath RTL is missing an identified induction/address FU instance")
    _, increment_ports = _rtl_instance_body(datapath, increment_name)
    _, shift_ports = _rtl_instance_body(datapath, shift_name)
    _, src_gep_ports = _rtl_instance_body(datapath, src_gep_name)
    _, dst_gep_ports = _rtl_instance_body(datapath, dst_gep_name)
    need(increment_ports.get("in1") == shift_ports.get("in1"),
         "induction increment and address shift do not consume the same RTL register")
    phi_reg_net = increment_ports.get("in1", "")
    need(bool(re.fullmatch(r"out_reg_\d+_reg_\d+", phi_reg_net)),
         "induction input does not resolve to a datapath register output")
    phi_registers = []
    for register_name in set(re.findall(r"\b(reg_\d+)\s*\(", datapath)):
        module, ports = instance_ports(datapath, register_name)
        if not module.startswith("register_"):
            continue
        if ports.get("out1") == phi_reg_net:
            phi_registers.append((register_name, ports))
    need(len(phi_registers) == 1, f"expected one datapath register for induction value, found {len(phi_registers)}")
    phi_register, phi_ports = phi_registers[0]
    increment_outputs = increment_ports.get("out1", "")
    need(_combinational_reaches(datapath, increment_outputs, phi_ports.get("in1", "")),
         "increment result is not fed back into the identified induction register")
    shift_output = shift_ports.get("out1", "")
    need(src_gep_ports.get("in2") == shift_output and dst_gep_ports.get("in2") == shift_output,
         "source/destination address GEPs do not use the induction-derived shift")
    src_address = src_gep_ports.get("out1", "")
    dst_address = dst_gep_ports.get("out1", "")
    endpoints = []
    endpoint_instances = set(re.findall(r"\b(shared_bambu_artificial_ParmMgr_duplex_\w+_endpoint(?:_\w+)*)\s*\(", datapath))
    for instance in endpoint_instances:
        module, ports = _rtl_instance_body(datapath, instance)
        if module.endswith("read_endpoint") or "read_endpoint" in module:
            endpoints.append(("read", instance, ports))
        elif module.endswith("write_endpoint") or "write_endpoint" in module:
            endpoints.append(("write", instance, ports))
    reads = [item for item in endpoints if item[0] == "read"]
    writes = [item for item in endpoints if item[0] == "write"]
    need(len(reads) == 1 and len(writes) == 1,
         "datapath must have exactly one read and one write duplex endpoint")
    need(_combinational_reaches(datapath, src_address, reads[0][2].get("in4", "")),
         "induction-derived source address does not reach the read endpoint address")
    need(_combinational_reaches(datapath, dst_address, writes[0][2].get("in4", "")),
         "induction-derived destination address does not reach the write endpoint address")
    need(state_params == state_cases, "RTL controller state parameters and case labels differ")
    return {"controller_module": controller_module, "controller_instance": controller_instance,
            "datapath_instance": datapath_instance,
            "hierarchical_present_state": f"{top}.{pbi_instance}.{controller_instance}.present_state",
            "state_parameters": ",".join(sorted(state_params)),
            "state_case_labels": ",".join(sorted(state_cases)),
            "phi_register": phi_register, "phi_register_signal": phi_reg_net,
            "increment_operation_instance": increment_name,
            "source_address_signal": src_address,
            "read_endpoint_instance": reads[0][1], "write_endpoint_instance": writes[0][1],
            "rtl_index_address_binding": "increment feedback register -> shift -> source/destination GEP -> endpoint in4"}


def _event_metrics(events_path: Path, mapping: dict) -> dict:
    try:
        data = json.loads(read_text(events_path))
    except json.JSONDecodeError as exc:
        raise CheckError(f"invalid iteration event JSON: {exc}") from exc
    need(data.get("schema_version") == 1, "iteration event schema_version must be 1")
    need(data.get("mapping_id") == mapping["mapping_id"], "event mapping_id does not match static mapping")
    need(data.get("event_type") == "controller_iteration_start",
         "events must be controller_iteration_start, not LOAD/transport completion samples")
    rows = data.get("events")
    expected_count = mapping["event_definition"]["expected_event_count"]
    expected_base = mapping["event_definition"]["expected_source_base_bytes"]
    need(data.get("source_base_bytes") == expected_base,
         "event source_base_bytes does not match the COPY_PARALLELISM_256 stimulus")
    need(isinstance(rows, list) and len(rows) == expected_count,
         f"expected exactly {expected_count} controller iteration-start events")
    states = set(mapping["event_definition"]["state_parameters"])
    cycles: list[int] = []
    for index, row in enumerate(rows):
        need(isinstance(row, dict), f"event {index} must be an object")
        iteration = row.get("iteration")
        need(isinstance(iteration, int) and not isinstance(iteration, bool) and iteration == index,
             f"iteration events must be non-boolean integers ordered from 0; row {index}")
        need(row.get("state") in states, f"event {index} uses a state outside the mapped iteration-start set")
        cycle = row.get("cycle")
        need(isinstance(cycle, int) and not isinstance(cycle, bool) and cycle >= 0,
             f"event {index} cycle must be a nonnegative integer")
        if cycles:
            need(cycle > cycles[-1], f"event {index} timestamp must strictly increase")
        loop_index = row.get("loop_index")
        need(isinstance(loop_index, int) and not isinstance(loop_index, bool) and loop_index == index,
             f"event {index} loop_index must be a non-boolean integer matching the induction index")
        source_address = row.get("source_address")
        need(isinstance(source_address, int) and not isinstance(source_address, bool) and
             source_address == expected_base + 4 * index,
             f"event {index} source_address must equal source base plus four times the induction index")
        cycles.append(cycle)
    intervals = [right - left for left, right in zip(cycles, cycles[1:])]
    return {"event_count": len(cycles), "timestamps": cycles, "intervals_cycles": intervals,
            "effective_ii_average_cycles": sum(intervals) / len(intervals),
            "effective_ii_min_cycles": min(intervals), "effective_ii_max_cycles": max(intervals),
        "throughput_iterations_per_cycle": (len(cycles) - 1) / (cycles[-1] - cycles[0])}


def _load_metrics(load_log: Path) -> dict:
    text = read_text(load_log)
    rows = re.findall(r"(?m)^DUPLEX_LOAD_EVENT CASE=COPY_PARALLELISM_256 INDEX=(\d+) COUNT=256 CYCLE=(\d+)\b.*$", text)
    need(len(rows) == 256, f"expected exactly 256 existing LOAD-completion samples; found {len(rows)}")
    indices = [int(index) for index, _ in rows]
    cycles = [int(cycle) for _, cycle in rows]
    need(indices == list(range(256)), "LOAD-completion indices must be exactly ordered from 0 through 255")
    need(all(right > left for left, right in zip(cycles, cycles[1:])),
         "LOAD-completion timestamps must strictly increase")
    intervals = [right - left for left, right in zip(cycles, cycles[1:])]
    return {"event_count": len(cycles), "timestamps": cycles, "intervals_cycles": intervals,
            "average_interval_cycles": sum(intervals) / len(intervals),
            "min_interval_cycles": min(intervals), "max_interval_cycles": max(intervals)}


def verify(generated: Path, profile: str, events_path: Path | None = None,
           load_log: Path | None = None) -> dict:
    generated = generated.resolve()
    need(generated.is_dir(), f"generated artifact directory missing: {generated}")
    top = "kernel"
    loop_id, bb, scheduled_ii, function_ii = _scheduled_loop(generated / "bambu_results.xml", top)
    need(function_ii == 0, f"expected disabled function_ii=0; found {function_ii}")
    ir = _canonical_ir_loop(read_text(generated / "panda-temp/after_CSROA.ll"), top)
    schedule_xml_path = generated / f"{top}_scheduling.xml"
    try:
        schedule_xml = ET.parse(schedule_xml_path).getroot()
    except ET.ParseError as exc:
        raise CheckError(f"invalid scheduling XML: {exc}") from exc
    schedule_rows = {}
    for item in schedule_xml.iter("scheduling_constraints"):
        name = item.get("vertex_name", "")
        match = re.fullmatch(r"kernel_(\d+)_(\d+)", name)
        if match:
            function_id, op_id = map(int, match.groups())
            need(op_id not in schedule_rows, f"duplicate scheduling record for operation {op_id}")
            schedule_rows[op_id] = {"vertex_name": name, "cstep": int(item.get("cstep", "-1")),
                                    "fu_name": item.get("fu_name", ""), "function_id": function_id}
    dot_dir = generated / "HLS_output/dot" / top
    scheduled_graph, graph_edges = _dot_nodes(dot_dir / f"HLS_scheduling_BB{bb}.dot")
    graph_defs = _definitions(scheduled_graph)
    op_nodes = {}
    for node_id, label in graph_defs.items():
        op = re.search(r"kernel_(\d+)_(\d+)\s+-\s+([^\n]+)", label)
        if op:
            op_nodes[node_id] = {"function_id": int(op.group(1)),
                                 "operation_id": int(op.group(2)), "kind": op.group(3), "label": label}
    phi_candidates = []
    for node_id, node in op_nodes.items():
        if "phi_stmt" not in node["kind"]:
            continue
        phi = re.search(r"([\w.]+)\s*=\s*phi_stmt\(<([\w.]+),\s*BB(\d+)>,\s*<([^,>]+),\s*BB(\d+)>\)", node["label"])
        if not phi:
            continue
        phi_value, back_value, back_bb, initial_value, initial_bb = phi.groups()
        if int(back_bb) != bb or int(initial_bb) == bb:
            continue
        add_matches = [(other_id, other) for other_id, other in op_nodes.items()
                       if "add_node" in other["kind"] and re.search(
                           rf"([\w.]+)\s*=.*?\b{re.escape(phi_value)}\s*\+\s*\(1u\)", other["label"])
                       and re.search(rf"=\s*\(unsigned int\)\({re.escape(phi_value)}\s*\+\s*\(1u\)\)", other["label"])]
        add_matches = [(add_id, add) for add_id, add in add_matches if
                       re.search(rf"\b{re.escape(back_value)}\s*=", add["label"])]
        if len(add_matches) != 1:
            continue
        add_node_id, add_node = add_matches[0]
        need((node_id, add_node_id) in [(src, dst) for src, dst, _ in graph_edges] and
             (add_node_id, node_id) in [(src, dst) for src, dst, _ in graph_edges],
             "induction phi/increment are not connected by scheduled dataflow edges")
        phi_candidates.append((node_id, node, add_node_id, add_node, phi_value, back_value))
    need(len(phi_candidates) == 1,
         f"scheduled loop BB{bb} must have exactly one canonical induction phi/increment; found {len(phi_candidates)}")
    phi_node_id, phi_node, increment_node_id, increment_node, phi_value, increment_value = phi_candidates[0]
    load_nodes = _fu_nodes({node_id: node["label"] for node_id, node in op_nodes.items()}, r"burst_read\(")
    store_nodes = _fu_nodes({node_id: node["label"] for node_id, node in op_nodes.items()}, r"store_write\(")
    need(len(load_nodes) == 1, f"scheduled loop BB{bb} must contain exactly one LOAD op; found {len(load_nodes)}")
    need(len(store_nodes) == 1, f"scheduled loop BB{bb} must contain exactly one STORE op; found {len(store_nodes)}")
    load_node_id, load_node_label = load_nodes[0]
    store_node_id, store_node_label = store_nodes[0]
    load_operation_id = op_nodes[load_node_id]["operation_id"]
    store_operation_id = op_nodes[store_node_id]["operation_id"]
    increment_operation_id = increment_node["operation_id"]
    need({load_operation_id, store_operation_id, increment_operation_id,
          op_nodes[phi_node_id]["operation_id"]} <= schedule_rows.keys(),
         "scheduler XML is missing one or more mapped loop operation identities")
    geps = [(node_id, node) for node_id, node in op_nodes.items() if "gep_node" in node["kind"]]
    src_geps = [(node_id, node) for node_id, node in geps if re.search(r"\bsrc\b", node["label"])]
    dst_geps = [(node_id, node) for node_id, node in geps if re.search(r"\bdst\b", node["label"])]
    need(len(src_geps) == 1 and len(dst_geps) == 1,
         "scheduled loop must have exactly one source and one destination address GEP")
    src_gep_id, src_gep_node = src_geps[0]
    dst_gep_id, dst_gep_node = dst_geps[0]
    src_gep_value = re.search(r"([\w.]+)\s*=", src_gep_node["label"])
    dst_gep_value = re.search(r"([\w.]+)\s*=", dst_gep_node["label"])
    load_value = re.search(r"([\w.]+)\s*=", load_node_label)
    need(src_gep_value and dst_gep_value and load_value and
         re.search(rf"\b{re.escape(src_gep_value.group(1))}\b", load_node_label) and
         re.search(rf"\b{re.escape(dst_gep_value.group(1))}\b", store_node_label) and
         re.search(rf"\b{re.escape(load_value.group(1))}\b", store_node_label),
         "scheduled LOAD/STORE do not consume the induction-indexed source/destination GEPs")

    shifts = [(node_id, node) for node_id, node in op_nodes.items()
              if "shl_node" in node["kind"] and re.search(rf"\b{re.escape(phi_value)}\b", node["label"])]
    need(len(shifts) == 1, f"expected one scheduled shift from induction phi; found {len(shifts)}")
    shift_operation_id = shifts[0][1]["operation_id"]
    mapped_operation_ids = {
        "phi": op_nodes[phi_node_id]["operation_id"],
        "increment": increment_operation_id,
        "load": load_operation_id,
        "store": store_operation_id,
        "shift": shift_operation_id,
        "src_gep": op_nodes[src_gep_id]["operation_id"],
        "dst_gep": op_nodes[dst_gep_id]["operation_id"],
    }
    raw_function_id = _pandA_loop_identity(generated, top, bb, mapped_operation_ids)
    for role, operation_id in mapped_operation_ids.items():
        dot_function_ids = {node["function_id"] for node in op_nodes.values()
                            if node["operation_id"] == operation_id}
        need(dot_function_ids == {raw_function_id},
             f"scheduled DOT {role} function identity does not match PandA raw function @{raw_function_id}")
        need(schedule_rows[operation_id]["function_id"] == raw_function_id and
             schedule_rows[operation_id]["vertex_name"] == f"kernel_{raw_function_id}_{operation_id}",
             f"scheduler XML {role} function identity does not match PandA raw function @{raw_function_id}")
    states = _state_map(dot_dir / "HLS_FSM.dot", bb)
    phi_operation_id = phi_node["operation_id"]
    start_states = sorted(state for state, operations in states.items()
                          if (raw_function_id, phi_operation_id) in operations and
                          (raw_function_id, shift_operation_id) in operations and
                          (raw_function_id, load_operation_id) in operations)
    wait_states = sorted(state for state, operations in states.items()
                         if (raw_function_id, load_operation_id) in operations and
                         (raw_function_id, phi_operation_id) not in operations)
    need(start_states, "FSM has no loop state containing induction phi, index shift, and LOAD")
    need(wait_states, "FSM has no LOAD-wait state separated from induction iteration starts")

    rtl = read_text(generated / f"{top}.v")
    rtl_mapping = _rtl_binding(rtl, top, increment_operation_id,
                               shift_operation_id,
                               op_nodes[src_gep_id]["operation_id"],
                               op_nodes[dst_gep_id]["operation_id"], raw_function_id)
    need(set(start_states + wait_states) <= set(re.findall(r"\bS_\d+\b", rtl_mapping["state_parameters"])),
         "RTL controller lacks one or more FSM states")
    need(set(start_states + wait_states) <= set(re.findall(r"\bS_\d+\b", module_body(rtl, rtl_mapping["controller_module"]))),
         "RTL controller does not implement one or more mapped state cases")

    mapping = {
        "schema_version": 1,
        "profile": profile,
        "model_context": {"device": "xcu250,-2L,figd2104", "clock_period_ns": 3.33,
                          "source": "bambu_axi_burst_duplex_copy.cpp"},
        "scheduled_loop": {"id": loop_id, "bb": bb, "ii": scheduled_ii,
                            "function_ii": function_ii},
        "induction": {"phi_operation_id": phi_operation_id, "phi_value": phi_value,
                      "increment_operation_id": increment_operation_id,
                      "increment_value": increment_value, "raw_ir": ir},
        "operations": {
            "load": {"operation_id": load_operation_id, "cstep": schedule_rows[load_operation_id]["cstep"],
                     "vertex_name": schedule_rows[load_operation_id]["vertex_name"],
                     "function_unit": schedule_rows[load_operation_id]["fu_name"]},
            "store": {"operation_id": store_operation_id, "cstep": schedule_rows[store_operation_id]["cstep"],
                      "vertex_name": schedule_rows[store_operation_id]["vertex_name"],
                      "function_unit": schedule_rows[store_operation_id]["fu_name"]},
        },
        "controller": rtl_mapping,
        "event_definition": {
            "type": "controller_iteration_start",
            "sampling_edge": "rising clock edge",
            "state_signal": rtl_mapping["hierarchical_present_state"],
            "state_parameters": start_states,
            "excluded_load_wait_state_parameters": wait_states,
            "iteration_index_binding": rtl_mapping["phi_register_signal"],
            "source_address_signal": rtl_mapping["source_address_signal"],
            "address_binding": rtl_mapping["rtl_index_address_binding"],
            "expected_event_count": 256,
            "expected_source_base_bytes": 0x2000,
            "warning": "Do not substitute LOAD completion or fifo_load_pop intervals for this event."
        },
    }
    canonical = json.dumps(mapping, sort_keys=True, separators=(",", ":")).encode()
    mapping["mapping_id"] = hashlib.sha256(canonical).hexdigest()
    if events_path is not None:
        mapping["observed_events"] = _event_metrics(events_path, mapping)
    if load_log is not None:
        mapping["load_completion_metrics"] = _load_metrics(load_log)
    return mapping


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("generated", type=Path, help="Bambu generated output directory")
    parser.add_argument("--profile", choices=("symmetric", "asymmetric"), required=True)
    parser.add_argument("--events", type=Path, help="optional controller iteration-start event JSON")
    parser.add_argument("--load-log", type=Path, help="existing G3.1 LOAD-observer scenario log")
    parser.add_argument("--report", type=Path, help="write the mapping and optional metrics as JSON")
    args = parser.parse_args(argv)
    try:
        result = verify(args.generated, args.profile, args.events, args.load_log)
        rendered = json.dumps(result, indent=2, sort_keys=True)
        if args.report:
            args.report.parent.mkdir(parents=True, exist_ok=True)
            args.report.write_text(rendered + "\n", encoding="utf-8")
        print(f"DUPLEX ITERATION MAPPING PASS PROFILE={args.profile} LOOP={result['scheduled_loop']['id']} "
              f"II={result['scheduled_loop']['ii']} START_STATES={','.join(result['event_definition']['state_parameters'])}")
        if "observed_events" in result:
            stats = result["observed_events"]
            print(f"DUPLEX EFFECTIVE II average={stats['effective_ii_average_cycles']:.6f} "
                  f"min={stats['effective_ii_min_cycles']} max={stats['effective_ii_max_cycles']} "
                  f"throughput={stats['throughput_iterations_per_cycle']:.6f}")
        if "load_completion_metrics" in result:
            stats = result["load_completion_metrics"]
            print(f"DUPLEX LOAD INTERVAL average={stats['average_interval_cycles']:.6f} "
                  f"min={stats['min_interval_cycles']} max={stats['max_interval_cycles']}")
        return 0
    except (CheckError, OSError, UnicodeError, ValueError) as exc:
        print(f"DUPLEX ITERATION CHECK FAILED: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
