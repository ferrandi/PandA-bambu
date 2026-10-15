#!/usr/bin/env python3
"""Fail-closed checks for manual AXI write-burst compiler artifacts."""
import argparse
import pathlib
import re
import sys
import xml.etree.ElementTree as ET

from check_burst_results import _parameterized_instances

ARTIFACT_SUFFIXES = {".v", ".sv", ".log", ".txt", ".ll", ".bambuir", ".raw", ".ir"}
WRITE_SUCCESS = re.compile(r"(?m)^\s*(?:---)?Configured write burst region for bundle ([A-Za-z0-9_.-]+) \(B=(\d+)\)\s*$")
READ_SUCCESS = re.compile(r"(?m)^\s*(?:---)?Configured read burst regions for bundle ([A-Za-z0-9_.-]+) \(B=\d+\)\s*$")
FALLBACK_DIAGNOSTIC = re.compile(
    r"^\s*(?:---)?Keeping legacy m_axi (?P<direction>writes|accesses) for bundle "
    r"(?P<bundle>[A-Za-z0-9_.-]+): (?P<reason>.+?)\s*$"
)
ERROR_DIAGNOSTICS = re.compile(r"^\s*(?:error\s*->\s*(?P<tool>.+)|.*:\s*error:\s*(?P<clang>.+))$")
WRITE_COMPONENT = re.compile(r"(?i)MinimalAXI4MasterPipelinedWrite\b")
READ_COMPONENT = re.compile(r"(?i)MinimalAXI4MasterPipelined(?!Write)\b")
WRITE_OPERATIONS = {
    "CONFIGURE": re.compile(r"fuselector_[A-Za-z0-9_]*configure_write\b"),
    "STORE": re.compile(r"fuselector_[A-Za-z0-9_]*store_write\b"),
    "DRAIN": re.compile(r"fuselector_[A-Za-z0-9_]*drain_write\b"),
}
READ_OPERATIONS = {
    "CONFIGURE": re.compile(r"fuselector_[A-Za-z0-9_]*configure_read\b"),
    "READ": re.compile(r"fuselector_[A-Za-z0-9_]*burst_read\b"),
}
BURST_COMPONENT = re.compile(r"(?i)(MinimalAXI4MasterPipelined(?:Write)?|MinimalAXI4(?:Read)?Burst)")
LEGACY_COMPONENT = re.compile(r"(?i)(MinimalAXI4AdapterSingleBeat|IOB_cache_axi)")


def read_tree(root):
    return [p for p in root.rglob("*") if p.is_file() and p.suffix.lower() in ARTIFACT_SUFFIXES]


def read_top_rtl(files):
    top_files = [p for p in files if p.suffix.lower() in {".v", ".sv"}
                 and not p.name.lower().startswith("panda_libtech")]
    return "\n".join(p.read_text(encoding="utf-8", errors="replace") for p in top_files)


def missing_write_rtl_evidence(rtl):
    missing = []
    if not WRITE_COMPONENT.search(rtl):
        missing.append("write-burst component instance absent from top-level RTL")
    for operation, pattern in WRITE_OPERATIONS.items():
        if not pattern.search(rtl):
            missing.append(f"write {operation} operation signal absent from top-level RTL")
    return missing


def check_bundle_wiring(rtl, bundle, direction):
    address_port = f"p_m_axi_{bundle}_{'araddr' if direction == 'read' else 'awaddr'}"
    address_wire = "burst_araddr" if direction == "read" else "burst_awaddr"
    assignment = re.compile(rf"assign\s+{re.escape(address_port)}\s*=\s*{address_wire}\b")
    return bool(assignment.search(rtl))


def diagnostic_messages(log):
    messages = []
    for line in log.splitlines():
        match = ERROR_DIAGNOSTICS.match(line)
        if match:
            messages.append(match.group("tool") or match.group("clang"))
    return messages


