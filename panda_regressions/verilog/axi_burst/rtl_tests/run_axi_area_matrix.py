#!/usr/bin/env python3
"""Compare the four AXI profiles with fixed and compiler-derived FIFO depths.

Run with Vivado on PATH. Output must be fresh; HDL, Tcl, logs, checkpoints and
machine-readable measurements are retained. Timing is synthesis-only, not routed.
Use --xml with a previous XML snapshot to obtain a matched baseline.
"""

import argparse
import concurrent.futures
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess

from extract_write_engine import extract


def main():
    root = Path(__file__).resolve().parents[4]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    parser.add_argument("--xml", type=Path, default=root / "etc/libtech/NC_AXI_IPs.xml")
    parser.add_argument("--part", default="xc7a35tcpg236-1")
    parser.add_argument("--period", type=float, default=5.0)
    parser.add_argument("--jobs", type=int, default=2)
    args = parser.parse_args()
    vivado = shutil.which("vivado")
    if not vivado or args.jobs < 1 or args.period <= 0:
        parser.error("Vivado on PATH, positive jobs and clock period are required")
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=False)
    xml = out / "NC_AXI_IPs.xml"
    xml.write_bytes(args.xml.read_bytes())
    # Preserve the memory dependency beside the AXI snapshot.
    memory = args.xml.parent / "NC_MEM_IPs.xml"
    if not memory.exists():
        memory = root / "etc/libtech/NC_MEM_IPs.xml"
    (out / memory.name).write_bytes(memory.read_bytes())
    provenance = {
        "git_head": subprocess.check_output(["git", "-C", str(root), "rev-parse", "HEAD"], text=True).strip(),
        "axi_sha256": hashlib.sha256(xml.read_bytes()).hexdigest(),
        "memory_sha256": hashlib.sha256(memory.read_bytes()).hexdigest(),
        "vivado": vivado,
        "version": subprocess.check_output([vivado, "-version"], text=True, timeout=30),
        "part": args.part, "period_ns": args.period,
    }
    (out / "provenance.json").write_text(json.dumps(provenance, indent=2))

    def run(profile):
        direction, policy, burst, outstanding = profile
        depth = 32 if policy == "fixed" else (
            1 << (max(2, outstanding) * burst - 1).bit_length()
            if direction == "read" else 1 << (2 * burst - 1).bit_length())
        name = f"{direction}-{policy}-B{burst}-O{outstanding}-D{depth}"
        work = out / name
        work.mkdir()
        top = "MinimalAXI4MasterPipelined" + ("Write" if direction == "write" else "")
        (work / "core.sv").write_text(extract(xml, top))
        (work / "clock.xdc").write_text(f"create_clock -period {args.period} [get_ports clock]\n")
        (work / "run.tcl").write_text(f"""set_param general.maxThreads 2
read_verilog -sv core.sv
read_xdc clock.xdc
synth_design -top {top} -part {args.part} -mode out_of_context -generic B_MAX={burst} -generic MAX_OUTSTANDING={outstanding} -generic FIFO_DEPTH={depth}
report_utilization -file utilization.rpt
report_timing_summary -file timing.rpt
set fd [open registers.txt w]
foreach c [get_cells -hier -filter {{IS_SEQUENTIAL == 1}}] {{puts $fd $c}}
close $fd
write_checkpoint -force synth.dcp
puts AXI_AREA_PASS
exit
""")
        with (work / "console.log").open("w") as log:
            result = subprocess.run([vivado, "-mode", "batch", "-nojournal", "-source", "run.tcl"],
                                    cwd=work, stdout=log, stderr=subprocess.STDOUT, timeout=900)
        if result.returncode or "AXI_AREA_PASS" not in (work / "console.log").read_text():
            raise RuntimeError(f"Vivado failed; see {work / 'console.log'}")
        report = (work / "utilization.rpt").read_text()
        metrics = {}
        for key, label in (("lut", "Slice LUTs*"), ("ff", "Slice Registers"), ("bram", "Block RAM Tile")):
            match = re.search(r"\|\s*" + re.escape(label) + r"\s*\|\s*([\d.]+)", report)
            if not match:
                raise RuntimeError(f"Missing {label} in {work}")
            metrics[key] = float(match[1])
        registers = (work / "registers.txt").read_text().splitlines()
        metrics["descriptor_pointer_registers"] = sum("descriptor_" in n and "ptr_reg" in n for n in registers)
        metrics["burst_boundary_registers"] = sum("boundary_" in n for n in registers)
        metrics["plan_queue_registers"] = sum(any(k in n for k in ("plan_address_reg", "plan_beats_reg", "plan_count_reg", "plan_rd_ptr_reg", "plan_wr_ptr_reg")) for n in registers)
        record = dict(direction=direction, policy=policy, burst=burst, outstanding=outstanding, depth=depth, **metrics)
        (work / "metrics.json").write_text(json.dumps(record, indent=2))
        print(json.dumps(record), flush=True)
        return record

    profiles = [(direction, policy, burst, outstanding)
                for direction in ("read", "write") for policy in ("fixed", "derived")
                for burst in (1, 16) for outstanding in (1, 16)]
    with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as pool:
        results = list(pool.map(run, profiles))
    (out / "results.json").write_text(json.dumps(results, indent=2))


if __name__ == "__main__":
    main()
