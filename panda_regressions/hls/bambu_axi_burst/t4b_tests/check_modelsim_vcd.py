#!/usr/bin/env python3
"""Fail-closed final-counter checker for generated burst-engine VCD traces.

The historical ``VCD N REPORT`` interface is deliberately kept byte-for-byte
compatible.  The richer T4B checker lives in :mod:`t4b_vcd` and is selected
only by its separate ``--mode`` CLI.
"""

import re
import sys


REQUIRED = {
    "issued", "returned", "consumed", "fifo_count",
    "burst_outstanding", "region_count",
}


def check(vcd, n):
    expected = max(n, 0)
    scope = []
    signals = {}
    ids = {}
    with open(vcd, encoding="ascii", errors="replace") as stream:
        in_defs = True
        for line in stream:
            text = line.strip()
            if not in_defs:
                break
            if text.startswith("$scope"):
                parts = text.split()
                if len(parts) >= 3:
                    scope.append(parts[2])
            elif text.startswith("$upscope"):
                if scope:
                    scope.pop()
            elif text.startswith("$var"):
                parts = text.split()
                if len(parts) >= 5 and "burst_engine" in scope and parts[4] in REQUIRED:
                    width, ident, name = int(parts[2]), parts[3], parts[4]
                    signals[name] = (ident, width)
                    ids[ident] = name
            elif text == "$enddefinitions $end":
                in_defs = False

    missing = sorted(REQUIRED - signals.keys())
    if missing:
        raise ValueError("required generated-engine counters absent: " + ",".join(missing))

    # None is deliberately stored for X/Z updates. It permits uninitialized
    # values before reset but ensures the last value cannot inherit an earlier
    # valid count if the signal becomes unknown later in the trace.
    values = {}
    peaks = {name: 0 for name in REQUIRED}
    with open(vcd, encoding="ascii", errors="replace") as stream:
        for line in stream:
            text = line.strip()
            if text.startswith("b"):
                match = re.match(r"b([01xXzZ]+)\s+(\S+)", text)
                if not match or match.group(2) not in ids:
                    continue
                raw, ident = match.groups()
            elif re.match(r"^[01xXzZ]\S+$", text):
                raw, ident = text[0], text[1:]
                if ident not in ids:
                    continue
            else:
                continue

            name = ids[ident]
            if any(bit in raw.lower() for bit in "xz"):
                values[name] = None
                continue
            value = int(raw, 2)
            values[name] = value
            peaks[name] = max(peaks[name], value)

    final = {name: values.get(name) for name in REQUIRED}
    failures = []
    for name in ("issued", "returned", "consumed", "region_count"):
        if final[name] != expected:
            failures.append(f"{name}={final[name]} expected={expected}")
    for name in ("fifo_count", "burst_outstanding"):
        if final[name] != 0:
            failures.append(f"{name}={final[name]} expected=0")
    if peaks["burst_outstanding"] > 1:
        failures.append(f"outstanding_peak={peaks['burst_outstanding']} exceeds O=1")

    report = ["coverage_status=" + ("FAIL" if failures else "PASS"),
              f"N={n}", f"expected_count={expected}", "B=16", "O=1"]
    report.extend(f"final_{name}={final[name]}" for name in (
        "issued", "returned", "consumed", "fifo_count", "burst_outstanding", "region_count"))
    report.append(f"outstanding_peak={peaks['burst_outstanding']}")
    if failures:
        report.append("failures=" + "; ".join(failures))
    return failures, "\n".join(report) + "\n"


def main(argv):
    if len(argv) > 1 and argv[1] == "--mode":
        try:
            from t4b_vcd import main as strict_main
        except ImportError as exc:
            print("VCD qualification failed: strict checker unavailable: " + str(exc), file=sys.stderr)
            return 1
        return strict_main(argv[1:])
    if len(argv) != 4:
        print("Usage: check_modelsim_vcd.py VCD N REPORT", file=sys.stderr)
        return 2
    vcd, n_text, report_path = argv[1:]
    try:
        n = int(n_text)
        failures, report = check(vcd, n)
        with open(report_path, "w", encoding="ascii") as output:
            output.write(report)
        if failures:
            raise ValueError("; ".join(failures))
        return 0
    except Exception as exc:
        print("VCD qualification failed: " + str(exc), file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main(sys.argv))
