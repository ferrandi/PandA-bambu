#!/usr/bin/env python3
"""Check the configure/read virtual-SSA ordering in Bambu raw IR dumps."""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path


CONFIGURE = "data_bambu_artificial_ParmMgr_configure_read"
BURST_READ = "data_bambu_artificial_ParmMgr_burst_read"
FUNCTION = "renamed_sum"
ALIAS_FUNCTION = "kernel"
ALIAS_CONFIGURE = "shared_bambu_artificial_ParmMgr_configure_read"
ALIAS_DRAIN = "shared_bambu_artificial_ParmMgr_drain_write"


def parse_raw(text: str) -> dict[str, dict[str, str]]:
    records: dict[str, dict[str, str]] = {}
    for line in text.splitlines():
        match = re.match(r"^(@\d+)\s+(\w+)\s*(.*)$", line)
        if not match:
            continue
        node, kind, attrs = match.groups()
        if node in records:
            raise ValueError(f"ambiguous duplicate raw-IR node {node}")
        records[node] = {"kind": kind, "attrs": attrs}
    if not records:
        raise ValueError("raw IR contains no parseable node records")
    return records


def refs(attrs: str, key: str) -> list[str]:
    return re.findall(rf"\b{re.escape(key)}:\s*(@\d+)", attrs)


def unique_node(records: dict[str, dict[str, str]], predicate, label: str) -> str:
    matches = [node for node, record in records.items() if predicate(node, record)]
    if len(matches) != 1:
        raise ValueError(f"expected exactly one {label}; found {len(matches)}")
    return matches[0]


def function_graph(records: dict[str, dict[str, str]], function_name: str = FUNCTION) -> tuple[str, str, str]:
    identifiers = {
        node for node, rec in records.items()
        if rec["kind"] == "identifier_node"
        and re.search(rf'\bstrg:\s*"{re.escape(function_name)}"', rec["attrs"])
    }
    functions = [
        node for node, rec in records.items()
        if rec["kind"] == "function_val_node"
        and len(set(refs(rec["attrs"], "name")) & identifiers) == 1
    ]
    if len(functions) != 1:
        raise ValueError(f"expected one function node named {function_name}; found {len(functions)}")
    fn = functions[0]
    bodies = refs(records[fn]["attrs"], "body")
    if len(bodies) != 1 or records.get(bodies[0], {}).get("kind") != "statement_list_node":
        raise ValueError(f"{function_name} has a missing or ambiguous statement-list body")
    return fn, bodies[0], next(iter(identifiers))


def _reachable(blocks: dict[int, dict[str, object]], start: int, target: int) -> bool:
    pending = [start]
    seen: set[int] = set()
    while pending:
        block = pending.pop()
        if block == target:
            return True
        if block in seen:
            continue
        seen.add(block)
        pending.extend(int(value) for value in blocks[block]["successors"])
    return False


