#!/usr/bin/env python3
"""Exercise two explicitly bound operations sharing one physical FU."""

import os
from pathlib import Path
import re
import shutil
import tempfile
import xml.etree.ElementTree as ET

from test_timed_capture import ROOT, resolve_modelsim_bin, run


SUITE = ROOT / "panda_regressions/hls/bambu_lp_array_channel"
SOURCE = SUITE / "shared_fu.cpp"
TESTBENCH = SUITE / "tb_shared_fu.v"
OVERLAY = SUITE / "lp_co_launch_ii2.xml"
FU_NAME = "ui_mul_node_FU_32_32_32_2"


def scheduling_rows(directory):
    rows = []
    for path in directory.glob("*_scheduling.xml"):
        rows.extend(ET.parse(path).getroot().iter("scheduling_constraints"))
    return rows


def multiply_vertices(directory):
    nodes = [row.get("vertex_name") for row in scheduling_rows(directory)
             if row.get("fu_name", "").startswith("ui_mul_node_FU_")]
    if len(nodes) != 2 or len(set(nodes)) != 2:
        raise RuntimeError(f"Expected two multiply vertices in {directory}, found {nodes}")
    return nodes


def write_binding(path, nodes):
    root = ET.Element("constraints")
    hls_constraints = ET.SubElement(root, "HLS_constraints")
    for node in nodes:
        ET.SubElement(hls_constraints, "binding_constraints", {
            "vertex_name": node,
            "fu_name": FU_NAME,
            "fu_library": "STD_FU",
            "fu_index": "0",
        })
    ET.ElementTree(root).write(path, encoding="utf-8", xml_declaration=True)


def bambu_command(binding=None):
    command = [
        "bambu",
        "--configuration-name=LP-ARRAY-CHANNEL-MODELSIM",
        "--generate-interface=INFER",
        "--top-fname=shared_fu",
        "--benchmark-name=shared_fu",
        "--clock-period=5",
        "--device-name=xc7z020,-1,clg484",
        "--debug=4",
        "--print-dot",
    ]
    if binding:
        command.append(f"--resource-constraints={FU_NAME}:STD_FU=1")
    command.append(str(SOURCE))
    if binding:
        command.append(str(binding))
    command.append(str(OVERLAY))
    return command


def check_shared_mapping(directory, vertices):
    rows = [row for row in scheduling_rows(directory) if row.get("vertex_name") in vertices]
    if len(rows) != 2:
        raise RuntimeError(f"Expected mappings for both multiply vertices, found {len(rows)}")
    for row in rows:
        actual = (row.get("fu_name"), row.get("fu_index"), row.get("library"))
        if actual != (FU_NAME, "0", "STD_FU"):
            raise RuntimeError(f"Unexpected shared FU mapping for {row.get('vertex_name')}: {actual}")


def check_one_physical_fu(directory):
    design = directory / "shared_fu.v"
    if not design.is_file():
        raise RuntimeError(f"Bambu did not emit {design.name}")
    text = design.read_text()
    instances = re.findall(r"(?m)^\s*ui_mul_node_FU\s+#\s*\(", text)
    if len(instances) != 1:
        raise RuntimeError(f"Expected one physical multiplier instance, found {len(instances)}")
    for required in (".PIPE_PARAMETER(2)", ".lhs(", ".rhs("):
        if required not in text:
            raise RuntimeError(f"Generated multiplier does not match the II=2 overlay: {required}")
    rtl = sorted(directory.glob("*.v"))
    if not any(path.name == "panda_libtech.v" for path in rtl):
        raise RuntimeError("Bambu did not emit panda_libtech.v")
    return rtl


def main():
    temp_root = ROOT / "documentation/tmp"
    temp_root.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="shared-fu-", dir=temp_root) as temp_dir:
        scratch = Path(temp_dir)
        env = os.environ.copy()
        env.update(TMPDIR=str(scratch), TMP=str(scratch), TEMP=str(scratch))
        modelsim_bin = resolve_modelsim_bin(env)
        env["PATH"] = modelsim_bin + os.pathsep + env.get("PATH", "")
        for tool in ("bambu", "vlib", "vlog", "vsim"):
            if not shutil.which(tool, path=env["PATH"]):
                raise SystemExit(f"Shared-FU check requires {tool} in PATH")

        unconstrained = scratch / "unconstrained"
        unconstrained.mkdir()
        run(bambu_command(), unconstrained, env)
        vertices = multiply_vertices(unconstrained)

        binding = scratch / "binding.xml"
        write_binding(binding, vertices)
        shared = scratch / "shared"
        shared.mkdir()
        run(bambu_command(binding), shared, env)
        check_shared_mapping(shared, vertices)
        rtl = check_one_physical_fu(shared)

        library = "shared_fu_work"
        run(["vlib", library], shared, env)
        run(["vlog", "-quiet", "-work", library, TESTBENCH, *rtl], shared, env)
        output = run(["vsim", "-c", "-quiet", "-L", library, f"{library}.tb_shared_fu",
                      "-do", "run -all; quit -f"], shared, env)
        if "PASS shared FU transactions=4" not in output:
            raise RuntimeError(f"ModelSim omitted shared-FU PASS marker:\n{output}")
        print("PASS shared FU: two operations, one instance, four transactions")


if __name__ == "__main__":
    main()
