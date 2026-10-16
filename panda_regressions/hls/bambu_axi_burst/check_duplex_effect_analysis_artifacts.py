#!/usr/bin/env python3
"""Fail-closed checker for the manual analysis-only duplex effect gate."""
from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import sys
import xml.etree.ElementTree as ET
from pathlib import Path

LEGACY_COMPONENT = re.compile(r"\b(?:MinimalAXI4AdapterSingleBeat|IOB_cache_axi)\b")
BURST_COMPONENTS = re.compile(r"\b(?:MinimalAXI4MasterPipelined|MinimalAXI4MasterPipelinedWrite)\b")
BURST_OPS = re.compile(r"fuselector_[A-Za-z0-9_]*(?:configure_read|burst_read|configure_write|store_write|drain_write)\b")
TRACE = re.compile(
    r'^\s*AXI_BURST_PREANALYSIS bundle=(\S+) owner_argument_index=(\d+) '
    r'read_accesses=(\d+) write_accesses=(\d+) read_requested=(\d) read_recognized=(\d) '
    r'read_fallback="([^"]*)" write_requested=(\d) write_recognized=(\d) '
    r'write_fallback="([^"]*)" same_base_duplex_candidate=(\d) '
    r'same_base_duplex_reason="([^"]*)"\s*$', re.MULTILINE)
PROOF_PREFIX = "AXI_BURST_DUPLEX_PROOF "
SUCCESS = "same-base pointwise read-before-write candidate"
FALLBACK_READ = "bundle contains stores"
FALLBACK_WRITE = "bundle contains reads; same-bundle read/write requires duplex support"
IDENTITY_FIELDS = {
    "owner_argument_index", "base_ssa_index", "load_statement_index", "store_statement_index",
    "read_preheader_bb", "read_header_bb", "read_branch_index", "read_guard_index",
    "read_bound_index", "read_induction_index", "write_preheader_bb", "write_header_bb",
    "write_exit_bb", "write_guard_index", "write_bound_index", "write_induction_index",
}
CASES: dict[str, tuple[str, int, str, str, bool, bool]] = {
    "same_base_pointwise": (SUCCESS, 1, FALLBACK_READ, FALLBACK_WRITE, True, True),
    "same_base_pointwise_reversed": (SUCCESS, 1, FALLBACK_READ, FALLBACK_WRITE, True, True),
    "same_base_cached": (SUCCESS, 1, "cache-enabled bundle", "cache-enabled bundle", True, False),
    "shifted_index": ("canonical contiguous loop recognition failed", 1, FALLBACK_READ, FALLBACK_WRITE, False, True),
    "stride_index": ("canonical contiguous loop recognition failed", 1, FALLBACK_READ, FALLBACK_WRITE, False, True),
    "conditional_store": ("canonical contiguous loop recognition failed", 1, FALLBACK_READ, FALLBACK_WRITE, False, True),
    "residual_store": ("original effects are not fully classified ordinary accesses", 1,
                       "memory writes or opaque effects; prefetch ordering is not proven",
                       "opaque, volatile, atomic, or unclassified memory effects", False, True),
    "volatile_effect": ("original effects are not fully classified ordinary accesses", 1,
                        "memory writes or opaque effects; prefetch ordering is not proven",
                        "opaque, volatile, atomic, or unclassified memory effects", False, True),
    "separate_phase": ("original effects are not fully classified ordinary accesses", 1,
                       "memory writes or opaque effects; prefetch ordering is not proven",
                       "opaque, volatile, atomic, or unclassified memory effects", False, True),
    "distinct_bases": ("not exactly one classified same-base read/store pair", 2,
                       FALLBACK_READ, FALLBACK_WRITE, False, True),
}


class CheckError(ValueError):
    pass


def need(condition: bool, message: str) -> None:
    if not condition:
        raise CheckError(message)


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def safe_file(path: Path, label: str) -> None:
    need(not path.is_symlink(), f"{label} must not be a symlink: {path}")
    need(path.is_file() and path.stat().st_size > 0, f"required non-empty {label} missing: {path}")


def parse_manifest(path: Path) -> dict[str, str]:
    safe_file(path, "provenance manifest")
    fields: dict[str, str] = {}
    for line in path.read_text(encoding="utf-8").splitlines():
        if not line or "=" not in line:
            raise CheckError(f"malformed provenance line: {line!r}")
        key, value = line.split("=", 1)
        if key in fields or not value:
            raise CheckError(f"duplicate or empty provenance field: {key}")
        fields[key] = value
    required = {"top", "source", "source_sha256", "bambu", "bambu_sha256", "plugin", "plugin_sha256",
                "checker", "checker_sha256", "runner", "runner_sha256", "raw_ir", "raw_ir_sha256"}
    need(fields.keys() == required, "provenance manifest fields are missing or unexpected")
    return fields


