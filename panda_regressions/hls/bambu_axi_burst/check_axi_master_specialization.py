#!/usr/bin/env python3
"""Mantis post-check for the four AXI master feature combinations."""

import argparse
from pathlib import Path
import re
import sys


def check(output: Path, direction: str, profile: int) -> None:
    rtl = (output / "kernel.v").read_text()
    # Comments must not satisfy an instance/parameter check.
    rtl = re.sub(r"//[^\n]*|/\*.*?\*/", "", rtl, flags=re.DOTALL)
    instances = re.findall(
        r"\b(MinimalAXI4MasterPipelined(?:Write)?)\s*#\s*\((.*?)\)\s+\w+\s*\(",
        rtl, re.DOTALL)
    read = [parameters for name, parameters in instances if name == "MinimalAXI4MasterPipelined"]
    write = [parameters for name, parameters in instances if name == "MinimalAXI4MasterPipelinedWrite"]
    scalar = re.search(r"\bMinimalAXI4AdapterSingleBeat\s*#", rtl) is not None
    if profile == 0:
        if read or write or not scalar:
            raise ValueError("B1/O1 must instantiate the historical single-beat adapter")
        return

    if scalar or len(read) != int(direction != "write") or len(write) != int(direction != "read"):
        raise ValueError(f"wrong AXI master selection for {direction}: read={len(read)}, write={len(write)}, scalar={scalar}")
    burst = 16 if profile in (1, 3) else 1
    outstanding = 3 if profile in (2, 3) else 1
    for parameters in read + write:
        for name, value in (("B_MAX", burst), ("MAX_OUTSTANDING", outstanding)):
            actual = re.findall(r"\." + name + r"\s*\(\s*(\d+)\s*\)", parameters)
            if actual != [str(value)]:
                raise ValueError(f"expected {name}={value}, found {actual}")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=Path("."))
    parser.add_argument("--direction", choices=("read", "write", "duplex"), required=True)
    parser.add_argument("--profile", type=int, choices=range(4), required=True)
    args = parser.parse_args()
    try:
        check(args.output, args.direction, args.profile)
    except (OSError, ValueError) as error:
        print(f"AXI_MASTER_SPECIALIZATION_FAIL: {error}", file=sys.stderr)
        return 1
    print(f"AXI_MASTER_SPECIALIZATION_PASS direction={args.direction} profile={args.profile}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
