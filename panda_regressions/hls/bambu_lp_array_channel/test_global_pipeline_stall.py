#!/usr/bin/env python3
# Copyright (C) 2026 Politecnico di Milano
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
"""Check the default Stallable contract on generated array interface FUs."""

import os
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile
import xml.etree.ElementTree as ET

from test_timed_capture import resolve_parallelism


ROOT = Path(__file__).resolve().parents[3]
SOURCE = ROOT / "panda_regressions/hls/bambu_lp_array_channel/lp_mul_array.cpp"
DATAFLOW_SOURCE = ROOT / "panda_regressions/hls/bambu_specific_test4/pipe_dataflow_stream.cpp"
SCRATCH = Path(tempfile.gettempdir())
RESOURCE_NAMES = ("a_bambu_artificial_ParmMgr", "b_bambu_artificial_ParmMgr")


def declared_ports(module, direction):
    return re.findall(
        r"(?m)^\s*" + direction + r"\s+(?:wire\s+)?(?:\[[^\]]+\]\s*)?([A-Za-z0-9_$]+)\s*;",
        module,
    )


def require_port_sequence(module, direction, expected, resource):
    actual = declared_ports(module, direction)
    if actual != expected:
        raise RuntimeError(
            f"{resource} {direction} ports differ from canonical sequence {expected}; declared {actual}"
        )


def require_write_output_order(module, resource, lanes=None):
    outputs = declared_ports(module, "output")
    write_enables = sorted(
        (port for port in outputs if re.search(r"_we[01]$", port)), key=lambda port: int(port[-1])
    )
    write_data = sorted(
        (port for port in outputs if re.search(r"_d[01]$", port)), key=lambda port: int(port[-1])
    )
    expected = write_enables + write_data
    if lanes is not None and (len(write_enables) != lanes or len(write_data) != lanes):
        raise RuntimeError(f"{resource} expected {lanes} write lanes, found {outputs}")
    if [port for port in outputs if re.search(r"_(?:we|d)[01]$", port)] != expected:
        raise RuntimeError(f"{resource} output ports do not place all write enables before data: {outputs}")


def write_overlay(path, value, explicit_operation=None, explicit_template=False):
    technology = ET.Element("technology")
    library = ET.SubElement(technology, "library")
    ET.SubElement(library, "name").text = "interface_library"
    if value is not None:
        for resource in RESOURCE_NAMES:
            cell = ET.SubElement(library, "cell")
            ET.SubElement(cell, "name").text = resource
            attribute = ET.SubElement(cell, "attribute", {"name": "Stallable", "value_type": "boolean"})
            attribute.text = value
            if explicit_operation:
                ET.SubElement(cell, "operation", {"operation_name": explicit_operation})
            if explicit_template:
                ET.SubElement(cell, "template", {"name": "array_template", "parameter": "1"})
    ET.ElementTree(technology).write(path, encoding="utf-8", xml_declaration=True)