NODE_LINE = re.compile(r"^@(\d+) ([a-zA-Z0-9_]+)(?: (.*))?$")
REF = re.compile(r"(?:^|\s)([a-zA-Z_][a-zA-Z0-9_]*): @(\d+)")


def parse_raw(raw_text: str) -> dict[int, tuple[str, str]]:
    nodes: dict[int, tuple[str, str]] = {}
    for line in raw_text.splitlines():
        match = NODE_LINE.match(line)
        if match:
            index = int(match.group(1))
            need(index not in nodes, f"duplicate raw IR node @{index}")
            nodes[index] = (match.group(2), match.group(3) or "")
    need(bool(nodes), "raw IR contains no graph nodes")
    return nodes


def fields(node: tuple[str, str]) -> dict[str, list[str]]:
    result: dict[str, list[str]] = {}
    for key, value in REF.findall(node[1]):
        result.setdefault(key, []).append(value)
    return result


def function_graph(raw_text: str, top: str) -> tuple[dict[int, tuple[str, str]], int, dict[int, tuple[int, str, list[int], list[str], list[str]]], set[int]]:
    nodes = parse_raw(raw_text)
    named = {index for index, (kind, attrs) in nodes.items()
             if kind == "identifier_node" and re.search(rf'\bstrg: "{re.escape(top)}"(?:\s|$)', attrs)}
    funcs = [index for index, (kind, attrs) in nodes.items()
             if kind == "function_val_node" and
             set(fields((kind, attrs)).get("name", [])) & {str(i) for i in named}]
    need(len(funcs) == 1, f"raw IR must contain exactly one selected function {top}")
    func = funcs[0]
    bodies = fields(nodes[func])["body"]
    need(len(bodies) == 1 and int(bodies[0]) in nodes and nodes[int(bodies[0])][0] == "statement_list_node",
         "selected function has no unique statement-list body")
    body_attrs = nodes[int(bodies[0])][1]
    block_matches = list(re.finditer(r"\bbloc: (\d+) loop_id: (\d+)(.*?)(?=\bbloc: \d+ loop_id: |$)",
                                     body_attrs))
    blocks: dict[int, tuple[int, str, list[int], list[str], list[str]]] = {}
    for match in block_matches:
        block, loop_id = int(match.group(1)), int(match.group(2))
        stmt_ids = [int(value) for value in re.findall(r"\bstmt: @(\d+)", match.group(3))]
        preds = re.findall(r"\bpred: (ENTRY|\d+)", match.group(3))
        succs = re.findall(r"\bsucc: (EXIT|\d+)", match.group(3))
        need(block not in blocks, f"duplicate statement-list block {block}")
        blocks[block] = (loop_id, match.group(3), stmt_ids, preds, succs)
    need(bool(blocks), "selected function has no statement-list blocks")
    reachable: set[int] = set()
    pending = [func]
    while pending and len(reachable) < 1_000_000:
        index = pending.pop()
        if index in reachable or index not in nodes:
            continue
        reachable.add(index)
        node_fields = fields(nodes[index])
        for key in ("body", "name", "arg", "stmt", "phi", "op", "res", "def", "def_stmt"):
            pending.extend(int(ref) for ref in node_fields.get(key, []))
    return nodes, func, blocks, reachable


def node_in_function(nodes: dict[int, tuple[str, str]], func: int, reachable: set[int],
                     index: int, label: str) -> tuple[str, str]:
    need(index in nodes, f"{label} @{index} does not resolve in raw IR")
    node = nodes[index]
    parent_refs = fields(node).get("parent", [])
    need(index in reachable and node[0] not in {"function_val_node", "identifier_node", "statement_list_node"}
         and (not parent_refs or str(func) in parent_refs),
         f"{label} @{index} is not a node in selected function")
    return node


def statement_block(nodes: dict[int, tuple[str, str]], blocks: dict[int, tuple[int, str, list[int], list[str], list[str]]],
                    func: int, reachable: set[int], index: int, label: str) -> tuple[int, int, int]:
    node = node_in_function(nodes, func, reachable, index, label)
    attrs = node[1]
    bb_match = re.search(r"\bbb_index: (\d+)", attrs)
    need(bb_match is not None, f"{label} @{index} is not a statement with a block index")
    bb = int(bb_match.group(1))
    need(bb in blocks and index in blocks[bb][2], f"{label} @{index} is absent from block {bb}")
    return bb, blocks[bb][0], blocks[bb][2].index(index)


def operation_refs(node: tuple[str, str]) -> list[int]:
    return [int(value) for value in fields(node).get("op", [])]