def validate_scalar_fallback(interface_text: str, phi_text: str,
                              adg_text: str, fdfg_text: str) -> dict[str, str]:
    """Prove the scalar configure/drain Use-Def contract and its graph consumers."""
    pre = parse_raw(interface_text)
    post = parse_raw(phi_text)

    def alias_statements(records: dict[str, dict[str, str]], fn_name: str):
        fn, body, _ = function_graph(records, fn_name)
        statements = {node: rec for node, rec in records.items()
                      if rec["kind"].endswith("stmt") and refs(rec["attrs"], "parent") == [fn]}
        calls = {node: call_name(records, node) for node in statements}
        configures = [node for node, name in calls.items() if name == ALIAS_CONFIGURE]
        drains = [node for node, name in calls.items() if name == ALIAS_DRAIN]
        return records, fn, body, statements, configures, drains

    pre, _, pre_body, pre_statements, pre_configures, pre_drains = alias_statements(pre, ALIAS_FUNCTION)
    if not pre_configures or not pre_drains:
        raise ValueError("InterfaceInfer: missing scalar configure/drain statements")
    pre_drains_by_def = {value: stmt for stmt in pre_drains
                         for value in refs(pre_statements[stmt]["attrs"], "vdef")}
    pre_pairs = [(configure, drain_vdef, drain)
                 for configure in pre_configures
                 for drain_vdef, drain in pre_drains_by_def.items()
                 if drain_vdef in refs(pre_statements[configure]["attrs"], "vuse")]
    if len(pre_pairs) != 1:
        raise ValueError(f"InterfaceInfer: expected one direct configure-to-drain vuse; found {len(pre_pairs)}")
    pre_configure, drain_vdef, pre_drain = pre_pairs[0]
    pre_blocks = blocks_from_body(pre[pre_body]["attrs"])
    pre_cattrs, pre_dattrs = pre_statements[pre_configure]["attrs"], pre_statements[pre_drain]["attrs"]
    cbb = re.search(r"\bbb_index:\s*(\d+)", pre_cattrs)
    dbb = re.search(r"\bbb_index:\s*(\d+)", pre_dattrs)
    if not cbb or not dbb:
        raise ValueError("InterfaceInfer: configure/drain lacks a CFG block")
    configure_bb, drain_bb = int(cbb.group(1)), int(dbb.group(1))
    if not (_reachable(pre_blocks, configure_bb, drain_bb) and _reachable(pre_blocks, drain_bb, configure_bb)):
        raise ValueError("InterfaceInfer: configure/drain are not connected in both loop directions")

    post, post_fn, post_body, post_statements, post_configures, post_drains = alias_statements(post, ALIAS_FUNCTION)
    drains_by_def = {value: stmt for stmt in post_drains
                     for value in refs(post_statements[stmt]["attrs"], "vdef")}
    candidates = []
    for configure in post_configures:
        attrs = post_statements[configure]["attrs"]
        for vdef, drain in drains_by_def.items():
            vuses = refs(attrs, "vuse")
            if vdef in vuses:
                candidates.append((configure, vdef, drain, vuses))
    if len(candidates) != 1:
        raise ValueError(f"BuildVirtualPhi: expected one direct scalar configure/drain pair; found {len(candidates)}")
    configure, vdef, drain, configure_vuses = candidates[0]
    configure_attrs = post_statements[configure]["attrs"]
    drain_attrs = post_statements[drain]["attrs"]
    configure_bb_match = re.search(r"\bbb_index:\s*(\d+)", configure_attrs)
    drain_bb_match = re.search(r"\bbb_index:\s*(\d+)", drain_attrs)
    if not configure_bb_match or not drain_bb_match:
        raise ValueError("BuildVirtualPhi: scalar configure/drain lacks a basic-block identity")
    configure_bb = int(configure_bb_match.group(1))
    drain_bb = int(drain_bb_match.group(1))
    post_blocks = blocks_from_body(post[post_body]["attrs"])
    if configure_bb != drain_bb:
        raise ValueError("BuildVirtualPhi: scalar configure and drain are not in the same rebuilt block")
    if configure_bb not in post_blocks or not post_blocks[configure_bb]["predecessors"]:
        raise ValueError("BuildVirtualPhi: scalar loop header has no CFG predecessor information")
    if int(post_blocks[configure_bb]["loop_id"]) == 0:
        raise ValueError("BuildVirtualPhi: scalar PHI block is outside a loop")
    matching_phis = [node for node, rec in post.items()
                     if rec["kind"] == "phi_stmt" and refs(rec["attrs"], "parent") == [post_fn]
                     and "virtual" in rec["attrs"]
                     and refs(rec["attrs"], "res") and refs(rec["attrs"], "res")[0] in configure_vuses
                     and vdef in refs(rec["attrs"], "def")
                     and re.search(r"\bbb_index:\s*(\d+)", rec["attrs"])
                     and int(re.search(r"\bbb_index:\s*(\d+)", rec["attrs"]).group(1)) == configure_bb]
    if len(matching_phis) != 1:
        raise ValueError(f"BuildVirtualPhi: expected one virtual PHI with drain backedge; found {len(matching_phis)}")
    phi = matching_phis[0]
    phi_attrs = post[phi]["attrs"]
    phi_result = refs(phi_attrs, "res")[0]
    incoming_pairs = [(value, int(edge)) for value, edge in
                      re.findall(r"\bdef:\s*(@\d+)\s+edge:\s*(\d+)", phi_attrs)]
    incoming = [value for value, _ in incoming_pairs]
    if len(incoming) < 2 or vdef not in incoming:
        raise ValueError("BuildVirtualPhi: virtual PHI lacks initial and drain backedge inputs")
    predecessor_ids = set(int(value) for value in post_blocks[configure_bb]["predecessors"])
    incoming_edges = [edge for _, edge in incoming_pairs]
    if len(incoming_pairs) != len(refs(phi_attrs, "def")) or len(set(incoming_edges)) != len(incoming_edges):
        raise ValueError("BuildVirtualPhi: virtual PHI has malformed or duplicate edge/value pairs")
    if set(incoming_edges) != predecessor_ids:
        raise ValueError("BuildVirtualPhi: virtual PHI edge IDs do not match CFG predecessors")
    edge_values = {edge: value for value, edge in incoming_pairs}
    if int(post_blocks[drain_bb]["loop_id"]) == 0 or edge_values.get(drain_bb) != vdef:
        raise ValueError("BuildVirtualPhi: drain vdef is not incoming on the loop backedge")
    seed_edges = predecessor_ids - {drain_bb}
    if not seed_edges or any(edge not in post_blocks or int(post_blocks[edge]["loop_id"]) != 0
                             for edge in seed_edges):
        raise ValueError("BuildVirtualPhi: PHI initial input does not arrive from an outside-loop predecessor")
    seed_values = [edge_values[edge] for edge in seed_edges]
    if not seed_values or any(post.get(value, {}).get("kind") != "ssa_node" for value in seed_values):
        raise ValueError("BuildVirtualPhi: virtual PHI lacks a resolvable initial incoming value")
    for value in seed_values:
        seed_defs = refs(post[value]["attrs"], "def_stmt")
        if len(seed_defs) != 1 or post.get(seed_defs[0], {}).get("kind") != "nop_stmt":
            raise ValueError("BuildVirtualPhi: initial incoming value is not seeded outside the loop")
    drain_over = refs(drain_attrs, "vover")
    if phi_result not in drain_over:
        raise ValueError("BuildVirtualPhi: scalar drain does not overwrite the reaching virtual PHI result")
    if phi_result not in configure_vuses:
        raise ValueError("BuildVirtualPhi: configure does not use the virtual PHI result")
    phi_ssa = post.get(phi_result, {})
    drain_ssa = post.get(vdef, {})
    if configure not in refs(phi_ssa.get("attrs", ""), "use_stmt") or \
            drain not in refs(phi_ssa.get("attrs", ""), "use_stmt"):
        raise ValueError("BuildVirtualPhi: PHI result reverse use list omits configure or drain")
    if configure not in refs(drain_ssa.get("attrs", ""), "use_stmt") or \
            phi not in refs(drain_ssa.get("attrs", ""), "use_stmt"):
        raise ValueError("BuildVirtualPhi: drain vdef reverse use list omits configure or PHI")

    # Require the downstream analyses to consume this relation in both directions.
    def graph_edges(text: str) -> tuple[set[int], set[tuple[int, int, str]]]:
        nodes: dict[int, str] = {}
        for match in re.finditer(r"(?m)^\s*(\d+)\s*\[.*?label=\"(.*?)\"\s*\];", text):
            nodes[int(match.group(1))] = match.group(2)
        edges = {(int(a), int(b), label) for a, b, label in re.findall(
            r"(?m)^\s*(\d+)\s*->\s*(\d+)\s*\[[^\]]*label=\"([^\"]*)\"", text)}
        return set(nodes), edges

    def dot_node(text: str, stmt: str) -> int:
        matches = [int(node) for node, label in re.findall(
            r"(?m)^\s*(\d+)\s*\[[^\]]*label=\"([^\"]*)\"", text)
            if re.search(rf"_{re.escape(stmt.lstrip('@'))} - ", label)]
        if len(matches) != 1:
            raise ValueError(f"dot graph: expected one node for IR statement {stmt}; found {len(matches)}")
        return matches[0]

    adg_configure, adg_drain = dot_node(adg_text, configure), dot_node(adg_text, drain)
    _, adg_edges = graph_edges(adg_text)
    vers_match = re.search(r"\bvers:\s*(\d+)", post[vdef]["attrs"])
    if not vers_match:
        raise ValueError("BuildVirtualPhi: scalar drain vdef has no SSA version")
    virtual_label = f"_{vers_match.group(1)}"
    label_pattern = re.compile(rf"(?<![A-Za-z0-9_]){re.escape(virtual_label)}(?![A-Za-z0-9_])")
    if not any(a == adg_configure and b == adg_drain and label_pattern.search(label)
               for a, b, label in adg_edges):
        raise ValueError("OP_ADG lacks scalar configure-to-drain anti-dependence")
    fdfg_configure, fdfg_drain = dot_node(fdfg_text, configure), dot_node(fdfg_text, drain)
    _, fdfg_edges = graph_edges(fdfg_text)
    if not any(a == fdfg_drain and b == fdfg_configure and label_pattern.search(label)
               for a, b, label in fdfg_edges):
        raise ValueError("OP_FDFG lacks scalar drain-to-configure feedback dependency")
    return {"configure": configure, "drain": drain, "phi": phi, "phi_result": phi_result,
            "drain_vdef": vdef, "configure_bb": str(configure_bb), "drain_bb": str(drain_bb)}


