#!/usr/bin/env python3
"""Fail-closed checker for the controlled four-profile AXI copy comparison."""
import argparse
import csv
import hashlib
import json
from pathlib import Path
import re
import shlex
import sys
import xml.etree.ElementTree as ET

HERE = Path(__file__).resolve().parent
EXPECTATIONS = HERE / "copy_comparison_expectations.tsv"
PROFILES = {
    "burst-two-bundles": {"macro": 1, "read_bundle": "input", "write_bundle": "output",
                          "read_requested": (16, 1, 64), "write_requested": (16, 1, 32),
                          "read_engine": (16, 1, 64), "write_engine": (16, 1, 32), "feature": 1},
    "legacy-two-bundles": {"macro": 2, "read_bundle": "input", "write_bundle": "output",
                           "read_requested": (16, 1, 64), "write_requested": (16, 1, 32),
                           "read_engine": None, "write_engine": None, "feature": 0},
    "legacy-shared": {"macro": 3, "read_bundle": "shared", "write_bundle": "shared",
                      "read_requested": (16, 1, 64), "write_requested": (16, 1, 32),
                      "read_engine": None, "write_engine": None, "feature": 0},
    "duplex-shared": {"macro": 4, "read_bundle": "shared", "write_bundle": "shared",
                      "read_requested": (16, 1, 64), "write_requested": (16, 1, 32),
                      "read_engine": (16, 1, 64), "write_engine": (16, 1, 32), "feature": 1},
}
RUNS = ("run1", "run2")
INTEGER = re.compile(r"(?:0|[1-9][0-9]*)\Z")
READ_CORE = "MinimalAXI4MasterPipelined"
WRITE_CORE = "MinimalAXI4MasterPipelinedWrite"
LEGACY_CORE = re.compile(r"\b(?:MinimalAXI4AdapterSingleBeat|IOB_cache_axi)\b", re.I)
LEGACY_MODULES = ("MinimalAXI4AdapterSingleBeat", "IOB_cache_axi")
COMMON_ARGS = (
    "--top-fname=kernel", "--compiler=I386_CLANG16", "--generate-interface=INFER",
    "--device-name=xcu250,-2L,figd2104", "--AXI-burst-type=INCREMENTAL",
    "--mem-delay-read=64", "--mem-delay-write=2", "--tb-queue-size=32",
    "--memory-allocation-policy=NO_BRAM", "--distram-threshold=0", "--clock-period=3.33",
    "--no-clean", "--simulate", "--simulator=VERILATOR", "--extra-cc-options=-DNDEBUG",
    "--extra-cc-options=-DBAMBU",
)
PARAM_ATTRS = {
    "read": ("max_read_burst_length", "num_read_outstanding", "read_fifo_depth"),
    "write": ("max_write_burst_length", "write_burst_outstanding", "write_fifo_depth"),
}
COPY_MAXN = 4096
COPY_N = 1024


def need(condition, message):
    if not condition:
        raise ValueError(message)


