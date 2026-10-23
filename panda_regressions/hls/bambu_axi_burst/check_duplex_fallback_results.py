#!/usr/bin/env python3
"""Fail-closed checker for the generated duplex fallback cases."""
from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import sys
import xml.etree.ElementTree as ET
from pathlib import Path


PROFILES = {
    "phased_erew_reverse_same_bundle": {
        "macro": "1",
        "read_reason": "memory writes or opaque effects; prefetch ordering is not proven",
        "write_reason": "opaque, volatile, atomic, or unclassified memory effects",
    },
    "overlap_rw_same_bundle": {
        "macro": "2",
        "read_reason": "bundle contains stores",
        "write_reason": "bundle contains reads; same-bundle read/write requires duplex support",
    },
    "conditional_read_or_write_same_bundle": {
        "macro": "3",
        "read_reason": "memory writes or opaque effects; prefetch ordering is not proven",
        "write_reason": "opaque, volatile, atomic, or unclassified memory effects",
        "same_base_duplex_reason": "original effects are not fully classified ordinary accesses",
        "require_unrecognized": True,
        "read_accesses": "2",
        "write_accesses": "3",
    },
    "residual_store_same_bundle": {
        "macro": "4",
        "read_reason": "memory writes or opaque effects; prefetch ordering is not proven",
        "write_reason": "opaque, volatile, atomic, or unclassified memory effects",
        "same_base_duplex_reason": "original effects are not fully classified ordinary accesses",
        "require_unrecognized": True,
        "read_accesses": "1",
        "write_accesses": "2",
    },
}
SOURCE_NAME = "bambu_axi_burst_duplex_fallback.cpp"
TESTBENCH_NAME = "duplex_fallback_tb.cpp"
MARKER = re.compile(
    r"(?m)^DUPLEX_FALLBACK_GOLDEN_PASS PROFILE=(phased_erew_reverse_same_bundle|overlap_rw_same_bundle|"
    r"conditional_read_or_write_same_bundle|residual_store_same_bundle) N=38\s*$"
)
FALLBACK = re.compile(
    r"(?m)^\s*(?:---)?Keeping legacy m_axi (?P<direction>accesses|writes|reads) "
    r"for bundle shared: (?P<reason>.+?)\s*$"
)
PREANALYSIS = re.compile(
    r'^\s*AXI_BURST_PREANALYSIS bundle=shared owner_argument_index=\d+ '
    r'read_accesses=(?P<read_accesses>\d+) write_accesses=(?P<write_accesses>\d+) '
    r'read_requested=(?P<read_requested>[01]) read_recognized=(?P<read_recognized>[01]) '
    r'read_fallback="(?P<read_reason>[^"]+)" '
    r'write_requested=(?P<write_requested>[01]) write_recognized=(?P<write_recognized>[01]) '
    r'write_fallback="(?P<write_reason>[^"]+)" '
    r'same_base_duplex_candidate=(?P<candidate>[01]) '
    r'same_base_duplex_reason="(?P<duplex_reason>[^"]*)" '
    r'production_activated=(?P<activated>[01]) activation_reason="(?P<activation_reason>[^"]*)"\s*$',
    re.MULTILINE,
)
DUPLEX_ACTIVATION = re.compile(
    r"(?m)^\s*(?:---\s*)?Activated (?:distinct-base restricted pointwise|same-base pointwise) "
    r"duplex m_axi bundle shared(?:\s|$)"
)
LEGACY = re.compile(r"\b(?:MinimalAXI4AdapterSingleBeat|IOB_cache_axi)\b")
LEGACY_INSTANCE = re.compile(
    r"\b(?:MinimalAXI4AdapterSingleBeat|IOB_cache_axi)\b\s*"
    r"(?:#\s*\([^;]*?\)\s*)?[A-Za-z_$][\w$]*\s*\("
)
BURST = re.compile(r"\b(?:MinimalAXI4MasterPipelined(?:Write)?|MinimalAXI4(?:Read)?Burst)\b")
BURST_OP = re.compile(r"\bfuselector_[A-Za-z0-9_]*(?:configure_(?:read|write)|burst_read|store_write|drain_write)\b")
ARTIFACT_SUFFIXES = {".v", ".sv", ".xml", ".log"}
PROVENANCE_HASH_FIELDS = ("source", "testbench", "checker", "runner", "bambu", "settings", "plugin", "simulator")


class CheckError(ValueError):
    pass


