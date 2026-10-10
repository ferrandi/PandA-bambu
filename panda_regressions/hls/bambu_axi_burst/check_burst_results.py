#!/usr/bin/env python3
"""Fail-closed cycle and generated burst-contract gate for the Mantis suite."""

from __future__ import annotations

import argparse
import csv
from pathlib import Path
import re
import sys
import xml.etree.ElementTree as ET


FIELDS = ("case", "cycles", "source", "burst_max", "outstanding", "fifo_depth", "addr_bits")
INTEGER = re.compile(r"(?:0|[1-9][0-9]*)\Z")
# The compiler accepts a 32 or 64 bit address bus for the burst resource; the
# gate witnesses that the generated instance carries the width the case asked
# for, so the expectation table has to name it.
ADDR_BITS_ALLOWED = {"32", "64"}
EXPECTED_CASES = {f"sum-o{o}-n{n}" for o in (5, 8, 16) for n in (1024, 2048)}
# A bundle 'latency' attribute is a property of the memory reached through that
# bundle, and it must reach the testbench memory model for both directions.
ATTR_LATENCY = re.compile(r"latency=(\d+)")


def fail(message: str) -> None:
    raise ValueError(message)


def read_expectations(path: Path) -> list[dict[str, str]]:
    try:
        with path.open(encoding="utf-8", newline="") as stream:
            reader = csv.DictReader(stream, delimiter="\t")
            if reader.fieldnames != list(FIELDS):
                fail(f"expectation header must be {list(FIELDS)}")
            rows = list(reader)
    except OSError as exc:
        fail(f"cannot read expectation file {path}: {exc}")
    if not rows:
        fail("expectation file contains no required cycle cases")
    actual_cases = {row.get("case", "") for row in rows}
    # The P2 matrix cases are mandatory; extra cases are allowed so that
    # non-saturated profiles (e.g. num_read_outstanding=1) can be gated too.
    missing = sorted(EXPECTED_CASES - actual_cases)
    if missing:
        fail(f"expectation file does not cover the P2 matrix; missing={missing}")
    seen = set()
    for row in rows:
        if None in row or any(not row.get(field) for field in FIELDS):
            fail("expectation row is incomplete or malformed")
        if row["case"] in seen:
            fail(f"duplicate expected case: {row['case']}")
        seen.add(row["case"])
        for field in ("cycles", "burst_max", "outstanding", "fifo_depth"):
            if not INTEGER.fullmatch(row[field]):
                fail(f"{row['case']}: {field} is not an unsigned decimal integer")
        if row["addr_bits"] not in ADDR_BITS_ALLOWED:
            fail(f"{row['case']}: addr_bits must be one of {sorted(ADDR_BITS_ALLOWED)}")
        if int(row["cycles"]) <= 0:
            fail(f"{row['case']}: expected cycle count must be positive")
        profile = path.parent / row["source"]
        if not profile.is_file():
            fail(f"{row['case']}: expected profile source is missing: {profile}")
        source = profile.read_text(encoding="utf-8")
        required = (f"max_read_burst_length={row['burst_max']}",
                    f"num_read_outstanding={row['outstanding']}",
                    f"read_fifo_depth={row['fifo_depth']}")
        if not all(token in source for token in required):
            fail(f"{row['case']}: static source pragma does not match expected profile")
    return rows


def check_case(results: Path, row: dict[str, str], suite_dir: Path) -> None:
    case_dir = results / "AXI-BURST" / row["case"]
    xml_path = case_dir / "bambu_results.xml"
    if not xml_path.is_file():
        fail(f"{row['case']}: missing Bambu results XML: {xml_path}")
    try:
        root = ET.parse(xml_path).getroot()
    except (OSError, ET.ParseError) as exc:
        fail(f"{row['case']}: malformed/unreadable Bambu results XML: {exc}")
    evaluations = [element for element in root.iter("evaluation") if "CYCLES" in element.attrib]
    if len(evaluations) != 1:
        fail(f"{row['case']}: expected exactly one evaluation CYCLES attribute")
    actual = evaluations[0].get("CYCLES", "")
    if not INTEGER.fullmatch(actual):
        fail(f"{row['case']}: malformed evaluation CYCLES={actual!r}")
    if int(actual) != int(row["cycles"]):
        fail(f"{row['case']}: expected {row['cycles']} cycles, got {actual}")

    check_profile_rtl(case_dir, row["case"], row["burst_max"],
                      row["outstanding"], row["fifo_depth"], row["addr_bits"])
    check_bundle_latency(case_dir, row["case"], suite_dir / row["source"])