def negative_scalar_contract_checks(interface_text: str, phi_text: str,
                                    adg_text: str, fdfg_text: str) -> None:
    result = validate_scalar_fallback(interface_text, phi_text, adg_text, fdfg_text)
    configure_line = re.search(rf"(?m)^{re.escape(result['configure'])}[^\n]*", phi_text)
    drain_line = re.search(rf"(?m)^{re.escape(result['drain'])}[^\n]*", phi_text)
    phi_line = re.search(rf"(?m)^{re.escape(result['phi'])}[^\n]*", phi_text)
    if not configure_line or not drain_line or not phi_line:
        raise ValueError("cannot construct scalar Use-Def negative mutations")
    phi_pairs = [(value, int(edge)) for value, edge in
                 re.findall(r"\bdef:\s*(@\d+)\s+edge:\s*(\d+)", phi_line.group(0))]
    seed_pairs = [pair for pair in phi_pairs if pair[0] != result["drain_vdef"]]
    drain_pairs = [pair for pair in phi_pairs if pair[0] == result["drain_vdef"]]
    if len(seed_pairs) != 1 or len(drain_pairs) != 1:
        raise ValueError("cannot construct PHI edge negative mutations")
    seed_value, seed_edge = seed_pairs[0]
    _, drain_edge = drain_pairs[0]
    swapped_phi_line = phi_line.group(0).replace(
        f"def: {seed_value} edge: {seed_edge}", f"def: {seed_value} edge: {drain_edge}", 1).replace(
            f"def: {result['drain_vdef']} edge: {drain_edge}",
            f"def: {result['drain_vdef']} edge: {seed_edge}", 1)
    version = re.search(r"\bvers:\s*(\d+)", parse_raw(phi_text)[result["drain_vdef"]]["attrs"])
    if not version:
        raise ValueError("cannot construct graph-label prefix collision mutation")
    virtual_label = f"_{version.group(1)}"
    label_pattern = re.compile(rf"(?<![A-Za-z0-9_]){re.escape(virtual_label)}(?![A-Za-z0-9_])")
    mutations = (
        (phi_text.replace(configure_line.group(0), configure_line.group(0).replace(
            f"vuse: {result['drain_vdef']} ", "", 1), 1), "direct configure vuse"),
        (phi_text.replace(phi_line.group(0), phi_line.group(0).replace(
            f"def: {result['drain_vdef']} edge:", "def: @0 edge:", 1), 1), "PHI drain backedge"),
        (phi_text.replace(drain_line.group(0), drain_line.group(0).replace(
            f"vover: {result['phi_result']} ", "", 1), 1), "drain vover"),
        (phi_text.replace(phi_line.group(0), phi_line.group(0).replace(
            f"edge: {seed_edge}", "edge: 999999", 1), 1), "invalid PHI edge"),
        (phi_text.replace(phi_line.group(0), swapped_phi_line, 1), "mismatched PHI edge definitions"),
    )
    for mutated, label in mutations:
        if mutated == phi_text:
            raise ValueError(f"failed to create {label} negative mutation")
        try:
            validate_scalar_fallback(interface_text, mutated, adg_text, fdfg_text)
        except ValueError:
            continue
        raise ValueError(f"negative mutation unexpectedly passed after removing {label}")
    bad_adg = label_pattern.sub(virtual_label + "0", adg_text)
    bad_fdfg = label_pattern.sub(virtual_label + "0", fdfg_text)
    if bad_adg == adg_text or bad_fdfg == fdfg_text:
        raise ValueError("failed to create graph-label prefix collision mutation")
    try:
        validate_scalar_fallback(interface_text, phi_text, bad_adg, bad_fdfg)
    except ValueError:
        pass
    else:
        raise ValueError("negative mutation unexpectedly passed with an SSA label prefix collision")