def need(condition: bool, message: str) -> None:
    if not condition:
        raise CheckError(message)


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def safe_file(path: Path, label: str, nonempty: bool = True, root: Path | None = None) -> Path:
    need(not path.is_symlink(), f"{label} is a symlink: {path}")
    if root is not None:
        root = root.resolve(strict=True)
        current = path.parent
        while current != current.parent:
            need(not current.is_symlink(), f"{label} path contains symlink ancestor: {current}")
            if current.resolve() == root:
                break
            current = current.parent
        try:
            path.resolve(strict=True).relative_to(root)
        except (OSError, ValueError) as exc:
            raise CheckError(f"{label} escapes profile directory: {path}") from exc
    need(path.is_file(), f"{label} is missing: {path}")
    if nonempty:
        need(path.stat().st_size > 0, f"{label} is empty: {path}")
    return path.resolve(strict=True)


def parse_key_values(path: Path, label: str) -> dict[str, str]:
    safe_file(path, label)
    fields: dict[str, str] = {}
    for number, line in enumerate(path.read_text(encoding="utf-8", errors="replace").splitlines(), 1):
        need(bool(line) and "=" in line, f"malformed {label} line {number}")
        key, value = line.split("=", 1)
        need(bool(key) and bool(value) and key not in fields, f"duplicate or empty {label} field at line {number}")
        fields[key] = value
    return fields


def contained_file(root: Path, raw_path: str, label: str) -> Path:
    candidate = Path(raw_path)
    if not candidate.is_absolute():
        candidate = root / candidate
    current = candidate
    while current != current.parent:
        need(not current.is_symlink(), f"{label} path contains symlink: {raw_path}")
        if current.resolve() == root.resolve():
            break
        current = current.parent
    resolved = safe_file(candidate, label)
    try:
        resolved.relative_to(root.resolve())
    except ValueError as exc:
        raise CheckError(f"{label} escapes profile directory: {raw_path}") from exc
    return resolved


def check_artifact_manifest(profile_dir: Path) -> dict[str, str]:
    manifest = profile_dir / "artifact-sha256.txt"
    safe_file(manifest, "artifact-sha256.txt")
    root = profile_dir.resolve()
    declared: dict[Path, str] = {}
    for number, line in enumerate(manifest.read_text(encoding="utf-8", errors="replace").splitlines(), 1):
        match = re.fullmatch(r"([0-9a-fA-F]{64})  (.+)", line)
        need(match is not None, f"malformed artifact manifest line {number}")
        path = contained_file(root, match.group(2), "manifest artifact")
        need(path not in declared, f"duplicate artifact manifest entry: {match.group(2)}")
        expected = match.group(1).lower()
        need(sha256(path) == expected, f"artifact digest mismatch: {match.group(2)}")
        declared[path] = expected
    actual: set[Path] = set()
    for path in profile_dir.rglob("*"):
        if path.is_symlink():
            need(path.suffix.lower() not in ARTIFACT_SUFFIXES,
                 f"generated artifact is a symlink: {path.relative_to(profile_dir)}")
            continue
        if path.is_file() and path.suffix.lower() in ARTIFACT_SUFFIXES:
            actual.add(path.resolve())
    need(bool(actual), "profile contains no generated RTL/XML/log artifacts")
    need(set(declared) == actual, "artifact manifest inventory does not exactly match generated RTL/XML/log files")
    return {str(path.relative_to(root)): value for path, value in declared.items()}


def top_rtl(profile_dir: Path) -> tuple[str, list[Path]]:
    rtl_files = [path for path in profile_dir.rglob("*") if path.is_file() and
                 path.suffix.lower() in {".v", ".sv"} and path.name.lower() != "panda_libtech.v"]
    need(bool(rtl_files), "generated top-level RTL is missing")
    carrier = profile_dir / "bambu_axi_burst_duplex_fallback.v"
    safe_file(carrier, "generated top RTL", root=profile_dir)
    need(carrier.resolve() in {path.resolve() for path in rtl_files},
         "exact generated top RTL is absent from the RTL inventory")
    text = "\n".join(path.read_text(encoding="utf-8", errors="replace") for path in rtl_files)
    need(bool(text.strip()), "generated top-level RTL is empty")
    carrier_text = carrier.read_text(encoding="utf-8", errors="replace")
    carrier_code = re.sub(r"/\*.*?\*/|//[^\n]*", "", carrier_text, flags=re.DOTALL)
    need(LEGACY_INSTANCE.search(carrier_code) is not None,
         "exact generated top RTL does not instantiate a legacy/cache memory resource")
    return text, rtl_files


