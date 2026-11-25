#!/usr/bin/env python3
"""keep_cell_select -- Phase-C CI helper (owner: test-worker, t4).

Reads the authoritative keep-list JSON, selects one cell, and mints the
per-cell benchmark list file that mantis.py consumes through
--benchmarks-list (a file path, resolved by mantis against
--benchmarks-root). This is the mechanical bridge from the keep-list
(t3, authoritative at coverage-analysis/keep_lists_draft.json) into the
CI matrix (minimal.yml, gcc_regression_simple jobs): each matrix entry
names exactly one keep-cell; the script under it calls this helper to
mint the selection and to derive compiler/simulator/opt from the cell
itself, so the keep-list is the single source of truth for what runs.

Law notes, stated as they are:
  * The trio test files are NOT deleted (GCC sources stay on disk);
    trio membership in a cell decides whether a cell runs them.
  * The per-test timeout in the coverage lane is cov_run.py --timeout
    (watchdog threading.Timer); GitHub timeout-minutes is a PER-JOB
    axis and is deliberately not added by this helper or its callers.
    The per-tool-execution limit in CI comes from mantis.py -t.
  * Refuse-not-heal: a missing keep-list, missing cell, empty list or
    non-ASCII payload is a refusal with rc 2, printed, never repaired.
  * The digest of the keep-list bytes read is printed next to the cell
    name, so every CI log row carries the identity of its input.

Usage:
  keep_cell_select.py --keep-list PATH --cell NAME --out FILE
Exit: 0 minted; 2 refusal. Output file: one test name per line.
"""
import argparse
import hashlib
import json
import os
import sys


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--keep-list", required=True)
    ap.add_argument("--cell", required=True)
    ap.add_argument("--expect-sha256", default="",
                    help="if given, the keep-list bytes MUST hash to "
                         "this value; a disagreement is refused (the "
                         "CI rows pin the ruling's object this way "
                         "until the carrier swap)")
    ap.add_argument("--out", required=True)
    ap.add_argument("--prefix", default="",
                    help="prepended to every minted name (e.g. "
                         "'gcc_regression_simple/') so entries resolve "
                         "against mantis benchmarks_root (-b), not only "
                         "against a suite positional argument")
    ap.add_argument("--print-coord", action="store_true",
                    help="print cell coordinates and exit 0 "
                         "(caller derives compiler/simulator/opt)")
    args = ap.parse_args()

    if not os.path.isfile(args.keep_list):
        print("REFUSE: keep-list not found: %s" % args.keep_list)
        return 2
    raw = open(args.keep_list, "rb").read()
    digest = hashlib.sha256(raw).hexdigest()
    print("KEEP-LIST %s size=%d sha256=%s (identity of the input read)"
          % (args.keep_list, len(raw), digest))
    if args.expect_sha256 and digest != args.expect_sha256:
        print("REFUSE: keep-list digest %s != expected %s -- the "
              "object of record disagrees with the caller's pin"
              % (digest, args.expect_sha256))
        return 2
    try:
        doc = json.loads(raw.decode("ascii"))  # non-ASCII -> refusal
    except (UnicodeDecodeError, ValueError) as e:
        print("REFUSE: keep-list not ASCII/JSON-parseable: %s" % e)
        return 2
    cells = doc.get("cells")
    if not isinstance(cells, dict) or args.cell not in cells:
        print("REFUSE: cell '%s' absent from keep-list (domain: cells"
              " keys: %s)" % (args.cell, sorted(cells or {})))
        return 2
    cell = cells[args.cell]
    lst = cell.get("list")
    if not isinstance(lst, list) or not lst:
        print("REFUSE: cell '%s' has no non-empty list" % args.cell)
        return 2
    for name in lst:
        if not isinstance(name, str) or not name:
            print("REFUSE: cell '%s' holds a non-string entry: %r"
                  % (args.cell, name))
            return 2
        if not name.isascii():
            print("REFUSE: cell '%s' holds a non-ASCII name: %r"
                  % (args.cell, name))
            return 2
    coord = {k: cell.get(k) for k in ("setup", "simulator", "opt",
                                      "variant")}
    print("CELL %s n=%d setup=%s simulator=%s opt=%s variant=%s"
          " (coordinates read from the cell, not recalled)"
          % (args.cell, len(lst), coord["setup"], coord["simulator"],
             coord["opt"], coord["variant"]))
    if args.print_coord:
        return 0
    prefix = args.prefix
    if prefix:
        if not prefix.isascii():
            print("REFUSE: --prefix not ASCII: %r" % (prefix,))
            return 2
        if (prefix.startswith("/") or ".." in prefix.split("/")
                or not prefix.strip("/")):
            print("REFUSE: --prefix escapes or is empty: %r -- the "
                  "prefix is input too (Lead meter, wave 221 probes)"
                  % (prefix,))
            return 2
        if not prefix.endswith("/"):
            prefix += "/"
        for name in lst:
            if name.startswith("/") or ".." in name.split("/"):
                print("REFUSE: entry escapes the suite root: %r" % (name,))
                return 2
    lines = [(prefix + name) if prefix else name for name in lst]
    part = args.out + ".part"
    with open(part, "w", encoding="ascii") as fh:
        fh.write("\n".join(lines) + "\n")
    os.rename(part, args.out)
    print("MINTED %s lines=%d (atomic .part+rename, one name per line)"
          % (args.out, len(lst)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