def call_name(records: dict[str, dict[str, str]], statement: str) -> str | None:
    rec = records[statement]
    attrs = rec["attrs"]
    fn_refs = refs(attrs, "fn")
    # assign_stmt wraps an expression in op: call_node rather than fn:.
    for operand in refs(attrs, "op"):
        if records.get(operand, {}).get("kind") == "call_node":
            fn_refs += refs(records[operand]["attrs"], "fn")
    names: list[str] = []
    for fn_ref in fn_refs:
        fn_rec = records.get(fn_ref)
        if not fn_rec or fn_rec["kind"] not in ("addr_node", "function_val_node"):
            continue
        value = refs(fn_rec["attrs"], "op") if fn_rec["kind"] == "addr_node" else []
        targets = value or [fn_ref]
        for target in targets:
            target_rec = records.get(target)
            if target_rec and target_rec["kind"] == "function_val_node":
                name_ids = refs(target_rec["attrs"], "name")
                for name_id in name_ids:
                    name_rec = records.get(name_id)
                    if name_rec and name_rec["kind"] == "identifier_node":
                        found = re.search(r'\bstrg:\s*"([^"]+)"', name_rec["attrs"])
                        if found:
                            names.append(found.group(1))
    if len(set(names)) > 1:
        raise ValueError(f"statement {statement} has ambiguous callee names: {sorted(set(names))}")
    return names[0] if names else None


