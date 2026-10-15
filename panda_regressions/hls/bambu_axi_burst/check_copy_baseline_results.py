#!/usr/bin/env python3
"""Fail-closed checker for measured copy baseline artifacts."""
import argparse
import csv
import hashlib
from pathlib import Path
import re
import sys
import xml.etree.ElementTree as ET

FIELDS = ("case", "profile", "cycles", "n", "maxn", "addr_bits", "burst_max", "outstanding",
          "fifo_depth", "read_bundle", "write_bundle")
CASES = {
    "copy-burst-two-bundles": ("burst", 1, "input", "output"),
    "copy-legacy-two-bundles": ("legacy-two", 2, "input", "output"),
    "copy-legacy-shared-bundle": ("legacy-shared", 3, "shared", "shared"),
}
INTEGER = re.compile(r"(?:0|[1-9][0-9]*)\Z")
READ_CORE = re.compile(r"\bMinimalAXI4MasterPipelined\b")
WRITE_CORE = re.compile(r"\bMinimalAXI4MasterPipelinedWrite\b")
LEGACY_CORE = re.compile(r"\bMinimalAXI4AdapterSingleBeat\b|\bIOB_cache_axi\b", re.I)


def fail(message):
    raise ValueError(message)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def read_rows(path, source, testbench):
    with path.open(encoding="utf-8", newline="") as stream:
        reader = csv.DictReader(stream, delimiter="\t")
        if reader.fieldnames != list(FIELDS):
            fail(f"copy expectation header must be {list(FIELDS)}")
        rows = list(reader)
    if {row.get("case") for row in rows} != set(CASES) or len(rows) != len(CASES):
        fail("copy expectation inventory must contain exactly the three required cases")
    text = source.read_text(encoding="utf-8")
    tb_text = testbench.read_text(encoding="utf-8")
    source_maxn = re.search(r"(?m)^\s*#define\s+COPY_BASELINE_MAXN\s+(\d+)\s*$", text)
    tb_maxn = re.search(r"(?m)^\s*#define\s+COPY_BASELINE_MAXN\s+(\d+)\s*$", tb_text)
    if not source_maxn or not tb_maxn or source_maxn.group(1) != tb_maxn.group(1) or source_maxn.group(1) != "4096":
        fail("copy source and testbench must agree on COPY_BASELINE_MAXN=4096")
    for row in rows:
        if None in row or any(not row.get(field) for field in FIELDS):
            fail(f"{row.get('case', '<unknown>')}: incomplete expectation")
        case = row["case"]
        profile, macro, read_bundle, write_bundle = CASES[case]
        if (row["profile"], row["read_bundle"], row["write_bundle"]) != \
                (profile, read_bundle, write_bundle):
            fail(f"{case}: unexpected profile or bundle mapping")
        expected = {"n": "1024", "maxn": "4096", "addr_bits": "32"}
        if macro == 1:
            expected.update(burst_max="16", outstanding="1", fifo_depth="32")
            attrs = ("max_read_burst_length=16", "num_read_outstanding=1", "read_fifo_depth=32",
                     "max_write_burst_length=16", "write_burst_outstanding=1", "write_fifo_depth=32")
        else:
            expected.update(burst_max="0", outstanding="0", fifo_depth="0")
            attrs = ()
        if f"COPY_BASELINE_PROFILE == {macro}" not in text:
            fail(f"{case}: source does not select the expected compile-time profile")
        for bundle in (read_bundle, write_bundle):
            if f"bundle={bundle}" not in text:
                fail(f"{case}: source does not declare bundle={bundle}")
        for key, value in expected.items():
            if row[key] != value:
                fail(f"{case}: {key} must be {value}, got {row[key]}")
        if not INTEGER.fullmatch(row["cycles"]) or int(row["cycles"]) <= 0:
            fail(f"{case}: cycle signature must be a positive integer")
        for attr in attrs:
            if attr not in text:
                fail(f"{case}: source is missing expected pragma attribute {attr}")
    return rows


