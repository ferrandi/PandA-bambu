#!/usr/bin/env python3
"""Fail-closed checker for the simulated FIXED-device burst fallback case."""

import argparse
from pathlib import Path
import sys

from check_burst_results import check_fixed_burst_fallback


def main(argv=None):
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=Path, default=Path("."),
                        help="Mantis case directory")
    parser.add_argument("--target-file", required=True, type=Path,
                        help="target XML used for this case")
    args = parser.parse_args(argv)
    try:
        check_fixed_burst_fallback(args.output, args.target_file)
    except (OSError, UnicodeError, ValueError) as exc:
        print(f"AXI_BURST_FIXED_FALLBACK_FAIL: {exc}", file=sys.stderr)
        return 1
    print(f"AXI_BURST_FIXED_FALLBACK_PASS: {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