def check_bundle_latency(case_dir: Path, case: str, source: Path) -> None:
    """Witness that a bundle 'latency' attribute drives that bundle's memory model.

    Cases without the attribute are skipped: their delay comes from the global
    --mem-delay-read/--mem-delay-write, which this gate does not know.
    """

    if not source.is_file():
        fail(f"{case}: expected case source is missing: {source}")
    latencies = set(ATTR_LATENCY.findall(source.read_text(encoding="utf-8")))
    if not latencies:
        return
    if len(latencies) > 1:
        fail(f"{case}: source declares several latency values: {sorted(latencies)}")
    latency = latencies.pop()
    testbenches = [path for path in case_dir.rglob("bambu_testbench.v") if path.is_file()]
    if not testbenches:
        fail(f"{case}: generated testbench is missing, cannot check the bundle latency")
    bodies = [path.read_text(encoding="utf-8") for path in testbenches]
    for name in ("READ_DELAY", "WRITE_DELAY"):
        if not any(re.search(rf"\.{name}\(\s*{latency}\s*\)", body) for body in bodies):
            fail(f"{case}: bundle declares latency={latency}, but the generated testbench "
                 f"carries no .{name}({latency})")


def check_profile_rtl(case_dir: Path, case: str, burst_max: str,
                      outstanding: str, fifo_depth: str, addr_bits: str) -> None:

    rtl_files = [path for path in case_dir.rglob("*")
                 if path.is_file() and path.suffix.lower() in (".v", ".sv")
                 and path.name != "panda_libtech.v"]
    if not rtl_files:
        fail(f"{case}: generated RTL is missing")
    pattern = re.compile(
        rf"MinimalAXI4MasterPipelined\s*#\s*\(\s*\.B_MAX\s*\(\s*{burst_max}\s*\)\s*,\s*"
        rf"\.MAX_OUTSTANDING\s*\(\s*{outstanding}\s*\)\s*,\s*"
        rf"\.FIFO_DEPTH\s*\(\s*{fifo_depth}\s*\)\s*"
        rf"(?:,\s*\.\w+\s*\(\s*[^()]*\s*\)\s*)*\)\s*\w+\s*\(", re.S)
    # The width parameters must be wired to the real port widths. Leaving them
    # at the XML defaults, or reading them from the wrong operand, silently
    # truncates a signal: a one-bit BITSIZE_in5 makes the engine fetch a single
    # element and the co-simulation hangs instead of finishing.
    width_patterns = [
        re.compile(rf"\.BITSIZE_{name}\s*\(\s*{width}\s*\)", re.S)
        for name, width in (("m_axi_araddr", addr_bits), ("in4", addr_bits),
                            ("m_axi_rdata", 32), ("in5", 32))
    ]
    for path in rtl_files:
        text = path.read_text(encoding="utf-8", errors="replace")
        if pattern.search(text) and all(p.search(text) for p in width_patterns):
            return
    fail(f"{case}: generated RTL does not contain the expected burst profile")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--results", required=True, type=Path)
    parser.add_argument("--expectations", required=True, type=Path)
    args = parser.parse_args(argv)
    try:
        if not args.results.is_dir():
            fail(f"Mantis results directory is missing: {args.results}")
        rows = read_expectations(args.expectations)
        for row in rows:
            check_case(args.results, row, args.expectations.parent)
        boundary_source = args.expectations.parent / "bambu_axi_burst_boundary.cpp"
        if not boundary_source.is_file():
            fail(f"valid-boundary-maxima: expected profile source is missing: {boundary_source}")
        source = boundary_source.read_text(encoding="utf-8")
        if not all(token in source for token in (
                "max_read_burst_length=256", "num_read_outstanding=16", "read_fifo_depth=4096")):
            fail("valid-boundary-maxima: static source pragma does not declare B=256/O=16/D=4096")
        boundary_dir = args.results / "AXI-BURST" / "valid-boundary-maxima"
        if not (boundary_dir / "bambu_results.xml").is_file():
            fail(f"valid-boundary-maxima: missing Bambu results XML: {boundary_dir / 'bambu_results.xml'}")
        check_profile_rtl(boundary_dir, "valid-boundary-maxima", "256", "16", "4096", "32")
    except (OSError, UnicodeError, ValueError) as exc:
        print(f"AXI_BURST_CYCLE_GATE_FAIL: {exc}", file=sys.stderr)
        return 1
    print(f"AXI_BURST_CYCLE_GATE_PASS: {len(rows)} cases; exact evaluation CYCLES and RTL profiles")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
