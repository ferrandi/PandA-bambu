#!/usr/bin/env python3
"""Create an auditable T4B fixture copy with only its AXI pragma changed."""

from __future__ import annotations

import argparse
import difflib
import hashlib
import json
from pathlib import Path
import re
import sys


HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[3]
if not (ROOT / "panda_regressions").is_dir():
    raise RuntimeError(f"unexpected T4B harness root: {ROOT}")
FIXTURES = {
    "sum": (HERE / "fixtures/sum_read_kernel.cpp", "kernel"),
    "independent": (HERE / "fixtures/first_read_independent_work.cpp", "first_read_independent_work"),
    "mulxor": (HERE / "fixtures/first_read_mulxor_work.cpp", "first_read_mulxor_work"),
}
DECIMAL = re.compile(r"(?:0|[1-9][0-9]*)\Z")


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def parse_positive(name: str, value: str, maximum: int) -> int:
    if not DECIMAL.fullmatch(value):
        raise ValueError(f"{name} must be an unsigned decimal integer")
    number = int(value)
    if not 1 <= number <= maximum:
        raise ValueError(f"{name} must be in 1..{maximum}")
    return number


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--fixture", choices=FIXTURES, required=True)
    parser.add_argument("--b", required=True)
    parser.add_argument("--o", required=True)
    parser.add_argument("--d", required=True)
    parser.add_argument("--l", required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--manifest", type=Path, required=True)
    args = parser.parse_args(argv)
    try:
        b = parse_positive("B", args.b, 256)
        o = parse_positive("O", args.o, 16)
        d = parse_positive("D", args.d, 4096)
        latency = parse_positive("L", args.l, 0xFFFFFFFF)
        output_path = args.output.resolve()
        manifest_path = args.manifest.resolve()
        for label, path in (("output", output_path), ("manifest", manifest_path)):
            if path.exists():
                raise ValueError(f"refusing to overwrite existing {label}: {path}")
        if output_path == manifest_path:
            raise ValueError("output source and manifest paths must differ")
        source, top = FIXTURES[args.fixture]
        if not source.is_file():
            raise ValueError(f"fixture does not exist: {source}")
        text = source.read_text(encoding="utf-8")
        pattern = re.compile(r"(?m)^(?P<indent>[ \t]*)#pragma HLS interface\b[^\n]*$")
        matches = [match for match in pattern.finditer(text)
                   if "max_read_burst_length" in match.group(0)
                   or "num_read_outstanding" in match.group(0)
                   or "read_fifo_depth" in match.group(0)]
        if len(matches) != 1:
            raise ValueError(f"expected exactly one interface pragma in {source}, found {len(matches)}")
        match = matches[0]
        old = match.group(0)
        # Keep the source's original interface identity (port/bundle/offset/mode)
        # and replace only the profile fields; normalize prior spellings away.
        preserved = re.sub(
            r"\s+(?:max_read_burst_length|num_read_outstanding|read_fifo_depth|latency)\s*=\s*[^\s]+",
            "", old,
        )
        new = f"{preserved} max_read_burst_length={b} num_read_outstanding={o} read_fifo_depth={d} latency={latency}"
        generated_text = text[:match.start()] + new + text[match.end():]
        output_path.parent.mkdir(parents=True, exist_ok=True)
        manifest_path.parent.mkdir(parents=True, exist_ok=True)
        output_path.write_text(generated_text, encoding="utf-8")
        diff = "".join(difflib.unified_diff(
            text.splitlines(keepends=True), generated_text.splitlines(keepends=True),
            fromfile=str(source), tofile=str(output_path),
        ))
        diff_path = output_path.with_suffix(output_path.suffix + ".diff")
        diff_path.write_text(diff, encoding="utf-8")
        payload = {
            "fixture": args.fixture,
            "top_function": top,
            "source": str(source),
            "generated_source": str(output_path),
            "source_sha256": sha256(source),
            "generated_sha256": sha256(output_path),
            "diff_file": str(diff_path),
            "diff_sha256": sha256(diff_path),
            "changed_interface_pragma": {"before": old, "after": new},
            "profile": {"B": b, "O": o, "D": d, "declared_latency": latency},
        }
        manifest_path.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")
    except (OSError, ValueError) as error:
        print(f"t4b_profile: {error}", file=sys.stderr)
        return 2
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