def check_case(args):
    root = args.output
    if not root.is_dir():
        raise ValueError(f"compiler output directory does not exist: {root}")
    files = read_tree(root)
    if not files:
        raise ValueError(f"no compiler artifacts found below {root}")
    log = "\n".join((root / name).read_text(encoding="utf-8", errors="replace")
                    for name in ("stdout.log", "stderr.log") if (root / name).is_file())
    top_rtl = read_top_rtl(files)
    if args.kind == "accepted":
        ir = "\n".join(p.read_text(encoding="utf-8", errors="replace") for p in files
                       if p.suffix.lower() in {".bambuir", ".raw", ".ir", ".ll"})
        missing = []
        if not ir:
            missing.append("no retained compiler IR; --no-clean may be missing")
        if not WRITE_SUCCESS.search(log):
            missing.append("write-region success diagnostic absent from compiler output")
        expected_burst = getattr(args, "expected_write_burst", None)
        if expected_burst is not None:
            write_matches = list(WRITE_SUCCESS.finditer(log))
            if len(write_matches) != 1 or int(write_matches[0].group(2)) != expected_burst:
                missing.append(f"expected exactly one write-burst diagnostic with B={expected_burst}")
        missing.extend(missing_write_rtl_evidence(top_rtl))
        if getattr(args, "single_write_bundle", None):
            write_matches = list(WRITE_SUCCESS.finditer(log))
            if len(write_matches) != 1 or write_matches[0].group(1) != args.single_write_bundle:
                missing.append(f"expected exactly one write-burst region for bundle {args.single_write_bundle}")
            if len(WRITE_COMPONENT.findall(top_rtl)) != 1:
                missing.append("expected exactly one generated write-burst component in top-level RTL")
            if LEGACY_COMPONENT.search(top_rtl):
                missing.append("inactive parameter caused a legacy memory resource in top-level RTL")
            if not check_bundle_wiring(top_rtl, args.single_write_bundle, "write"):
                missing.append("write-burst component address is not wired to its declared bundle")
        if getattr(args, "copy", False):
            read_matches = list(READ_SUCCESS.finditer(log))
            write_matches = list(WRITE_SUCCESS.finditer(log))
            if not read_matches:
                missing.append("copy read-region success diagnostic absent from compiler output")
            if not READ_COMPONENT.search(top_rtl):
                missing.append("copy read-burst component instance absent from top-level RTL")
            for operation, pattern in READ_OPERATIONS.items():
                if not pattern.search(top_rtl):
                    missing.append(f"copy read {operation} operation signal absent from top-level RTL")
            if read_matches and not check_bundle_wiring(top_rtl, read_matches[-1].group(1), "read"):
                missing.append("copy read component address is not wired to its diagnosed bundle")
            if write_matches and not check_bundle_wiring(top_rtl, write_matches[-1].group(1), "write"):
                missing.append("copy write component address is not wired to its diagnosed bundle")
        if args.count_pattern and not re.search(args.count_pattern, ir, re.M):
            missing.append("runtime count pattern absent from retained compiler IR")
        if missing:
            raise ValueError("; ".join(missing))
    elif args.kind == "fallback":
        fallback_matches = [FALLBACK_DIAGNOSTIC.match(line) for line in log.splitlines()]
        fallback_matches = [match for match in fallback_matches if match]
        matches_expected = [match for match in fallback_matches
                            if match.group("bundle") == args.bundle and
                            match.group("direction") == getattr(args, "direction", "writes")]
        if not matches_expected:
            raise ValueError(f"legacy fallback diagnostic absent for bundle {args.bundle} "
                             f"and direction {args.direction}")
        expected_reason = ("explicit write-burst request ignored because experimental-m-axi-burst=0"
                           if getattr(args, "feature_off", False) else args.reason)
        if not any(match.group("reason") == expected_reason for match in matches_expected):
            actual = sorted({match.group("reason") for match in matches_expected})
            raise ValueError(f"legacy fallback reason mismatch for bundle {args.bundle}: "
                             f"expected {expected_reason!r}, got {actual!r}")
        if any(pattern.search(top_rtl) for pattern in (*WRITE_OPERATIONS.values(), *READ_OPERATIONS.values())):
            raise ValueError("fallback top-level RTL contains residual read/write burst operation signals")
        if BURST_COMPONENT.search(top_rtl):
            raise ValueError("fallback top-level RTL selects a read or write burst component")
        if not LEGACY_COMPONENT.search(top_rtl):
            raise ValueError("fallback top-level RTL does not instantiate a legacy/cache memory resource")
    elif args.kind == "legacy":
        if not getattr(args, "bundle", None):
            raise ValueError("legacy checks require --bundle")
        if not getattr(args, "require_legacy", False):
            raise ValueError("legacy checks require --require-legacy")
        if any(pattern.search(top_rtl) for pattern in (*WRITE_OPERATIONS.values(), *READ_OPERATIONS.values())) or \
           BURST_COMPONENT.search(top_rtl):
            raise ValueError("legacy top-level RTL contains residual read/write burst signals or resources")
        feature_off_reason = "explicit write-burst request ignored because experimental-m-axi-burst=0"
        for line in log.splitlines():
            match = FALLBACK_DIAGNOSTIC.match(line)
            if match and match.group("bundle") == args.bundle and match.group("reason") == feature_off_reason:
                raise ValueError("feature-off fallback diagnostic emitted without an explicit burst request")
        if not LEGACY_COMPONENT.search(top_rtl):
            raise ValueError("legacy top-level RTL does not instantiate a legacy/cache memory resource")
    elif args.kind == "invalid":
        expected = re.compile(args.reason, re.I)
        if not any(expected.search(message) for message in diagnostic_messages(log)):
            raise ValueError(f"invalid-profile diagnostic did not match /{args.reason}/")
        emitted_rtl = [path for path in files if path.suffix.lower() in {".v", ".sv"}]
        if emitted_rtl:
            raise ValueError(f"invalid profile emitted RTL: {[str(path.relative_to(root)) for path in emitted_rtl]}")
    if args.kind == "profile":
        architecture_files = list(root.rglob("architecture.xml"))
        if args.expect_profile_attribute and not architecture_files:
            raise ValueError("no architecture.xml found to verify canonical write profile")
        bundles = []
        for path in architecture_files:
            parsed = ET.parse(path).getroot()
            bundles.extend(node.attrib for node in parsed.findall(".//bundle"))
        for item in args.expect_profile_attribute:
            name, value = item.split("=", 1)
            if not any(attrs.get(name) == value for attrs in bundles):
                raise ValueError(f"architecture.xml does not serialize {name}={value}")
        expected_parameters = dict(item.split("=", 1)
                                   for item in getattr(args, "expect_engine_parameter", []))
        if expected_parameters:
            instances = _parameterized_instances(top_rtl, "MinimalAXI4MasterPipelinedWrite")
            if not any(all(parameters.get(name) == value for name, value in expected_parameters.items())
                       for parameters in instances):
                profile = ", ".join(f"{name}={value}" for name, value in expected_parameters.items())
                raise ValueError(f"top-level RTL does not instantiate one write engine with {profile}")
    print(f"WRITE-COMPILER {args.kind.upper()} PASS: {root}")
    return 0


