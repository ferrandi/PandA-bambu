#!/usr/bin/env python3
"""Require nested read traffic to use the explicit legacy adapter fallback."""

from pathlib import Path
import re
import sys


def main() -> int:
    rtl = Path("kernel.v")
    if rtl.is_symlink() or not rtl.is_file():
        raise SystemExit("generated top RTL is missing or unsafe")
    text = rtl.read_text(encoding="utf-8", errors="strict")
    if not re.search(r"\bMinimalAXI4AdapterSingleBeat\s*#\s*\(", text):
        raise SystemExit("nested read fallback lacks the single-beat adapter")
    if re.search(r"\bMinimalAXI4MasterPipelined(?:Write)?\s*#\s*\(", text):
        raise SystemExit("nested read unexpectedly instantiated a burst engine")
    print("NESTED_READ_FALLBACK_PASS adapter=single-beat burst-engine=absent")
    return 0


if __name__ == "__main__":
    sys.exit(main())