def check_provenance(profile_dir: Path, profile: str, args: argparse.Namespace) -> dict[str, str]:
    fields = parse_key_values(profile_dir / "provenance.txt", "provenance.txt")
    expected = {
        "profile": profile,
        "macro": PROFILES[profile]["macro"],
        "feature": "1",
        "output": str(profile_dir.resolve()),
        "source": str(args.source.resolve()),
        "testbench": str(args.testbench.resolve()),
        "checker": str(Path(__file__).resolve()),
        "runner": str(args.runner.resolve()),
        "bambu": str(args.bambu.resolve()),
        "settings": str(args.settings.resolve()),
        "plugin": str(args.plugin.resolve()),
        "simulator": str(args.simulator.resolve()),
    }
    for key, value in expected.items():
        need(fields.get(key) == value, f"provenance {key} does not match the selected run")
    need(bool(re.fullmatch(r"[0-9a-fA-F]{7,40}", fields.get("git_head", ""))),
         "provenance git_head is missing or malformed")
    for key in PROVENANCE_HASH_FIELDS:
        path = Path(expected[key])
        safe_file(path, f"provenance input {key}")
        digest_key = f"{key}_sha256"
        need(fields.get(digest_key) == sha256(path), f"provenance digest mismatch for {key}")
    argv = fields.get("argv", "")
    need(argv and "--bambu-parameter=experimental-m-axi-burst=1" in argv,
         "provenance argv does not enable experimental-m-axi-burst=1")
    need(f"-DDUPLEX_FALLBACK_PROFILE={PROFILES[profile]['macro']}" in argv,
         "provenance argv does not select the expected profile macro")
    return fields


