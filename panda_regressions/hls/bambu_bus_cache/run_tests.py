#!/usr/bin/env python3
# Copyright (C) 2026 Politecnico di Milano
# SPDX-License-Identifier: Apache-2.0
"""Sequential bus/cache regressions using the fixed-latency AXI testbench."""

import argparse
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
import re
import shlex
import shutil
import subprocess
import time
import xml.etree.ElementTree as ET

HERE = Path(__file__).resolve().parent
DIAGNOSTIC = "Non-banked bus caches do not support MEM_ACC_NN"


def cases():
    for pipelined in (False, True):
        suffix = "pipelined" if pipelined else "ordinary"
        for channels_type, channels in (("MEM_ACC_11", 1), ("MEM_ACC_N1", 2)):
            yield f"cache-{channels_type}-{suffix}", True, False, channels_type, channels, pipelined, False
        yield f"banked-cache-{suffix}", True, True, "MEM_ACC_NN", 2, pipelined, False
        # Parameter checking normalizes any one-channel mode to MEM_ACC_11.
        yield f"cache-normalized-NN1-{suffix}", True, False, "MEM_ACC_NN", 1, pipelined, False
        yield f"reject-incoherent-cache-{suffix}", True, False, "MEM_ACC_NN", 2, pipelined, True
    for channels_type in ("MEM_ACC_N1", "MEM_ACC_NN"):
        yield f"axi-{channels_type}", False, False, channels_type, 2, True, False


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--bambu", default=shutil.which("bambu"))
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--junit", type=Path, required=True)
    parser.add_argument("--jobs", type=int, default=1)
    args = parser.parse_args()
    if not args.bambu or args.jobs < 1:
        parser.error("bambu must be installed and --jobs must be positive")
    bambu = str(Path(args.bambu).resolve())
    out = args.out.resolve()
    out.mkdir(parents=True, exist_ok=True)

    tasks = [(case, None, not case[1]) for case in cases()]
    for name, cache, axi, pipelined in (
            ("full-banked-cache-ordinary", True, False, False),
            ("full-banked-cache-pipelined", True, False, True),
            ("full-banked-minimal", False, False, True),
            ("full-banked-axi", False, True, True)):
        tasks.append(((name, cache, True, "MEM_ACC_NN", 2, pipelined, False), "FULL", axi))
    tasks.append((("resized-banked-cache", True, True, "MEM_ACC_NN", 2, False, False), "RESIZED", False))

    def run(task):
        case, address_mode, axi = task
        name, cache, banked, channels_type, channels, pipelined, rejected = case
        directory = out / name
        directory.mkdir(exist_ok=False)
        command = [bambu, str(HERE / "kernel.cpp"),
                   f"--generate-tb={HERE / 'input.xml'}", "--top-fname=bus_cache_kernel",
                   "--compiler=I386_CLANG13", "--simulator=VERILATOR", "--simulate",
                   "--generate-interface=INFER", "--memory-allocation-policy=NO_BRAM",
                   "--clock-period=10", "--max-sim-cycles=5000", "-m64", "-lm",
                   f"-DUSE_CACHE={int(cache)}", f"-DBANKED={int(banked)}", f"-DUSE_AXI={int(axi)}",
                   f"--channels-type={channels_type}", f"--channels-number={channels}"]
        if address_mode:
            command.append(f"--banked-memory-address-size={address_mode}")
        if pipelined:
            command.append("--bus-pipelined")
        (directory / "command.txt").write_text(shlex.join(command) + "\n")
        start = time.monotonic()
        error = None
        try:
            with (directory / "run.log").open("w") as log:
                result = subprocess.run(command, cwd=directory, stdout=log,
                                        stderr=subprocess.STDOUT, timeout=180)
            output = (directory / "run.log").read_text(errors="replace")
            if rejected:
                if result.returncode == 0 or DIAGNOSTIC not in output:
                    error = "Expected rejection for missing cache coherence"
            elif result.returncode != 0:
                error = f"Co-simulation failed with exit code {result.returncode}"
            elif address_mode:
                testbench = (directory / "HLS_output/simulation/bambu_testbench.v").read_text()
                has_adapters = "TBBankAd" in testbench
                if has_adapters != (address_mode == "RESIZED"):
                    error = "Unexpected presence or absence of testbench bank adapters"
                rtl = (directory / "bus_cache_kernel.v").read_text()
                widths = {int(w) for w in re.findall(r"\.BITSIZE_Mout_addr_ram_out\((\d+)\)", rtl)}
                expected_width = 64 if address_mode == "FULL" else 62  # Four banks, 64-bit accelerator.
                if widths != {expected_width}:
                    error = f"Unexpected bank address widths: {widths}, expected {expected_width}"
        except subprocess.TimeoutExpired:
            error = "Bambu exceeded the 180-second timeout"
        elapsed = time.monotonic() - start
        print(f"{'FAIL' if error else 'PASS'} {name}" + (f": {error}" if error else ""), flush=True)
        return name, elapsed, error, directory

    with ThreadPoolExecutor(max_workers=args.jobs) as pool:
        results = list(pool.map(run, tasks))
    failures = sum(error is not None for _, _, error, _ in results)
    suite = ET.Element("testsuite", name="bambu-bus-cache", tests=str(len(results)),
                       failures=str(failures), time=f"{sum(r[1] for r in results):.3f}")
    for name, elapsed, error, directory in results:
        case = ET.SubElement(suite, "testcase", name=name, classname="bambu.bus_cache", time=f"{elapsed:.3f}")
        if error:
            log = (directory / "run.log").read_text(errors="replace")
            ET.SubElement(case, "failure", message=error).text = log[-6000:]
    args.junit.parent.mkdir(parents=True, exist_ok=True)
    ET.ElementTree(suite).write(args.junit, encoding="utf-8", xml_declaration=True)
    return bool(failures)


if __name__ == "__main__":
    raise SystemExit(main())
