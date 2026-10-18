#!/usr/bin/env python3
"""Fail-closed checker for generated independent-read/write duplex runs."""
from __future__ import annotations

import argparse
import hashlib
import json
import re
import sys
import xml.etree.ElementTree as ET
from pathlib import Path

from check_duplex_effect_analysis_artifacts import (
    CheckError, TRACE, check_production_duplex, fields,
    function_graph, need, parse_proof_records, sha256, verify_success_record,
)
from check_burst_results import _parameterized_instances

SOURCE_NAME = "bambu_axi_burst_duplex_independent.cpp"
TESTBENCH_NAME = "duplex_independent_tb.cpp"
TOP = "bambu_axi_burst_duplex_independent"
PROFILE_DATA = {
    "symmetric": {"macro": "1", "read": ("16", "1", "64"), "write": ("16", "1", "32")},
    "asymmetric": {"macro": "2", "read": ("16", "2", "64"), "write": ("8", "1", "16")},
}
MARKER = re.compile(r"^DUPLEX_INDEPENDENT_GOLDEN_PASS PROFILE=(symmetric|asymmetric) N=38$")
REASON = "distinct-base restricted pointwise read-before-write candidate"
ACTIVATION = f"Activated distinct-base restricted pointwise duplex m_axi bundle shared for root {TOP}"
WARNING = ("WARNING: distinct-base duplex relies on the caller's __restrict contract; the caller must "
           "ensure the accessed regions do not overlap (no runtime range validation is performed)")
EXPECTED_INTERMEDIATE_FALLBACKS = (
    "Keeping legacy m_axi writes for bundle shared: write-burst CFG/SSA/effect recognition failed",
    "Keeping legacy m_axi reads for bundle shared: read-burst CFG/SSA recognition failed or bundle is not exclusive",
)
ARTIFACT_SUFFIXES = {".v", ".sv", ".xml", ".log", ".raw"}
INPUTS = ("source", "testbench", "checker", "runner", "bambu", "settings", "plugin", "simulator",
          "parallel_helper")


def safe_file(path: Path, label: str, root: Path | None = None) -> Path:
    need(not path.is_symlink(), f"{label} is a symlink: {path}")
    current = path.parent
    while current != current.parent:
        need(not current.is_symlink(), f"{label} path contains symlink ancestor: {current}")
        if root is not None and current.resolve() == root.resolve():
            break
        current = current.parent
    need(path.is_file() and path.stat().st_size > 0, f"required non-empty {label} missing: {path}")
    if root is not None:
        try:
            path.resolve(strict=True).relative_to(root.resolve(strict=True))
        except (OSError, ValueError) as exc:
            raise CheckError(f"{label} escapes profile directory: {path}") from exc
    return path.resolve(strict=True)


def parse_key_values(path: Path, label: str) -> dict[str, str]:
    safe_file(path, label)
    result: dict[str, str] = {}
    for number, line in enumerate(path.read_text(encoding="utf-8", errors="replace").splitlines(), 1):
        need(bool(line) and "=" in line, f"malformed {label} line {number}")
        key, value = line.split("=", 1)
        need(bool(key) and bool(value) and key not in result, f"duplicate or empty {label} key at line {number}")
        result[key] = value
    return result


def check_manifest(profile_dir: Path) -> dict[str, str]:
    manifest_path = safe_file(profile_dir / "artifact-sha256.txt", "artifact-sha256.txt", profile_dir)
    declared: dict[Path, str] = {}
    for number, line in enumerate(manifest_path.read_text(encoding="utf-8", errors="replace").splitlines(), 1):
        match = re.fullmatch(r"([0-9a-fA-F]{64})  (.+)", line)
        need(match is not None, f"malformed artifact manifest line {number}")
        relative = Path(match.group(2))
        need(not relative.is_absolute(), f"artifact manifest path is absolute: {relative}")
        path = safe_file(profile_dir / relative, "manifest artifact", profile_dir)
        need(path not in declared, f"duplicate artifact manifest entry: {relative}")
        digest = match.group(1).lower()
        need(sha256(path) == digest, f"artifact digest mismatch: {relative}")
        declared[path] = digest
    actual: set[Path] = set()
    for path in profile_dir.rglob("*"):
        if path.is_symlink():
            try:
                target = path.resolve(strict=True)
                target.relative_to(profile_dir.resolve(strict=True))
            except (OSError, RuntimeError, ValueError) as exc:
                raise CheckError(f"profile symlink escapes profile or has no valid target: {path.relative_to(profile_dir)}") from exc
            need(path.suffix.lower() not in ARTIFACT_SUFFIXES and
                 not (target.is_file() and target.suffix.lower() in ARTIFACT_SUFFIXES),
                 f"profile contains an artifact symlink: {path.relative_to(profile_dir)}")
        elif path.is_file() and path.suffix.lower() in ARTIFACT_SUFFIXES:
            actual.add(path.resolve())
    need(bool(actual) and set(declared) == actual,
         "artifact manifest inventory does not exactly match generated RTL/XML/log/raw files")
    return {str(path.relative_to(profile_dir.resolve())): digest for path, digest in declared.items()}