def sha256(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def read_inventory(path=EXPECTATIONS):
    with path.open(encoding="utf-8", newline="") as stream:
        reader = csv.DictReader(stream, delimiter="\t")
        rows = list(reader)
    fieldnames = ["profile", "macro", "read_bundle", "write_bundle",
                  "requested_read_burst", "requested_read_outstanding", "requested_read_fifo",
                  "requested_write_burst", "requested_write_outstanding", "requested_write_fifo",
                  "actual_read_engine_burst", "actual_read_engine_outstanding", "actual_read_engine_fifo",
                  "actual_write_engine_burst", "actual_write_engine_outstanding", "actual_write_engine_fifo"]
    need(reader.fieldnames == fieldnames, "copy comparison expectation header is invalid")
    need(len(rows) == 4 and {row["profile"] for row in rows} == set(PROFILES),
         "copy comparison inventory must contain exactly four profiles")
    for row in rows:
        profile = row["profile"]
        values = PROFILES[profile]
        expected = {"macro": str(values["macro"]), "read_bundle": values["read_bundle"],
                    "write_bundle": values["write_bundle"]}
        for direction in ("read", "write"):
            requested = values[f"{direction}_requested"]
            engine = values[f"{direction}_engine"]
            expected.update({f"requested_{direction}_burst": str(requested[0]),
                             f"requested_{direction}_outstanding": str(requested[1]),
                             f"requested_{direction}_fifo": str(requested[2])})
            engine_fields = ("burst", "outstanding", "fifo")
            expected.update({f"actual_{direction}_engine_{field}":
                             "N/A" if engine is None else str(engine[index])
                             for index, field in enumerate(engine_fields)})
        need(all(row.get(key) == value for key, value in expected.items()),
             f"{profile}: expectation profile parameters are invalid")
    return rows


def module_instance_details(text, name):
    for match in re.finditer(rf"\b{re.escape(name)}\b", text):
        if re.search(r"\bmodule\s+$", text[:match.start()]):
            continue
        pos = match.end()
        while pos < len(text) and text[pos].isspace():
            pos += 1
        params = {}
        if pos < len(text) and text[pos] == "#":
            pos += 1
            while pos < len(text) and text[pos].isspace():
                pos += 1
            if pos >= len(text) or text[pos] != "(":
                continue
            start = pos + 1
            depth = 1
            pos += 1
            while pos < len(text) and depth:
                depth += (text[pos] == "(") - (text[pos] == ")")
                pos += 1
            if depth:
                continue
            params = {key: value.strip() for key, value in re.findall(
                r"\.(\w+)\s*\(\s*([^()]*)\s*\)", text[start:pos - 1])}
        instance = re.match(r"\s*[A-Za-z_$][\w$]*\s*\(", text[pos:])
        if not instance:
            continue
        open_pos = pos + instance.end() - 1
        end_pos = open_pos + 1
        depth = 1
        while end_pos < len(text) and depth:
            depth += (text[end_pos] == "(") - (text[end_pos] == ")")
            end_pos += 1
        if depth:
            continue
        body = text[open_pos + 1:end_pos - 1]
        ports = {}
        for port_match in re.finditer(r"\.(\w+)\s*\(", body):
            arg_start = port_match.end()
            arg_end = arg_start
            arg_depth = 1
            while arg_end < len(body) and arg_depth:
                arg_depth += (body[arg_end] == "(") - (body[arg_end] == ")")
                arg_end += 1
            if arg_depth:
                continue
            ports[port_match.group(1)] = body[arg_start:arg_end - 1].strip()
        yield params, ports


def module_instances(text, name):
    for params, _ in module_instance_details(text, name):
        yield params


def generated_files(run_dir):
    return {p.resolve() for p in run_dir.rglob("*") if p.is_file() and not p.is_symlink() and
            (p.suffix.lower() in {".v", ".sv", ".xml"} or p.name.endswith(".log"))}


def check_manifest(run_dir):
    manifest = run_dir / "artifact-sha256.txt"
    need(not manifest.is_symlink() and manifest.is_file(), f"{run_dir}: artifact manifest missing")
    root = run_dir.resolve()
    declared = {}
    for number, line in enumerate(manifest.read_text(encoding="utf-8").splitlines(), 1):
        match = re.fullmatch(r"([0-9a-f]{64})  (.+)", line)
        need(match is not None, f"{run_dir}: malformed artifact manifest line {number}")
        checksum, raw_path = match.groups()
        path = Path(raw_path)
        if not path.is_absolute():
            path = run_dir / path
        need(not path.is_symlink(), f"{run_dir}: artifact symlink is forbidden: {raw_path}")
        try:
            resolved = path.resolve(strict=True)
            resolved.relative_to(root)
        except (OSError, ValueError):
            need(False, f"{run_dir}: manifest artifact missing/outside run: {raw_path}")
        need(resolved not in declared and resolved.is_file(), f"{run_dir}: duplicate/invalid artifact {raw_path}")
        need(sha256(resolved) == checksum, f"{run_dir}: artifact digest mismatch: {raw_path}")
        declared[resolved] = checksum
    need(set(declared) == generated_files(run_dir),
         f"{run_dir}: manifest does not cover every generated RTL/XML/log artifact exactly once")


def check_provenance(run_dir, profile, run, inputs):
    path = run_dir / "provenance.txt"
    need(not path.is_symlink() and path.is_file(), f"{run_dir}: provenance missing")
    text = path.read_text(encoding="utf-8", errors="strict")
    metadata_rows = re.findall(r"(?m)^(profile|run|macro|feature|source|testbench)=(.*)$", text)
    metadata = dict(metadata_rows)
    config = PROFILES[profile]
    macro = config["macro"]
    expected = {"profile": profile, "run": run, "macro": str(macro),
                "feature": str(config["feature"]),
                "source": str(inputs["source"]), "testbench": str(inputs["testbench"])}
    need(len(metadata_rows) == len(expected) and metadata == expected,
         f"{run_dir}: provenance identity mismatch")
    for key, file_path in inputs.items():
        checksum = sha256(file_path)
        need(re.search(rf"(?m)^{checksum}\s+{re.escape(str(file_path))}$", text) is not None,
             f"{run_dir}: provenance hash missing/mismatched for {key}")
    argv_match = re.search(r"(?m)^argv=(.*)$", text)
    need(argv_match is not None, f"{run_dir}: invocation argv missing from provenance")
    argv = shlex.split(argv_match.group(1))
    need(len(argv) >= 2 and argv[0] == str(inputs["bambu"]) and argv[1] == str(inputs["source"]),
         f"{run_dir}: invocation executable/source mismatch")
    feature_args = [arg for arg in argv if arg.startswith("--bambu-parameter=experimental-m-axi-burst=")]
    need(feature_args == [f"--bambu-parameter=experimental-m-axi-burst={config['feature']}"],
         f"{run_dir}: expected exactly one experimental-m-axi-burst={config['feature']} option")
    macro_args = [arg for arg in argv if arg.startswith("--extra-cc-options=-DCOPY_COMPARISON_PROFILE=")]
    need(macro_args == [f"--extra-cc-options=-DCOPY_COMPARISON_PROFILE={macro}"],
         f"{run_dir}: invocation does not select exactly profile {profile}")
    common = [arg for arg in argv[2:] if arg not in feature_args and arg not in macro_args]
    expected_common = [*COMMON_ARGS[:-2], f"--generate-tb={inputs['testbench']}", *COMMON_ARGS[-2:]]
    need(common == expected_common,
         f"{run_dir}: invocation common settings differ from the controlled comparison")


def parse_cycles(xml_path):
    need(not xml_path.is_symlink() and xml_path.is_file(), f"{xml_path}: results XML missing")
    root = ET.parse(xml_path).getroot()
    values = [node.get("CYCLES", "") for node in root.iter("evaluation") if "CYCLES" in node.attrib]
    need(len(values) == 1 and INTEGER.fullmatch(values[0]) and int(values[0]) > 0,
         f"{xml_path}: expected exactly one positive integer CYCLES value")
    return int(values[0])


def check_define(path, macro, expected):
    text = path.read_text(encoding="utf-8", errors="strict")
    definitions = re.findall(rf"(?m)^\s*#\s*define\s+{re.escape(macro)}\b(.*)$", text)
    need(len(definitions) == 1, f"{path}: expected exactly one {macro} definition")
    value = definitions[0].split("//", 1)[0].split("/*", 1)[0].strip()
    need(INTEGER.fullmatch(value) and int(value) == expected,
         f"{path}: {macro} must be the integer {expected}")


def check_copy_dimensions(inputs):
    check_define(inputs["source"], "COPY_COMPARISON_MAXN", COPY_MAXN)
    check_define(inputs["testbench"], "COPY_COMPARISON_MAXN", COPY_MAXN)
    check_define(inputs["testbench"], "COPY_COMPARISON_N", COPY_N)


def check_architecture(run_dir, profile):
    path = run_dir / "panda-temp" / "architecture.xml"
    need(not path.is_symlink() and path.is_file(), f"{run_dir}: architecture.xml missing")
    root = ET.parse(path).getroot()
    functions = [node for node in root.iter("function") if node.get("name") == "kernel"]
    need(len(functions) == 1, f"{run_dir}: expected exactly one kernel architecture function")
    function = functions[0]
    params = [node for node in function.findall("./parameters/parameter")
              if node.get("port") in {"src", "dst"}]
    need(len(params) == 2 and {node.get("port") for node in params} == {"src", "dst"},
         f"{run_dir}: architecture.xml must map src and dst exactly once")
    config = PROFILES[profile]
    owners = {node.get("port"): node.get("bundle") for node in params}
    need(owners == {"src": config["read_bundle"], "dst": config["write_bundle"]},
         f"{run_dir}: architecture.xml src/dst bundle mapping is incorrect")
    m_axi_bundles = [node for node in function.findall("./bundles/bundle") if node.get("mode") == "m_axi"]
    expected_bundle_names = {config["read_bundle"], config["write_bundle"]}
    need({node.get("name") for node in m_axi_bundles} == expected_bundle_names and
         len(m_axi_bundles) == len(expected_bundle_names),
         f"{run_dir}: architecture.xml m_axi bundle inventory is incorrect")
    need(all(node.get("latency") == "0" for node in m_axi_bundles),
         f"{run_dir}: every m_axi bundle must explicitly have latency=0")
    bundles = {node.get("name"): node for node in m_axi_bundles}
    for direction, port in (("read", "src"), ("write", "dst")):
        bundle_name = owners[port]
        bundle = bundles.get(bundle_name)
        need(bundle is not None,
             f"{run_dir}: {port} m_axi bundle {bundle_name} missing")
        requested = config[f"{direction}_requested"]
        for attr, value in zip(PARAM_ATTRS[direction], requested):
            need(bundle.get(attr) == str(value),
                 f"{run_dir}: {port} bundle {bundle_name} must explicitly request {attr}={value}")


def check_legacy_topology(rtl, run_dir, profile, expected):
    resources = [instance for module in LEGACY_MODULES
                 for instance in module_instance_details(rtl, module)]
    resource_count = 2 if profile == "legacy-two-bundles" else 1
    need(len(resources) == resource_count,
         f"{run_dir}: {profile} must instantiate exactly {resource_count} legacy resources")
    read_bundle, write_bundle = expected["read_bundle"], expected["write_bundle"]
    read_signal = f"p_m_axi_{read_bundle}_araddr"
    write_signal = f"p_m_axi_{write_bundle}_awaddr"
    mappings = {(ports.get("m_axi_araddr"), ports.get("m_axi_awaddr"))
                for _, ports in resources}
    expected_mappings = {(read_signal, f"p_m_axi_{read_bundle}_awaddr")}
    if profile == "legacy-two-bundles":
        expected_mappings.add((f"p_m_axi_{write_bundle}_araddr", write_signal))
    need(mappings == expected_mappings,
         f"{run_dir}: {profile} legacy resources have incorrect read/write bundle wiring")


def check_rtl(run_dir, profile):
    rtl_paths = [p for p in run_dir.rglob("*") if p.is_file() and p.suffix.lower() in {".v", ".sv"}]
    need(bool(rtl_paths), f"{run_dir}: generated RTL missing")
    rtl = "\n".join(p.read_text(encoding="utf-8", errors="replace") for p in rtl_paths)
    need(re.search(r"(?m)^\s*module\s+kernel\s*\(", rtl), f"{run_dir}: generated kernel top missing")
    expected = PROFILES[profile]
    burst_profile = profile in {"burst-two-bundles", "duplex-shared"}
    if burst_profile:
        read = list(module_instances(rtl, READ_CORE))
        write = list(module_instances(rtl, WRITE_CORE))
        read_details = list(module_instance_details(rtl, READ_CORE))
        write_details = list(module_instance_details(rtl, WRITE_CORE))
        need(len(read) == 1 and len(write) == 1,
             f"{run_dir}: expected exactly one read and one write burst engine")
        read_params, write_params = read[0], write[0]
        read_need = {"B_MAX": str(expected["read_engine"][0]),
                     "MAX_OUTSTANDING": str(expected["read_engine"][1]),
                     "FIFO_DEPTH": str(expected["read_engine"][2]), "BITSIZE_m_axi_araddr": "32"}
        write_need = {"B_MAX": str(expected["write_engine"][0]),
                      "MAX_OUTSTANDING": str(expected["write_engine"][1]),
                      "FIFO_DEPTH": str(expected["write_engine"][2]), "BITSIZE_m_axi_awaddr": "32"}
        need(all(read_params.get(k) == v for k, v in read_need.items()),
             f"{run_dir}: read engine profile/width parameters are incorrect or split across instances")
        need(all(write_params.get(k) == v for k, v in write_need.items()),
             f"{run_dir}: write engine profile/width parameters are incorrect or split across instances")
        read_ports = read_details[0][1]
        write_ports = write_details[0][1]
        read_address_signal = f"p_m_axi_{expected['read_bundle']}_araddr"
        write_address_signal = f"p_m_axi_{expected['write_bundle']}_awaddr"
        read_address = read_ports.get("m_axi_araddr", "")
        write_address = write_ports.get("m_axi_awaddr", "")
        need(bool(re.fullmatch(r"[A-Za-z_$][\w$]*", read_address)) and
             (read_address == read_address_signal or re.search(
            rf"\bassign\s+{re.escape(read_address_signal)}\s*=\s*{re.escape(read_address)}\s*;", rtl),
             ),
             f"{run_dir}: read engine address is not wired to expected bundle {expected['read_bundle']}")
        need(bool(re.fullmatch(r"[A-Za-z_$][\w$]*", write_address)) and
             (write_address == write_address_signal or re.search(
            rf"\bassign\s+{re.escape(write_address_signal)}\s*=\s*{re.escape(write_address)}\s*;", rtl),
             ),
             f"{run_dir}: write engine address is not wired to expected bundle {expected['write_bundle']}")
        for port in ("m_axi_arready", "m_axi_rdata"):
            need(read_ports.get(port) == f"p_m_axi_{expected['read_bundle']}_{port.removeprefix('m_axi_')}",
                 f"{run_dir}: read engine {port} is not wired to expected bundle {expected['read_bundle']}")
        for port in ("m_axi_awready", "m_axi_wready", "m_axi_bresp"):
            need(write_ports.get(port) == f"p_m_axi_{expected['write_bundle']}_{port.removeprefix('m_axi_')}",
                 f"{run_dir}: write engine {port} is not wired to expected bundle {expected['write_bundle']}")
        need(not LEGACY_CORE.search(rtl), f"{run_dir}: burst profile contains legacy memory resources")
    else:
        need(not list(module_instances(rtl, READ_CORE)) and not list(module_instances(rtl, WRITE_CORE)),
             f"{run_dir}: legacy profile emits burst engine")
        check_legacy_topology(rtl, run_dir, profile, expected)
        need(not re.search(r"\b(?:configure_read|configure_write|burst_read|store_write)\b", rtl, re.I),
             f"{run_dir}: legacy profile contains burst operation resources")
    for bundle, direction in ((expected["read_bundle"], "ar"), (expected["write_bundle"], "aw")):
        signal = f"p_m_axi_{bundle}_{direction}addr"
        need(re.search(rf"\[[^\]]+\]\s+{re.escape(signal)}\b", rtl),
             f"{run_dir}: expected 32-bit {direction.upper()} address owner/bundle {bundle} missing")


def check_run(run_dir, profile, run, inputs):
    need(run_dir.is_dir() and not run_dir.is_symlink(), f"required run missing: {run_dir}")
    check_provenance(run_dir, profile, run, inputs)
    check_manifest(run_dir)
    check_architecture(run_dir, profile)
    cycles = parse_cycles(run_dir / "bambu_results.xml")
    sim_log = run_dir / "HLS_output" / "simulation" / "testbench.log"
    need(not sim_log.is_symlink() and sim_log.is_file(), f"{run_dir}: exact simulation testbench.log missing")
    marker = f"COPY_COMPARISON_GOLDEN_PASS PROFILE={profile} N={COPY_N}"
    need(sum(line.strip() == marker for line in sim_log.read_text(encoding="utf-8", errors="strict").splitlines()) == 1,
         f"{run_dir}: exact golden marker absent/duplicated in simulation log")
    check_rtl(run_dir, profile)
    stdout = run_dir / "stdout.log"
    need(stdout.is_file() and stdout.stat().st_size > 0, f"{run_dir}: compiler stdout missing/empty")
    stderr = run_dir / "stderr.log"
    need(stderr.is_file() and stderr.stat().st_size > 0, f"{run_dir}: compiler stderr missing/empty")
    log = stderr.read_text(encoding="utf-8", errors="replace")
    if PROFILES[profile]["feature"] == 0:
        stdout_text = stdout.read_text(encoding="utf-8", errors="replace")
        for direction, bundle in (("read", PROFILES[profile]["read_bundle"]),
                                  ("write", PROFILES[profile]["write_bundle"])):
            diagnostic = (f"Keeping legacy m_axi {direction}s for bundle {bundle}: explicit "
                          f"{direction}-burst request ignored because experimental-m-axi-burst=0")
            matches = re.findall(rf"(?m)^\s*{re.escape(diagnostic)}\s*$", log)
            need(len(matches) == 1,
                 f"{run_dir}: expected exactly one feature-off {direction} diagnostic for bundle {bundle}")
            need(diagnostic not in stdout_text,
                 f"{run_dir}: feature-off {direction} diagnostic must be emitted on stderr")
    if profile == "duplex-shared":
        need(re.search(r"(?m)^[ \t]*Activated (?:same-base pointwise|distinct-base restricted pointwise) "
                       r"duplex m_axi bundle shared for root kernel[ \t]*$", log),
             f"{run_dir}: duplex activation witness absent")
    return cycles


def verify(output, inputs, report):
    output = output.resolve()
    need(output.is_dir() and not output.is_symlink(), f"output directory missing/invalid: {output}")
    required = {f"{profile}/{run}" for profile in PROFILES for run in RUNS}
    found = {p.relative_to(output).as_posix() for p in output.glob("*/*") if p.is_dir()}
    need(found == required, "output inventory must contain exactly four profiles x run1/run2")
    check_copy_dimensions(inputs)
    results = {}
    for profile in PROFILES:
        results[profile] = {}
        for run in RUNS:
            results[profile][run] = check_run(output / profile / run, profile, run, inputs)
        need(results[profile]["run1"] == results[profile]["run2"],
             f"{profile}: run1/run2 CYCLES are not reproducible "
             f"({results[profile]['run1']} vs {results[profile]['run2']})")
    duplex = results["duplex-shared"]
    comparisons = {}
    for profile, runs in results.items():
        comparisons[profile] = {}
        for run, cycles in runs.items():
            comparisons[profile][run] = {"cycles": cycles,
                                         "ratio_to_duplex_same_run": cycles / duplex[run]}
    report_data = {
        "n": COPY_N,
        "maxn": COPY_MAXN,
        "profiles": {
            profile: {
                "macro": values["macro"], "feature_enabled": bool(values["feature"]),
                "read_bundle": values["read_bundle"], "write_bundle": values["write_bundle"],
                "requested_parameters": {"read": dict(zip(("burst", "outstanding", "fifo"),
                                                              values["read_requested"])),
                                          "write": dict(zip(("burst", "outstanding", "fifo"),
                                                               values["write_requested"]))},
                "actual_engine_parameters": {
                    "read": None if values["read_engine"] is None else
                    dict(zip(("burst", "outstanding", "fifo"), values["read_engine"])),
                    "write": None if values["write_engine"] is None else
                    dict(zip(("burst", "outstanding", "fifo"), values["write_engine"]))},
            }
            for profile, values in PROFILES.items()
        },
        "measurements": comparisons,
        "interpretation": {
            "fixed_cycle_expectations": False,
            "minimum_speedup_threshold": None,
            "equal_cycles_allowed": True,
            "confounds": [
                "Transport topology differs between two-bundle and shared-bundle profiles.",
                "Legacy and burst profiles may receive different schedules and initiation intervals.",
                "The comparison measures end-to-end generated design cycles, not an isolated transport effect.",
                "Distinct-base __restrict is a caller contract and is not proof of non-aliasing."
            ]
        }
    }
    report.parent.mkdir(parents=True, exist_ok=True)
    report.write_text(json.dumps(report_data, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    return report_data


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    for name in ("source", "testbench", "bambu", "settings", "plugin", "runner", "report"):
        parser.add_argument(f"--{name}", required=True, type=Path)
    args = parser.parse_args(argv)
    try:
        inputs = {name: getattr(args, name).resolve() for name in
                  ("source", "testbench", "bambu", "settings", "plugin", "runner")}
        for name, path in inputs.items():
            need(path.is_file() and not path.is_symlink(), f"{name} path missing/invalid: {path}")
        read_inventory()
        verify(args.output, inputs, args.report)
    except (OSError, ValueError, ET.ParseError, UnicodeError) as exc:
        print(f"COPY COMPARISON CHECK FAILED: {exc}", file=sys.stderr)
        return 1
    print(f"COPY_COMPARISON_GATE_PASS: 4 profiles x 2 runs; report={args.report}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
