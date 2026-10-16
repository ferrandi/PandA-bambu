#!/usr/bin/env python3
"""Fail-closed structural checks for the manual M-B1.1 fixtures."""
from __future__ import annotations

import argparse
import re
import sys
import xml.etree.ElementTree as ET
from pathlib import Path

READ_COMPONENT = re.compile(r"\bMinimalAXI4MasterPipelined\b")
WRITE_COMPONENT = re.compile(r"\bMinimalAXI4MasterPipelinedWrite\b")
LEGACY_COMPONENT = re.compile(r"\b(?:MinimalAXI4AdapterSingleBeat|IOB_cache_axi)\b")
OPS = {
    "configure_read": re.compile(r"fuselector_[A-Za-z0-9_]*configure_read\b"),
    "load": re.compile(r"fuselector_[A-Za-z0-9_]*burst_read\b"),
    "configure_write": re.compile(r"fuselector_[A-Za-z0-9_]*configure_write\b"),
    "store": re.compile(r"fuselector_[A-Za-z0-9_]*store_write\b"),
    "drain": re.compile(r"fuselector_[A-Za-z0-9_]*drain_write\b"),
}


class CheckError(ValueError):
    pass


def need(condition: bool, message: str) -> None:
    if not condition:
        raise CheckError(message)


def component_blocks(rtl: str, component: str) -> list[str]:
    blocks = []
    for match in re.finditer(rf"\b{re.escape(component)}\b", rtl):
        end = rtl.find(");", match.end())
        need(end >= 0, f"unterminated {component} instance")
        blocks.append(rtl[match.start():end + 2])
    return blocks


def raw_text(path: Path) -> str:
    return path.read_text(encoding="utf-8", errors="replace")


def dataflow_verify(output: Path, top: str, expected: str, rtl: str, log: str, root: ET.Element) -> None:
    functions = [node for node in root.findall(".//function")]
    need(any(node.get("name") == top for node in functions),
         f"architecture.xml has no DATAFLOW top function {top}")
    if expected == "dataflow_forwarding":
        task_functions = [node for node in functions if node.get("name") == "forwarded_write"]
        need(len(task_functions) == 1, "architecture.xml must retain exactly one forwarded_write task")
        parameters = [node for node in task_functions[0].findall("./parameters/parameter")
                      if node.get("port") == "dst"]
        need(len(parameters) == 1, "forwarded_write task must retain one dst parameter")
        task_bundle_name = parameters[0].get("bundle")
        need(task_bundle_name, "forwarded_write dst parameter has no architecture bundle")
        task_bundles = [node for node in task_functions[0].findall("./bundles/bundle")
                        if node.get("name") == task_bundle_name]
        need(len(task_bundles) == 1 and task_bundles[0].get("mode") == "m_axi" and
             task_bundles[0].get("max_write_burst_length") == "16" and
             task_bundles[0].get("write_burst_outstanding") == "1",
             "architecture.xml must retain the original forwarded_write dst m_axi B=16/O=1 profile")
        sections = re.findall(
            r"Analyzing function forwarded_write\b(.*?)(?=Analyzed function forwarded_write\b)",
            log, re.DOTALL)
        need(len(sections) == 1, "debug log must contain one forwarded_write analysis section")
        section = sections[0]
        need("Parameter dst forwarded from caller function" in section,
             "forwarded_write debug section lacks the parameter-forwarding witness")
        need(re.search(r"Protocol\s*:\s*default\b", section) and
             re.search(r"Bundle\s*:\s*task_output\b", section),
             "forwarded_write debug section does not show adapted default protocol on task_output")
        need(not WRITE_COMPONENT.search(rtl), "forwarded DATAFLOW task unexpectedly instantiated a write-burst engine")
        need(not any(pattern.search(rtl) for key, pattern in OPS.items()
                     if key in {"configure_write", "store", "drain"}),
             "forwarded DATAFLOW task contains write CONFIGURE/STORE/DRAIN ABI signals")
        need(not re.search(r"Configured write bursts? for bundle .* require AXI INCREMENTAL burst type", log),
             "forwarded task emitted an AXI INCREMENTAL requirement error under FIXED mode")
        need(not re.search(r"Configured write burst region for bundle", log),
             "forwarded DATAFLOW task unexpectedly configured a write burst")
        raw_path = output / "panda-temp/after_Frontend::InterfaceInfer.raw"
        need(raw_path.is_file() and raw_path.stat().st_size > 0,
             f"required post-InterfaceInfer raw IR missing: {raw_path}")
        post_raw = raw_text(raw_path)
        need("_bambu_artificial_ParmMgr_Write" in post_raw,
             "post-InterfaceInfer raw IR lacks the expected legacy write adapter")
        need(not re.search(r"_ParmMgr_\w*(?:ConfigureRead|ConfigureWrite|Store|Drain|BurstRead|Load)\b",
                           post_raw, re.IGNORECASE),
             "post-InterfaceInfer raw IR contains burst CONFIGURE/STORE/DRAIN/LOAD operations")
    else:
        raise CheckError(f"unknown DATAFLOW expectation: {expected}")