def check_provenance(profile_dir: Path, profile: str, args: argparse.Namespace) -> dict[str, str]:
    provenance = parse_key_values(profile_dir / "provenance.txt", "provenance.txt")
    expected: dict[str, str] = {
        "profile": profile, "macro": PROFILE_DATA[profile]["macro"], "feature": "1",
        "output": str(profile_dir.resolve()),
        "read_profile": "/".join(PROFILE_DATA[profile]["read"]),
        "write_profile": "/".join(PROFILE_DATA[profile]["write"]),
        "cwd": str(profile_dir.resolve()), "TMPDIR": str((profile_dir / "tmp").resolve()),
    }
    paths = {
        "source": args.source, "testbench": args.testbench, "checker": Path(__file__),
        "runner": args.runner, "bambu": args.bambu, "settings": args.settings,
        "plugin": args.plugin, "simulator": args.simulator,
        "parallel_helper": args.runner.resolve().parent / "parallel_helpers.sh",
    }
    for name, path in paths.items():
        safe_file(path, f"provenance input {name}")
        expected[name] = str(path.resolve())
        expected[f"{name}_sha256"] = sha256(path)
    for key, value in expected.items():
        need(provenance.get(key) == value, f"provenance {key} does not match the selected run")
    need(bool(re.fullmatch(r"[0-9a-fA-F]{7,40}", provenance.get("git_head", ""))),
         "provenance git_head is missing or malformed")
    argv = provenance.get("argv", "")
    need(argv and "--bambu-parameter=experimental-m-axi-burst=1" in argv and
         f"-DDUPLEX_INDEPENDENT_PROFILE={PROFILE_DATA[profile]['macro']}" in argv and
         f"--top-fname={TOP}" in argv and "--simulator=VERILATOR" in argv,
         "provenance argv does not enable the feature and selected fixture profile")
    need(set(provenance) == set(expected) | {"git_head", "argv"},
         "provenance fields are missing or unexpected")
    return provenance


def check_architecture(profile_dir: Path, profile: str, raw_text: str,
                       proof: dict[str, object]) -> dict[str, str]:
    path = safe_file(profile_dir / "panda-temp/architecture.xml", "architecture.xml", profile_dir)
    root = ET.parse(path).getroot()
    functions = [node for node in root.findall(".//function") if node.get("name") == TOP]
    need(len(functions) == 1, f"architecture XML must contain exactly one {TOP} function")
    bundles = [node for node in functions[0].findall("./bundles/bundle") if node.get("name") == "shared"]
    need(len(bundles) == 1, "architecture XML must retain exactly one shared bundle")
    attrs = bundles[0].attrib
    read_burst, read_outstanding, read_fifo = PROFILE_DATA[profile]["read"]
    write_burst, write_outstanding, write_fifo = PROFILE_DATA[profile]["write"]
    need(attrs.get("mode") == "m_axi" and attrs.get("max_read_burst_length") == read_burst and
         attrs.get("num_read_outstanding", "1") == read_outstanding and attrs.get("read_fifo_depth") == read_fifo and
         attrs.get("max_write_burst_length") == write_burst and
         attrs.get("write_burst_outstanding", "1") == write_outstanding and
         attrs.get("write_fifo_depth") == write_fifo,
         "architecture XML does not preserve the requested independent read/write profile")
    parameters = functions[0].find("./parameters")
    need(parameters is not None, "architecture XML has no parameter metadata")
    raw_nodes, function_node, _blocks, _reachable = function_graph(raw_text, TOP)
    args_in_order = [int(value) for value in fields(raw_nodes[function_node]).get("arg", [])]
    expected_owners = (("src", int(proof["read_owner_argument_index"])),
                       ("dst", int(proof["write_owner_argument_index"])))
    by_index: dict[int, list[ET.Element]] = {}
    for parameter in parameters.findall("./parameter"):
        index = parameter.get("index")
        if index is not None and index.isdecimal():
            by_index.setdefault(int(index), []).append(parameter)
    for port, owner in expected_owners:
        need(owner in args_in_order, f"{port} proof owner is not a formal argument")
        entries = by_index.get(args_in_order.index(owner), [])
        need(len(entries) == 1 and entries[0].get("port") == port and
             entries[0].get("bundle") == "shared" and entries[0].get("source_restrict") == "1",
             f"architecture XML does not retain restricted shared-bundle metadata for {port}")
    return attrs


