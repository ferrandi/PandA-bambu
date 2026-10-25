#!/usr/bin/env python3
"""Extract only TestbenchMEMAXI for its local behavioral fixture."""

from __future__ import annotations

import argparse
import importlib.util
from pathlib import Path


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--xml", type=Path, required=True)
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--t4b-extractor", type=Path, required=True)
    args = parser.parse_args()

    spec = importlib.util.spec_from_file_location("t4b_extractor", args.t4b_extractor)
    if spec is None or spec.loader is None:
        parser.error(f"cannot load shared extractor: {args.t4b_extractor}")
    extractor = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(extractor)
    try:
        module, body = extractor.find_cell(args.xml, "TestbenchMEMAXI")
        count = body.count("automatic ptr_t currAddr;") + body.count("automatic ptr_t endAddr;")
        body = extractor.normalize_questa_body("TestbenchMEMAXI", body)
    except (OSError, ValueError) as error:
        parser.error(str(error))
    text = extractor.module_text(module, body, memory=True)
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(text, encoding="utf-8")
    print(f"extracted TestbenchMEMAXI with {count} Questa local-variable rewrites")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
