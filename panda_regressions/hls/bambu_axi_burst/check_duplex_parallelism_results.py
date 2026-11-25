#!/usr/bin/env python3
"""Fail-closed checker and summary for observed duplex-copy parallelism."""
from __future__ import annotations

import argparse
from pathlib import Path
import re
import sys


CASE = "COPY_PARALLELISM_256"
CARRIER = "copy"
WORDS = 256
SUMMARY_FIELDS = {
    "CASE", "PROFILE", "CARRIER", "INVOCATIONS", "EXPECTED_WORDS", "READ_BEATS",
    "WRITE_BEATS", "READ_WRITE_OUTSTANDING_CYCLES", "SIMULTANEOUS_RW_HANDSHAKES",
    "FIRST_AR_CYCLE", "LAST_R_CYCLE", "FIRST_AW_CYCLE", "LAST_W_CYCLE",
    "ACTIVE_WINDOW_CYCLES", "TOTAL_CYCLES", "READ_B_MAX", "READ_OUTSTANDING",
    "READ_FIFO_DEPTH", "WRITE_B_MAX", "WRITE_OUTSTANDING", "WRITE_FIFO_DEPTH",
    "AWREADY_PERIOD", "WREADY_PERIOD", "ARREADY_PERIOD", "RVALID_PERIOD",
    "BVALID_PERIOD", "FINAL_B_RELEASE_DELAY",
}
EVENT_FIELDS = {"CASE", "INDEX", "COUNT", "CYCLE", "READ_B_MAX", "READ_OUTSTANDING",
                "READ_FIFO_DEPTH"}
PERIODS = {"AWREADY_PERIOD": 5, "WREADY_PERIOD": 7, "ARREADY_PERIOD": 4,
           "RVALID_PERIOD": 3, "BVALID_PERIOD": 5, "FINAL_B_RELEASE_DELAY": 3}
PROFILES = {
    "symmetric": {"READ_B_MAX": 16, "READ_OUTSTANDING": 1, "READ_FIFO_DEPTH": 64,
                  "WRITE_B_MAX": 16, "WRITE_OUTSTANDING": 1, "WRITE_FIFO_DEPTH": 32},
    "asymmetric": {"READ_B_MAX": 16, "READ_OUTSTANDING": 2, "READ_FIFO_DEPTH": 64,
                   "WRITE_B_MAX": 8, "WRITE_OUTSTANDING": 1, "WRITE_FIFO_DEPTH": 16},
}
INTEGER = re.compile(r"(?:0|[1-9][0-9]*)\Z")
SUMMARY_MARKER = "DUPLEX_PARALLELISM_METRIC"
EVENT_MARKER = "DUPLEX_LOAD_EVENT"


class CheckError(ValueError):
    """Raised when measured evidence is incomplete or inconsistent."""


def _need(condition: bool, message: str) -> None:
    if not condition:
        raise CheckError(message)


def _fields(line: str, marker: str, expected: set[str], source: Path, line_number: int) -> dict[str, str]:
    tokens = line.split()
    _need(bool(tokens) and tokens[0] == marker,
          f"{source}:{line_number}: unexpected marker record")
    values: dict[str, str] = {}
    for token in tokens[1:]:
        _need(token.count("=") == 1, f"{source}:{line_number}: malformed field {token!r}")
        key, value = token.split("=", 1)
        _need(key in expected, f"{source}:{line_number}: unknown field {key!r}")
        _need(key not in values, f"{source}:{line_number}: duplicate field {key!r}")
        _need(bool(value), f"{source}:{line_number}: empty field {key!r}")
        values[key] = value
    missing = expected - values.keys()
    _need(not missing, f"{source}:{line_number}: missing fields {sorted(missing)}")
    return values


def _number(value: str, label: str, source: Path) -> int:
    _need(INTEGER.fullmatch(value) is not None, f"{source}: {label} must be a nonnegative integer")
    return int(value)


