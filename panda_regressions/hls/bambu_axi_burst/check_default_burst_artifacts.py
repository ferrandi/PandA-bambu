#!/usr/bin/env python3
"""Check compiler evidence for default-on AXI burst profiles."""

from __future__ import annotations

import argparse
from pathlib import Path
import re
import shlex
import sys
import xml.etree.ElementTree as ET


def need(condition: bool, message: str) -> None:
    if not condition:
        raise ValueError(message)


def normalize_hdl_identifiers(text: str) -> str:
    node_ids: dict[tuple[str, str], int] = {}
    identifier = re.compile(r"[A-Za-z_$][A-Za-z0-9_$]*")
    based_number = re.compile(r"(?:[0-9][0-9_]*)?'[sS]?[bBoOdDhH][0-9a-fA-F_xXzZ?]+")
    unbased_number = re.compile(r"'[01xXzZ]")
    decimal_number = re.compile(r"[0-9][0-9_]*(?:\.[0-9_]*)?(?:[eE][+-]?[0-9][0-9_]*)?")
    normalized: list[str] = []
    index = 0

    while index < len(text):
        if text.startswith("//", index):
            end = text.find("\n", index)
            if end < 0:
                normalized.append(text[index:])
                break
            normalized.append(text[index:end + 1])
            index = end + 1
        elif text.startswith("/*", index):
            end = text.find("*/", index + 2)
            end = len(text) if end < 0 else end + 2
            normalized.append(text[index:end])
            index = end
        elif text[index] == '"':
            end = index + 1
            while end < len(text):
                if text[end] == "\\":
                    end += 2
                elif text[end] == '"':
                    end += 1
                    break
                else:
                    end += 1
            normalized.append(text[index:end])
            index = end
        elif text[index] == "\\":
            end = index + 1
            while end < len(text) and not text[end].isspace():
                end += 1
            normalized.append(text[index:end])
            index = end
        else:
            number = based_number.match(text, index) or unbased_number.match(text, index)
            if number is None and text[index].isdigit():
                number = decimal_number.match(text, index)
            if number is not None:
                normalized.append(number.group(0))
                index = number.end()
                continue

            token = identifier.match(text, index)
            if token is None:
                normalized.append(text[index])
                index += 1
                continue

            name = token.group(0)
            suffix = re.search(r"_(\d{6,})_(\d{6,})$", name)
            if suffix is not None:
                pair = (suffix.group(1), suffix.group(2))
                node_id = node_ids.setdefault(pair, len(node_ids))
                name = name[:suffix.start()] + f"_NODEID_{node_id}"
            normalized.append(name)
            index = token.end()

    return "".join(normalized)


def normalized_rtl(text: str) -> str:
    lines = text.splitlines()
    for index, line in enumerate(lines[:8]):
        if line.startswith("// Code created using PandA - Version: ") and " - Date " in line:
            prefix, _, _ = line.rpartition(" - Date ")
            lines[index] = prefix + " - Date <timestamp>"
        elif line.startswith("// Bambu executed with: "):
            argv = shlex.split(line.removeprefix("// Bambu executed with: "))
            burst_options = [arg for arg in argv if arg.startswith("--bambu-parameter=m-axi-burst=")]
            need(len(burst_options) <= 1, "generated RTL invocation repeats m-axi-burst")
            argv = [arg for arg in argv if not arg.startswith("--bambu-parameter=m-axi-burst=")]
            lines[index] = "// Bambu executed with: " + " ".join(shlex.quote(arg) for arg in argv)
    return normalize_hdl_identifiers("\n".join(lines) + ("\n" if text.endswith("\n") else ""))


def rtl_snapshot(root: Path) -> dict[str, str]:
    need(root.is_dir() and not root.is_symlink(), f"RTL output directory is missing or a symlink: {root}")
    files = sorted(root.rglob("*.v"))
    need(bool(files), f"generated RTL missing below {root}")
    need(all(not path.is_symlink() for path in files), f"generated RTL contains a symlink below {root}")
    return {path.relative_to(root).as_posix(): normalized_rtl(path.read_text(encoding="utf-8"))
            for path in files}