def block_targets(node: tuple[str, str]) -> list[str]:
    return re.findall(r"\bbloc: (\d+)", node[1])


def conditional_branch(node: tuple[str, str], label: str) -> tuple[int, int, int]:
    conditions = operation_refs(node)
    targets = block_targets(node)
    need(node[0] == "multi_way_if_stmt" and len(conditions) == 1 and len(targets) == 2,
         f"{label} does not encode one true condition and one default branch")
    return conditions[0], int(targets[0]), int(targets[1])


def is_signed_32bit_value(nodes: dict[int, tuple[str, str]], index: int) -> bool:
    if index not in nodes:
        return False
    attrs = fields(nodes[index])
    type_ids = attrs.get("type", [])
    return (len(type_ids) == 1 and int(type_ids[0]) in nodes and
            nodes[int(type_ids[0])][0] == "integer_ty_node" and
            re.search(r"\bbitsize(?:alloc)?: 32(?:\s|$)", nodes[int(type_ids[0])][1]) is not None and
            not re.search(r"\bunsigned(?:\s|$)", nodes[int(type_ids[0])][1]))


def is_unsigned_32bit_value(nodes: dict[int, tuple[str, str]], index: int) -> bool:
    if index not in nodes:
        return False
    type_ids = fields(nodes[index]).get("type", [])
    return (len(type_ids) == 1 and int(type_ids[0]) in nodes and
            nodes[int(type_ids[0])][0] == "integer_ty_node" and
            re.search(r"\bbitsize(?:alloc)?: 32(?:\s|$)", nodes[int(type_ids[0])][1]) is not None and
            re.search(r"\bunsigned(?:\s|$)", nodes[int(type_ids[0])][1]) is not None)


def assignment_rhs(nodes: dict[int, tuple[str, str]], statement: tuple[str, str], label: str) -> int:
    operands = operation_refs(statement)
    need(statement[0] in {"assign_stmt", "gimple_assign"} and len(operands) == 2,
         f"{label} is not a two-operand assignment")
    return operands[1]


def operand_depends(nodes: dict[int, tuple[str, str]], root: int, target: int,
                    limit: int = 4096) -> bool:
    """Follow only value operands and SSA definitions, never ownership/use/type links."""
    pending, visited = [root], set()
    while pending and len(visited) < limit:
        index = pending.pop()
        if index == target:
            return True
        if index in visited or index not in nodes:
            continue
        visited.add(index)
        kind, _attrs = nodes[index]
        if kind == "ssa_node":
            defs = fields(nodes[index]).get("def_stmt", [])
            if len(defs) == 1 and int(defs[0]) in nodes:
                statement = nodes[int(defs[0])]
                if statement[0] in {"assign_stmt", "gimple_assign"}:
                    pending.extend(operation_refs(statement)[1:])
                elif statement[0] == "phi_stmt":
                    pending.extend(int(value) for value in fields(statement).get("def", []))
        elif kind in {"assign_stmt", "gimple_assign"}:
            pending.extend(operation_refs(nodes[index])[1:])
        else:
            pending.extend(operation_refs(nodes[index]))
    return False


def memory_operand(nodes: dict[int, tuple[str, str]], statement: tuple[str, str], role: str,
                   expected_width: int) -> tuple[int, int]:
    operands = operation_refs(statement)
    need(statement[0] in {"assign_stmt", "gimple_assign"} and len(operands) == 2,
         f"{role} is not a two-operand assignment")
    access_index = operands[1] if role == "LOAD" else operands[0]
    value_index = operands[0] if role == "LOAD" else operands[1]
    need(access_index in nodes and nodes[access_index][0] == "mem_access_node",
         f"{role} assignment has no memory access in the expected {'RHS' if role == 'LOAD' else 'LHS'} position")
    access_kind, access_attrs = nodes[access_index]
    need("ordinary_memory_access" in access_attrs and "volatile" not in access_attrs and "atomic" not in access_attrs,
         f"{role} is not an ordinary non-volatile memory access")
    type_refs = fields((access_kind, access_attrs)).get("type", [])
    need(len(type_refs) == 1 and int(type_refs[0]) in nodes,
         f"{role} has no unique access type")
    type_attrs = nodes[int(type_refs[0])][1]
    width = re.search(r"\b(?:bitsizealloc|bitsize): (\d+)", type_attrs)
    need(width is not None and int(width.group(1)) == expected_width,
         f"{role} access width is not the expected {expected_width} bits")
    address = operation_refs((access_kind, access_attrs))
    need(len(address) == 1, f"{role} has no unique address operand")
    return int(address[0]), value_index


