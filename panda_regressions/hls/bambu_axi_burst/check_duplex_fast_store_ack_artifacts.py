#!/usr/bin/env python3
"""Fail-closed checks for the always-on fast STORE acknowledgement.

The compiler no longer exposes a switch for the fast STORE acknowledgement: every generated m_axi
burst write engine (duplex wrapper and standalone write-only interface) is emitted with
FAST_STORE_ACK(1), and the libtech module declares the parameter with default 1. The checker proves
that, and proves that the historical option name is inert.
"""

import argparse
import pathlib
import re
import sys


def balanced_end(text, opening):
    depth = 0
    in_string = False
    escaped = False
    for index in range(opening, len(text)):
        char = text[index]
        if in_string:
            if escaped:
                escaped = False
            elif char == "\\":
                escaped = True
            elif char == '"':
                in_string = False
        elif char == '"':
            in_string = True
        elif char == "(":
            depth += 1
        elif char == ")":
            depth -= 1
            if depth == 0:
                return index
    return None


def split_top_level(text):
    parts = []
    start = 0
    depth = 0
    in_string = False
    escaped = False
    for index, char in enumerate(text):
        if in_string:
            if escaped:
                escaped = False
            elif char == "\\":
                escaped = True
            elif char == '"':
                in_string = False
        elif char == '"':
            in_string = True
        elif char in "([{":
            depth += 1
        elif char in ")]}":
            depth -= 1
        elif char == "," and depth == 0:
            parts.append(text[start:index].strip())
            start = index + 1
    tail = text[start:].strip()
    if tail:
        parts.append(tail)
    return parts


def parameterized_instances(text, module_name):
    instances = []
    marker = re.compile(rf"(?<![\w$]){re.escape(module_name)}\s*#\s*\(")
    for match in marker.finditer(text):
        parameter_open = text.find("(", match.start(), match.end())
        parameter_close = balanced_end(text, parameter_open)
        if parameter_close is None:
            continue
        cursor = parameter_close + 1
        instance = re.match(r"\s*[A-Za-z_$][\w$]*\s*\(", text[cursor:])
        if not instance:
            continue
        params = {}
        for item in split_top_level(text[parameter_open + 1:parameter_close]):
            entry = re.fullmatch(r"\.(\w+)\s*\(\s*(.*?)\s*\)", item, re.S)
            if entry:
                params[entry.group(1)] = re.sub(r"\s+", "", entry.group(2))
        instances.append(params)
    return instances


def fail(message):
    raise ValueError(message)


def read_top(output):
    if not output.is_dir() or output.is_symlink():
        fail(f"output is not a regular directory: {output}")
    candidates = [path for path in output.glob("*.v") if path.is_file() and not path.is_symlink()
                  and path.name != "panda_libtech.v"]
    if len(candidates) != 1:
        fail(f"expected exactly one top-level generated Verilog file, found {len(candidates)}")
    return candidates[0].read_text(encoding="utf-8", errors="replace")


def generated_library(output):
    libraries = [path for path in output.rglob("panda_libtech.v")
                 if path.is_file() and not path.is_symlink()]
    if len(libraries) != 1:
        fail(f"expected one generated panda_libtech.v, found {len(libraries)}")
    return libraries[0].read_text(encoding="utf-8", errors="replace")


def check_profile(instances, module, expected, exact_count=1):
    if len(instances) != exact_count:
        fail(f"expected {exact_count} {module} instance(s), found {len(instances)}")
    params = instances[0]
    mismatches = {key: (str(value), params.get(key)) for key, value in expected.items()
                  if params.get(key) != str(value)}
    if mismatches:
        fail(f"{module} instance profile mismatch: {mismatches}")
    return params


# The generated libtech declares the parameter after the port list, so the declaration is part of
# the module body. The trailing separator keeps prose that mentions FAST_STORE_ACK=0 from matching.
PARAMETER_DECLARATION = re.compile(r"\bFAST_STORE_ACK\s*=\s*(\d+)\s*(?:,|;|\))")


def check_writer_default_one(output):
    text = generated_library(output)
    module = re.search(
        r"\bmodule\s+MinimalAXI4MasterPipelinedWrite\s*\([^;]*\);(?P<body>.*?)\bendmodule\b",
        text, re.S)
    if not module:
        fail("generated write-engine module MinimalAXI4MasterPipelinedWrite not found")
    declarations = PARAMETER_DECLARATION.findall(module.group("body"))
    if not declarations:
        fail("generated write-engine declares no FAST_STORE_ACK default")
    if any(value != "1" for value in declarations):
        fail(f"generated write-engine default FAST_STORE_ACK is not proven to be 1: {declarations}")


def writer_profile(args):
    return {"B_MAX": args.write_burst, "MAX_OUTSTANDING": args.write_outstanding,
            "FIFO_DEPTH": args.write_fifo, "FAST_STORE_ACK": 1}


def reader_profile(args):
    return {"B_MAX": args.read_burst, "MAX_OUTSTANDING": args.read_outstanding,
            "FIFO_DEPTH": args.read_fifo}


