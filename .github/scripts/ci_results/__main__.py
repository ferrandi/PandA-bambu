"""Command-line interface for PandA CI result bundles."""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

from .bundle import BundleValidationError, validate_bundle
from .generate import generate_bundle
from .regressions import run_regression_suite
from .schema import SchemaValidationError
from .serialization import SerializationError
from .summary import render_bundle_summary


def _append(path: Path, text: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("a", encoding="utf-8", newline="\n") as stream:
        stream.write(text)


def _positive_integer(value: str) -> int:
    parsed = int(value)
    if parsed < 1:
        raise argparse.ArgumentTypeError("value must be at least 1")
    return parsed


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    subparsers = parser.add_subparsers(dest="command", required=True)

    generate_parser = subparsers.add_parser("generate", help="generate and validate a bundle")
    generate_parser.add_argument("--output", type=Path, default=Path(".ci-results"))
    generate_parser.add_argument("--repository", type=Path, default=Path.cwd())
    generate_parser.add_argument(
        "--regression-results",
        type=Path,
        help="directory produced by the fast-regression runner",
    )
    generate_parser.add_argument("--summary", type=Path)

    regression_parser = subparsers.add_parser(
        "run-regressions", help="run the hosted Bambu fast-regression suite"
    )
    regression_parser.add_argument("--repository", type=Path, default=Path.cwd())
    regression_parser.add_argument("--bambu", type=Path, required=True)
    regression_parser.add_argument(
        "--results-directory", type=Path, default=Path(".ci-regression-results")
    )
    regression_parser.add_argument(
        "--evidence-directory", type=Path, default=Path(".ci-regression-evidence")
    )
    regression_parser.add_argument("--compiler", default="I386_CLANG16")
    regression_parser.add_argument(
        "--parallel-backend", type=_positive_integer, default=2
    )
    regression_parser.add_argument(
        "--timeout-seconds", type=_positive_integer, default=300
    )

    validate_parser = subparsers.add_parser("validate", help="validate an existing bundle")
    validate_parser.add_argument("bundle", type=Path)

    summary_parser = subparsers.add_parser("summary", help="render a validated bundle")
    summary_parser.add_argument("bundle", type=Path)
    summary_parser.add_argument("--append", type=Path)

    args = parser.parse_args(argv)
    try:
        if args.command == "generate":
            generate_bundle(
                args.output,
                repository=args.repository,
                regression_results=args.regression_results,
            )
            summary = render_bundle_summary(args.output)
            if args.summary:
                _append(args.summary, summary)
            print(f"Generated and validated CI result bundle: {args.output}")
            return 0
        if args.command == "run-regressions":
            suite = run_regression_suite(
                repository=args.repository,
                bambu=args.bambu,
                results_directory=args.results_directory,
                evidence_directory=args.evidence_directory,
                compiler=args.compiler,
                parallel_backend=args.parallel_backend,
                timeout_seconds=args.timeout_seconds,
            )
            print(
                "Fast-regression suite: "
                f"{suite['passed_count']}/{suite['task_count']} passed in "
                f"{suite['duration_seconds']} seconds"
            )
            return int(suite["exit_status"])
        if args.command == "validate":
            validate_bundle(args.bundle)
            print(f"CI result bundle validation passed: {args.bundle}")
            return 0
        summary = render_bundle_summary(args.bundle)
        if args.append:
            _append(args.append, summary)
        else:
            print(summary, end="")
        return 0
    except (
        BundleValidationError,
        SchemaValidationError,
        SerializationError,
        OSError,
        ValueError,
    ) as error:
        print(f"{type(error).__name__}: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
