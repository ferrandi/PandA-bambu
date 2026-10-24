#!/usr/bin/env python3
"""Check canonical O/D metadata in a generated architecture.xml."""
import pathlib
import sys
import xml.etree.ElementTree as ET


EXPECTED = {
    "oa": ("1", None),
    "ob": ("3", None),
    "oc": ("15", None),
    "od": ("16", None),
    "oe": (None, "1"),
    "of": (None, "8"),
    "og": (None, "32"),
    "oh": (None, "256"),
    "oi": (None, "4096"),
    "oj": ("3", "8"),
}


def main() -> int:
    if len(sys.argv) != 3 or sys.argv[2] != "positive":
        print("usage: check_read_profile_architecture.py ARCHITECTURE_XML positive", file=sys.stderr)
        return 2
    path = pathlib.Path(sys.argv[1])
    root = ET.parse(path).getroot()
    fn = next((node for node in root.findall("function") if node.get("symbol") == "read_profile_valid"), None)
    if fn is None:
        raise SystemExit(f"read_profile_valid missing from {path}")
    bundles = {node.get("name"): node.attrib for node in fn.findall("./bundles/bundle")}
    for name, (outstanding, depth) in EXPECTED.items():
        attrs = bundles.get(name)
        if attrs is None:
            raise SystemExit(f"bundle {name!r} missing from {path}")
        if outstanding is None:
            if "num_read_outstanding" in attrs:
                raise SystemExit(f"default num_read_outstanding unexpectedly serialized for {name}: {attrs}")
        elif attrs.get("num_read_outstanding") != outstanding:
            raise SystemExit(f"bundle {name} has noncanonical num_read_outstanding: {attrs}")
        if depth is None:
            if "read_fifo_depth" in attrs:
                raise SystemExit(f"default read_fifo_depth unexpectedly serialized for {name}: {attrs}")
        elif attrs.get("read_fifo_depth") != depth:
            raise SystemExit(f"bundle {name} has noncanonical read_fifo_depth: {attrs}")
    print(f"architecture profile PASS: {path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