def check_profile(profile_dir: Path, profile: str, args: argparse.Namespace) -> dict[str, object]:
    need(profile_dir.name == profile and profile_dir.is_dir() and not profile_dir.is_symlink(),
         f"profile directory missing or invalid: {profile_dir}")
    profile_dir = profile_dir.resolve(strict=True)
    required = ["bambu_results.xml", "stdout.log", "stderr.log", "HLS_output/simulation/testbench.log",
                f"{TOP}.v", "panda-temp/before_Frontend::InterfaceInfer.raw", "panda-temp/architecture.xml"]
    for name in required:
        safe_file(profile_dir / name, name, profile_dir)
    manifest = check_manifest(profile_dir)
    need(set(required) <= set(manifest), "artifact manifest omits required proof, RTL, XML, or simulation-log evidence")
    provenance = check_provenance(profile_dir, profile, args)
    testbench_log = (profile_dir / "HLS_output/simulation/testbench.log").read_text(
        encoding="utf-8", errors="replace")
    expected_marker = f"DUPLEX_INDEPENDENT_GOLDEN_PASS PROFILE={profile} N=38"
    marker_lines = [line for line in testbench_log.splitlines() if MARKER.fullmatch(line)]
    need(marker_lines == [expected_marker], "exact golden marker is missing or duplicated in testbench.log")

    stdout = (profile_dir / "stdout.log").read_text(encoding="utf-8", errors="replace")
    stderr = (profile_dir / "stderr.log").read_text(encoding="utf-8", errors="replace")
    combined = stdout + "\n" + stderr
    fallback_lines = [re.sub(r"^\s*(?:---)?", "", line).strip()
                      for line in combined.splitlines()
                      if re.match(r"^\s*(?:---)?Keeping legacy m_axi\b", line)]
    need(sorted(fallback_lines) == sorted(EXPECTED_INTERMEDIATE_FALLBACKS),
         "compiler emitted missing, unexpected, or duplicate legacy fallback diagnostics")
    need(len(re.findall(rf"(?m)^\s*(?:---)?{re.escape(ACTIVATION)}\s*$", combined)) == 1,
         "real distinct-base activation marker is missing or duplicated")
    need(len(re.findall(rf"(?m)^\s*{re.escape(WARNING)}\s*$", combined)) == 1,
         "caller __restrict non-overlap contract warning is missing or duplicated")
    traces = list(TRACE.finditer(stderr))
    trace_lines = [line for line in stderr.splitlines() if "AXI_BURST_PREANALYSIS" in line]
    need(len(traces) == 2 and len(trace_lines) == 2,
         "expected exactly two well-formed preanalysis owner records")
    owners: set[str] = set()
    directions: set[tuple[str, str]] = set()
    for trace in traces:
        (bundle, owner, reads, writes, read_requested, read_recognized, read_fallback,
         write_requested, write_recognized, write_fallback, candidate, reason,
         activated, activation_reason) = trace.groups()
        need(bundle == "shared" and owner not in owners, "preanalysis owner records are not unique shared-bundle entries")
        owners.add(owner)
        need((read_requested, write_requested, candidate, reason, activated, activation_reason) ==
             ("1", "1", "1", REASON, "1", "frozen application-wide proof and profile accepted"),
             "preanalysis does not prove requested, recognized, activated duplex for both owners")
        need(read_fallback == "-" and write_fallback == "-",
             "preanalysis owner record contains a fallback reason")
        directions.add((reads, writes, read_recognized, write_recognized))
    need(directions == {("1", "0", "1", "0"), ("0", "1", "0", "1")},
         "preanalysis records do not identify one read owner and one write owner")

    raw_path = profile_dir / "panda-temp/before_Frontend::InterfaceInfer.raw"
    raw_text = raw_path.read_text(encoding="utf-8", errors="replace")
    proof_records = parse_proof_records(stderr)
    need(len(proof_records) == 1, "expected exactly one structured semantic proof record")
    proof = proof_records[0]
    need(proof.get("bundle") == "shared" and proof.get("semantic_proof") is True and
         proof.get("semantic_reason") == REASON and proof.get("proof_kind") == "distinct_base_restricted_pointwise" and
         proof.get("profile_eligible") is True and proof.get("profile_reason") == "read/write profile is eligible",
         "structured semantic proof does not witness the expected restricted pointwise candidate")
    verify_success_record(proof, raw_text, TOP, require_load_store_data_dependency=False)
    attrs = check_architecture(profile_dir, profile, raw_text, proof)
    rtl_path = profile_dir / f"{TOP}.v"
    rtl = rtl_path.read_text(encoding="utf-8", errors="replace")
    check_production_duplex(rtl, combined, TOP, attrs, distinct_base_override=True)
    for name, expected in (("MinimalAXI4MasterPipelined", PROFILE_DATA[profile]["read"]),
                           ("MinimalAXI4MasterPipelinedWrite", PROFILE_DATA[profile]["write"])):
        instances = list(_parameterized_instances(rtl, name))
        need(len(instances) == 1, f"expected one {name} instance with a complete profile")
        actual = instances[0]
        expected_parameters = {"B_MAX": expected[0], "MAX_OUTSTANDING": expected[1], "FIFO_DEPTH": expected[2],
                              "PEER_FAULT_ENABLE": "1"}
        need(all(actual.get(key) == value for key, value in expected_parameters.items()),
             f"{name} does not preserve its complete independent profile in one instance")
    return {"profile": profile, "marker": expected_marker, "git_head": provenance["git_head"],
            "artifact_manifest": manifest}