def compare_rtl(first: Path, second: Path, expect_equal: bool) -> None:
    first_files = rtl_snapshot(first)
    second_files = rtl_snapshot(second)
    changed = sorted(name for name in first_files.keys() | second_files.keys()
                     if first_files.get(name) != second_files.get(name))
    if expect_equal:
        need(not changed, f"RTL differs after provenance-only normalization: {changed[:5]}")
    else:
        need(bool(changed), "positive control unexpectedly produced identical RTL")
    relation = "identical" if expect_equal else "different"
    print(f"DEFAULT BURST RTL {relation.upper()} PASS: {first} vs {second}")


def artifacts(root: Path) -> tuple[str, str, str]:
    logs = "\n".join(path.read_text(errors="replace") for path in root.rglob("*.log"))
    rtl_files = [path for path in root.rglob("*.v") if path.name != "panda_libtech.v"]
    rtl = "\n".join(path.read_text(errors="replace") for path in rtl_files)
    xmls = list(root.rglob("architecture.xml"))
    need(bool(logs), f"compiler logs missing below {root}")
    need(bool(rtl_files), f"generated RTL missing below {root}")
    need(len(xmls) == 1, f"expected one architecture.xml below {root}, found {len(xmls)}")
    return logs, rtl, str(xmls[0])


