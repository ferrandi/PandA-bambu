"""Validate and load the table of parameter-only compiler profile cases."""

# Author: Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>

from __future__ import annotations

import argparse
from dataclasses import dataclass
import json
import os
from pathlib import Path
import re
import shlex
import sys

import compiler_test_support as support


FIELDS = ("direction", "legacy_id", "top", "macro", "tag", "outcome", "burst",
          "outstanding", "fifo", "diagnostic", "profile_attributes", "engine_parameters")


@dataclass(frozen=True)
class ProfileCase:
    direction: str
    legacy_id: str
    top: str
    macro: str
    tag: str
    outcome: str
    burst: str
    outstanding: str
    fifo: str
    diagnostic: str
    profile_attributes: tuple[str, ...]
    engine_parameters: tuple[str, ...]


def load_cases(path: Path) -> list[ProfileCase]:
    cases: list[ProfileCase] = []
    ids: set[str] = set()
    tags: set[str] = set()
    macros: set[str] = set()
    for line_number, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        if not line or line.startswith("#"):
            continue
        values = line.split("|")
        if len(values) != len(FIELDS):
            raise ValueError(f"{path}:{line_number}: expected {len(FIELDS)} fields, got {len(values)}")
        row = dict(zip(FIELDS, values, strict=True))
        if row["direction"] not in {"read", "write"} or row["outcome"] not in {"accepted", "invalid"}:
            raise ValueError(f"{path}:{line_number}: invalid direction or outcome")
        for key in ("legacy_id", "tag", "macro"):
            if not row[key] or row[key] in {"-"}:
                raise ValueError(f"{path}:{line_number}: missing {key}")
        if row["legacy_id"] in ids or row["tag"] in tags or row["macro"] in macros:
            raise ValueError(f"{path}:{line_number}: duplicate legacy id, tag or macro")
        ids.add(row["legacy_id"])
        tags.add(row["tag"])
        macros.add(row["macro"])
        if row["outcome"] == "accepted":
            try:
                burst, outstanding, fifo = (int(row[key]) for key in ("burst", "outstanding", "fifo"))
            except ValueError as exc:
                raise ValueError(f"{path}:{line_number}: accepted case needs numeric B/O/FIFO values") from exc
            if not (1 <= burst <= 256 and 1 <= outstanding <= 16 and
                    1 <= fifo <= 4096 and fifo & (fifo - 1) == 0):
                raise ValueError(f"{path}:{line_number}: accepted profile is out of range")
            if row["diagnostic"]:
                raise ValueError(f"{path}:{line_number}: accepted case cannot have a diagnostic")
        elif not row["diagnostic"] or any(row[key] for key in ("burst", "outstanding", "fifo")):
            raise ValueError(f"{path}:{line_number}: invalid case needs only a diagnostic expectation")
        cases.append(ProfileCase(
            row["direction"], row["legacy_id"], row["top"], row["macro"], row["tag"], row["outcome"],
            row["burst"], row["outstanding"], row["fifo"], row["diagnostic"],
            tuple(filter(None, row["profile_attributes"].split(","))),
            tuple(filter(None, row["engine_parameters"].split(","))),
        ))
    if not cases:
        raise ValueError(f"{path}: no profile cases")
    return cases


def validate_fixture(cases: list[ProfileCase], fixture: Path, direction: str | None = None) -> None:
    source = fixture.read_text(encoding="utf-8")
    for case in cases:
        if direction is not None and case.direction != direction:
            continue
        if f"defined({case.macro})" not in source:
            raise ValueError(f"{fixture}: missing case macro {case.macro}")
        if re.search(rf"\b{re.escape(case.top)}\s*\(", source) is None:
            raise ValueError(f"{fixture}: missing top {case.top}")


def execute_compiler(bambu: Path, output: Path, argv: list[str], expected_failure: str = "") -> int:
    bambu = bambu.resolve(strict=True)
    bambu_hls = os.environ.get("BAMBU_HLS")
    if not bambu_hls:
        raise support.CompilerTestError("BAMBU_HLS is unset; refusing to guess the ASTAnalyzer plugin")
    selected_install = Path(bambu_hls).resolve(strict=True)
    plugin = selected_install / "lib/panda/clang-16/ASTAnalyzer.so"
    support.validate_install(bambu, selected_install, plugin)
    output = support.create_output_dir(output)
    command = [str(bambu), *argv]
    (output / "argv.json").write_text(json.dumps(command, indent=2) + "\n", encoding="utf-8")
    (output / "argv.txt").write_text(support.format_argv(command) + "\n", encoding="utf-8")
    result = support.run_compiler(
        command, cwd=output, timeout=600,
        expected_failure=diagnostic_substring(expected_failure) if expected_failure else None,
        sandbox_markers=("Bad system call", "Operation not permitted", "Failed to create stream fd"),
        diagnostic_output_dir=output,
    )
    if result.returncode >= 128:
        raise support.CompilerTestError(f"compiler exited with crash status {result.returncode}")
    (output / "stdout.log").write_text(result.stdout, encoding="utf-8")
    (output / "stderr.log").write_text(result.stderr, encoding="utf-8")
    (output / "exit_code.txt").write_text(f"{result.returncode}\n", encoding="ascii")
    # The manifest command reports harness success; preserve Bambu's actual
    # status in exit_code.txt for diagnostics and provenance.
    return 0


def diagnostic_substring(pattern: str) -> str:
    """Return the stable literal prefix from the manifest's grep/regex expectation."""
    return pattern.split(".*", 1)[0].replace(r"\[", "[").replace(r"\]", "]")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    subparsers = parser.add_subparsers(dest="command", required=True)
    validate_parser = subparsers.add_parser("validate")
    validate_parser.add_argument("manifest", type=Path)
    validate_parser.add_argument("--fixture", type=Path, required=True)
    validate_parser.add_argument("--direction", choices=("read", "write"))
    execute_parser = subparsers.add_parser("execute")
    execute_parser.add_argument("--output", type=Path, required=True)
    execute_parser.add_argument("--bambu", type=Path, required=True)
    execute_parser.add_argument("--expected-failure", default="")
    execute_parser.add_argument("compiler_argv", nargs=argparse.REMAINDER)
    args = parser.parse_args(argv)
    try:
        if args.command == "execute":
            if not args.compiler_argv:
                raise ValueError("execution requires compiler arguments after --")
            command_args = args.compiler_argv[1:] if args.compiler_argv[0] == "--" else args.compiler_argv
            return execute_compiler(args.bambu, args.output, command_args, args.expected_failure)
        cases = load_cases(args.manifest)
        validate_fixture(cases, args.fixture, args.direction)
    except (OSError, ValueError, support.CompilerTestError) as exc:
        print(exc, file=sys.stderr)
        return 2
    print(f"validated {len(cases)} compiler profile cases from {args.manifest}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
