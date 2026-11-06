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
WAIT_SOURCE = SUITE / "lp_shared_fu_wait.cpp"
WAIT_TESTBENCH = SUITE / "tb_lp_shared_fu_wait.v"
OVERLAY = SUITE / "lp_co_launch_ii2.xml"
FU_NAME = "ui_mul_node_FU_32_32_32_2"
WAIT_TOP = "lp_shared_fu_wait"
WAIT_DESIGN = "p_Z17lp_shared_fu_waitRA16_iS0_S0_S0_R10ac_channelIiE"


def scheduling_rows(directory):
    rows = []
    for path in directory.glob("*_scheduling.xml"):
        rows.extend(ET.parse(path).getroot().iter("scheduling_constraints"))
    return rows


def dot_multiply_vertices(directory):
    pattern = re.compile(r'label="([^\"]+) - mul_node\\n')
    nodes = set()
    dot_files = directory.glob("HLS_output/dot/*/HLS_scheduling_BB*.dot")
    for path in dot_files:
        nodes.update(pattern.findall(path.read_text(encoding="utf-8", errors="replace")))
    if len(nodes) != 2:
        raise RuntimeError(f"Expected two mul_node vertices in scheduling DOT under {directory}, found {sorted(nodes)}")
    return sorted(nodes)


def multiply_vertices(directory):
    rows = [row for row in scheduling_rows(directory)
            if row.get("fu_name", "").startswith("ui_mul_node_FU_")]
    if rows:
        nodes = [row.get("vertex_name") for row in rows]
        if len(nodes) != 2 or len(set(nodes)) != 2:
            raise RuntimeError(f"Expected two multiply vertices in {directory}, found {nodes}")
        return nodes
    return dot_multiply_vertices(directory)


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


def bambu_command(binding=None, source=SOURCE, top="shared_fu", benchmark="shared_fu"):
    command = [
        "bambu",
        "--configuration-name=LP-ARRAY-CHANNEL-MODELSIM",
        "--generate-interface=INFER",
        f"--top-fname={top}",
        f"--benchmark-name={benchmark}",
        "--clock-period=5",
        "--device-name=xc7z020,-1,clg484",
        "--print-dot",
    ]
    if binding:
        command.append("--resource-constraints=ui_mul_node_FU:STD_FU=1")
    command.append(str(source))
    if binding:
        command.append(str(binding))
    command.append(str(OVERLAY))
    return command


def check_shared_mapping(directory, vertices):
    rows = [row for row in scheduling_rows(directory) if row.get("vertex_name") in vertices]
    if not rows:
        # In Release builds the scheduler XML is omitted. The binding request,
        # single-instance RTL check below, and transaction simulation together
        # verify that the two operations share the constrained physical FU.
        return
    if len(rows) != 2:
        raise RuntimeError(f"Expected mappings for both multiply vertices, found {len(rows)}")
    for row in rows:
        actual = (row.get("fu_name"), row.get("fu_index"), row.get("library"))
        if actual != (FU_NAME, "0", "STD_FU"):
            raise RuntimeError(f"Unexpected shared FU mapping for {row.get('vertex_name')}: {actual}")


def check_one_physical_fu(directory, design_name="shared_fu"):
    design = directory / f"{design_name}.v"
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


def check_pipelined_wait_design(directory, vertices, output):
    if not re.search(r"II=[1-9][0-9]* solution found for loop \(BB[0-9]+\)", output):
        raise RuntimeError("Shared-FU wait loop was not pipelined")
    check_shared_mapping(directory, vertices)
    rtl = check_one_physical_fu(directory, WAIT_TOP)
    text = "\n".join(path.read_text(encoding="utf-8", errors="replace") for path in rtl)
    if not re.search(r"(?m)^\s*TIMED_CAPTURE\s+#", text):
        raise RuntimeError("Shared-FU wait design has no TIMED_CAPTURE result protection")
    if WAIT_DESIGN not in text:
        raise RuntimeError(f"Generated RTL lacks the expected top module {WAIT_DESIGN}")
    return rtl


