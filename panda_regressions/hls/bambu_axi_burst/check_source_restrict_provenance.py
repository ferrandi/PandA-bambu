#!/usr/bin/env python3
"""Fail-closed checks for AST-derived parameter restrict metadata."""

from __future__ import annotations

import argparse
import sys
import xml.etree.ElementTree as ET
from pathlib import Path


EXPECTED = {
    "restrict_both": {"input": "1", "output": "1"},
    "restrict_typedef": {"input": "1", "output": "0"},
    "restrict_absent": {"input": "0", "output": "0"},
    "restrict_one": {"input": "1", "output": "0"},
}


def check_architecture(path: Path, top: str) -> None:
    if top not in EXPECTED:
        raise ValueError(f"unknown restrict provenance fixture: {top}")
    if not path.is_file() or path.stat().st_size == 0:
        raise ValueError(f"missing or empty architecture XML: {path}")
    try:
        root = ET.parse(path).getroot()
    except ET.ParseError as error:
        raise ValueError(f"invalid architecture XML: {path}: {error}") from error
    functions = [node for node in root.iter("function") if node.get("symbol") == top]
    if len(functions) != 1:
        raise ValueError(f"expected exactly one architecture function for {top}, got {len(functions)}")
    parameters = functions[0].find("parameters")
    if parameters is None:
        raise ValueError(f"architecture function {top} has no parameter metadata")
    by_port: dict[str, list[ET.Element]] = {}
    for parameter in parameters.findall("parameter"):
        port = parameter.get("port")
        if port:
            by_port.setdefault(port, []).append(parameter)
    for port, expected in EXPECTED[top].items():
        entries = by_port.get(port, [])
        if len(entries) != 1:
            raise ValueError(f"expected exactly one parameter {port} in {top}, got {len(entries)}")
        actual = entries[0].get("source_restrict")
        if actual not in {"0", "1"}:
            raise ValueError(f"parameter {top}.{port} lacks canonical source_restrict=0|1 metadata")
        if actual != expected:
            raise ValueError(f"parameter {top}.{port} has source_restrict={actual}, expected {expected}")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True,
                        help="runner output containing <top>/panda-temp/architecture.xml")
    args = parser.parse_args()
    for top in EXPECTED:
        check_architecture(args.output / top / "panda-temp" / "architecture.xml", top)
    print(f"SOURCE-RESTRICT-PROVENANCE PASS: {len(EXPECTED)} compiler cases")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except ValueError as error:
        print(f"SOURCE-RESTRICT-PROVENANCE FAIL: {error}", file=sys.stderr)
        sys.exit(1)