def verify_success_record(record: dict[str, object], raw_text: str, top: str) -> None:
    need(IDENTITY_FIELDS <= record.keys(), "semantic proof record is missing identity fields")
    for key in IDENTITY_FIELDS:
        need(type(record[key]) is int, f"semantic identity field {key} is not numeric")
    nodes, func, blocks, reachable = function_graph(raw_text, top)
    owner = int(record["owner_argument_index"])
    owner_node = node_in_function(nodes, func, reachable, owner, "owner argument")
    need(owner_node[0] == "argument_val_node", "owner identity is not a function argument")
    base_index = int(record["base_ssa_index"])
    base = node_in_function(nodes, func, reachable, base_index, "base SSA")
    need(base[0] == "ssa_node", "base identity is not SSA")
    base_var = fields(base).get("var", [])
    need(str(owner) in base_var,
         "base SSA does not name the declared owner argument directly")

    load_index, store_index = int(record["load_statement_index"]), int(record["store_statement_index"])
    load = node_in_function(nodes, func, reachable, load_index, "LOAD statement")
    store = node_in_function(nodes, func, reachable, store_index, "STORE statement")
    need(load[0] in {"assign_stmt", "gimple_assign"} and store[0] in {"assign_stmt", "gimple_assign"},
         "declared LOAD/STORE are not assignment statements")
    load_bb, load_loop, load_pos = statement_block(nodes, blocks, func, reachable, load_index, "LOAD")
    store_bb, store_loop, store_pos = statement_block(nodes, blocks, func, reachable, store_index, "STORE")
    need(load_bb == store_bb and load_loop != 0 and load_loop == store_loop,
         "LOAD and STORE are not in the same original loop block")
    need(load_pos < store_pos, "LOAD does not precede STORE in the original iteration")
    load_address, load_value = memory_operand(nodes, load, "LOAD", 32)
    store_address, store_value = memory_operand(nodes, store, "STORE", 32)
    need(load_address == store_address,
         "LOAD and STORE do not use the same original affine address/index")
    need(operand_depends(nodes, store_value, load_value),
         "STORE value is not derived from the preceding LOAD result")

    address = node_in_function(nodes, func, reachable, load_address, "memory address")
    need(address[0] == "ssa_node", "memory address is not an SSA value")
    address_defs = fields(address).get("def_stmt", [])
    need(len(address_defs) == 1 and int(address_defs[0]) in nodes,
         "memory address has no unique defining assignment")
    address_def = nodes[int(address_defs[0])]
    address_expr = assignment_rhs(nodes, address_def, "memory address")
    gep = node_in_function(nodes, func, reachable, address_expr, "memory address expression")
    gep_ops = operation_refs(gep)
    need(gep[0] == "gep_node" and len(gep_ops) == 2,
         "memory address is not a canonical base-plus-index GEP")
    gep_base, gep_offset = gep_ops
    need(gep_base == base_index,
         "canonical address does not use the declared base as its direct GEP base")
    induction_id = int(record["read_induction_index"])
    offset = node_in_function(nodes, func, reachable, gep_offset, "byte offset")
    offset_defs = fields(offset).get("def_stmt", []) if offset[0] == "ssa_node" else []
    need(len(offset_defs) == 1 and int(offset_defs[0]) in nodes,
         "byte offset has no unique defining assignment")
    offset_expr = nodes[int(offset_defs[0])]
    offset_rhs = assignment_rhs(nodes, offset_expr, "byte offset")
    scale = node_in_function(nodes, func, reachable, offset_rhs, "byte offset expression")
    scale_ops = operation_refs(scale)
    scaled_induction = False
    if scale[0] == "mul_node" and len(scale_ops) == 2:
        scaled_induction = ((scale_ops[0] == induction_id and
                             nodes.get(scale_ops[1], ("", ""))[0] == "constant_int_val_node" and
                             re.search(r"\bvalue: 4(?:\s|$)", nodes[scale_ops[1]][1])) or
                            (scale_ops[1] == induction_id and
                             nodes.get(scale_ops[0], ("", ""))[0] == "constant_int_val_node" and
                             re.search(r"\bvalue: 4(?:\s|$)", nodes[scale_ops[0]][1])))
    elif scale[0] == "shl_node" and len(scale_ops) == 2:
        scaled_induction = (scale_ops[0] == induction_id and
                            nodes.get(scale_ops[1], ("", ""))[0] == "constant_int_val_node" and
                            re.search(r"\bvalue: 2(?:\s|$)", nodes[scale_ops[1]][1]))
    need(scaled_induction,
         "byte offset is not the declared induction scaled by four bytes")

    common = ("read_preheader_bb", "read_header_bb", "read_guard_index", "read_bound_index", "read_induction_index")
    loop_branch: tuple[str, str] | None = None
    loop_comparison: tuple[str, str] | None = None
    for side in ("read", "write"):
        header = int(record[f"{side}_header_bb"])
        preheader = int(record[f"{side}_preheader_bb"])
        need(header in blocks and preheader in blocks, f"{side} loop block identity does not resolve")
        need(blocks[header][0] != 0, f"{side} header is not marked as a loop block")
        need(header == load_bb, f"{side} header is not the memory-operation block")
        if side == "read":
            branch_index = int(record["read_branch_index"])
            branch = node_in_function(nodes, func, reachable, branch_index, "read branch")
            need(branch[0] == "multi_way_if_stmt", "read branch identity is not a branch statement")
            branch_bb, branch_loop, _ = statement_block(nodes, blocks, func, reachable, branch_index, "read branch")
            need(branch_bb == preheader and branch_loop == blocks[preheader][0],
                 "read guard branch is not in the declared preheader")
            guard_id = int(record["read_guard_index"])
            bound_id = int(record["read_bound_index"])
            induction_id = int(record["read_induction_index"])
            branch_condition, _true_target, _default_target = conditional_branch(branch, "read entry branch")
            need(branch_condition == guard_id,
                 "read branch does not consume the declared guard")
            need(operand_depends(nodes, guard_id, bound_id),
                 "read guard does not depend on the declared bound")
            loop_branches = [statement for statement in blocks[header][2]
                             if nodes[statement][0] == "multi_way_if_stmt"]
            need(len(loop_branches) == 1, "loop header has no unique conditional exit branch")
            loop_branch = nodes[loop_branches[0]]
            loop_condition = operation_refs(loop_branch)
            need(len(loop_condition) == 1, "loop branch has no unique condition operand")
            condition = node_in_function(nodes, func, reachable, loop_condition[0], "loop condition")
            condition_defs = fields(condition).get("def_stmt", []) if condition[0] == "ssa_node" else []
            need(len(condition_defs) == 1 and int(condition_defs[0]) in nodes,
                 "loop condition has no unique defining assignment")
            comparison = node_in_function(nodes, func, reachable,
                                          assignment_rhs(nodes, nodes[int(condition_defs[0])], "loop condition"),
                                          "loop comparison")
            comparison_operands = operation_refs(comparison)
            need(comparison[0] == "eq_node" and
                 len(comparison_operands) == 2 and
                 any(operand_depends(nodes, value, induction_id) for value in comparison_operands) and
                 any(operand_depends(nodes, value, bound_id) for value in comparison_operands),
                 "loop comparison is not equality over the declared bound and induction")
            loop_comparison = comparison
        guard = node_in_function(nodes, func, reachable, int(record[f"{side}_guard_index"]), f"{side} guard")
        bound = node_in_function(nodes, func, reachable, int(record[f"{side}_bound_index"]), f"{side} bound")
        induction = node_in_function(nodes, func, reachable, int(record[f"{side}_induction_index"]), f"{side} induction")
        need(guard[0] in {"ssa_node", "assign_stmt"}, f"{side} guard has an unexpected node kind")
        need(bound[0] in {"ssa_node", "argument_val_node", "constant_int_val_node"},
             f"{side} bound has an unexpected node kind")
        need(induction[0] == "ssa_node", f"{side} induction is not SSA")
        need(blocks[header][0] == load_loop, f"{side} loop context differs from memory operations")
    read_context = tuple(record[key] for key in common)
    write_context = (record["write_preheader_bb"], record["write_header_bb"], record["write_guard_index"],
                     record["write_bound_index"], record["write_induction_index"])
    need(read_context == write_context, "read and write proof regions do not share canonical loop context")
    preheader = int(record["read_preheader_bb"])
    header = int(record["read_header_bb"])
    exit_bb = int(record["write_exit_bb"])
    need(str(preheader) in blocks[header][3] and str(header) in blocks[preheader][4],
         "declared preheader is not an actual predecessor/entry edge of the loop header")
    need(set(blocks[header][3]) == {str(preheader), str(header)},
         "loop header has unexpected CFG predecessors")
    need(str(header) in blocks[header][4] and str(header) in blocks[header][3],
         "loop header does not have the required self-edge")
    need(exit_bb in blocks and str(exit_bb) in blocks[header][4] and
         str(header) in blocks[exit_bb][3] and exit_bb != header,
         "declared write exit is not an actual loop-header successor with a matching predecessor")
    need(set(blocks[header][4]) == {str(header), str(exit_bb)},
         "loop header has unexpected CFG successors")
    need(set(blocks[preheader][4]) == {str(header), str(exit_bb)},
         "preheader entry branch does not have the declared header and exit CFG edges")
    entry_branches = [statement for statement in blocks[preheader][2]
                      if nodes[statement][0] == "multi_way_if_stmt"]
    need(len(entry_branches) == 1, "preheader has no unique loop-entry branch")
    entry_condition, entry_true_target, entry_default_target = conditional_branch(
        nodes[entry_branches[0]], "preheader branch")
    need(entry_condition == int(record["read_guard_index"]),
         "preheader branch condition differs from the declared signed-bound guard")
    need(entry_true_target == header and entry_default_target == exit_bb,
         "preheader branch polarity must map true to loop entry and default/false to exit")
    loop_condition, loop_true_target, loop_default_target = conditional_branch(loop_branch, "loop branch")
    need(loop_true_target == exit_bb and loop_default_target == header,
         "loop branch polarity must map true to exit and default/false to the backedge")
    comparison_operands = operation_refs(loop_comparison)
    induction_id = int(record["read_induction_index"])
    increment_defs = fields(nodes[induction_id]).get("def_stmt", [])
    need(len(increment_defs) == 1 and int(increment_defs[0]) in nodes and
         nodes[int(increment_defs[0])][0] == "phi_stmt",
         "declared induction has no unique PHI definition")
    phi_for_increment = nodes[int(increment_defs[0])]
    phi_incoming = re.findall(r"\bdef: @(\d+) edge: (\d+)", phi_for_increment[1])
    phi_by_edge = {int(edge): int(value) for value, edge in phi_incoming}
    increment_id = phi_by_edge.get(header)
    need(increment_id is not None and set(comparison_operands) ==
         {increment_id, int(record["read_bound_index"])},
         "loop predicate is not exactly increment == declared bound")
    guard_defs = fields(nodes[entry_condition]).get("def_stmt", [])
    need(len(guard_defs) == 1 and int(guard_defs[0]) in nodes,
         "entry guard has no unique defining assignment")
    guard_comparison = nodes[assignment_rhs(nodes, nodes[int(guard_defs[0])], "entry guard")]
    guard_operands = operation_refs(guard_comparison)
    need(guard_comparison[0] == "gt_node" and len(guard_operands) == 2 and
         nodes.get(guard_operands[1], ("", ""))[0] == "constant_int_val_node" and
         re.search(r"\bvalue: 0(?:\s|$)", nodes[guard_operands[1]][1]) and
         is_signed_32bit_value(nodes, guard_operands[0]) and
         is_signed_32bit_value(nodes, guard_operands[1]),
         "entry predicate is not signed declared-bound > 0")
    signed_bound = guard_operands[0]
    signed_bound_defs = fields(nodes[signed_bound]).get("def_stmt", [])
    need(nodes[signed_bound][0] == "ssa_node" and len(signed_bound_defs) == 1 and
         int(signed_bound_defs[0]) in nodes,
         "entry signed-bound SSA has no unique defining assignment")
    cast_assignment = nodes[int(signed_bound_defs[0])]
    cast_operands = operation_refs(cast_assignment)
    need(cast_assignment[0] in {"assign_stmt", "gimple_assign"} and len(cast_operands) == 2 and
         cast_operands[0] == signed_bound and cast_operands[1] in nodes and
         nodes[cast_operands[1]][0] == "nop_node" and
         operation_refs(nodes[cast_operands[1]]) == [int(record["read_bound_index"])],
         "entry bound conversion is not a single nop of the declared bound")
    signed_type = fields(nodes[signed_bound]).get("type", [])
    conversion_type = fields(nodes[cast_operands[1]]).get("type", [])
    need(len(signed_type) == 1 and conversion_type == signed_type and
         is_signed_32bit_value(nodes, cast_operands[1]) and
         is_unsigned_32bit_value(nodes, int(record["read_bound_index"])),
         "entry bound conversion type is not unsigned32 to the matching signed32 guard type")
    body_node = nodes[int(fields(nodes[func])["body"][0])]
    phis = [int(value) for value in fields(body_node).get("phi", [])]
    induction_phis = [nodes[index] for index in phis if index in nodes and
                      nodes[index][0] == "phi_stmt" and
                      fields(nodes[index]).get("res", []) == [str(induction_id)]]
    need(len(induction_phis) == 1, "loop header lacks a unique induction PHI")
    phi = induction_phis[0]
    incoming = re.findall(r"\bdef: @(\d+) edge: (\d+)", phi[1])
    need(len(incoming) == 2, "induction PHI does not have exactly preheader and backedge values")
    by_edge = {int(edge): int(value) for value, edge in incoming}
    need(len(by_edge) == 2 and set(by_edge) == {preheader, header},
         "induction PHI edges do not match the declared preheader and loop header")
    initial = nodes.get(by_edge[preheader], ("", ""))
    need(initial[0] == "constant_int_val_node" and re.search(r"\bvalue: 0(?:\s|$)", initial[1]),
         "induction PHI does not start at zero on the preheader edge")
    increment = nodes.get(by_edge[header], ("", ""))
    increment_defs = fields(increment).get("def_stmt", []) if increment[0] == "ssa_node" else []
    need(len(increment_defs) == 1 and int(increment_defs[0]) in nodes,
         "induction PHI backedge is not a uniquely defined SSA increment")
    increment_rhs = nodes[int(increment_defs[0])]
    increment_node = nodes.get(assignment_rhs(nodes, increment_rhs, "induction increment"), ("", ""))
    increment_ops = operation_refs(increment_node)
    need(increment_node[0] == "add_node" and len(increment_ops) == 2 and
         any(value == induction_id for value in increment_ops) and
         any(nodes.get(value, ("", ""))[0] == "constant_int_val_node" and
             re.search(r"\bvalue: 1(?:\s|$)", nodes[value][1]) for value in increment_ops),
         "induction PHI backedge does not increment by one")