def check_profile(root: Path, kind: str, bundle: str, read_fifo: int = 256,
                  write_fifo: int = 32, read_burst: int = 16, read_outstanding: int = 16,
                  write_burst: int = 16, write_outstanding: int = 16,
                  read_burst_origin: str = "default", write_burst_origin: str = "default") -> None:
    logs, rtl, xml_path = artifacts(root)
    need("AXI_BURST_PROFILE " in logs, "minimum-verbosity profile record is absent")
    line = next((line for line in logs.splitlines()
                 if re.match(r"^\s*(?:---)?AXI_BURST_PROFILE ", line) and
                 f"bundle={bundle} " in line), None)
    need(line is not None, f"minimum-verbosity profile for bundle {bundle} is absent")
    for token in ("enabled=1", f"read_max={read_burst}", f"read_outstanding={read_outstanding}",
                  f"read_fifo={read_fifo}", f"write_max={write_burst}",
                  f"write_outstanding={write_outstanding}", f"write_fifo={write_fifo}"):
        need(token in line, f"profile record lacks {token}: {line}")
    need("burst_type=INCREMENTAL(device)" in line,
         f"profile does not identify the device-derived INCREMENTAL type: {line}")
    for field, value, origin in (("read_max", read_burst, read_burst_origin),
                                 ("write_max", write_burst, write_burst_origin)):
        expected_origin = origin if origin.startswith("single-beat-default:") else (
            "default" if origin == "default" else "explicit")
        need(f"{field}={value}({expected_origin})" in line,
             f"profile does not report {expected_origin} for {field}: {line}")
    for field, value in (("read_outstanding", read_outstanding), ("write_outstanding", write_outstanding)):
        origin = "default" if value == 16 else "explicit"
        need(f"{field}={value}({origin})" in line,
             f"profile does not report the {origin} origin for {field}: {line}")
    architecture = ET.parse(xml_path).getroot()
    bundles = [node for node in architecture.findall(".//bundle") if node.get("name") == bundle]
    need(len(bundles) == 1, f"architecture.xml must contain exactly bundle {bundle}")
    attrs = bundles[0].attrib
    need(attrs.get("mode") == "m_axi", f"architecture bundle {bundle} is not mode=m_axi")
    # architecture.xml is emitted by frontend analysis before InterfaceInfer applies
    # synthesized defaults. Require explicit source overrides there; for derived
    # values, accept omission or the exact effective value if a later stage serializes it.
    for key, value, origin in (("max_read_burst_length", read_burst, read_burst_origin),
                               ("max_write_burst_length", write_burst, write_burst_origin)):
        actual = attrs.get(key)
        if origin.startswith("single-beat-default:"):
            need(actual is None, f"architecture bundle {bundle} serialized inferred {key}={actual!r}")
        elif origin == "explicit":
            need(actual == str(value),
                 f"architecture bundle {bundle} has {key}={actual!r}, expected explicit value {value}")
        else:
            need(actual in (None, "16"),
                 f"architecture bundle {bundle} has unexpected {key}={actual!r}")
    for key, value in (("num_read_outstanding", read_outstanding),
                       ("write_burst_outstanding", write_outstanding)):
        actual = attrs.get(key)
        if value != 16:
            need(actual == str(value),
                 f"architecture bundle {bundle} has {key}={actual!r}, expected explicit value {value}")
        else:
            need(actual in (None, "16"),
                 f"architecture bundle {bundle} has unexpected {key}={actual!r}")
    for key, value in (("read_fifo_depth", read_fifo), ("write_fifo_depth", write_fifo)):
        need(attrs.get(key) in (None, str(value)),
             f"architecture bundle {bundle} has unexpected derived {key}={attrs.get(key)!r}")

    if kind in {"read", "duplex"}:
        read_instance = re.compile(
            rf"MinimalAXI4MasterPipelined\s*#\s*\(\s*\.B_MAX\({read_burst}\),\s*"
            rf"\.MAX_OUTSTANDING\({read_outstanding}\),\s*\.FIFO_DEPTH\({read_fifo}\)")
        need(read_instance.search(rtl) is not None,
             f"generated RTL lacks reader instance B={read_burst}/O={read_outstanding}/D={read_fifo}")
    if kind in {"write", "duplex"}:
        write_instance = re.compile(
            rf"MinimalAXI4MasterPipelinedWrite\s*#\s*\(\s*\.B_MAX\({write_burst}\),\s*"
            rf"\.MAX_OUTSTANDING\({write_outstanding}\),\s*\.FIFO_DEPTH\({write_fifo}\)")
        need(write_instance.search(rtl) is not None,
             f"generated RTL lacks writer instance B={write_burst}/O={write_outstanding}/D={write_fifo}")

    if kind == "read":
        decision = next((line for line in logs.splitlines()
                         if re.match(r"^\s*(?:---)?AXI_BURST_DECISION ", line) and
                         f"bundle={bundle} " in line), "")
        need("read=recognized(" in decision and "write=not-applicable-no-write-access" in decision,
             f"read optimization decision is not logged as recognized: {decision}")
        need("Configured read burst regions for bundle " + bundle + " (B=" + str(read_burst) + ")" in logs,
             "reader activation diagnostic absent")
        need("Configured write burst region for bundle " + bundle not in logs,
             "read-only function unexpectedly activated a writer")
    elif kind == "write":
        decision = next((line for line in logs.splitlines()
                         if re.match(r"^\s*(?:---)?AXI_BURST_DECISION ", line) and
                         f"bundle={bundle} " in line), "")
        need("write=recognized(" in decision and "read=not-applicable-no-read-access" in decision,
             f"write optimization decision is not logged as recognized: {decision}")
        need("Configured write burst region for bundle " + bundle + " (B=" + str(write_burst) + ")" in logs,
             "writer activation diagnostic absent")
        need("Configured read burst regions for bundle " + bundle not in logs,
             "write-only function unexpectedly activated a reader")
    elif kind == "duplex":
        decision = next((line for line in logs.splitlines()
                         if re.match(r"^\s*(?:---)?AXI_BURST_DECISION ", line) and
                         f"bundle={bundle} " in line), "")
        need("read=recognized(" in decision and "write=recognized(" in decision,
             f"duplex optimization decisions are not both logged as recognized: {decision}")
        need("Configured read burst regions for bundle " + bundle + " (B=" + str(read_burst) + ")" in logs,
             "duplex reader activation diagnostic absent")
        need("Configured write burst region for bundle " + bundle + " (B=" + str(write_burst) + ")" in logs,
             "duplex writer activation diagnostic absent")


