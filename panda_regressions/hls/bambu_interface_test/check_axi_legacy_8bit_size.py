#!/usr/bin/env python3
"""Check that active legacy 8-bit AXI address transactions carry AxSIZE=0."""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path


class CheckError(ValueError):
    pass


def check_vcd(path: Path) -> tuple[int, int]:
    if path.is_symlink() or not path.is_file():
        raise CheckError(f"missing or unsafe VCD: {path}")
    text = path.read_text(encoding="ascii", errors="strict")
    if "$enddefinitions $end" not in text:
        raise CheckError("VCD has no $enddefinitions")

    declarations: dict[tuple[str, ...], dict[str, dict[int | None, str]]] = {}
    widths: dict[tuple[tuple[str, ...], str], int] = {}
    clock_declarations: dict[tuple[str, ...], list[tuple[str, int, int | None]]] = {}
    scope_stack: list[str] = []
    for line in text.splitlines():
        fields = line.split()
        if fields and fields[0] == "$scope" and len(fields) >= 4 and fields[-1] == "$end":
            scope_stack.append(fields[2])
            continue
        if fields and fields[0] == "$upscope" and fields[-1] == "$end":
            if not scope_stack:
                raise CheckError("VCD contains an unmatched $upscope")
            scope_stack.pop()
            continue
        if len(fields) < 6 or fields[0] != "$var" or fields[-1] != "$end":
            continue
        try:
            width = int(fields[2])
        except ValueError as exc:
            raise CheckError("invalid VCD signal width") from exc
        identifier, name = fields[3], fields[4]
        scope = tuple(scope_stack)
        bit = None
        if len(fields) >= 7 and re.fullmatch(r"\[\d+\]", fields[5]):
            bit = int(fields[5][1:-1])
        if name == "clock":
            clock_declarations.setdefault(scope, []).append((identifier, width, bit))
        if name.startswith("sig_m_axi_"):
            declarations.setdefault(scope, {}).setdefault(name, {})[bit] = identifier
            widths[(scope, name)] = width

    suffixes = ("arvalid", "arready", "arsize", "awvalid", "awready", "awsize")
    candidates = []
    for scope, scoped_declarations in declarations.items():
        prefixes = {name[: -len("arsize")] for name in scoped_declarations if name.endswith("arsize")}
        for prefix in prefixes:
            names = {suffix: prefix + suffix for suffix in suffixes}
            if not all(name in scoped_declarations for name in names.values()):
                continue
            size_bits_valid = all(set(scoped_declarations[names[size]].keys()) == {0, 1, 2} and
                                  widths[(scope, names[size])] == 1 for size in ("arsize", "awsize"))
            scalar_controls = all(widths[(scope, names[name])] == 1 and
                                  scoped_declarations[names[name]].keys() == {None}
                                  for name in ("arvalid", "arready", "awvalid", "awready"))
            if size_bits_valid and scalar_controls:
                candidates.append((scope, names))
    if len(candidates) != 1:
        raise CheckError(f"expected one complete scalarized AXI master, found {len(candidates)}")
    scope, signals = candidates[0]
    clocks = clock_declarations.get(scope, [])
    if len(clocks) != 1 or clocks[0][1:] != (1, None):
        raise CheckError("expected one scalar clock in the AXI master scope")
    clock_id = clocks[0][0]
    scoped_declarations = declarations[scope]
    values = {clock_id: "x"}
    for parts in scoped_declarations.values():
        values.update({identifier: "x" for identifier in parts.values()})
    selected_ids = set(values)
    widths_by_id = {
        identifier: widths[(scope, name)]
        for name, parts in scoped_declarations.items()
        for identifier in parts.values()
    }
    counts = {"ar": 0, "aw": 0}

    def sample() -> None:
        for channel in ("ar", "aw"):
            if values[scoped_declarations[signals[channel + "valid"]][None]] != "1":
                continue
            bits = "".join(values[scoped_declarations[signals[channel + "size"]][bit]] for bit in (2, 1, 0))
            if any(bit not in "01" for bit in bits):
                raise CheckError(f"{channel.upper()}SIZE contains X/Z while VALID is high at a rising clock edge")
            if bits != "000":
                raise CheckError(
                    f"{channel.upper()}SIZE={bits} while VALID is high at a rising clock edge, expected 000")
            if values[scoped_declarations[signals[channel + "ready"]][None]] == "1":
                counts[channel] += 1

    in_values = False
    timestamp: int | None = None
    timestamp_values: dict[str, str] = {}
    rising_edges = 0

    def record_value(identifier: str, value: str) -> None:
        nonlocal rising_edges
        if timestamp is None:
            values[identifier] = value
            return
        previous = timestamp_values.get(identifier, values[identifier])
        if identifier == clock_id and previous == "0" and value == "1":
            rising_edges += 1
        timestamp_values[identifier] = value

    def finish_timestamp() -> None:
        nonlocal rising_edges
        for _ in range(rising_edges):
            sample()
        values.update(timestamp_values)
        timestamp_values.clear()
        rising_edges = 0

    for line in text.splitlines():
        line = line.strip()
        if line.startswith("$enddefinitions"):
            in_values = True
            continue
        if not in_values or line.startswith("$"):
            continue
        if line.startswith("#"):
            try:
                next_timestamp = int(line[1:])
            except ValueError as exc:
                raise CheckError("invalid VCD timestamp") from exc
            if timestamp is not None and next_timestamp < timestamp:
                raise CheckError("VCD timestamps are not monotonic")
            if timestamp is not None and next_timestamp != timestamp:
                finish_timestamp()
            timestamp = next_timestamp
            continue
        scalar = re.fullmatch(r"([01xXzZ])([^\s]+)", line)
        if scalar and scalar.group(2) in selected_ids:
            record_value(scalar.group(2), scalar.group(1).lower())
            continue
        vector = re.fullmatch(r"[bB]([01xXzZ]+)\s+([^\s]+)", line)
        if vector and vector.group(2) in selected_ids:
            identifier = vector.group(2)
            if widths_by_id[identifier] != len(vector.group(1)):
                raise CheckError("wrong vector width for an AXI master signal")
            record_value(identifier, vector.group(1).lower())
    if timestamp is not None:
        finish_timestamp()
    if counts["ar"] < 1 or counts["aw"] < 1:
        raise CheckError(f"missing active read/write AXI handshakes: AR={counts['ar']} AW={counts['aw']}")
    return counts["ar"], counts["aw"]


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--vcd", type=Path, default=Path("HLS_output/simulation/test.vcd"))
    args = parser.parse_args()
    try:
        ar, aw = check_vcd(args.vcd)
    except (OSError, UnicodeError, CheckError) as exc:
        print(f"AXI legacy SIZE check failed: {exc}", file=sys.stderr)
        return 1
    print(f"AXI_LEGACY_8BIT_SIZE_PASS AR_HANDSHAKES={ar} AW_HANDSHAKES={aw} ARSIZE=0 AWSIZE=0")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