def parse_proof_records(stderr: str) -> list[dict[str, object]]:
    lines = [line.strip() for line in stderr.splitlines() if PROOF_PREFIX in line]
    records: list[dict[str, object]] = []
    for line in lines:
        need(line.startswith(PROOF_PREFIX), "duplex proof record is not a standalone debug record")
        try:
            def unique_object(pairs: list[tuple[str, object]]) -> dict[str, object]:
                result: dict[str, object] = {}
                for key, value in pairs:
                    need(key not in result, f"duplicate JSON field in duplex proof record: {key}")
                    result[key] = value
                return result

            value = json.loads(line[len(PROOF_PREFIX):], object_pairs_hook=unique_object)
        except json.JSONDecodeError as exc:
            raise CheckError(f"malformed duplex proof JSON record: {exc}") from exc
        need(isinstance(value, dict), "duplex proof JSON record is not an object")
        records.append(value)
    return records


def verify(output: Path, top: str, source: Path, bambu: Path, plugin: Path,
           checker: Path, runner: Path) -> None:
    need(top in CASES, f"unknown case: {top}")
    need(not output.is_symlink() and output.is_dir(), f"invalid output directory: {output}")
    manifest = parse_manifest(output / "provenance.txt")
    raw_path = Path(manifest["raw_ir"])
    expected_paths = {"source": source, "bambu": bambu, "plugin": plugin,
                      "checker": checker, "runner": runner}
    need(manifest["top"] == top, "provenance top does not match selected case")
    for name, path in expected_paths.items():
        safe_file(path, f"{name} provenance input")
        need(Path(manifest[name]).resolve() == path.resolve(), f"wrong {name} provenance path")
        need(manifest[f"{name}_sha256"] == sha256(path), f"wrong {name} provenance digest")
    safe_file(raw_path, "raw pre-InterfaceInfer IR")
    need(not raw_path.parent.is_symlink(), "raw IR parent directory must not be a symlink")
    need(manifest["raw_ir_sha256"] == sha256(raw_path), "wrong raw IR provenance digest")
    need(os.path.commonpath((str(raw_path.resolve()), str(output.resolve()))) == str(output.resolve()),
         "raw IR path escapes compiler output directory")
    need(raw_path.name == "before_Frontend::InterfaceInfer.raw" and raw_path.parent.name == "panda-temp",
         "raw IR input is not the pre-InterfaceInfer graph dump")

    rtl_path = output / f"{top}.v"
    xml_path = output / "panda-temp/architecture.xml"
    stderr_path = output / "stderr.log"
    for artifact, label in ((rtl_path, "top RTL"), (xml_path, "architecture XML"),
                            (stderr_path, "compiler stderr log")):
        safe_file(artifact, label)
    rtl = rtl_path.read_text(encoding="utf-8", errors="replace")
    stderr = stderr_path.read_text(encoding="utf-8", errors="replace")
    trace_lines = [line for line in stderr.splitlines() if "AXI_BURST_PREANALYSIS" in line]
    matches = list(TRACE.finditer(stderr))
    (expected_reason, expected_count, expected_read_fallback, expected_write_fallback,
     expected_semantic, expected_profile_eligible) = CASES[top]
    need(len(trace_lines) == len(matches), "malformed or ambiguous preanalysis trace line")
    need(len(matches) == expected_count,
         f"expected exactly {expected_count} preanalysis trace record(s), found {len(matches)}")
    owners: set[str] = set()
    for match in matches:
        (bundle, owner, _reads, _writes, read_requested, read_recognized, read_fallback,
         write_requested, write_recognized, write_fallback, candidate, reason) = match.groups()
        need(bundle == "shared", f"trace names unexpected bundle {bundle}")
        need(owner not in owners, f"duplicate trace owner {owner}")
        owners.add(owner)
        need((read_requested, read_recognized, read_fallback) == ("1", "0", expected_read_fallback),
             "read request did not retain the exact legacy fallback in preanalysis trace")
        need((write_requested, write_recognized, write_fallback) == ("1", "0", expected_write_fallback),
             "write request did not retain the exact legacy fallback in preanalysis trace")
        expected_candidate = "1" if expected_semantic else "0"
        need(candidate == expected_candidate and reason == expected_reason,
             f"candidate outcome/reason mismatch: got {candidate}/{reason!r}")

    proof_records = parse_proof_records(stderr)
    if expected_semantic:
        need(len(proof_records) == expected_count,
             "expected exactly one structured duplex proof record per owner")
    else:
        need(len(proof_records) in {0, 1},
             "failed candidates may omit the bundle proof record or provide one non-success record")
    proof_by_owner: dict[int, dict[str, object]] = {}
    for record in proof_records:
        required = {"bundle", "semantic_proof", "semantic_reason", "profile_eligible", "profile_reason"}
        need(required <= record.keys(), "duplex proof record is missing required contract fields")
        need(record["bundle"] == "shared", "duplex proof record names an unexpected bundle")
        need(type(record["semantic_proof"]) is bool and type(record["profile_eligible"]) is bool,
             "semantic/profile proof flags must be booleans")
        need(isinstance(record["semantic_reason"], str) and isinstance(record["profile_reason"], str),
             "semantic/profile reasons must be strings")
        semantic = bool(record["semantic_proof"])
        need(semantic == expected_semantic and record["semantic_reason"] == expected_reason,
             "structured semantic proof outcome/reason mismatch")
        if semantic:
            need(IDENTITY_FIELDS <= record.keys(), "semantic proof record is missing identity fields")
        else:
            need(not (IDENTITY_FIELDS & record.keys()), "failed candidate exposes success-only identity fields")
        if semantic:
            owner_id = int(record["owner_argument_index"])
            need(str(owner_id) in owners, "structured semantic proof owner has no matching preanalysis trace")
            need(owner_id not in proof_by_owner, f"duplicate structured proof owner {owner_id}")
            proof_by_owner[owner_id] = record
            verify_success_record(record, raw_path.read_text(encoding="utf-8", errors="replace"), top)
        need(record["profile_eligible"] is expected_profile_eligible,
             "structured profile eligibility does not match the selected profile")
        if top == "same_base_cached":
            need(record["profile_reason"] == "cache-enabled bundle",
                 "cache-negative case lacks the explicit cache profile reason")

    root = ET.parse(xml_path).getroot()
    functions = [node for node in root.findall(".//function") if node.get("name") == top]
    need(len(functions) == 1, f"architecture XML must contain exactly one function named {top}")
    bundles = [node for node in functions[0].findall("./bundles/bundle") if node.get("name") == "shared"]
    need(len(bundles) == 1, "architecture XML must retain exactly one shared bundle")
    attrs = bundles[0].attrib
    need(attrs.get("mode") == "m_axi" and attrs.get("max_read_burst_length") == "16" and
         attrs.get("max_write_burst_length") == "16" and attrs.get("write_burst_outstanding") == "1",
         "architecture XML does not preserve requested read/write m_axi profiles")
    need(not BURST_COMPONENTS.search(rtl), "analysis-only case unexpectedly emits a burst engine")
    need(not BURST_OPS.search(rtl), "analysis-only case unexpectedly emits a burst ABI operation")
    need(LEGACY_COMPONENT.search(rtl), "analysis-only case does not retain a legacy memory resource")
    print(f"DUPLEX EFFECT ANALYSIS CHECK PASS CASE={top} SEMANTIC={1 if expected_semantic else 0} "
          f"PROFILE={1 if expected_profile_eligible else 0}")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("top", choices=tuple(CASES))
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--source", required=True, type=Path)
    parser.add_argument("--bambu", required=True, type=Path)
    parser.add_argument("--plugin", required=True, type=Path)
    parser.add_argument("--checker", required=True, type=Path)
    parser.add_argument("--runner", required=True, type=Path)
    args = parser.parse_args(argv)
    try:
        verify(args.output, args.top, args.source, args.bambu, args.plugin, args.checker, args.runner)
    except (OSError, ET.ParseError, CheckError, ValueError) as exc:
        print(f"DUPLEX EFFECT ANALYSIS CHECK FAILED: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