def run_case(bambu, work, name, channels, value):
    overlay = work / f"{name}.xml"
    output = work / f"{name}_out"
    output.mkdir()
    if value is not None:
        write_overlay(overlay, value)
    env = os.environ.copy()
    env.update(TMPDIR=str(work), TMP=str(work), TEMP=str(work))
    command = [
        str(bambu), "--configuration-name=LP-ARRAY-CHANNEL-MODELSIM", "--generate-interface=INFER",
        f"--channels-number={channels}", str(SOURCE), "--top-fname=mul_stage", f"--benchmark-name={name}",
        "--clock-period=5", "--device-name=xc7z020,-1,clg484", f"--output-directory={output}",
    ]
    if value is not None:
        command.append(str(overlay))
    result = subprocess.run(command, cwd=work, env=env, text=True, stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT, check=False)
    if result.returncode:
        raise RuntimeError(f"Bambu failed in {name} ({result.returncode}):\n{result.stdout}")
    rtl_files = list(output.glob("*.v"))
    if not rtl_files:
        raise RuntimeError(f"Bambu emitted no Verilog in {output}")
    rtl = "\n".join(path.read_text(encoding="utf-8", errors="replace") for path in rtl_files)
    stallable = value != "false"
    for bundle, resource in zip(("a", "b"), RESOURCE_NAMES):
        matches = re.findall(r"(?ms)^module\s+" + re.escape(resource) + r"_modgen\s*\(.*?^endmodule", rtl)
        if len(matches) != 1:
            raise RuntimeError(f"expected one generated wrapper for {resource}, found {len(matches)}")
        module = matches[0]
        input_ports = ["clock", "reset", "start_port"]
        if stallable:
            if not re.search(r"(?m)^\s*input\s+stall_port\s*;", module):
                raise RuntimeError(f"{resource} did not expose scalar stall_port")
            input_ports.append("stall_port")
            stall_term = " & ~stall_port"
        else:
            if re.search(r"\bstall_port\b", module):
                raise RuntimeError(f"non-stallable {resource} unexpectedly exposes stall_port")
            stall_term = ""
        input_ports.extend(("in1", "in2", "in3", "in4"))
        q_ports = [port for port in declared_ports(module, "input") if re.search(r"_q[01]$", port)]
        q_ports.sort(key=lambda port: int(port[-1]))
        if len(q_ports) != channels:
            raise RuntimeError(f"{resource} expected {channels} q inputs, found {q_ports}")
        input_ports.extend(q_ports)
        require_port_sequence(module, "input", input_ports, resource)
        require_write_output_order(module, resource)
        ports = (0,) if channels == 1 else (0, 1)
        for port in ports:
            expected_ce = f"assign p_{bundle}_ce{port} = start_port[{port}]{stall_term};"
            if expected_ce not in module:
                raise RuntimeError(f"{resource} lacks expected CE expression: {expected_ce}")
            expected_q = (f"assign out1[BITSIZE_out1*{port}+:BITSIZE_out1] = p_{bundle}_q{port};")
            if expected_q not in module:
                raise RuntimeError(f"{resource} does not pass q{port} directly to out1")
        instances = re.findall(
            r"(?ms)^\s*" + re.escape(resource) + r"_modgen\s*#\(.*?\)\s*fu_[A-Za-z0-9_$]+\s*\((.*?)\);",
            rtl,
        )
        if len(instances) != 1:
            raise RuntimeError(f"{name} expected one {resource} instance, found {len(instances)}")
        stall_connections = re.findall(r"\.stall_port\s*\(\s*selector_stallable_fus\s*\)", instances[0])
        if len(stall_connections) != (1 if stallable else 0):
            raise RuntimeError(f"{name} {resource} has {len(stall_connections)} stall selector connections")
    capture_count = len(re.findall(r"(?m)^\s*TIMED_CAPTURE\s*#\(", rtl))
    expected_captures = 0 if stallable else 2
    if capture_count != expected_captures:
        raise RuntimeError(f"{name} expected {expected_captures} array-result captures, found {capture_count}")
    return f"PASS: {name} Stallable={'true' if stallable else 'false'} channels={channels}"