def main(argv=None):
    parser = argparse.ArgumentParser()
    parser.add_argument("kind", choices=("accepted", "fallback", "invalid", "profile", "legacy"))
    parser.add_argument("--output", required=True, type=pathlib.Path)
    parser.add_argument("--reason", help="exact fallback reason or diagnostic regex for invalid cases")
    parser.add_argument("--bundle", help="bundle name required in the complete fallback diagnostic")
    parser.add_argument("--direction", choices=("writes", "accesses"), default="writes")
    parser.add_argument("--feature-off", action="store_true",
                        help="require the exact diagnostic for an explicit request with the feature disabled")
    parser.add_argument("--require-legacy", action="store_true",
                        help="for legacy checks, require a recognizable legacy/cache memory resource")
    parser.add_argument("--count-pattern", help="required runtime-count regex in accepted-case IR")
    parser.add_argument("--copy", action="store_true",
                        help="require concrete read and write paths and coherent burst modes")
    parser.add_argument("--single-write-bundle",
                        help="require exactly one write burst engine and no legacy resource on this bundle")
    parser.add_argument("--expected-write-burst", type=int,
                        help="require exactly one configured write-burst diagnostic with this B value")
    parser.add_argument("--expect-profile-attribute", action="append", default=[],
                        help="required exact key=value in an architecture.xml bundle")
    parser.add_argument("--expect-engine-parameter", action="append", default=[],
                        help="required exact key=value in a write-engine RTL instance")
    args = parser.parse_args(argv)
    if args.kind in {"fallback", "invalid"} and not args.reason:
        parser.error("fallback and invalid checks require --reason")
    if args.kind == "fallback" and not args.bundle:
        parser.error("fallback checks require --bundle")
    if args.kind == "fallback" and args.feature_off and args.direction != "writes":
        parser.error("feature-off fallback applies only to write requests")
    if args.kind == "legacy" and not args.bundle:
        parser.error("legacy checks require --bundle")
    if args.kind == "legacy" and not args.require_legacy:
        parser.error("legacy checks require --require-legacy")
    if args.kind == "profile" and not (args.expect_profile_attribute or args.expect_engine_parameter):
        parser.error("profile checks require at least one serialized profile or engine parameter")
    try:
        return check_case(args)
    except (OSError, ValueError) as exc:
        print(f"WRITE-COMPILER CHECK FAILED: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