def verify(output: Path, top: str, expected: str) -> None:
    need(output.is_dir() and not output.is_symlink(), f"invalid output directory: {output}")
    rtl_path = output / f"{top}.v"
    xml_path = output / "panda-temp/architecture.xml"
    log_paths = (output / "stdout.log", output / "stderr.log")
    required = (rtl_path, xml_path)
    for path in required:
        need(path.is_file() and path.stat().st_size > 0, f"required non-empty artifact missing: {path}")
    for path in log_paths:
        need(path.is_file(), f"required compiler log missing: {path}")

    rtl = rtl_path.read_text(encoding="utf-8", errors="replace") if rtl_path.is_file() else ""
    log = "\n".join(path.read_text(encoding="utf-8", errors="replace") for path in log_paths)
    root = ET.parse(xml_path).getroot()
    functions = [node for node in root.findall(".//function") if node.get("name") == top]
    need(len(functions) == 1, f"architecture.xml must contain exactly one function named {top}")
    if expected == "dataflow_forwarding":
        dataflow_verify(output, top, expected, rtl, log, root)
        print(f"BUNDLE PREANALYSIS CHECK PASS CASE={top} EXPECTATION={expected}")
        return
    bundles = {node.get("name"): node.attrib for node in functions[0].findall("./bundles/bundle")}
    need(bool(bundles), "architecture.xml contains no bundle records")

    expected_read = expected == "distinct"
    expected_write = expected in {"distinct", "write_direction_only", "write_only"}
    expected_burst = expected_read or expected_write
    read_bundle = "shared" if expected in {"same_bundle_legacy", "inplace_legacy"} else "input"
    write_bundle = {"same_bundle_legacy": "shared", "inplace_legacy": "shared",
                    "mixed_legacy": "data"}.get(expected, "output")
    if expected in {"distinct", "write_direction_only", "same_bundle_legacy", "inplace_legacy"}:
        read_attrs = bundles.get(read_bundle, {})
        need(read_attrs.get("mode") == "m_axi" and read_attrs.get("max_read_burst_length") == "16",
             f"architecture.xml does not retain the {read_bundle} read-burst profile")
    if expected in {"distinct", "write_direction_only", "write_only", "same_bundle_legacy",
                    "inplace_legacy", "mixed_legacy"}:
        write_attrs = bundles.get(write_bundle, {})
        need(write_attrs.get("mode") == "m_axi" and write_attrs.get("max_write_burst_length") == "16" and
             write_attrs.get("write_burst_outstanding") == "1",
             f"architecture.xml does not retain the O=1 write-burst profile on {write_bundle}")

    has_read = bool(READ_COMPONENT.search(rtl))
    has_write = bool(WRITE_COMPONENT.search(rtl))
    op_present = {name: bool(pattern.search(rtl)) for name, pattern in OPS.items()}
    legacy = bool(LEGACY_COMPONENT.search(rtl))
    if expected_burst:
        need(has_read == expected_read, f"read-burst engine presence mismatch: expected {expected_read}")
        need(has_write == expected_write, f"write-burst engine presence mismatch: expected {expected_write}")
        need(op_present["configure_read"] == expected_read and op_present["load"] == expected_read,
             "read CONFIGURE/LOAD ABI operation mismatch")
        need(op_present["configure_write"] == expected_write and op_present["store"] == expected_write and
             op_present["drain"] == expected_write, "write CONFIGURE/STORE/DRAIN ABI operation mismatch")
        if expected == "write_direction_only":
            legacy_blocks = component_blocks(rtl, "MinimalAXI4AdapterSingleBeat")
            need(any(".m_axi_araddr(p_m_axi_input_araddr)" in block for block in legacy_blocks),
                 "legacy read engine is not wired to the rejected input bundle")
            need(all("p_m_axi_output_" not in block for block in legacy_blocks),
                 "legacy engine is spuriously wired to the accepted output bundle")
            write_blocks = component_blocks(rtl, "MinimalAXI4MasterPipelinedWrite")
            need(len(write_blocks) == 1 and ".m_axi_awready(p_m_axi_output_awready)" in write_blocks[0] and
                 "p_m_axi_input_" not in write_blocks[0],
                 "write-burst engine is not wired to the accepted output bundle")
        else:
            need(not legacy, "burst case unexpectedly instantiates a legacy memory resource")
    else:
        need(not has_read and not has_write and not any(op_present.values()),
             "legacy fallback contains residual burst resources or ABI operations")
        need(legacy, "legacy fallback does not instantiate a legacy/cache memory resource")

    if expected == "distinct":
        need("Configured read burst regions for bundle input (B=16)" in log,
             "read-burst success diagnostic missing for input")
        need("Configured write burst region for bundle output (B=16)" in log,
             "write-burst success diagnostic missing for output")
    elif expected == "write_direction_only":
        need("Keeping legacy m_axi accesses for bundle input: read-burst CFG/SSA recognition failed or bundle is not exclusive"
             in log,
             "non-qualifiable read direction lacks its bundle-specific fallback diagnostic")
        need("Configured write burst region for bundle output (B=16)" in log,
             "independent write direction was not configured")
        need(not re.search(r"Keeping legacy m_axi \w+ for bundle output:", log),
             "accepted output bundle has a spurious legacy fallback diagnostic")
    elif expected == "write_only":
        need("Configured write burst region for bundle output (B=16)" in log,
             "write-burst success diagnostic missing for output")
    elif expected == "same_bundle_legacy":
        need("Keeping legacy m_axi writes for bundle shared: bundle contains reads; same-bundle read/write requires duplex support"
             in log, "same-bundle fallback diagnostic missing for the write direction")
        need("Keeping legacy m_axi accesses for bundle shared: bundle contains stores" in log,
             "same-bundle fallback diagnostic missing for the read direction")
    elif expected == "inplace_legacy":
        need("Keeping legacy m_axi writes for bundle shared: bundle contains reads; same-bundle read/write requires duplex support"
             in log, "in-place fallback diagnostic missing for the write direction")
        need("Keeping legacy m_axi accesses for bundle shared: bundle contains stores" in log,
             "in-place fallback diagnostic missing for the read direction")
    elif expected == "mixed_legacy":
        need("Keeping legacy m_axi writes for bundle data: opaque, volatile, atomic, or unclassified memory effects"
             in log,
             "mixed-access fallback diagnostic missing for data")
    else:
        raise CheckError(f"unknown expectation: {expected}")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("expectation", choices=("distinct", "write_direction_only", "write_only",
                                                "same_bundle_legacy", "inplace_legacy", "mixed_legacy",
                                                "dataflow_forwarding"))
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--top", required=True)
    args = parser.parse_args(argv)
    try:
        verify(args.output, args.top, args.expectation)
    except (OSError, ET.ParseError, CheckError) as exc:
        print(f"BUNDLE PREANALYSIS CHECK FAILED: {exc}", file=sys.stderr)
        return 1
    print(f"BUNDLE PREANALYSIS CHECK PASS CASE={args.top} EXPECTATION={args.expectation}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
