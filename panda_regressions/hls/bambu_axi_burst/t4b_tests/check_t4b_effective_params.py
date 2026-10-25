#!/usr/bin/env python3
"""Validate effective AXI slave parameters from a completed simulation VCD."""

from __future__ import annotations

import argparse
import json
from pathlib import Path
import sys


class VcdParameterError(ValueError):
    pass


def parse_vcd_parameters(path: Path) -> tuple[dict[str, dict[str, int]], dict[str, str]]:
    scopes: list[str] = []
    target_scopes: list[str] = []
    declarations: dict[str, dict[str, list[tuple[str, int]]]] = {}
    values: dict[str, str] = {}
    in_dumpvars = False
    ended_definitions = False
    ended_dumpvars = False

    try:
        with path.open(encoding="ascii", errors="strict") as stream:
            for line in stream:
                parts = line.strip().split()
                if not parts:
                    continue
                directive = parts[0]
                if directive == "$scope":
                    if len(parts) < 4:
                        raise VcdParameterError("malformed $scope")
                    scopes.append(parts[2])
                    scope_path = "/".join(scopes)
                    if parts[2] == "if_m_axi_gmem_fu" and "system" in scopes[:-1]:
                        target_scopes.append(scope_path)
                elif directive == "$upscope":
                    if not scopes:
                        raise VcdParameterError("unbalanced $upscope")
                    scopes.pop()
                elif directive == "$var":
                    if len(parts) < 6:
                        raise VcdParameterError("malformed $var")
                    width = int(parts[2])
                    scope_path = "/".join(scopes)
                    declarations.setdefault(scope_path, {}).setdefault(parts[4], []).append((parts[3], width))
                elif directive == "$enddefinitions":
                    ended_definitions = True
                elif directive == "$dumpvars":
                    if not ended_definitions:
                        raise VcdParameterError("$dumpvars precedes $enddefinitions")
                    in_dumpvars = True
                elif in_dumpvars:
                    if directive == "$end":
                        in_dumpvars = False
                        ended_dumpvars = True
                    elif directive.startswith("b") or directive.startswith("B"):
                        if len(parts) != 2:
                            raise VcdParameterError("malformed binary dump value")
                        values[parts[1]] = directive[1:].lower()
                    elif directive[0] in "01xXzZ" and len(directive) > 1:
                        values[directive[1:]] = directive[0].lower()
                    elif directive[0] in "rR" and len(parts) == 2:
                        values[parts[1]] = "real:" + directive[1:]
                    else:
                        raise VcdParameterError(f"unsupported dump value: {line.strip()}")
    except (OSError, UnicodeError, ValueError) as exc:
        if isinstance(exc, VcdParameterError):
            raise
        raise VcdParameterError(f"cannot parse VCD: {exc}") from exc

    if not ended_definitions or not ended_dumpvars:
        raise VcdParameterError("VCD is missing $enddefinitions or initial $dumpvars")
    if scopes:
        raise VcdParameterError("unbalanced VCD scopes")
    if len(target_scopes) != 1:
        raise VcdParameterError(f"expected one if_m_axi_gmem_fu scope, found {len(target_scopes)}")
    parent = target_scopes[0]
    if not parent.endswith("/system/if_m_axi_gmem_fu"):
        raise VcdParameterError(f"unexpected wrapper scope path: {parent}")
    child = parent + "/axi4_tb"
    if child not in declarations:
        raise VcdParameterError(f"missing direct axi4_tb child scope: {child}")

    effective: dict[str, dict[str, int]] = {}
    for scope, names in ((parent, ("READ_DELAY", "WRITE_DELAY")),
                         (child, ("READ_DELAY", "WRITE_DELAY", "QUEUE_SIZE"))):
        effective[scope] = {}
        for name in names:
            matches = declarations.get(scope, {}).get(name, [])
            if len(matches) != 1:
                raise VcdParameterError(f"expected exactly one {name} declaration in {scope}, found {len(matches)}")
            ident, width = matches[0]
            if width <= 0:
                raise VcdParameterError(f"{name} in {scope} has non-positive declared width")
            raw = values.get(ident)
            if raw is None or any(bit not in "01" for bit in raw):
                raise VcdParameterError(f"{name} in {scope} has no known initial value")
            if len(raw) > width:
                raise VcdParameterError(f"{name} in {scope} initial value exceeds declared width")
            effective[scope][name] = int(raw, 2)
    return effective, {"parent": parent, "axi4_tb": child}


def validate(vcd_path: Path, manifest_path: Path, latency: int, queue_size: int) -> None:
    effective, scopes = parse_vcd_parameters(vcd_path)
    parent = effective[scopes["parent"]]
    child = effective[scopes["axi4_tb"]]
    expected = {
        scopes["parent"]: {"READ_DELAY": latency, "WRITE_DELAY": latency},
        scopes["axi4_tb"]: {"READ_DELAY": latency - 1, "WRITE_DELAY": latency - 1,
                            "QUEUE_SIZE": queue_size + 1},
    }
    for scope, parameters in expected.items():
        for name, value in parameters.items():
            actual = effective[scope][name]
            if actual != value:
                raise VcdParameterError(f"{name} in {scope} is {actual}, expected {value}")

    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    manifest["effective_hdl_parameters"] = {
        "source": str(vcd_path),
        "scopes": scopes,
        "values": effective,
        "expected": expected,
    }
    manifest["read_delay_derived"] = latency - 1
    manifest_path.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--vcd", type=Path, required=True)
    parser.add_argument("--manifest", type=Path, required=True)
    parser.add_argument("--latency", type=int, required=True)
    parser.add_argument("--queue-size", type=int, required=True)
    args = parser.parse_args()
    try:
        validate(args.vcd, args.manifest, args.latency, args.queue_size)
    except (OSError, UnicodeError, json.JSONDecodeError, VcdParameterError) as exc:
        print(f"effective AXI parameter validation failed: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
