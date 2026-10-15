#!/usr/bin/env python3
"""Fail-closed cycle and generated burst-contract gate for the Mantis suite."""

from __future__ import annotations

import argparse
import csv
from pathlib import Path
import re
import sys
import xml.etree.ElementTree as ET


FIELDS = ("case", "mode", "cycles", "source", "burst_max", "outstanding", "fifo_depth", "addr_bits")
INTEGER = re.compile(r"(?:0|[1-9][0-9]*)\Z")
# The compiler accepts a 32 or 64 bit address bus for the burst resource; the
# gate witnesses that the generated instance carries the width the case asked
# for, so the expectation table has to name it.
ADDR_BITS_ALLOWED = {"32", "64"}
# This inventory is intentionally independent of the TSV: deleting a row from
# the table must not silently remove a qualified read or write case.
REQUIRED_CASE_MODES = {
    **{f"sum-o{o}-n{n}": "read" for o in (5, 8, 16) for n in (1024, 2048)},
    "sum-o1-n1024": "read",
    "sum-o1-n2048": "read",
    "sum-o1-n1024-addr64": "read",
    "sum-o1-n1024-lat128": "read",
    "write-burst-n1024": "write",
    "write-burst-n2048": "write",
    "write-burst-n1024-addr64": "write",
}
# A bundle 'latency' attribute is a property of the memory reached through that
# bundle, and it must reach the testbench memory model for both directions.
ATTR_LATENCY = re.compile(r"latency=(\d+)")
LEGACY_FIELDS = ("case", "cycles", "source")
CONFIGURED_BURST_CORE = re.compile(
    r"\bMinimalAXI4MasterPipelined\b|\bMinimalAXI4MasterPipelinedWrite\b|"
    r"configure_read|configure_write|burst_configure", re.I)


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
    actual_modes = {row.get("case", ""): row.get("mode", "") for row in rows}
    # Extra cases are allowed, but the current qualified read/write inventory
    # and each case's direction are mandatory.
    missing = sorted(REQUIRED_CASE_MODES.keys() - actual_modes.keys())
    if missing:
        fail(f"expectation file does not cover required cycle cases; missing={missing}")
    wrong_modes = sorted((case, mode, actual_modes[case])
                         for case, mode in REQUIRED_CASE_MODES.items()
                         if actual_modes[case] != mode)
    if wrong_modes:
        fail(f"required cycle case mode does not match inventory: {wrong_modes}")
    seen = set()
    for row in rows:
        if None in row or any(not row.get(field) for field in FIELDS):
            fail("expectation row is incomplete or malformed")
        if row["case"] in seen:
            fail(f"duplicate expected case: {row['case']}")
        seen.add(row["case"])
        if row["mode"] not in {"read", "write"}:
            fail(f"{row['case']}: mode must be 'read' or 'write'")
        if row["cycles"] == "PENDING":
            fail(f"{row['case']}: cycle signature is PENDING; measure and qualify it before enabling this case")
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
        profile_attrs = {
            "read": ("max_read_burst_length", "num_read_outstanding", "read_fifo_depth"),
            "write": ("max_write_burst_length", "write_burst_outstanding", "write_fifo_depth"),
        }[row["mode"]]
        required = tuple(f"{attribute}={value}" for attribute, value in zip(
            profile_attrs, (row["burst_max"], row["outstanding"], row["fifo_depth"])))
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

    check_profile_rtl(case_dir, row["case"], row["mode"], row["burst_max"],
                      row["outstanding"], row["fifo_depth"], row["addr_bits"])
    check_bundle_latency(case_dir, row["case"], suite_dir / row["source"])


def read_legacy_expectations(path: Path) -> list[dict[str, str]]:
    try:
        with path.open(encoding="utf-8", newline="") as stream:
            reader = csv.DictReader(stream, delimiter="\t")
            if reader.fieldnames != list(LEGACY_FIELDS):
                fail(f"legacy expectation header must be {list(LEGACY_FIELDS)}")
            rows = list(reader)
    except OSError as exc:
        fail(f"cannot read legacy expectation file {path}: {exc}")
    if not rows:
        fail("legacy expectation file contains no cases")
    seen = set()
    for row in rows:
        if None in row or any(not row.get(field) for field in LEGACY_FIELDS):
            fail("legacy expectation row is incomplete or malformed")
        if row["case"] in seen:
            fail(f"duplicate legacy expected case: {row['case']}")
        seen.add(row["case"])
        if not INTEGER.fullmatch(row["cycles"]) or int(row["cycles"]) <= 0:
            fail(f"{row['case']}: expected cycle count must be a positive unsigned integer")
        if not (path.parent / row["source"]).is_file():
            fail(f"{row['case']}: expected legacy source is missing: {path.parent / row['source']}")
    required_legacy = "plain-m-axi-write-n1024"
    if required_legacy not in seen:
        fail(f"legacy expectation file is missing required case: {required_legacy}")
    return rows


