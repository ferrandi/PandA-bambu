#!/usr/bin/env python3
"""Extract the configured AXI write engine from its XML source cell."""

from __future__ import annotations

import argparse
import html
import re
import sys
import xml.etree.ElementTree as ET
from pathlib import Path


IDENTIFIER = re.compile(r"[A-Za-z_][A-Za-z0-9_]*\Z")
CELL_NAME = "MinimalAXI4MasterPipelinedWrite"
TARGETS = ("MinimalAXI4MasterPipelined", CELL_NAME)


def _fail(message: str) -> None:
    raise ValueError(message)


def _raw_body(xml_path: Path, target: str) -> str:
    source = xml_path.read_text(encoding="utf-8")
    matches = list(re.finditer(r"<cell(?:\s[^>]*)?>.*?</cell>", source, re.DOTALL))
    matching = []
    for match in matches:
        if re.search(rf"<name>\s*{re.escape(target)}\s*</name>", match.group()):
            matching.append(match.group())
    if len(matching) != 1:
        _fail(f"expected exactly one raw XML cell named {target}, found {len(matching)}")
    attribute = re.search(r'\bVERILOG_PROVIDED="([^"]*)"', matching[0], re.DOTALL)
    if attribute is None:
        _fail(f"{target} is missing VERILOG_PROVIDED")
    return html.unescape(attribute.group(1)).replace("\r\n", "\n")


def extract(xml_path: Path, target: str = CELL_NAME, *, async_reset: bool = False,
            active_low: bool = True) -> str:
    try:
        root = ET.parse(xml_path).getroot()
    except (ET.ParseError, OSError) as error:
        _fail(f"cannot parse XML {xml_path}: {error}")
    cells = [cell for cell in root.findall(".//cell") if cell.findtext("name") == target]
    if len(cells) != 1:
        _fail(f"expected exactly one XML cell named {target}, found {len(cells)}")
    module = cells[0].find("circuit/module_o")
    if module is None or module.get("id") != target:
        _fail(f"unsupported XML structure for {target}: missing matching circuit/module_o")

    parameters: dict[str, str] = {}
    for item in module.findall("parameter"):
        name, value = item.get("name"), (item.text or "").strip()
        if not name or not IDENTIFIER.fullmatch(name):
            _fail(f"invalid parameter descriptor in {target}")
        if name in parameters:
            _fail(f"duplicate parameter {name} in {target}")
        if not re.fullmatch(r"[0-9]+", value):
            _fail(f"unsupported non-integer default for parameter {name}: {value!r}")
        parameters[name] = value

    ports: dict[str, tuple[str, int]] = {}
    for item in module.findall("port_o"):
        name, direction = item.get("id"), item.get("dir")
        if not name or not IDENTIFIER.fullmatch(name) or direction not in ("IN", "OUT"):
            _fail(f"invalid port descriptor in {target}")
        if name in ports:
            _fail(f"duplicate port {name} in {target}")
        descriptor = item.find("structural_type_descriptor")
        if descriptor is None or descriptor.get("size") != "1":
            _fail(f"unsupported structural type for port {name}")
        kind = descriptor.get("type")
        if kind == "BOOL":
            width = 1
        elif kind == "VECTOR_BOOL":
            value = descriptor.get("vector_size", "")
            if not re.fullmatch(r"[1-9][0-9]*", value):
                _fail(f"invalid vector width for port {name}: {value!r}")
            width = int(value)
        else:
            _fail(f"unsupported structural type {kind!r} for port {name}")
        ports[name] = (direction.lower(), width)

    implementations = module.findall("NP_functionality")
    if len(implementations) != 1:
        _fail(f"expected exactly one NP_functionality for {target}, found {len(implementations)}")
    functionality = implementations[0]
    library = (functionality.get("LIBRARY") or "").split()
    if len(library) != len(set(library)):
        _fail(f"duplicate token in NP_functionality LIBRARY for {target}")
    library_set = set(library)
    if target not in library_set:
        _fail(f"NP_functionality LIBRARY is missing component token {target}")
    for name in parameters:
        if name not in library_set:
            _fail(f"NP_functionality LIBRARY is missing parameter {name}")
    for token in library_set - {target}:
        if token not in parameters and token not in ports:
            _fail(f"NP_functionality LIBRARY token {token} has no XML parameter or port")
    body = _raw_body(xml_path, target)
    parsed_body = functionality.get("VERILOG_PROVIDED") or ""
    if not body.strip():
        _fail(f"{target} has an empty VERILOG_PROVIDED implementation")
    if " ".join(body.split()) != " ".join(parsed_body.split()):
        _fail(f"raw and parsed VERILOG_PROVIDED content disagree for {target}")
    reset_edge = "or negedge reset" if active_low else "or posedge reset"
    body = body.replace("1RESET_EDGE_FORCE", reset_edge)
    body = body.replace("1RESET_EDGE", reset_edge if async_reset else "")
    body = body.replace("1RESET_VALUE", f"reset == 1'b{0 if active_low else 1}")
    unresolved = re.search(r"1[A-Z][A-Z0-9_]*", body)
    if unresolved:
        _fail(f"unresolved HDL placeholder {unresolved.group(0)} in {target}")
    referenced_widths = set(re.findall(r"\bBITSIZE_([A-Za-z_][A-Za-z0-9_]*)", body))
    for name in referenced_widths:
        if name not in ports:
            _fail(f"HDL references width for undeclared XML port {name}")
        if name not in library_set:
            _fail(f"NP_functionality LIBRARY is missing port {name}")

    param_lines = [f"    parameter integer {name} = {value}" for name, value in parameters.items()]
    port_lines: list[str] = []
    for name, (direction, width) in ports.items():
        sv_direction = "input" if direction == "in" else "output"
        if name in library_set:
            param_lines.append(f"    parameter integer BITSIZE_{name} = {width}")
            range_text = f" [BITSIZE_{name}-1:0]"
        else:
            range_text = "" if width == 1 else f" [{width - 1}:0]"
        port_lines.append(f"    {sv_direction} wire{range_text} {name}")
    header = (f"// Generated from {xml_path.name}\nmodule {target} #(\n" +
              ",\n".join(param_lines) + "\n) (\n" +
              ",\n".join(port_lines) + "\n);\n")
    return header + body + "\nendmodule\n"


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--xml", type=Path, required=True)
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--target", choices=TARGETS, default=CELL_NAME)
    parser.add_argument("--reset-mode", choices=("sync", "async"), default="sync")
    parser.add_argument("--reset-polarity", choices=("active-low", "active-high"),
                         default="active-low")
    args = parser.parse_args(argv)
    try:
        text = extract(args.xml, target=args.target, async_reset=args.reset_mode == "async",
                       active_low=args.reset_polarity == "active-low")
    except (OSError, ValueError) as error:
        parser.error(str(error))
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(text, encoding="utf-8")
    print(f"Extracted {args.target} from {args.xml}; generated {args.out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
