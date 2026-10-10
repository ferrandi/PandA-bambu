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


def function_graph(records: dict[str, dict[str, str]]) -> tuple[str, str, str]:
    identifiers = {
        node for node, rec in records.items()
        if rec["kind"] == "identifier_node"
        and re.search(rf'\bstrg:\s*"{re.escape(FUNCTION)}"', rec["attrs"])
    }
    functions = [
        node for node, rec in records.items()
        if rec["kind"] == "function_val_node"
        and len(set(refs(rec["attrs"], "name")) & identifiers) == 1
    ]
    if len(functions) != 1:
        raise ValueError(f"expected one function node named {FUNCTION}; found {len(functions)}")
    fn = functions[0]
    bodies = refs(records[fn]["attrs"], "body")
    if len(bodies) != 1 or records.get(bodies[0], {}).get("kind") != "statement_list_node":
        raise ValueError(f"{FUNCTION} has a missing or ambiguous statement-list body")
    return fn, bodies[0], next(iter(identifiers))


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
        blocks[block_id] = {"loop_id": loop_id, "successors": successors}
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