def check_success(args):
    text = read_top(args.output)
    writers = parameterized_instances(text, "MinimalAXI4MasterPipelinedWrite")
    readers = parameterized_instances(text, "MinimalAXI4MasterPipelined")

    if args.kind == "duplex":
        check_profile(writers, "MinimalAXI4MasterPipelinedWrite", writer_profile(args))
        check_profile(readers, "MinimalAXI4MasterPipelined", reader_profile(args))
    elif args.kind == "write-only":
        check_profile(writers, "MinimalAXI4MasterPipelinedWrite", writer_profile(args))
        check_writer_default_one(args.output)
        if readers:
            fail("write-only kernel unexpectedly instantiates a read burst engine")
    else:
        if writers:
            fail("read-only kernel unexpectedly instantiates a write burst engine")
        check_profile(readers, "MinimalAXI4MasterPipelined", reader_profile(args))
    return writers, readers


# Burst geometry and the fast acknowledgement must agree between the duplex wrapper and the
# standalone write-only flow. PEER_FAULT_ENABLE and the port widths legitimately differ there.
BURST_PROFILE_KEYS = ("B_MAX", "MAX_OUTSTANDING", "FIFO_DEPTH", "FAST_STORE_ACK")


def compare_writers(left_output, right_output):
    left = parameterized_instances(read_top(left_output), "MinimalAXI4MasterPipelinedWrite")
    right = parameterized_instances(read_top(right_output), "MinimalAXI4MasterPipelinedWrite")
    if not left or len(left) != len(right):
        fail(f"expected matching write engines: left={len(left)}, right={len(right)}")
    for left_params, right_params in zip(left, right):
        left_profile = {key: left_params.get(key) for key in BURST_PROFILE_KEYS}
        right_profile = {key: right_params.get(key) for key in BURST_PROFILE_KEYS}
        if left_profile != right_profile:
            fail(f"write engine burst profile differs between the compared kernels: "
                 f"{left_profile} != {right_profile}")
        if left_params.get("FAST_STORE_ACK") != "1":
            fail("FAST_STORE_ACK is not 1 in the compared write engines")
    return left, right


def check_inert_option(args):
    """The historical option name must not change anything: the run succeeds and is fast-ack."""
    logs = [args.output / "stdout.log", args.output / "stderr.log"]
    for path in logs:
        if not path.is_file() or path.is_symlink():
            fail(f"required compiler log missing: {path}")
        content = path.read_text(encoding="utf-8", errors="replace")
        if "error ->" in content:
            fail(f"removed option produced a compiler error: {path}")
    writers = parameterized_instances(read_top(args.output), "MinimalAXI4MasterPipelinedWrite")
    if not writers:
        fail("no write burst engine found in the kernel built with the removed option")
    for params in writers:
        if params.get("FAST_STORE_ACK") != "1":
            fail(f"kernel built with the removed option is not fast-ack: {params}")
    check_writer_default_one(args.output)
    return writers


def main():
    parser = argparse.ArgumentParser()
    subparsers = parser.add_subparsers(dest="command", required=True)
    success = subparsers.add_parser("success")
    success.add_argument("--output", type=pathlib.Path, required=True)
    success.add_argument("--kind", choices=("duplex", "write-only", "read-only"), required=True)
    success.add_argument("--write-burst", type=int, default=16)
    success.add_argument("--write-outstanding", type=int, default=1)
    success.add_argument("--write-fifo", type=int, default=32)
    success.add_argument("--read-burst", type=int, default=16)
    success.add_argument("--read-outstanding", type=int, default=1)
    success.add_argument("--read-fifo", type=int, default=64)
    inert = subparsers.add_parser("inert-option")
    inert.add_argument("--output", type=pathlib.Path, required=True)
    compare = subparsers.add_parser("compare-readers")
    compare.add_argument("--left", type=pathlib.Path, required=True)
    compare.add_argument("--right", type=pathlib.Path, required=True)
    compare_writers_cmd = subparsers.add_parser("compare-writers")
    compare_writers_cmd.add_argument("--left", type=pathlib.Path, required=True)
    compare_writers_cmd.add_argument("--right", type=pathlib.Path, required=True)
    args = parser.parse_args()
    try:
        if args.command == "success":
            check_success(args)
        elif args.command == "inert-option":
            check_inert_option(args)
        elif args.command == "compare-writers":
            compare_writers(args.left, args.right)
        else:
            left = parameterized_instances(read_top(args.left), "MinimalAXI4MasterPipelined")
            right = parameterized_instances(read_top(args.right), "MinimalAXI4MasterPipelined")
            if not left or left != right:
                fail(f"reader instance parameters changed: left={left}, right={right}")
        print(f"DUPLEX_FAST_STORE_ACK CHECK PASS {args.command}")
        return 0
    except (OSError, ValueError) as error:
        print(f"DUPLEX_FAST_STORE_ACK CHECK FAILED: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
