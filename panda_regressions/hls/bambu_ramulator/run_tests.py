#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
"""Small end-to-end Ramulator suite; install the dependency once before running."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shlex
import signal
import subprocess
import time
import xml.etree.ElementTree as ET

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
KERNELS = {"add": 0, "update": 1, "conditional": 2}
MODES = {
    "minimal-bus": (0, 0, 0, 0),
    "axi-bus": (0, 1, 0, 1),
    "banked-minimal": (1, 0, 0, 0),
    "banked-axi": (1, 1, 0, 4),
    "axi-dedicated": (0, 1, 1, 3),
}
CASES = [f"{kernel}/{mode}" for kernel in KERNELS for mode in MODES]


def parallelism(path):
    """Sample settled VCD values on rising edges, counting independent requests."""
    scopes, ids, values = [], {}, {}
    with path.open() as stream:
        for line in stream:
            words = line.split()
            if words[:1] == ["$scope"]:
                scopes.append(words[2])
            elif words[:1] == ["$upscope"]:
                scopes.pop()
            elif words[:1] == ["$var"]:
                scope, name = ".".join(scopes), words[4]
                if scope.endswith(".system") and name == "clock":
                    ids["clock"] = words[3]
                elif scope.endswith(".DUT.top"):
                    match = re.fullmatch(r"sig_in_Mout_(oe|we)_ram_([0-3])", name)
                    if match:
                        ids[f"{match[1]}{match[2]}"] = words[3]
                elif scope.endswith(".system.DUT"):
                    match = re.fullmatch(r"m_axi_([0-3])_ar(valid|ready)", name)
                    if match:
                        ids[f"ar{match[2]}{match[1]}"] = words[3]
                    if name in ("Mout_oe_ram", "Mout_we_ram"):
                        ids[name] = words[3]
            elif words[:1] == ["$enddefinitions"]:
                break
        if "clock" not in ids or not all(f"oe{i}" in ids for i in range(4)):
            raise ValueError("VCD does not expose the expected four-channel banked circuit")
        previous_clock, peak, edges = "0", 0, 0
        active_banks = set()

        def sample():
            nonlocal previous_clock, peak, edges
            def bit(name):
                return values.get(ids.get(name), "0") == "1"
            clock = values.get(ids["clock"], "0")
            if clock == "1" and previous_clock == "0":
                edges += 1
                peak = max(peak, sum(bit(f"oe{i}") or bit(f"we{i}") for i in range(4)))
                for i in range(4):
                    if bit(f"arvalid{i}") and bit(f"arready{i}"):
                        active_banks.add(i)
                for name in ("Mout_oe_ram", "Mout_we_ram"):
                    bits = values.get(ids.get(name), "0")
                    active_banks.update(i for i, value in enumerate(reversed(bits)) if value == "1")
            previous_clock = clock

        selected = set(ids.values())
        for line in stream:
            if line.startswith("#"):
                sample()
            elif line[:1] in ("0", "1", "x", "z") and line[1:].strip() in selected:
                values[line[1:].strip()] = line[0]
            elif line.startswith("b"):
                value, key = line[1:].split()
                if key in selected:
                    values[key] = value
        sample()
    result = {"peak_parallel_requests": peak, "active_banks": sorted(active_banks), "clock_edges": edges}
    if peak < 2 or len(active_banks) < 2:
        raise ValueError(f"insufficient parallel banked memory traffic: {result}")
    return result


def validate(directory, expected_axi, banked, cycle_limit):
    result = ET.parse(directory / "bambu_results.xml").getroot()
    simulation = result.find("timing/simulation")
    if simulation is None or simulation.get("return_value") != "0":
        raise ValueError("RTL/golden co-simulation did not return success")
    cycles = int(result.find("evaluation").get("CYCLES"))
    if not 0 < cycles <= cycle_limit:
        raise ValueError(f"simulation used {cycles} cycles (limit {cycle_limit})")
    log = (directory / "run.log").read_text(errors="replace")
    if "Ramulator2 successfully initialized" not in log:
        raise ValueError("Ramulator initialization was not confirmed")
    completions = re.findall(r"\[MDPI-PROFILE\].*OP=READ,.*LATENCY=(\d+)", log)
    if len(completions) < 20 or not any(int(value) > 0 for value in completions):
        raise ValueError("missing actual Ramulator read completions and positive latency")
    if "Ramulator2 initialization failed" in log:
        raise ValueError("Ramulator initialization failure in driver log")
    tb = (directory / "HLS_output/simulation/bambu_testbench.v").read_text()
    axi_count = len(re.findall(r"\bTestbenchMEMAXIRamulatorIntegrated\s*#", tb))
    if axi_count != expected_axi:
        raise ValueError(f"expected {expected_axi} AXI memory models, generated {axi_count}")
    memory = "TestbenchMEMRamulatorIntegratedNoC" if banked else "TestbenchMEMRamulatorIntegrated"
    if not re.search(rf"\b{memory}\s*#", tb):
        raise ValueError(f"missing expected {memory} model")
    evidence = {"cycles": cycles, "ramulator_read_completions": len(completions), "axi_models": axi_count}
    if banked:
        evidence["parallelism"] = parallelism(directory / "HLS_output/simulation/test.vcd")
    return evidence


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--list", action="store_true")
    parser.add_argument("--case", action="append", choices=CASES)
    parser.add_argument("--bambu", type=Path)
    parser.add_argument("--out", type=Path, default=ROOT / "out_ramulator_ci")
    parser.add_argument("--timeout", type=int, default=180, help="wall seconds per case, including compilation")
    parser.add_argument("--max-cycles", type=int, default=5000)
    args = parser.parse_args()
    if args.list:
        print("\n".join(CASES))
        return 0
    if not args.bambu or not os.environ.get("BAMBU_HLS"):
        parser.error("source the Bambu installation settings.sh and provide --bambu")
    if args.timeout <= 0 or args.max_cycles <= 0:
        parser.error("timeout and max-cycles must be positive")
    if not os.environ.get("RAMULATOR2_DIR"):
        parser.error("RAMULATOR2_DIR must identify the once-prepared installation")
    out, bambu = args.out.resolve(), args.bambu.resolve()
    if not bambu.is_file() or not os.access(bambu, os.X_OK):
        parser.error(f"Bambu executable is missing or not executable: {bambu}")
    prefix = Path(os.environ["RAMULATOR2_DIR"]).resolve()
    if not (prefix / "include/ramulator/base/config.h").is_file():
        parser.error(f"not a standard Ramulator installation prefix: {prefix}")
    if out.exists():
        parser.error(f"output directory must be fresh: {out}")
    out.mkdir(parents=True)
    env = os.environ.copy()
    env["RAMULATOR2_DIR"] = str(prefix)
    env["RAMULATOR2_CONFIG"] = str(ROOT / "etc/libmdpi/configs/default_ddr4.yaml")
    env["RAMULATOR_OVERRIDE_CHANNELS"] = "1"
    env.pop("LD_LIBRARY_PATH", None)
    records = []
    suite = ET.Element("testsuite", name="bambu-ramulator")
    for name in args.case or CASES:
        kernel, mode = name.split("/")
        banked, axi, dedicated, axi_models = MODES[mode]
        directory = out / name
        directory.mkdir(parents=True)
        command = [str(bambu), str(HERE / "kernel.cpp"), f"--generate-tb={HERE / 'input.xml'}",
                   "--top-fname=ramulator_kernel", "--compiler=I386_CLANG13", "--simulator=VERILATOR",
                   "--simulate", "--generate-interface=INFER", "--enable-ramulator-integrated",
                   "--memory-allocation-policy=NO_BRAM", "--clock-period=10", "-m64", "-lm", "-v4",
                   f"--max-sim-cycles={args.max_cycles}", f"-DTEST_KERNEL={KERNELS[kernel]}",
                   f"-DBANKED={banked}", f"-DUSE_AXI={axi}", f"-DDEDICATED_AXI={dedicated}",
                   f"--channels-type={'MEM_ACC_NN' if banked else 'MEM_ACC_11'}",
                   f"--channels-number={4 if banked else 1}"]
        if banked or axi:
            command.append("--bus-pipelined")
        if banked:
            command.extend(["-fopenmp", "--generate-vcd"])
        (directory / "command.txt").write_text(shlex.join(command) + "\n")
        start = time.monotonic()
        record = {"name": name, "status": "PASS"}
        try:
            with (directory / "run.log").open("w") as log:
                with subprocess.Popen(command, cwd=directory, env=env, stdout=log,
                                      stderr=subprocess.STDOUT, start_new_session=True) as process:
                    try:
                        returncode = process.wait(timeout=args.timeout)
                    except subprocess.TimeoutExpired:
                        # Stop compiler/simulator descendants as well as Bambu.
                        try:
                            os.killpg(process.pid, signal.SIGKILL)
                        except ProcessLookupError:
                            pass
                        process.wait()
                        raise
            if returncode:
                raise ValueError(f"Bambu exited with {returncode}; see {directory / 'run.log'}")
            record.update(validate(directory, axi_models, banked, args.max_cycles))
        except (ValueError, OSError, ET.ParseError, AttributeError, subprocess.TimeoutExpired) as error:
            record.update(status="FAIL", error=str(error))
        record["seconds"] = round(time.monotonic() - start, 3)
        (directory / "evidence.json").write_text(json.dumps(record, indent=2) + "\n")
        records.append(record)
        case = ET.SubElement(suite, "testcase", name=mode, classname=kernel, time=str(record["seconds"]))
        if record["status"] == "FAIL":
            ET.SubElement(case, "failure", message=record["error"]).text = record["error"]
        print(f"{record['status']:4} {name}: {record.get('error', str(record.get('cycles')) + ' cycles')}", flush=True)
    failures = sum(record["status"] != "PASS" for record in records)
    suite.set("tests", str(len(records)))
    suite.set("failures", str(failures))
    ET.ElementTree(suite).write(out / "junit.xml", encoding="utf-8", xml_declaration=True)
    provenance = {"bambu_sha256": hashlib.sha256(bambu.read_bytes()).hexdigest(),
                  "ramulator_prefix": env["RAMULATOR2_DIR"], "tests": records}
    (out / "summary.json").write_text(json.dumps(provenance, indent=2) + "\n")
    rows = ["| Test | Result | Cycles |", "|---|---|---:|"]
    rows += [f"| {r['name']} | {r['status']} | {r.get('cycles', '—')} |" for r in records]
    (out / "summary.md").write_text("\n".join(rows) + "\n")
    return int(bool(failures))


if __name__ == "__main__":
    raise SystemExit(main())