def run_writer_first_case(bambu, work, name, channels):
    output = work / f"{name}_out"
    output.mkdir()
    env = os.environ.copy()
    env.update(TMPDIR=str(work), TMP=str(work), TEMP=str(work))
    command = [
        str(bambu), "--configuration-name=LP-ARRAY-CHANNEL-MODELSIM", "--generate-interface=INFER",
        f"--channels-number={channels}", str(DATAFLOW_SOURCE), "--top-fname=top", f"--benchmark-name={name}",
        "--clock-period=5", "--device-name=xc7z020,-1,clg484", f"--output-directory={output}",
    ]
    result = subprocess.run(command, cwd=work, env=env, text=True, stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT, check=False)
    if result.returncode:
        raise RuntimeError(f"Bambu failed in {name} ({result.returncode}):\n{result.stdout}")
    rtl_files = list(output.glob("*.v"))
    if not rtl_files:
        raise RuntimeError(f"Bambu emitted no Verilog in {output}")
    rtl = "\n".join(path.read_text(encoding="utf-8", errors="replace") for path in rtl_files)
    modules = re.findall(r"(?ms)^module\s+([A-Za-z0-9_$]+)\s*\(.*?^endmodule", rtl)
    array_modules = [module for module in modules if module.endswith("bambu_artificial_ParmMgr_modgen")]
    read_modules = 0
    for module_name in array_modules:
        module_match = re.search(
            r"(?ms)^module\s+" + re.escape(module_name) + r"\s*\(.*?^endmodule", rtl
        )
        module = module_match.group(0)
        q_ports = re.findall(
            r"(?m)^\s*input\s+(?:\[[^\]]+\]\s*)?([A-Za-z0-9_$]*_q([01]))\s*;", module
        )
        if not q_ports:
            if re.search(r"(?m)^\s*assign\s+out1\[", module):
                raise RuntimeError(f"write-only array wrapper {module_name} assigns read data")
            continue
        read_modules += 1
        if not re.search(r"(?m)^\s*input\s+stall_port\s*;", module):
            raise RuntimeError(f"writer-first array wrapper {module_name} lacks stall_port")
        expected_inputs = ["clock", "reset", "start_port", "stall_port", "in1", "in2", "in3", "in4"]
        expected_inputs.extend(port for port, _ in sorted(q_ports, key=lambda item: int(item[1])))
        require_port_sequence(module, "input", expected_inputs, module_name)
        q_by_lane = {int(lane): port for port, lane in q_ports}
        q0_port = q_by_lane.get(0)
        if q0_port is None:
            raise RuntimeError(f"{module_name} lacks q0")
        require_write_output_order(module, module_name, channels)
        for lane in range(channels):
            q_port = q_by_lane.get(lane)
            if q_port is None:
                raise RuntimeError(f"{module_name} lacks q{lane} for channels={channels}")
            expected_q = f"assign out1[BITSIZE_out1*{lane}+:BITSIZE_out1] = {q_port};"
            if expected_q not in module:
                raise RuntimeError(f"{module_name} does not pass q{lane} directly to out1")
            ce_ports = re.findall(r"(?m)^\s*output\s+([A-Za-z0-9_$]*_ce" + str(lane) + r")\s*;", module)
            if len(ce_ports) != 1:
                raise RuntimeError(f"{module_name} has {len(ce_ports)} CE{lane} outputs")
            expected_ce = f"assign {ce_ports[0]} = start_port[{lane}] & ~stall_port;"
            if expected_ce not in module:
                raise RuntimeError(f"{module_name} lacks expected CE gating: {expected_ce}")
        if re.search(r"(?m)^\s*assign\s+out1\[.*\]\s*=\s*stall_port\s*;", module):
            raise RuntimeError(f"{module_name} connects stall_port to read data")
    if not array_modules or not read_modules:
        raise RuntimeError(f"{name} emitted no readable array interface wrappers")
    return f"PASS: {name} writer-first shared arrays channels={channels}"