def check_profile(profile_dir: Path, profile: str, args: argparse.Namespace) -> dict[str, object]:
    need(profile_dir.name == profile, f"unexpected profile directory name: {profile_dir.name}")
    need(profile_dir.is_dir() and not profile_dir.is_symlink(), f"profile directory missing or symlink: {profile_dir}")
    profile_dir = profile_dir.resolve()
    required_paths = [safe_file(profile_dir / name, f"required {name}", root=profile_dir)
                      for name in ("bambu_results.xml", "stdout.log", "stderr.log",
                                   "HLS_output/simulation/testbench.log",
                                   "panda-temp/architecture.xml",
                                   "bambu_axi_burst_duplex_fallback.v")]
    manifest = check_artifact_manifest(profile_dir)
    manifest_paths = {str((profile_dir / name).resolve()) for name in manifest}
    need(all(str(path) in manifest_paths for path in required_paths),
         "one or more required evidence files are absent from artifact-sha256.txt")
    provenance = check_provenance(profile_dir, profile, args)
    testbench_log = profile_dir / "HLS_output/simulation/testbench.log"
    marker_lines = [line for line in testbench_log.read_text(encoding="utf-8", errors="replace").splitlines()
                    if MARKER.fullmatch(line)]
    expected_marker = f"DUPLEX_FALLBACK_GOLDEN_PASS PROFILE={profile} N=38"
    need(marker_lines == [expected_marker], f"exact golden marker missing or duplicated in {testbench_log}")

    combined_log = "\n".join((profile_dir / name).read_text(encoding="utf-8", errors="replace")
                               for name in ("stdout.log", "stderr.log"))
    expected = PROFILES[profile]
    fallback_lines = list(FALLBACK.finditer(combined_log))
    for direction, field in (("accesses", "read_reason"), ("writes", "write_reason")):
        reason = expected[field]
        matches = [match for match in fallback_lines
                   if match.group("direction") == direction and match.group("reason") == reason]
        need(len(matches) == 1,
             f"expected one exact InterfaceInfer {direction} fallback {reason!r}; got "
             f"{[(match.group('direction'), match.group('reason')) for match in fallback_lines]!r}")
    trace_matches = [match for match in PREANALYSIS.finditer(combined_log)]
    matching_traces = []
    for match in trace_matches:
        fields_match = (match.group("read_requested") == "1" and
                        match.group("write_requested") == "1" and
                        match.group("candidate") == "0" and
                        match.group("activated") == "0" and
                        match.group("read_reason") == expected["read_reason"] and
                        match.group("write_reason") == expected["write_reason"])
        if "read_accesses" in expected:
            fields_match = (fields_match and
                            match.group("read_accesses") == expected["read_accesses"] and
                            match.group("write_accesses") == expected["write_accesses"])
        if expected.get("require_unrecognized"):
            fields_match = (fields_match and match.group("read_recognized") == "0" and
                            match.group("write_recognized") == "0" and
                            match.group("duplex_reason") == expected["same_base_duplex_reason"])
        if fields_match:
            matching_traces.append(match)
    need(len(matching_traces) == 1,
         "AXI_BURST_PREANALYSIS trace does not witness both requested directions and exact fallback reasons")
    need(DUPLEX_ACTIVATION.search(combined_log) is None,
         "fallback compiler log contains a final duplex activation marker")

    rtl, rtl_files = top_rtl(profile_dir)
    need(LEGACY.search(rtl) is not None, "fallback RTL does not instantiate a legacy/cache memory resource")
    need(BURST.search(rtl) is None, "fallback RTL contains a duplex or burst engine")
    need(BURST_OP.search(rtl) is None, "fallback RTL contains residual burst ABI operations")
    try:
        architecture = safe_file(profile_dir / "panda-temp/architecture.xml", "architecture.xml", root=profile_dir)
        root = ET.parse(architecture).getroot()
    except (OSError, ET.ParseError) as exc:
        raise CheckError(f"architecture.xml missing or malformed: {exc}") from exc
    bundles = [node for node in root.findall(".//bundle") if node.get("name") == "shared"]
    need(len(bundles) == 1, "architecture.xml must retain exactly one shared bundle")
    attrs = bundles[0].attrib
    need(attrs.get("mode") == "m_axi" and attrs.get("max_read_burst_length") == "16" and
         attrs.get("num_read_outstanding") == "1" and attrs.get("read_fifo_depth") == "64" and
         attrs.get("max_write_burst_length") == "16" and attrs.get("write_burst_outstanding") == "1" and
         attrs.get("write_fifo_depth") == "32",
         "architecture.xml does not retain the requested shared read/write burst profiles")
    need(any(node.get("port") == "data" and node.get("bundle") == "shared"
             for node in root.findall(".//parameter")),
         "architecture.xml does not map the single data pointer to bundle shared")
    return {"profile": profile, "marker": expected_marker,
            "read_fallback_reason": expected["read_reason"],
            "write_fallback_reason": expected["write_reason"],
            "rtl_files": [str(path.relative_to(profile_dir)) for path in rtl_files],
            "artifact_manifest": manifest, "git_head": provenance["git_head"]}


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser()
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
        need(run_dir.is_dir() and not args.run_dir.is_symlink(), f"run directory missing or symlink: {args.run_dir}")
        need(not args.source.is_symlink() and args.source.name == SOURCE_NAME and args.source.is_file(),
             f"source must be the regular fixture {SOURCE_NAME}")
        need(not args.testbench.is_symlink() and args.testbench.name == TESTBENCH_NAME and args.testbench.is_file(),
             f"testbench must be the regular fixture {TESTBENCH_NAME}")
        children = list(run_dir.iterdir())
        job_logs = [path for path in children if path.name == "job_logs"]
        if job_logs:
            need(len(job_logs) == 1 and not job_logs[0].is_symlink() and job_logs[0].is_dir(),
                 "runner job_logs infrastructure must be a real non-symlink directory")
        profiles = sorted(path.name for path in children if path.name in PROFILES and
                          path.is_dir() and not path.is_symlink())
        # The gate keeps its scratch TMPDIR ("tmp") inside the run directory, so it is part of
        # the expected inventory.
        unexpected = sorted(path.name for path in children if path.name not in PROFILES and
                            path.name not in {"job_logs", "tmp"} and path.is_dir())
        need(not unexpected, f"unexpected run directories: {unexpected}")
        need(profiles == sorted(PROFILES), f"profile inventory mismatch: expected {sorted(PROFILES)}, got {profiles}")
        results = {profile: check_profile(run_dir / profile, profile, args) for profile in PROFILES}
        report = {"run_dir": str(run_dir), "profiles": results}
        report_path = args.report
        if not report_path.is_absolute():
            report_path = run_dir / report_path
        report_parent = report_path.parent.resolve(strict=True)
        try:
            report_parent.relative_to(run_dir)
        except ValueError as exc:
            raise CheckError("report path escapes run directory") from exc
        current = report_path
        while current != current.parent:
            need(not current.is_symlink(), f"report path contains symlink: {current}")
            if current.resolve() == run_dir:
                break
            current = current.parent
        need(not report_path.exists() or report_path.is_file(), "report path is not a regular file")
        report_path.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")
        print(f"DUPLEX FALLBACK CHECK PASS: {run_dir}")
        return 0
    except (OSError, ValueError, ET.ParseError) as exc:
        print(f"DUPLEX FALLBACK CHECK FAILED: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
