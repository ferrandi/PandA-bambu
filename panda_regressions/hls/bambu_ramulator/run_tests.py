#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
"""Smoke and intermediate HBM Ramulator suite; install the dependency once."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import random
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
SMOKE_CASES = [f"{kernel}/{mode}" for kernel in KERNELS for mode in MODES]
HBM_SIZES = {"hbm-gather": 4096, "hbm-scatter": 4096, "hbm-chase": 2048}
HBM_CASES = [f"{kernel}/{mode}" for kernel in HBM_SIZES
             for mode in ("banked-minimal", "banked-axi")]
CASES = SMOKE_CASES + HBM_CASES
HBM_SEED = 0x504E4E4C


def hbm_input(path, size):
    """Seeded permutation gives reproducible random access and race-free scatter."""
    rng = random.Random(HBM_SEED)
    indices = list(range(size))
    rng.shuffle(indices)
    root = ET.Element("function")
    arrays = {"a": [rng.randrange(-1000, 1001) for _ in range(size)],
              "b": [rng.randrange(-1000, 1001) for _ in range(size)],
              "c": [rng.randrange(-100, 101) for _ in range(size)], "indices": indices}
    ET.SubElement(root, "testbench", **{
        name: "{" + ",".join(map(str, values)) + "}" for name, values in arrays.items()})
    ET.ElementTree(root).write(path, encoding="utf-8", xml_declaration=True)


def parallelism(path, banks=4, minimum_parallel=2, minimum_banks=2):
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
                    match = re.fullmatch(r"sig_in_Mout_(oe|we)_ram_(\d+)", name)
                    if match:
                        ids[f"{match[1]}{match[2]}"] = words[3]
                elif scope.endswith(".system.DUT"):
                    match = re.fullmatch(r"m_axi_(\d+)_ar(valid|ready)", name)
                    if match:
                        ids[f"ar{match[2]}{match[1]}"] = words[3]
                    if name in ("Mout_oe_ram", "Mout_we_ram"):
                        ids[name] = words[3]
            elif words[:1] == ["$enddefinitions"]:
                break
        if "clock" not in ids or not all(f"oe{i}" in ids for i in range(banks)):
            raise ValueError(f"VCD does not expose the expected {banks}-channel banked circuit")
        previous_clock, peak, edges = "0", 0, 0
        active_banks = set()

        def sample():
            nonlocal previous_clock, peak, edges
            def bit(name):
                return values.get(ids.get(name), "0") == "1"
            clock = values.get(ids["clock"], "0")
            if clock == "1" and previous_clock == "0":
                edges += 1
                peak = max(peak, sum(bit(f"oe{i}") or bit(f"we{i}") for i in range(banks)))
                for i in range(banks):
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
    if peak < minimum_parallel or len(active_banks) < minimum_banks:
        raise ValueError(f"insufficient parallel banked memory traffic: {result}")
    return result


def validate(directory, expected_axi, banked, cycle_limit, banks=4, hbm=False):
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
        evidence["parallelism"] = parallelism(directory / "HLS_output/simulation/test.vcd", banks,
                                               8 if hbm else 2, banks if hbm else 2)
    return evidence


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--list", action="store_true")
    parser.add_argument("--case", action="append", choices=CASES)
    parser.add_argument("--group", choices=("all", "smoke", "hbm"), default="all")
    parser.add_argument("--bambu", type=Path)
    parser.add_argument("--out", type=Path, default=ROOT / "out_ramulator_ci")
    parser.add_argument("--timeout", type=int, default=180, help="wall seconds per case, including compilation")
    parser.add_argument("--max-cycles", type=int, default=5000)
    parser.add_argument("--hbm-max-cycles", type=int, default=12000)
    parser.add_argument("--hbm-timeout", type=int, default=600)
    args = parser.parse_args()
    if args.list:
        print("\n".join(SMOKE_CASES if args.group == "smoke" else HBM_CASES if args.group == "hbm" else CASES))
        return 0
    if not args.bambu or not os.environ.get("BAMBU_HLS"):
        parser.error("source the Bambu installation settings.sh and provide --bambu")
    if min(args.timeout, args.max_cycles, args.hbm_max_cycles, args.hbm_timeout) <= 0:
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
    selected = args.case or (SMOKE_CASES if args.group == "smoke" else HBM_CASES if args.group == "hbm" else CASES)
    for name in selected:
        kernel, mode = name.split("/")
        banked, axi, dedicated, axi_models = MODES[mode]
        hbm = kernel in HBM_SIZES
        banks = 32 if hbm else 4 if banked else 1
        cycles_limit = args.hbm_max_cycles if hbm else args.max_cycles
        timeout = args.hbm_timeout if hbm else args.timeout
        if hbm and axi:
            axi_models = banks
        directory = out / name
        directory.mkdir(parents=True)
        source, inputs = HERE / "kernel.cpp", HERE / "input.xml"
        case_env = env.copy()
        if hbm:
            source, inputs = HERE / f"{kernel.replace('-', '_')}.cpp", directory / "input.xml"
            hbm_input(inputs, HBM_SIZES[kernel])
            case_env["RAMULATOR2_CONFIG"] = str(HERE / "hbm2.yaml")
            case_env["RAMULATOR_OVERRIDE_CHANNELS"] = "0"
        command = [str(bambu), str(source), f"--generate-tb={inputs}",
                   "--top-fname=ramulator_kernel", "--compiler=I386_CLANG13", "--simulator=VERILATOR",
                   "--simulate", "--generate-interface=INFER", "--enable-ramulator",
                   "--memory-allocation-policy=NO_BRAM", f"--clock-period={5 if hbm else 10}", "-m64", "-lm", "-v4",
                   f"--max-sim-cycles={cycles_limit}", f"-DTEST_KERNEL={KERNELS.get(kernel, 0)}",
                   f"-DBANKED={banked}", f"-DUSE_AXI={axi}", f"-DDEDICATED_AXI={dedicated}",
                   f"--channels-type={'MEM_ACC_NN' if banked else 'MEM_ACC_11'}",
                   f"--channels-number={banks}"]
        if hbm:
            command.append(f"-DHBM_SIZE={HBM_SIZES[kernel]}")
        if banked or axi:
            command.append("--bus-pipelined")
        if banked:
            command.extend(["-fopenmp", "--generate-vcd"])
        (directory / "command.txt").write_text(shlex.join(command) + "\n")
        start = time.monotonic()
        record = {"name": name, "status": "PASS", "banks": banks, "cycle_limit": cycles_limit,
                  "ramulator_config": case_env["RAMULATOR2_CONFIG"], "source": source.name,
                  "input_sha256": hashlib.sha256(inputs.read_bytes()).hexdigest()}
        if hbm:
            record.update(elements=HBM_SIZES[kernel], seed=HBM_SEED, threads=32)
        try:
            with (directory / "run.log").open("w") as log:
                with subprocess.Popen(command, cwd=directory, env=case_env, stdout=log,
                                      stderr=subprocess.STDOUT, start_new_session=True) as process:
                    try:
                        returncode = process.wait(timeout=timeout)
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
            record.update(validate(directory, axi_models, banked, cycles_limit, banks, hbm))
            if banked:
                # Successful cases keep measured evidence; retain full waveforms only
                # on failure, especially for the larger 32-thread circuits.
                (directory / "HLS_output/simulation/test.vcd").unlink()
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