def run_shared_bundle_case(bambu, work, name, channels, argument_order, writer_only=False):
    source = work / f"{name}.cpp"
    output = work / f"{name}_out"
    output.mkdir()
    if writer_only:
        params = "int b[16], unsigned index"
        body = "b[index & 15U] = (int)index; return 0;"
    elif argument_order == "reader_first":
        params = "const int a[16], int b[16], unsigned index"
        body = "const int value = a[index & 15U]; b[index & 15U] = value + 1; return value;"
    else:
        params = "int b[16], const int a[16], unsigned index"
        body = "const int value = a[index & 15U]; b[index & 15U] = value + 1; return value;"
    pragmas = ""
    if not writer_only:
        pragmas += "#pragma HLS interface port=a mode=array elem_count=16 bundle=shared\n"
    pragmas += "#pragma HLS interface port=b mode=array elem_count=16 bundle=shared\n"
    source.write_text(
        pragmas + f"int shared_rw({params}) {{ {body} }}\n",
        encoding="utf-8",
    )
    env = os.environ.copy()
    env.update(TMPDIR=str(work), TMP=str(work), TEMP=str(work))
    command = [
        str(bambu), "--configuration-name=LP-ARRAY-CHANNEL-MODELSIM", "--generate-interface=INFER",
        f"--channels-number={channels}", str(source), "--top-fname=shared_rw", f"--benchmark-name={name}",
        "--clock-period=5", "--device-name=xc7z020,-1,clg484", f"--output-directory={output}",
    ]
    result = subprocess.run(command, cwd=work, env=env, text=True, stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT, check=False)
    if result.returncode:
        raise RuntimeError(f"Bambu failed in {name} ({result.returncode}):\n{result.stdout}")
    rtl_files = list(output.glob("*.v"))
    if not rtl_files:
        raise RuntimeError(f"Bambu emitted no Verilog in {output}")
    rtl = "\n".join(path.read_text(encoding="utf-8", errors="replace") for path in rtl_files)
    modules = re.findall(r"(?ms)^module\s+([A-Za-z0-9_$]+)\s*\(.*?^endmodule", rtl)
    arrays = [module for module in modules if module.endswith("bambu_artificial_ParmMgr_modgen")]
    if len(arrays) != 1:
        raise RuntimeError(f"{name} expected one shared array wrapper, found {len(arrays)}")
    module = re.search(r"(?ms)^module\s+" + re.escape(arrays[0]) + r"\s*\(.*?^endmodule", rtl).group(0)
    expected_inputs = ["clock", "reset", "start_port", "stall_port", "in1", "in2", "in3", "in4"]
    q_ports = [port for port in declared_ports(module, "input") if re.search(r"_q[01]$", port)]
    q_ports.sort(key=lambda port: int(port[-1]))
    expected_inputs.extend(q_ports)
    require_port_sequence(module, "input", expected_inputs, arrays[0])
    require_write_output_order(module, arrays[0], channels)
    if writer_only:
        if q_ports or re.search(r"(?m)^\s*assign\s+out1\[", module):
            raise RuntimeError(f"{name} write-only wrapper has read-data inputs or assignments")
    else:
        if len(q_ports) != channels:
            raise RuntimeError(f"{name} expected {channels} q inputs, found {q_ports}")
        for lane, q_port in enumerate(q_ports):
            expected_q = f"assign out1[BITSIZE_out1*{lane}+:BITSIZE_out1] = {q_port};"
            if expected_q not in module:
                raise RuntimeError(f"{arrays[0]} does not map read result lane {lane} from {q_port}")
    return f"PASS: {name} canonical shared array ports channels={channels}"


def run_rejection_case(bambu, work, name, explicit_operation=None, explicit_template=False):
    overlay = work / f"{name}.xml"
    output = work / f"{name}_out"
    output.mkdir()
    write_overlay(overlay, "true", explicit_operation, explicit_template)
    env = os.environ.copy()
    env.update(TMPDIR=str(work), TMP=str(work), TEMP=str(work))
    command = [
        str(bambu), "--configuration-name=LP-ARRAY-CHANNEL-MODELSIM", "--generate-interface=INFER",
        "--channels-number=2", str(SOURCE), "--top-fname=mul_stage", f"--benchmark-name={name}",
        "--clock-period=5", "--device-name=xc7z020,-1,clg484", f"--output-directory={output}", str(overlay),
    ]
    result = subprocess.run(command, cwd=work, env=env, text=True, stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT, check=False)
    expected = "explicitly declares an operation" if explicit_operation else "explicitly declares a template"
    if result.returncode == 0 or expected not in result.stdout:
        raise RuntimeError(f"{name} did not fail with {expected!r}:\n{result.stdout}")
    return f"PASS: {name} rejected explicit metadata"


