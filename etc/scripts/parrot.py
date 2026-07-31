#!/usr/bin/env python3
"""Collect completed Bambu experiments into CSV reports.

This is the standalone successor of the result-collection block that used to
live at the end of mantis.py.  It consumes already-produced bambu_results XML
files; it never launches experiments.
"""

import argparse
import csv
import logging
import math
import os
import sys
import xml.etree.ElementTree as ET
from dataclasses import dataclass
from typing import Dict, Iterable, List, Optional, Sequence


@dataclass
class Column:
    name: str
    source: str
    text_format: str = "text"
    precision: Optional[int] = None


@dataclass
class Result:
    path: str
    values: Dict[str, str]
    application: ET.Element


DEFAULT_COLUMNS = [
    Column("Benchmark Name", "benchmark_name"),
    Column("Num Cycles", "CYCLES", "number", 0),
    Column("Registers", "REGISTERS", "number", 0),
    Column("LUTs", "AREA", "number", 0),
    Column("DSPs", "DSPS", "number", 0),
    Column("BRAMs", "BRAMS", "number", 0),
    Column("DRAMs", "DRAMS", "number", 0),
    Column("Clock Frequency (MHz)", "FREQUENCY", "number", 2),
    Column("Clock Slack (ns)", "CLOCK_SLACK", "number", 2),
    Column("HLS Time(s)", "HLS_execution_time", "number", 2),
]


def discover_results(paths: Sequence[str]) -> List[str]:
    candidates = []
    for path in paths:
        if os.path.isfile(path):
            candidates.append(path)
            continue
        if not os.path.isdir(path):
            logging.warning("skipping missing input: %s", path)
            continue
        for root, dirs, files in os.walk(path):
            dirs.sort()
            for filename in sorted(files):
                if filename.startswith("bambu_results") and filename.endswith(".xml"):
                    candidates.append(os.path.join(root, filename))

    seen = set()
    results = []
    for path in candidates:
        absolute = os.path.abspath(path)
        if absolute not in seen:
            seen.add(absolute)
            results.append(absolute)
    return results


def _simulation_runs(app: ET.Element) -> List[float]:
    runs = app.findall(".//timing//simulation//run")
    if not runs:
        runs = app.findall(".//timing//evaluation//run")
    values = []
    for run in runs:
        try:
            values.append(float((run.text or "").strip()))
        except ValueError:
            pass
    return values


def _set_alias(values: Dict[str, str], destination: str, source: str):
    if destination not in values and source in values:
        values[destination] = values[source]


def flatten_application(app: ET.Element) -> Dict[str, str]:
    """Flatten both legacy element/value and current attribute-based results."""
    values = dict(app.attrib)
    benchmark = app.get("benchmark") or app.get("benchmark_name") or ""
    values["benchmark"] = benchmark
    values["benchmark_name"] = benchmark
    values["workdir"] = app.get("workdir", "")

    for node in app.iter():
        for key, value in node.attrib.items():
            # Evaluation values intentionally override resource estimates.
            if node.tag == "evaluation" or key not in values:
                values[key] = value
        value = node.get("value")
        if value is not None:
            values[node.tag] = value

    runs = _simulation_runs(app)
    if runs:
        total = sum(runs)
        values.setdefault("TOTAL_CYCLES", f"{total:g}")
        values.setdefault("CYCLES", f"{total / len(runs):.3f}")
        values.setdefault("RUNS", str(len(runs)))

    _set_alias(values, "PERIOD", "DELAY")
    _set_alias(values, "CLOCK_SLACK", "SLACK")
    _set_alias(values, "AREA", "SLICES")
    return values


def read_results(paths: Iterable[str]) -> List[Result]:
    results = []
    for path in paths:
        try:
            root = ET.parse(path).getroot()
        except (ET.ParseError, OSError) as error:
            logging.warning("skipping unreadable XML %s: %s", path, error)
            continue

        applications = [root] if root.tag == "application" else root.findall("application")
        if not applications:
            logging.warning("skipping XML without <application>: %s", path)
            continue
        for app in applications:
            if not app.get("workdir"):
                app.set("workdir", os.path.abspath(os.path.dirname(path)))
            results.append(Result(path, flatten_application(app), app))
    return results


def _number(value: str) -> Optional[float]:
    try:
        number = float(value)
        return number if math.isfinite(number) else None
    except (TypeError, ValueError):
        return None


def format_value(value: Optional[str], column: Column) -> str:
    if value is None or value == "":
        return "-"
    if column.text_format != "number":
        return str(value)
    number = _number(value)
    if number is None:
        return str(value)
    precision = column.precision if column.precision is not None else 2
    return f"{number:.{precision}f}"


def write_csv(path: str, columns: Sequence[Column], results: Sequence[Result]):
    _ensure_parent(path)
    with open(path, "w", newline="", encoding="utf-8") as output:
        writer = csv.writer(output)
        writer.writerow(column.name for column in columns)
        for result in results:
            writer.writerow(
                format_value(result.values.get(column.source), column)
                for column in columns
            )


def _ensure_parent(path: str):
    parent = os.path.dirname(os.path.abspath(path))
    os.makedirs(parent, exist_ok=True)


def parse_args(argv=None):
    parser = argparse.ArgumentParser(
        prog="parrot",
        description="Collect completed Bambu result XML files into a CSV report.",
    )
    parser.add_argument(
        "inputs",
        nargs="+",
        help="Completed Mantis output directories or explicit bambu_results XML files.",
    )
    parser.add_argument(
        "--csv",
        default="results.csv",
        help="CSV report path (default: %(default)s).",
    )
    return parser.parse_args(argv)


def main(argv=None):
    args = parse_args(argv)
    logging.basicConfig(level=logging.INFO, format="%(levelname)s: %(message)s")

    paths = discover_results(args.inputs)
    if not paths:
        logging.error("no bambu_results*.xml files found")
        return 2
    results = read_results(paths)
    if not results:
        logging.error("no valid <application> results found")
        return 3
    logging.info("collected %d result(s)", len(results))

    write_csv(args.csv, DEFAULT_COLUMNS, results)
    logging.info("wrote: %s", os.path.abspath(args.csv))
    return 0


if __name__ == "__main__":
    sys.exit(main())