def _read_log(path: Path, profile: str) -> tuple[dict[str, int | str], list[int]]:
    _need(not path.is_symlink() and path.is_file(), f"required metric log missing or symlink: {path}")
    text = path.read_text(encoding="utf-8", errors="strict")
    summaries: list[tuple[int, dict[str, str]]] = []
    events: list[tuple[int, dict[str, str]]] = []
    for line_number, line in enumerate(text.splitlines(), 1):
        stripped = line.strip()
        if not stripped:
            continue
        if SUMMARY_MARKER in stripped:
            _need(stripped.startswith(SUMMARY_MARKER + " "),
                  f"{path}:{line_number}: malformed metric marker row")
            summaries.append((line_number, _fields(stripped, SUMMARY_MARKER, SUMMARY_FIELDS,
                                                    path, line_number)))
        elif EVENT_MARKER in stripped:
            _need(stripped.startswith(EVENT_MARKER + " "),
                  f"{path}:{line_number}: malformed load-event marker row")
            events.append((line_number, _fields(stripped, EVENT_MARKER, EVENT_FIELDS,
                                                  path, line_number)))
    _need(len(summaries) == 1, f"{path}: expected exactly one {SUMMARY_MARKER} record")
    _need(len(events) == WORDS, f"{path}: expected exactly {WORDS} {EVENT_MARKER} records")

    line_number, raw = summaries[0]
    summary: dict[str, int | str] = dict(raw)
    _need(raw["CASE"] == CASE, f"{path}:{line_number}: unexpected CASE")
    _need(raw["PROFILE"] == profile, f"{path}:{line_number}: unexpected PROFILE")
    _need(raw["CARRIER"] == CARRIER, f"{path}:{line_number}: unexpected CARRIER")
    numeric = SUMMARY_FIELDS - {"CASE", "PROFILE", "CARRIER"}
    for field in numeric:
        summary[field] = _number(raw[field], field, path)
    _need(summary["INVOCATIONS"] == 1, f"{path}: INVOCATIONS must be 1")
    for field in ("EXPECTED_WORDS", "READ_BEATS", "WRITE_BEATS"):
        _need(summary[field] == WORDS, f"{path}: {field} must be {WORDS}")
    _need(summary["READ_WRITE_OUTSTANDING_CYCLES"] > 0,
          f"{path}: read/write outstanding overlap must be positive")
    _need(summary["SIMULTANEOUS_RW_HANDSHAKES"] > 0,
          f"{path}: simultaneous read/write handshakes must be positive")
    for field, expected in PROFILES[profile].items():
        _need(summary[field] == expected, f"{path}: {field} must be {expected}")
    for field, expected in PERIODS.items():
        _need(summary[field] == expected, f"{path}: {field} must be {expected}")

    first_ar, last_r = summary["FIRST_AR_CYCLE"], summary["LAST_R_CYCLE"]
    first_aw, last_w = summary["FIRST_AW_CYCLE"], summary["LAST_W_CYCLE"]
    total, active = summary["TOTAL_CYCLES"], summary["ACTIVE_WINDOW_CYCLES"]
    _need(first_ar <= last_r and first_aw <= last_w,
          f"{path}: first/last channel cycle order is invalid")
    _need(total > 0 and max(last_r, last_w) < total,
          f"{path}: channel timing lies outside TOTAL_CYCLES")
    expected_active = max(last_r, last_w) - min(first_ar, first_aw) + 1
    _need(active == expected_active and active > 0,
          f"{path}: ACTIVE_WINDOW_CYCLES must equal observed channel window {expected_active}")

    load_cycles: list[int] = []
    for index, (event_line, event_raw) in enumerate(events):
        event = {key: _number(value, key, path) for key, value in event_raw.items()
                 if key != "CASE"}
        _need(event_raw["CASE"] == CASE, f"{path}:{event_line}: unexpected event CASE")
        _need(event["INDEX"] == index, f"{path}:{event_line}: load indices must be ordered 0..255")
        _need(event["COUNT"] == WORDS, f"{path}:{event_line}: COUNT must be {WORDS}")
        _need(event["READ_B_MAX"] == PROFILES[profile]["READ_B_MAX"] and
              event["READ_OUTSTANDING"] == PROFILES[profile]["READ_OUTSTANDING"] and
              event["READ_FIFO_DEPTH"] == PROFILES[profile]["READ_FIFO_DEPTH"],
              f"{path}:{event_line}: event has wrong read profile parameters")
        cycle = event["CYCLE"]
        if load_cycles:
            _need(cycle > load_cycles[-1], f"{path}:{event_line}: LOAD timestamps must strictly increase")
        load_cycles.append(cycle)
    _need(len(set(load_cycles)) == WORDS, f"{path}: duplicate LOAD timestamps")
    return summary, load_cycles


def verify(output: Path) -> dict[str, dict[str, float | int | str]]:
    output = output.resolve()
    _need(output.is_dir(), f"output directory missing: {output}")
    expected = {
        "symmetric": output / "symmetric_copy" / "scenarios" / f"{CASE}.log",
        "asymmetric": output / "asymmetric_copy" / "scenarios" / f"{CASE}.log",
    }
    discovered = list(output.rglob(f"{CASE}.log"))
    _need(len(discovered) == 2 and {path.resolve() for path in discovered} ==
          {path.resolve() for path in expected.values()},
          f"output must contain exactly the two expected {CASE}.log files")
    report: dict[str, dict[str, float | int | str]] = {}
    for profile, path in expected.items():
        summary, cycles = _read_log(path, profile)
        intervals = [right - left for left, right in zip(cycles, cycles[1:])]
        margin = max(PROFILES[profile]["READ_B_MAX"], PROFILES[profile]["WRITE_B_MAX"])
        central_events = cycles[margin:WORDS - margin]
        central_intervals = [right - left for left, right in zip(central_events, central_events[1:])]
        report[profile] = {
            "throughput_words_per_active_cycle": summary["EXPECTED_WORDS"] / summary["ACTIVE_WINDOW_CYCLES"],
            "completion_interval_average_cycles": sum(intervals) / len(intervals),
            "completion_interval_min_cycles": min(intervals),
            "completion_interval_max_cycles": max(intervals),
            "central_interval_average_cycles": sum(central_intervals) / len(central_intervals),
            "central_interval_min_cycles": min(central_intervals),
            "central_interval_max_cycles": max(central_intervals),
            "central_interval_count": len(central_intervals),
        }
    return report


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path, help="G3.1 output directory")
    args = parser.parse_args(argv)
    try:
        report = verify(args.output)
    except (CheckError, OSError, UnicodeError) as exc:
        print(f"DUPLEX PARALLELISM CHECK FAILED: {exc}", file=sys.stderr)
        return 1
    for profile, metrics in report.items():
        print(f"DUPLEX PARALLELISM PASS PROFILE={profile}")
        print(f"  throughput={metrics['throughput_words_per_active_cycle']:.6f} words/active-cycle")
        print("  completion interval average/min/max="
              f"{metrics['completion_interval_average_cycles']:.3f}/"
              f"{metrics['completion_interval_min_cycles']}/"
              f"{metrics['completion_interval_max_cycles']} cycles")
        print("  central completion interval average/min/max="
              f"{metrics['central_interval_average_cycles']:.3f}/"
              f"{metrics['central_interval_min_cycles']}/"
              f"{metrics['central_interval_max_cycles']} cycles "
              f"(n={metrics['central_interval_count']})")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