def check_provenance(case_dir, source, testbench, bambu, settings, plugin, macro):
    path = case_dir / "provenance.txt"
    if not path.is_file():
        fail(f"{case_dir.name}: provenance is missing")
    text = path.read_text(encoding="utf-8", errors="replace")
    profile = CASES[case_dir.name][0]
    metadata = {key: value for key, value in re.findall(r"(?m)^(case|profile|macro|source|testbench)=(.*)$", text)}
    expected_metadata = {"case": case_dir.name, "profile": profile, "macro": str(macro),
                         "source": str(source), "testbench": str(testbench)}
    if any(metadata.get(key) != value for key, value in expected_metadata.items()):
        fail(f"{case_dir.name}: case provenance identity does not match case directory/inputs")
    required = {
        str(source): digest(source), str(testbench): digest(testbench), str(bambu): digest(bambu),
        str(settings): digest(settings), str(plugin): digest(plugin),
    }
    for filename, checksum in required.items():
        if not re.search(rf"(?m)^{re.escape(checksum)}\s+{re.escape(filename)}$", text):
            fail(f"{case_dir.name}: provenance hash missing/mismatched for {filename}")
    argv = re.search(r"(?m)^argv=(.*)$", text)
    required_args = (
        "--top-fname=kernel", "--compiler=I386_CLANG16", "--generate-interface=INFER",
        r"--device-name=xcu250\,-2L\,figd2104", "--AXI-burst-type=INCREMENTAL",
        "--mem-delay-read=64", "--mem-delay-write=2", "--tb-queue-size=32",
        "--memory-allocation-policy=NO_BRAM", "--distram-threshold=0", "--clock-period=3.33",
        "--simulate", "--simulator=MODELSIM", "--generate-tb=", "--no-clean",
        "--bambu-parameter=experimental-m-axi-burst=1",
        f"-DCOPY_BASELINE_PROFILE={macro}")
    if not argv or any(argument not in argv.group(1) for argument in required_args):
        fail(f"{case_dir.name}: provenance argv has an incorrect/missing compile or simulation setting")


def check_run_provenance(results):
    path = results / "provenance.txt"
    if path.is_symlink() or not path.is_file():
        fail("run provenance is missing or symlink")
    output = re.search(r"(?m)^output=(.*)$", path.read_text(encoding="utf-8", errors="replace"))
    if not output or Path(output.group(1)).resolve() != results.resolve():
        fail("run provenance output does not match --results directory")


def check_artifact_manifest(case_dir):
    manifest = case_dir / "artifact-sha256.txt"
    if manifest.is_symlink() or not manifest.is_file():
        fail(f"{case_dir.name}: artifact manifest missing or symlink")
    root = case_dir.resolve()
    declared = {}
    for line_number, line in enumerate(manifest.read_text(encoding="utf-8").splitlines(), 1):
        match = re.fullmatch(r"([0-9a-fA-F]{64})  (.+)", line)
        if not match:
            fail(f"{case_dir.name}: malformed artifact manifest line {line_number}")
        checksum, raw_path = match.groups()
        artifact = Path(raw_path)
        if not artifact.is_absolute():
            artifact = case_dir / artifact
        current = artifact
        while current != current.parent:
            if current.is_symlink():
                fail(f"{case_dir.name}: artifact manifest path contains symlink: {raw_path}")
            if current.resolve() == root:
                break
            current = current.parent
        try:
            resolved = artifact.resolve(strict=True)
            resolved.relative_to(root)
        except (OSError, ValueError):
            fail(f"{case_dir.name}: artifact manifest path is missing or outside case directory: {raw_path}")
        if not resolved.is_file() or resolved in declared:
            fail(f"{case_dir.name}: invalid or duplicate artifact manifest path: {raw_path}")
        declared[resolved] = checksum.lower()
        if digest(resolved) != checksum.lower():
            fail(f"{case_dir.name}: artifact digest mismatch: {raw_path}")
    current_artifacts = set()
    for artifact in case_dir.rglob("*"):
        if artifact.is_symlink():
            if artifact.suffix.lower() in {".v", ".sv"} or artifact.name == "bambu_results.xml":
                fail(f"{case_dir.name}: generated artifact is a symlink: {artifact}")
            continue
        if artifact.is_file() and (artifact.suffix.lower() in {".v", ".sv"} or
                                   artifact.name in {"bambu_results.xml", "testbench.log"}):
            current_artifacts.add(artifact.resolve())
    if set(declared) != current_artifacts:
        fail(f"{case_dir.name}: artifact manifest inventory does not match generated RTL/XML/simulation-log files")