def blocks_from_body(body_attrs: str) -> dict[int, dict[str, object]]:
    starts = list(re.finditer(r"\bbloc:\s*(\d+)\s+loop_id:\s*(\d+)", body_attrs))
    if not starts:
        raise ValueError("function CFG has no basic blocks")
    blocks: dict[int, dict[str, object]] = {}
    for index, start in enumerate(starts):
        end = starts[index + 1].start() if index + 1 < len(starts) else len(body_attrs)
        block_id, loop_id = map(int, start.groups())
        if block_id in blocks:
            raise ValueError(f"ambiguous duplicate basic block BB{block_id}")
        section = body_attrs[start.end():end]
        successors = [int(value) for value in re.findall(r"\bsucc:\s*(\d+)", section)]
        predecessors = [int(value) for value in re.findall(r"\bpred:\s*(\d+)", section)]
        blocks[block_id] = {"loop_id": loop_id, "successors": successors, "predecessors": predecessors}
    return blocks


def reaches_configure(records: dict[str, dict[str, str]], value: str,
                      configure_vdef: str, visiting: set[str]) -> bool:
    if value == configure_vdef:
        return True
    if value in visiting:
        return False
    value_rec = records.get(value)
    if value_rec is None:
        raise ValueError(f"unresolved virtual SSA value {value}")
    if value_rec["kind"] != "ssa_node":
        raise ValueError(f"virtual-use edge {value} does not resolve to an SSA node")
    if "virtual" not in value_rec["attrs"]:
        raise ValueError(f"virtual-use edge {value} resolves to a non-virtual SSA value")
    defs = refs(value_rec["attrs"], "def_stmt")
    if len(defs) != 1:
        raise ValueError(f"virtual SSA value {value} has {len(defs)} defining statements")
    definition = records.get(defs[0])
    if definition is None:
        raise ValueError(f"unresolved defining statement {defs[0]} for {value}")
    if definition["kind"] != "phi_stmt":
        # The default virtual definition is a legitimate path that precedes configure.
        if "virtual default" in value_rec["attrs"]:
            return False
        return False
    if "virtual" not in definition["attrs"]:
        raise ValueError(f"virtual-use chain reaches non-virtual phi {defs[0]}")
    incoming = refs(definition["attrs"], "def")
    if not incoming:
        raise ValueError(f"virtual phi {defs[0]} has no resolvable incoming definitions")
    reaches = [reaches_configure(records, edge, configure_vdef, visiting | {value})
               for edge in incoming]
    return any(reaches)