def check_run_inventory(run_dir: Path) -> None:
    entries = list(run_dir.iterdir())
    need(all(not path.is_symlink() for path in entries), "run directory contains a symlink")
    children = {path.name for path in entries if path.is_dir()}
    expected = set(PROFILE_DATA) | {"job_logs"}
    need(children == expected,
         f"run directory inventory mismatch: expected {sorted(expected)}, got {sorted(children)}")
    need((run_dir / "job_logs").is_dir() and not (run_dir / "job_logs").is_symlink(),
         "runner-owned job_logs directory is missing or invalid")


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("run_dir", type=Path)
    parser.add_argument("--source", required=True, type=Path)
    parser.add_argument("--testbench", required=True, type=Path)
    parser.add_argument("--bambu", required=True, type=Path)
    parser.add_argument("--settings", required=True, type=Path)
    parser.add_argument("--plugin", required=True, type=Path)
    parser.add_argument("--simulator", required=True, type=Path)
    parser.add_argument("--runner", required=True, type=Path)
    parser.add_argument("--report", required=True, type=Path)
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)
    try:
        run_dir = args.run_dir.resolve(strict=True)
        need(not args.run_dir.is_symlink() and run_dir.is_dir(), "run directory is missing or a symlink")
        current = args.run_dir.absolute()
        while current != current.parent:
            need(not current.is_symlink(), f"run directory path contains symlink: {current}")
            current = current.parent
        need(args.source.name == SOURCE_NAME and args.testbench.name == TESTBENCH_NAME,
             "source/testbench do not name the required independent-RW fixtures")
        safe_file(args.source, "source")
        safe_file(args.testbench, "testbench")
        check_run_inventory(run_dir)
        results = {profile: check_profile(run_dir / profile, profile, args) for profile in PROFILE_DATA}
        report_path = args.report
        if not report_path.is_absolute():
            report_path = run_dir / report_path
        parent = report_path.parent.resolve(strict=True)
        need(parent == run_dir or run_dir in parent.parents, "report path escapes run directory")
        current = report_path.absolute()
        while current != current.parent:
            need(not current.is_symlink(), f"report path contains symlink: {current}")
            if current.resolve() == run_dir:
                break
            current = current.parent
        need(not report_path.is_symlink() and (not report_path.exists() or report_path.is_file()),
             "report path is not a regular file")
        report_path.write_text(json.dumps({"run_dir": str(run_dir), "profiles": results}, indent=2,
                                          sort_keys=True) + "\n", encoding="utf-8")
        print(f"DUPLEX INDEPENDENT CHECK PASS: {run_dir}")
        return 0
    except (OSError, ET.ParseError, ValueError) as exc:
        print(f"DUPLEX INDEPENDENT CHECK FAILED: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