def check_no_engine(root: Path, kind: str, reason: str | None, direction: str,
                    bundle: str) -> None:
    logs, rtl, xml_path = artifacts(root)
    need("MinimalAXI4MasterPipelined" not in rtl,
         "burst engine is present in a non-activated case")
    need("Configured read burst regions" not in logs and "Configured write burst region" not in logs,
         "burst activation was reported for a non-activated case")
    if kind == "optout":
        need("AXI_BURST_ANALYSIS " in logs and "enabled=0 reason=m-axi-burst=0" in logs,
             "explicit opt-out analysis record is absent")
    elif kind == "rtl-fallback":
        decision = next((line for line in logs.splitlines()
                         if re.match(r"^\s*(?:---)?AXI_BURST_DECISION ", line) and
                         f"bundle={bundle} " in line), "")
        need(bool(decision) and "legacy-fallback(" in decision,
             "fallback decision with its reason is absent")
        need("Keeping legacy m_axi" in logs,
             "fallback diagnostic with a reason is absent")
        architecture = ET.parse(xml_path).getroot()
        bundles = [node for node in architecture.findall(".//bundle") if node.get("name") == bundle]
        need(len(bundles) == 1 and bundles[0].get("mode") == "m_axi",
             f"expected one legacy m_axi architecture bundle named {bundle}")
    elif kind == "no-access":
        need("not-applicable-no-read-access" in logs,
             "no-access decision record is absent")
        need("Keeping legacy m_axi" not in logs,
             "unused pointer was incorrectly diagnosed as a failed burst optimization")
    elif kind == "fallback":
        decision = next((line for line in logs.splitlines()
                         if re.match(r"^\s*(?:---)?AXI_BURST_DECISION ", line)), "")
        need(bool(decision), "fallback disposition is absent from the profile decision")
        if direction == "read":
            need("read=legacy-fallback" in decision and "write=not-applicable-no-write-access" in decision,
                 f"read-only fallback decision has a wrong direction classification: {decision}")
            need("Keeping legacy m_axi writes for bundle " not in logs and
                 "Configured write burst region" not in logs,
                 "read-only fallback reported an unavailable write optimization")
            diagnostic = re.compile(rf"Keeping legacy m_axi (?:reads|accesses) for bundle {re.escape(bundle)}: ")
        else:
            need("write=legacy-fallback" in decision and "read=not-applicable-no-read-access" in decision,
                 f"write-only fallback decision has a wrong direction classification: {decision}")
            need("Keeping legacy m_axi reads for bundle " not in logs and
                 "Configured read burst regions" not in logs,
                 "write-only fallback reported an unavailable read optimization")
            diagnostic = re.compile(rf"Keeping legacy m_axi writes for bundle {re.escape(bundle)}: ")
        fallback_lines = [line for line in logs.splitlines() if diagnostic.search(line)]
        need(len(fallback_lines) == 1, "expected exactly one fallback diagnostic for the accessed direction")
        need(reason is not None and fallback_lines[0].endswith(": " + reason),
             f"fallback diagnostic reason mismatch: expected {reason!r}, got {fallback_lines}")
    elif kind == "fixed":
        need("burst_type=FIXED(device)" in logs, "device-derived FIXED type is absent from profile")
        need("AXI burst type is FIXED" in logs, "FIXED fallback explanation is absent")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("kind", choices=("read", "write", "duplex", "fallback", "optout",
                                          "fixed", "no-access", "rtl-fallback", "rtl-equal",
                                          "rtl-different"))
    parser.add_argument("output", type=Path)
    parser.add_argument("--compare", type=Path)
    parser.add_argument("--bundle", default="data")
    parser.add_argument("--reason")
    parser.add_argument("--direction", choices=("read", "write"), default="read")
    parser.add_argument("--read-burst", type=int, default=16)
    parser.add_argument("--read-outstanding", type=int, default=16)
    parser.add_argument("--write-burst", type=int, default=16)
    parser.add_argument("--write-outstanding", type=int, default=16)
    parser.add_argument("--read-fifo", type=int, default=256)
    parser.add_argument("--write-fifo", type=int, default=32)
    parser.add_argument("--read-burst-origin", default="default")
    parser.add_argument("--write-burst-origin", default="default")
    args = parser.parse_args()
    try:
        if args.kind in {"rtl-equal", "rtl-different"}:
            need(args.compare is not None, f"{args.kind} requires --compare")
            compare_rtl(args.output, args.compare, args.kind == "rtl-equal")
            return 0
        if args.kind in {"read", "write", "duplex"}:
            check_profile(args.output, args.kind, args.bundle, args.read_fifo, args.write_fifo,
                          args.read_burst, args.read_outstanding, args.write_burst,
                          args.write_outstanding, args.read_burst_origin, args.write_burst_origin)
        else:
            check_no_engine(args.output, args.kind, args.reason, args.direction, args.bundle)
    except (OSError, ET.ParseError, ValueError) as exc:
        print(f"DEFAULT BURST CHECK FAILED: {exc}", file=sys.stderr)
        return 1
    print(f"DEFAULT BURST {args.kind.upper()} PASS: {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