def run_parallel_cases(cases, jobs, scratch):
    def run_case_isolated(name, operation):
        with tempfile.TemporaryDirectory(prefix=f"{name}-", dir=scratch) as temp:
            return operation(Path(temp))

    if not cases:
        return []

    with ThreadPoolExecutor(max_workers=min(jobs, len(cases))) as executor:
        futures = [executor.submit(run_case_isolated, name, operation) for name, operation in cases]
        results = []
        for (name, _), future in zip(cases, futures):
            try:
                results.append((name, future.result(), None))
            except Exception as error:
                detail = str(error) or repr(error) or type(error).__name__
                results.append((name, None, f"{type(error).__name__}: {detail}"))
    return results


def report_case_results(results):
    failed = False
    for name, result, error in results:
        if error is None:
            print(result)
        else:
            failed = True
            print(f"FAIL: {name}: {error}", file=sys.stderr)
    if failed:
        raise SystemExit(1)


def main(argv=None):
    jobs = resolve_parallelism(sys.argv[1:] if argv is None else argv)
    bambu = os.environ.get("BAMBU") or shutil.which("bambu")
    if not bambu:
        raise SystemExit("bambu is required; set BAMBU or source the installed settings.sh")
    bambu = Path(bambu).resolve()
    SCRATCH.mkdir(parents=True, exist_ok=True)
    cases = [
        ("stallable_dual", lambda work: run_case(bambu, work, "stallable_dual", 2, "true")),
        ("stallable_single", lambda work: run_case(bambu, work, "stallable_single", 1, "true")),
        ("nonstallable_dual", lambda work: run_case(bambu, work, "nonstallable_dual", 2, "false")),
        ("nonstallable_single", lambda work: run_case(bambu, work, "nonstallable_single", 1, "false")),
        ("default_dual", lambda work: run_case(bambu, work, "default_dual", 2, None)),
        ("default_single", lambda work: run_case(bambu, work, "default_single", 1, None)),
        ("writer_first_shared_single", lambda work: run_writer_first_case(bambu, work, "writer_first_shared_single", 1)),
        ("writer_first_shared_dual", lambda work: run_writer_first_case(bambu, work, "writer_first_shared_dual", 2)),
        ("shared_reader_first_single", lambda work: run_shared_bundle_case(
            bambu, work, "shared_reader_first_single", 1, "reader_first")),
        ("shared_reader_first_dual", lambda work: run_shared_bundle_case(
            bambu, work, "shared_reader_first_dual", 2, "reader_first")),
        ("shared_writer_first_single", lambda work: run_shared_bundle_case(
            bambu, work, "shared_writer_first_single", 1, "writer_first")),
        ("shared_writer_first_dual", lambda work: run_shared_bundle_case(
            bambu, work, "shared_writer_first_dual", 2, "writer_first")),
        ("shared_writer_only", lambda work: run_shared_bundle_case(
            bambu, work, "shared_writer_only", 1, "reader_first", writer_only=True)),
        ("shared_writer_only_dual", lambda work: run_shared_bundle_case(
            bambu, work, "shared_writer_only_dual", 2, "reader_first", writer_only=True)),
        ("explicit_fu_operation",
         lambda work: run_rejection_case(bambu, work, "explicit_fu_operation", explicit_operation=RESOURCE_NAMES[0])),
        ("explicit_read_operation",
         lambda work: run_rejection_case(bambu, work, "explicit_read_operation",
                                         explicit_operation=f"{RESOURCE_NAMES[0]}_Read")),
        ("explicit_template",
         lambda work: run_rejection_case(bambu, work, "explicit_template", explicit_template=True)),
    ]
    report_case_results(run_parallel_cases(cases, jobs, SCRATCH))


if __name__ == "__main__":
    main()