def check_legacy_case(results: Path, row: dict[str, str]) -> None:
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
    rtl_files = [path for path in case_dir.rglob("*")
                 if path.is_file() and path.suffix.lower() in (".v", ".sv")
                 and path.name != "panda_libtech.v"]
    if not rtl_files:
        fail(f"{row['case']}: generated RTL is missing")
    for rtl in rtl_files:
        text = rtl.read_text(encoding="utf-8", errors="replace")
        match = CONFIGURED_BURST_CORE.search(text)
        if match:
            fail(f"{row['case']}: legacy scalar write unexpectedly contains configured burst core "
                 f"{match.group(0)} in {rtl}")


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


def _parameterized_instances(text: str, module_name: str):
    """Yield named parameters from exact module instances, not across instances."""
    marker = re.compile(rf"\b{re.escape(module_name)}\s*#\s*\(")
    for match in marker.finditer(text):
        start = match.end()
        depth = 1
        index = start
        while index < len(text) and depth:
            if text[index] == "(":
                depth += 1
            elif text[index] == ")":
                depth -= 1
            index += 1
        if depth:
            continue
        if not re.match(r"\s*[A-Za-z_$][\w$]*\s*\(", text[index:]):
            continue
        body = text[start:index - 1]
        yield {name: value.strip() for name, value in re.findall(
            r"\.(\w+)\s*\(\s*([^()]*)\s*\)", body)}


def check_profile_rtl(case_dir: Path, case: str, mode: str, burst_max: str,
                      outstanding: str, fifo_depth: str, addr_bits: str) -> None:

    rtl_files = [path for path in case_dir.rglob("*")
                 if path.is_file() and path.suffix.lower() in (".v", ".sv")
                 and path.name != "panda_libtech.v"]
    if not rtl_files:
        fail(f"{case}: generated RTL is missing")
    module_name = {"read": "MinimalAXI4MasterPipelined",
                   "write": "MinimalAXI4MasterPipelinedWrite"}[mode]
    required = {"B_MAX": burst_max, "MAX_OUTSTANDING": outstanding,
                "FIFO_DEPTH": fifo_depth}
    widths = ({"BITSIZE_m_axi_araddr": addr_bits, "BITSIZE_in4": addr_bits,
               "BITSIZE_m_axi_rdata": "32", "BITSIZE_in5": "32"}
              if mode == "read" else
              {"BITSIZE_in4": addr_bits, "BITSIZE_in5": "32",
               "BITSIZE_m_axi_awaddr": addr_bits, "BITSIZE_in3": "32",
               "BITSIZE_m_axi_wdata": "32"})
    required.update(widths)
    for path in rtl_files:
        text = path.read_text(encoding="utf-8", errors="replace")
        for parameters in _parameterized_instances(text, module_name):
            if all(parameters.get(name) == value for name, value in required.items()):
                return
    fail(f"{case}: generated RTL does not contain the expected {mode} burst profile in one {module_name} instance")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--results", required=True, type=Path)
    parser.add_argument("--expectations", required=True, type=Path)
    parser.add_argument("--legacy-expectations", type=Path)
    args = parser.parse_args(argv)
    try:
        if not args.results.is_dir():
            fail(f"Mantis results directory is missing: {args.results}")
        rows = read_expectations(args.expectations)
        for row in rows:
            check_case(args.results, row, args.expectations.parent)
        legacy_rows = []
        if args.legacy_expectations:
            legacy_rows = read_legacy_expectations(args.legacy_expectations)
            for row in legacy_rows:
                check_legacy_case(args.results, row)
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
        check_profile_rtl(boundary_dir, "valid-boundary-maxima", "read", "256", "16", "4096", "32")
    except (OSError, UnicodeError, ValueError) as exc:
        print(f"AXI_BURST_CYCLE_GATE_FAIL: {exc}", file=sys.stderr)
        return 1
    mode_counts = {mode: sum(row["mode"] == mode for row in rows) for mode in ("read", "write")}
    print(f"AXI_BURST_CYCLE_GATE_PASS: {mode_counts['read']} read cases, "
          f"{mode_counts['write']} write cases, and {len(legacy_rows)} legacy cases; "
          "exact evaluation CYCLES and RTL profiles")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