def check_wait_capture_overlap(vcd_path):
    names = {}
    in_definitions = True
    for line in vcd_path.read_text(encoding="utf-8", errors="replace").splitlines():
        fields = line.split()
        if in_definitions:
            if fields and fields[0] == "$var" and len(fields) >= 5:
                names.setdefault(fields[3], set()).add(fields[4])
            elif fields and fields[0] == "$enddefinitions":
                in_definitions = False
                break
    full_ids = {identifier for identifier, refs in names.items() if "out_full_n" in refs}
    # This is the latency-aligned event inside TIMED_CAPTURE, distinct from
    # the controller's later consume selector.
    capture_due_ids = {identifier for identifier, refs in names.items() if "capture_due" in refs}
    starts = {}
    completions = {}
    for identifier, refs in names.items():
        for reference in refs:
            start = re.fullmatch(r"selector_IN_UNBOUNDED_(.+)", reference)
            if start:
                starts.setdefault(start.group(1), set()).add(identifier)
            completion = re.fullmatch(r"OUT_UNBOUNDED_(.+)", reference)
            if completion:
                completions.setdefault(completion.group(1), set()).add(identifier)
    paired = set(starts) & set(completions)

    state = {}
    previous_state = {}
    outstanding = {key: False for key in paired}
    overlaps = 0
    snapshot_changed = False
    blocked_samples = 0
    due_samples = 0

    def check_snapshot():
        nonlocal blocked_samples, due_samples, previous_state
        for key in paired:
            if any(state.get(identifier) == "1" and previous_state.get(identifier) != "1"
                   for identifier in starts[key]):
                outstanding[key] = True
            if any(state.get(identifier) == "1" and previous_state.get(identifier) != "1"
                   for identifier in completions[key]):
                outstanding[key] = False
        previous_state = state.copy()
        if not snapshot_changed or not any(state.get(identifier) == "0" for identifier in full_ids):
            return 0
        blocked = any(
            outstanding[key] and any(state.get(identifier) == "0" for identifier in completions[key])
            for key in paired
        )
        due = any(state.get(identifier) == "1" for identifier in capture_due_ids)
        blocked_samples += int(blocked)
        due_samples += int(due)
        return int(blocked and due)

    in_definitions = True
    for line in vcd_path.read_text(encoding="utf-8", errors="replace").splitlines():
        fields = line.split()
        if in_definitions:
            if fields and fields[0] == "$enddefinitions":
                in_definitions = False
            continue
        if line.startswith("#"):
            overlaps += check_snapshot()
            snapshot_changed = False
        elif fields and fields[0][0] in "01xXzZ":
            identifier = fields[0][1:] if len(fields[0]) > 1 else fields[1]
            state[identifier] = fields[0][0].lower()
            snapshot_changed = True
        elif line and line[0] in "bB" and len(fields) == 2:
            state[fields[1]] = fields[0][1:].lower()
            snapshot_changed = True
    overlaps += check_snapshot()
    if overlaps == 0:
        full_values = {state.get(identifier) for identifier in full_ids}
        due_values = {state.get(identifier) for identifier in capture_due_ids}
        completion_values = {state.get(identifier) for key in paired for identifier in completions[key]}
        raise RuntimeError(
            "No cycle showed backpressure, a pending unbounded operation, and TIMED_CAPTURE capture_due together "
            f"(blocked={blocked_samples}, outstanding={any(outstanding.values())}, capture_due={due_samples}; "
            f"final full={full_values}, done={completion_values}, due={due_values})"
        )
    return overlaps


def check_wait_capture_overlap_self_test(directory):
    header = """$scope module tb $end
$var wire 1 ! out_full_n $end
$var wire 1 \" capture_due $end
$var wire 1 # selector_IN_UNBOUNDED_fu $end
$var wire 1 $ OUT_UNBOUNDED_fu $end
$var wire 1 % selector_IN_UNBOUNDED_array $end
$var wire 1 & OUT_UNBOUNDED_array $end
$upscope $end
$enddefinitions $end
"""
    completed = directory / "completed_wait.vcd"
    completed.write_text(header + "#0\n0!\n0\"\n0#\n0$\n0%\nz&\n#1\n1#\n#5\n1$\n#6\n1%\n#7\n0%\n#10\n0$\n0!\n1\"\n")
    try:
        check_wait_capture_overlap(completed)
    except RuntimeError:
        pass
    else:
        raise RuntimeError("VCD overlap check accepted a completed unbounded operation")

    pending = directory / "pending_wait.vcd"
    pending.write_text(header + "#0\n0!\n0\"\n0#\n0$\n#1\n1#\n#2\n0#\n0$\n0!\n1\"\n")
    if check_wait_capture_overlap(pending) != 1:
        raise RuntimeError("VCD overlap check missed a pending unbounded operation")


def run_wait_case(scratch, env, modelsim_bin):
    check_wait_capture_overlap_self_test(scratch)
    unconstrained = scratch / "wait-unconstrained"
    unconstrained.mkdir()
    output = run(bambu_command(source=WAIT_SOURCE, top=WAIT_TOP, benchmark=WAIT_TOP), unconstrained, env)
    vertices = multiply_vertices(unconstrained)

    binding = scratch / "wait-binding.xml"
    write_binding(binding, vertices)
    shared = scratch / "wait-shared"
    shared.mkdir()
    output = run(bambu_command(binding, WAIT_SOURCE, WAIT_TOP, WAIT_TOP), shared, env)
    rtl = check_pipelined_wait_design(shared, vertices, output)

    library = "shared_fu_wait_work"
    run(["vlib", library], shared, env)
    run(["vlog", "-quiet", "-work", library, WAIT_TESTBENCH, *rtl], shared, env)
    for bp_mode in range(3):
        output = run(["vsim", "-c", "-quiet", "-L", library, f"{library}.tb_lp_shared_fu_wait",
                      "-voptargs=+acc", f"-gBP_MODE={bp_mode}", "-do", "run -all; quit -f"], shared, env)
        if f"PASS [BP={bp_mode}] outputs=16" not in output:
            raise RuntimeError(f"Shared-FU wait simulation failed for BP={bp_mode}:\n{output}")
        if bp_mode and not re.search(r"SHARED_FU_WAIT bp=[12] outputs=16 stalls=[1-9][0-9]*", output):
            raise RuntimeError(f"Shared-FU wait test did not exercise backpressure at BP={bp_mode}:\n{output}")
        if bp_mode == 2:
            overlap = check_wait_capture_overlap(shared / "shared_fu_wait.vcd")
            print(f"SHARED_FU_OVERLAP bp={bp_mode} blocked_capture_cycles={overlap}")


def main():
    with tempfile.TemporaryDirectory(prefix="shared-fu-", dir=os.environ.get("TMPDIR")) as temp_dir:
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

        run_wait_case(scratch, env, modelsim_bin)
        print("PASS shared FU wait: pipelined loop, one instance, capture, BP=0/1/2")


if __name__ == "__main__":
    main()