def module_instances(text, module_name):
    marker = re.compile(rf"\b{re.escape(module_name)}\b")
    for match in marker.finditer(text):
        if re.search(r"\bmodule\s+$", text[:match.start()]):
            continue
        index = match.end()
        while index < len(text) and text[index].isspace():
            index += 1
        parameters = {}
        if index < len(text) and text[index] == "#":
            index += 1
            while index < len(text) and text[index].isspace():
                index += 1
            if index >= len(text) or text[index] != "(":
                continue
            start = index + 1
            depth = 1
            index += 1
            while index < len(text) and depth:
                if text[index] == "(":
                    depth += 1
                elif text[index] == ")":
                    depth -= 1
                index += 1
            if depth:
                continue
            parameters = {name: value.strip() for name, value in re.findall(
                r"\.(\w+)\s*\(\s*([^()]*)\s*\)", text[start:index - 1])}
        instance = re.match(r"\s*[A-Za-z_$][\w$]*\s*\(", text[index:])
        if instance:
            yield parameters


def check_case(results, row, source, testbench, bambu, settings, plugin):
    case = row["case"]
    profile, macro, read_bundle, write_bundle = CASES[case]
    case_dir = results / case
    if not case_dir.is_dir() or case_dir.is_symlink():
        fail(f"{case}: result directory missing or symlink")
    check_provenance(case_dir, source, testbench, bambu, settings, plugin, macro)
    check_artifact_manifest(case_dir)
    for log_name in ("stdout.log", "stderr.log"):
        if not (case_dir / log_name).is_file():
            fail(f"{case}: {log_name} missing")
    simulation_log = case_dir / "HLS_output" / "simulation" / "testbench.log"
    if simulation_log.is_symlink() or not simulation_log.is_file():
        fail(f"{case}: expected HLS_output/simulation/testbench.log missing or symlink")
    logs = simulation_log.read_text(encoding="utf-8", errors="replace")
    marker = re.compile(r"COPY_BASELINE_GOLDEN_PASS\s+N=1024")
    if not marker.search(logs):
        fail(f"{case}: simulation log has no copy golden/canary/source-integrity pass marker")
    xml_path = case_dir / "bambu_results.xml"
    if not xml_path.is_file():
        fail(f"{case}: bambu_results.xml missing")
    try:
        root = ET.parse(xml_path).getroot()
    except (OSError, ET.ParseError) as exc:
        fail(f"{case}: malformed results XML: {exc}")
    evaluations = [node for node in root.iter("evaluation") if "CYCLES" in node.attrib]
    if len(evaluations) != 1:
        fail(f"{case}: expected exactly one evaluation CYCLES value")
    cycles = evaluations[0].get("CYCLES", "")
    if not INTEGER.fullmatch(cycles) or int(cycles) <= 0:
        fail(f"{case}: cycle signature is not a positive integer: {cycles!r}")
    if row["cycles"] != cycles:
        fail(f"{case}: expected {row['cycles']} cycles, observed {cycles}")
    rtl_files = [path for path in case_dir.rglob("*") if path.is_file() and
                 path.suffix.lower() in {".v", ".sv"} and path.name != "panda_libtech.v"]
    if not rtl_files:
        fail(f"{case}: generated RTL missing")
    rtl = "\n".join(path.read_text(encoding="utf-8", errors="replace") for path in rtl_files)
    if not re.search(r"(?m)^module\s+kernel\s*\(", rtl):
        fail(f"{case}: generated kernel top missing")
    if profile == "burst":
        if not READ_CORE.search(rtl) or not WRITE_CORE.search(rtl):
            fail(f"{case}: burst copy lacks read and write burst engines")
        if LEGACY_CORE.search(rtl):
            fail(f"{case}: burst copy contains a legacy scalar adapter/cache")
        for pattern, label in ((r"configure_read", "read configure"), (r"burst_read", "read operation"),
                               (r"configure_write", "write configure"), (r"store_write", "write operation")):
            if not re.search(pattern, rtl, re.I):
                fail(f"{case}: burst RTL lacks {label}")
        for bundle, direction, engine_wire in ((read_bundle, "ar", "burst_araddr"),
                                               (write_bundle, "aw", "burst_awaddr")):
            port = f"p_m_axi_{bundle}_{direction}addr"
            if not re.search(rf"assign\s+{re.escape(port)}\s*=\s*{engine_wire}\b", rtl):
                fail(f"{case}: {direction.upper()} burst engine is not wired to bundle {bundle}")
        requirements = {
            "MinimalAXI4MasterPipelined": {"B_MAX": "16", "MAX_OUTSTANDING": "1", "FIFO_DEPTH": "32",
                                            "BITSIZE_m_axi_araddr": row["addr_bits"],
                                            "BITSIZE_in4": row["addr_bits"], "BITSIZE_m_axi_rdata": "32",
                                            "BITSIZE_in5": "32"},
            "MinimalAXI4MasterPipelinedWrite": {"B_MAX": "16", "MAX_OUTSTANDING": "1", "FIFO_DEPTH": "32",
                                                 "BITSIZE_in4": row["addr_bits"], "BITSIZE_in5": "32",
                                                 "BITSIZE_m_axi_awaddr": row["addr_bits"],
                                                 "BITSIZE_in3": "32", "BITSIZE_m_axi_wdata": "32"},
        }
        for module, expected in requirements.items():
            instances = list(module_instances(rtl, module))
            if len(instances) != 1 or not all(
                    instances[0].get(key) == value for key, value in expected.items()):
                fail(f"{case}: expected exactly one {module} instance with the required B/O/F/width profile")
    else:
        if READ_CORE.search(rtl) or WRITE_CORE.search(rtl):
            fail(f"{case}: legacy profile unexpectedly emits a burst engine")
        if not LEGACY_CORE.search(rtl):
            fail(f"{case}: legacy wrapper/resource not found")
        if re.search(r"configure_(?:read|write)|burst_read|store_write", rtl, re.I):
            fail(f"{case}: legacy profile unexpectedly contains burst operations")
        for bundle, direction in ((read_bundle, "ar"), (write_bundle, "aw")):
            port = f"p_m_axi_{bundle}_{direction}addr"
            if not re.search(rf"\.m_axi_{direction}addr\s*\(\s*{re.escape(port)}\s*\)", rtl):
                fail(f"{case}: legacy scalar wrapper is not wired to bundle {bundle}")
    for bundle, direction in ((read_bundle, "ar"), (write_bundle, "aw")):
        if not re.search(rf"p_m_axi_{re.escape(bundle)}_{direction}addr\b", rtl):
            fail(f"{case}: generated RTL lacks {direction.upper()} address interface for {bundle}")
    print(f"COPY-BASELINE PASS: {case} cycles={cycles} profile={profile}")
    return int(cycles)


