#!/usr/bin/env python3
import argparse
import logging
import os
import sys
from typing import List
import xml.etree.ElementTree as ET


def _attribute_priority(tag: str) -> List[str]:
    if tag.startswith("function_"):
        return ["id", "name", "function_pipelined", "function_ii"]
    if tag.startswith("loop_"):
        return ["bb", "ii"]
    if tag == "application":
        return ["benchmark", "version", "verbosity", "timestamp", "args", "workdir"]
    if tag == "backend":
        return ["parallel", "bambu_results", "discrepancy", "assert", "fast", "sdc_ext_file", "vcd"]
    return []


def normalize_attribute_order(node: ET.Element):
    preferred = _attribute_priority(node.tag)
    if node.attrib:
        ordered = {}
        for key in preferred:
            if key in node.attrib:
                ordered[key] = node.attrib[key]
        for key in sorted(node.attrib):
            if key not in ordered:
                ordered[key] = node.attrib[key]
        node.attrib.clear()
        node.attrib.update(ordered)
    for child in node:
        normalize_attribute_order(child)

def find_simulation_runs(app: ET.Element):
    runs = app.findall('.//timing//simulation//run')
    if runs:
        return runs
    return app.findall('.//timing//evaluation//run')

def findall(explicit_files, input_dirs, target_name="bambu_results.xml"):
    candidates = []
    candidates.extend(explicit_files or [])
    for d in input_dirs or []:
        if not os.path.isdir(d):
            logging.warning("--input path is not a directory: %s", d)
            continue
        for root, _, files in os.walk(d):
            if target_name in files:
                candidates.append(os.path.join(root, target_name))
    # De-duplicate while preserving order
    seen = set()
    out = []
    for f in candidates:
        af = os.path.abspath(f)
        if af not in seen:
            seen.add(af)
            out.append(af)
    return out

def build_aggregate(app_elems: List[ET.Element]):
    top = ET.Element("benchmarks")
    top.extend(app_elems)
    return ET.ElementTree(top)

def build_evaluation_summary(app_elems: List[ET.Element]):
    """Build a compressed summary tree.

    For each <application> in app_elems produce an <application> element
    that contains only a single <resources .../> child with the same
    attributes plus any top-level <evaluation .../> attributes. If
    timing/run data is present, compute total and average cycles across
    all simulation runs and add them as
    attributes on the <resources> element: TOTAL_CYCLES, CYCLES, RUNS.
    """
    top = ET.Element("benchmarks")
    for app in app_elems:
        new_app = ET.Element("application")
        # copy application attributes (e.g. name, workdir)
        for k, v in app.attrib.items():
            new_app.set(k, v)

        # find resources element and copy its attributes only
        res = app.find("resources")
        res_summary = ET.Element("resources")
        if res is not None:
            for k, v in res.attrib.items():
                res_summary.set(k, v)
        eval_elm = app.find("evaluation")
        if eval_elm is not None:
            for k, v in eval_elm.attrib.items():
                res_summary.set(k, v)
        
        # collect all simulation runs, falling back to the legacy timing/evaluation path
        runs = find_simulation_runs(app)
        run_vals = []
        for r in runs:
            if r is None or r.text is None:
                continue
            txt = r.text.strip()
            if not txt:
                continue
            try:
                run_vals.append(int(txt))
            except Exception:
                continue

        if run_vals:
            total = sum(run_vals)
            count = len(run_vals)
            avg = float(total) / float(count) if count else 0.0
            res_summary.set("TOTAL_CYCLES", str(total))
            res_summary.set("CYCLES", f"{avg:.3f}")
            res_summary.set("RUNS", str(count))

        new_app.append(res_summary)

        top.append(new_app)

    return ET.ElementTree(top)

def build_library(targets: List[ET.Element]):
    target = ET.Element("target")
    tech = ET.SubElement(target, "technology")
    for t in targets:
        for lib in t.findall('.//library'):
            libname = lib.findtext('./name')
            cells = lib.findall('./cell')
            tlib = tech.find(f"./library[name='{libname}']")
            if tlib is None:
                tlib = ET.SubElement(tech, "library")
                ET.SubElement(tlib, "name").text = libname
            tlib.extend(cells)
    return ET.ElementTree(target)

def main(argv=None):
    p = argparse.ArgumentParser(
        prog="spider",
        description="Aggregate multiple XML files into a unique XML file."
    )
    p.add_argument("-i", "--input", action="append", default=[],
                   help="Directory to recursively search for result files (can be provided multiple times).")
    p.add_argument("-t", "--target", default="bambu_results.xml",
                   help="Target filename to look for when scanning directories (default: bambu_results.xml).")
    p.add_argument("-o", "--output", default="-",
                   help="Output XML file path (use '-' for stdout).")
    p.add_argument("--summary", action="store_true",
                   help="Produce a compressed evaluation summary (resources + timing aggregates).")
    p.add_argument("--mode", choices=["bambu", "eucalyptus"],
                   help="Aggregation mode (choices: bambu, eucalyptus).",
                   default="bambu")
    p.add_argument("files", nargs="*", help="Explicit bambu_results.xml files to aggregate.")
    args = p.parse_args(argv)

    logging.basicConfig(level=logging.INFO, format="%(levelname)s: %(message)s")

    target_name = "bambu_results.xml"
    tree_builder = build_evaluation_summary if args.summary else build_aggregate
    if args.mode == "eucalyptus":
        target_name = "characterization.xml"
        tree_builder = build_library

    # Determine input set (use the configured target filename when scanning dirs)
    sources = findall(args.files, args.input, target_name)
    if not sources:
        logging.error("no input XML files found. Provide files as arguments or use --input DIR.")
        return 2

    # Collect applications
    roots = []
    for path in sources:
        if not os.path.isfile(path):
            logging.warning("skipping missing file: %s", path)
            continue
        try:
            tree = ET.parse(path)
        except ET.ParseError as e:
            logging.warning("skipping malformed XML: %s: %s", path, e)
            continue
        root = tree.getroot()
        wdir = os.path.abspath(os.path.dirname(path))
        root.set("workdir", wdir)
        roots.append(root)

    if not roots:
        logging.error("no <application> elements found in provided inputs.")
        return 3

    # Build output tree
    tree = tree_builder(roots)
    out_root = tree.getroot()
    if out_root is None:
        logging.error("empty XML tree")
        return 3
    normalize_attribute_order(out_root)
    try:
        # Pretty print (Python ≥3.9)
        try:
            ET.indent(tree, space="  ", level=0)  # type: ignore[attr-defined]
        except Exception:
            pass
        if args.output == "-" or args.output == "":
            tree.write(sys.stdout.buffer, encoding="utf-8", xml_declaration=True)
        else:
            out_path = os.path.abspath(args.output)
            os.makedirs(os.path.dirname(out_path) or ".", exist_ok=True)
            tree.write(out_path, encoding="utf-8", xml_declaration=True)
            logging.info("wrote: %s", out_path)
    except OSError as e:
        logging.error("failed to write output: %s", e)
        return 4

    return 0

if __name__ == "__main__":
    sys.exit(main())