def validate_dump(text: str, stage: str) -> dict[str, object]:
    records = parse_raw(text)
    fn, body, _ = function_graph(records)
    statements = {
        node: rec for node, rec in records.items()
        if rec["kind"].endswith("stmt") and refs(rec["attrs"], "parent") == [fn]
    }
    configure_stmts = [node for node in statements if call_name(records, node) == CONFIGURE]
    read_stmts = [node for node in statements if call_name(records, node) == BURST_READ]
    if len(configure_stmts) != 1:
        raise ValueError(f"{stage}: expected one configure call in {FUNCTION}; found {len(configure_stmts)}")
    if not read_stmts:
        raise ValueError(f"{stage}: no transformed burst-read calls found in {FUNCTION}")

    configure = configure_stmts[0]
    configure_attrs = statements[configure]["attrs"]
    vdefs = refs(configure_attrs, "vdef")
    if len(vdefs) != 1:
        raise ValueError(f"{stage}: configure has {len(vdefs)} virtual definitions")
    configure_vdef = vdefs[0]
    configure_bb_match = re.search(r"\bbb_index:\s*(\d+)", configure_attrs)
    if not configure_bb_match:
        raise ValueError(f"{stage}: configure has no basic-block identity")
    configure_bb = int(configure_bb_match.group(1))

    body_attrs = records[body]["attrs"]
    blocks = blocks_from_body(body_attrs)
    if configure_bb not in blocks:
        raise ValueError(f"{stage}: configure BB{configure_bb} is absent from CFG")
    if blocks[configure_bb]["loop_id"] != 0:
        raise ValueError(f"{stage}: configure BB{configure_bb} is inside a loop")

    read_bbs: list[int] = []
    for read in read_stmts:
        attrs = statements[read]["attrs"]
        bb_match = re.search(r"\bbb_index:\s*(\d+)", attrs)
        if not bb_match:
            raise ValueError(f"{stage}: burst read {read} has no basic-block identity")
        read_bb = int(bb_match.group(1))
        if read_bb not in blocks or int(blocks[read_bb]["loop_id"]) == 0:
            raise ValueError(f"{stage}: burst read {read} is not inside a loop")
        read_bbs.append(read_bb)
        vuses = refs(attrs, "vuse")
        if not vuses:
            raise ValueError(f"{stage}: burst read {read} has no virtual use")
        reaching_edges = [reaches_configure(records, vuse, configure_vdef, set())
                          for vuse in vuses]
        if not any(reaching_edges):
            raise ValueError(f"{stage}: configure vdef {configure_vdef} does not reach burst read {read}")

    # The configure block must directly enter a loop containing the reads. A
    # loop edge back to the configure block would re-run configuration.
    loop_read_blocks = {bb for bb in read_bbs if int(blocks[bb]["loop_id"]) != 0}
    if not set(blocks[configure_bb]["successors"]) & loop_read_blocks:
        raise ValueError(f"{stage}: configure BB{configure_bb} is not a loop preheader for the reads")
    for bb, info in blocks.items():
        if int(info["loop_id"]) != 0 and configure_bb in info["successors"]:
            raise ValueError(f"{stage}: loop backedge from BB{bb} re-enters configure BB{configure_bb}")

    return {"stage": stage, "configure": configure, "configure_bb": configure_bb,
            "configure_vdef": configure_vdef, "reads": read_stmts,
            "read_bbs": sorted(set(read_bbs))}


def negative_mutation_check(text: str, stage: str) -> None:
    result = validate_dump(text, stage)
    direct_ref = f"vuse: {result['configure_vdef']}"
    if direct_ref not in text:
        # This focused fixture is expected to expose configure directly in the
        # burst-read vuse list; retain this guard so the mutation stays relevant.
        raise ValueError(f"{stage}: cannot construct negative check; direct configure vuse absent")
    mutated, count = re.subn(re.escape(direct_ref), "", text, count=1)
    if count != 1:
        raise ValueError(f"{stage}: failed to remove configure vuse for negative check")
    try:
        validate_dump(mutated, f"{stage} negative mutation")
    except ValueError:
        return
    raise ValueError(f"{stage}: negative mutation unexpectedly passed without configure vuse")


def one_file(directory: Path, pattern: str, label: str) -> Path:
    matches = sorted(directory.glob(pattern))
    if len(matches) != 1:
        raise ValueError(f"expected exactly one {label} in {directory}; found {len(matches)}")
    return matches[0]


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("dump_dir", type=Path)
    args = parser.parse_args()
    panda_temp = args.dump_dir / "panda-temp"
    interface_path = one_file(panda_temp, "after_Frontend::InterfaceInfer.raw", "InterfaceInfer raw dump")
    build_phi_path = one_file(panda_temp, "after_Frontend::BuildVirtualPhi::renamed_sum*.raw",
                              "renamed_sum BuildVirtualPhi raw dump")
    interface_text = interface_path.read_text(errors="strict")
    build_phi_text = build_phi_path.read_text(errors="strict")
    for stage, text in (("InterfaceInfer", interface_text), ("BuildVirtualPhi", build_phi_text)):
        result = validate_dump(text, stage)
        negative_mutation_check(text, stage)
        print(f"PASS {stage}: configure BB{result['configure_bb']} vdef {result['configure_vdef']} "
              f"reaches {len(result['reads'])} burst read(s) in BB{result['read_bbs']}; "
              "preheader is outside loop and no backedge re-enters it; negative mutation rejected")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (OSError, ValueError) as exc:
        print(f"FAIL: {exc}", file=sys.stderr)
        sys.exit(1)