def main(argv=None):
    parser = argparse.ArgumentParser()
    parser.add_argument("--results", required=True, type=Path)
    parser.add_argument("--expectations", required=True, type=Path)
    parser.add_argument("--source", required=True, type=Path)
    parser.add_argument("--testbench", required=True, type=Path)
    parser.add_argument("--bambu", required=True, type=Path)
    parser.add_argument("--settings", required=True, type=Path)
    parser.add_argument("--plugin", required=True, type=Path)
    args = parser.parse_args(argv)
    try:
        for name, path in (("results", args.results), ("source", args.source),
                           ("testbench", args.testbench), ("Bambu", args.bambu),
                           ("settings", args.settings), ("plugin", args.plugin)):
            if not path.exists() or path.is_symlink():
                fail(f"{name} path missing or symlink: {path}")
        rows = read_rows(args.expectations, args.source, args.testbench)
        check_run_provenance(args.results)
        cycles = [check_case(args.results, row, args.source, args.testbench, args.bambu,
                             args.settings, args.plugin) for row in rows]
        if len(set(cycles)) != len(cycles):
            fail(f"cycle signatures must be unique across profiles, got {cycles}")
    except (OSError, ValueError) as exc:
        print(f"COPY-BASELINE FAIL: {exc}", file=sys.stderr)
        return 1
    print("COPY_BASELINE_GATE_PASS: three profiles, unique cycle signatures, golden/canary and source immutability")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
