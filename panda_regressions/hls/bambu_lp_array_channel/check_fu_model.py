#!/usr/bin/env python3
# Copyright (C) 2026 Politecnico di Milano
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
"""Check the selected FU model, assignment, and FSM initiation interval."""

import os
from pathlib import Path
import re
import sys
import xml.etree.ElementTree as ET

SUITE = Path(__file__).resolve().parent


def fail(message):
    print(f"FAIL: {message}", file=sys.stderr)
    raise SystemExit(1)


def main():
    model = os.environ["LP_EXPECT_FU_MODEL"]
    operation = os.environ["LP_EXPECT_FU_OPERATION"]
    expected_ii = int(os.environ["LP_EXPECT_FU_II"])
    expected_latency = int(os.environ["LP_EXPECT_FU_LATENCY"])
    expected_pipe = os.environ["LP_EXPECT_PIPE_PARAMETER"]
    function = os.environ["MANTIS_EXPECT_FUNCTION"]
    if expected_ii > expected_latency:
        fail(f"invalid test expectation: FU II={expected_ii} exceeds latency={expected_latency}")

    case_dir = Path.cwd()
    log_path = case_dir / "bambu.log"
    technology_path = case_dir / "technology_out.xml"
    log = log_path.read_text(encoding="utf-8") if log_path.is_file() else ""
    if not technology_path.is_file():
        metadata_path = Path(os.environ.get("LP_EXPECT_FU_TECHNOLOGY", ""))
        if not metadata_path.is_absolute():
            metadata_path = SUITE / metadata_path
        if not metadata_path.is_file():
            fail("Bambu did not emit technology_out.xml and no expected FU metadata XML was supplied")
        technology_path = metadata_path

    try:
        technology = ET.parse(technology_path).getroot()
    except ET.ParseError as error:
        fail(f"cannot parse technology_out.xml: {error}")
    matching_models = [cell for cell in technology.iter("cell")
                       if cell.findtext("name") == model]
    if not matching_models:
        fail(f"technology_out.xml lacks selected model {model}")
    if not any(
        any(op.get("operation_name") == operation
            and op.get("initiation_time") == str(expected_ii)
            and op.get("cycles") == str(expected_latency)
            and op.get("pipe_parameters") == expected_pipe
            for op in cell.findall("operation"))
        for cell in matching_models
    ):
        fail(f"model {model} does not declare {operation} with II={expected_ii}, latency={expected_latency}, "
             f"and PIPE_PARAMETER={expected_pipe}")

    vertices = set()
    scheduling_files = list(case_dir.glob("*_scheduling.xml"))
    if scheduling_files:
        for path in scheduling_files:
            try:
                root = ET.parse(path).getroot()
            except ET.ParseError as error:
                fail(f"cannot parse {path.name}: {error}")
            for constraint in root.iter("scheduling_constraints"):
                vertex = constraint.get("vertex_name", "")
                if (constraint.get("fu_name") == model
                        and vertex.startswith(function + "_")
                        and f"Processing operation: {operation} - {vertex} " in log):
                    vertices.add(vertex)
        if not vertices:
            fail(f"no {operation} in {function} is assigned to {model}")

        for vertex in vertices:
            pattern = re.compile(
                rf"Analyzing operation {re.escape(vertex)} [^\n]*\n"
                rf"\s+initiation_time {expected_ii}\n"
                rf"\s+Analyzed operation {re.escape(vertex)}"
            )
            if pattern.search(log):
                print(f"PASS: {vertex} assigned to {model}, II={expected_ii}, latency={expected_latency}")
                return
        fail(f"buildFSM did not report initiation_time {expected_ii} for assigned {operation} {sorted(vertices)}")

    # Release builds omit the detailed scheduling XML and operation traces.
    # Keep all three checks explicit: the selected cell metadata, the model
    # responsible for the minimum loop II, and the emitted FU instance.
    loop_ii = re.compile(
        rf"Initiation Interval \(II\) for function {re.escape(function)}:BB\d+ = {expected_ii}(?:\n|$)"
    )
    if not loop_ii.search(log):
        fail(f"Bambu log lacks loop II={expected_ii} for {function}")
    model_ii = re.compile(
        rf"Minimum II={expected_ii} for \(BB\d+\) due to multi-cycles operations: "
        rf"{re.escape(model)}_[0-9]+(?:\s|$)"
    )
    if not model_ii.search(log):
        fail(f"Bambu log does not attribute Minimum II={expected_ii} to {model}")

    template = matching_models[0].find("template")
    template_name = template.get("name", "") if template is not None else ""
    rtl_paths = list(case_dir.glob("*.v"))
    rtl_text = "\n".join(path.read_text(encoding="utf-8", errors="replace") for path in rtl_paths)
    fu_instance = re.compile(
        rf"(?ms)^\s*{re.escape(template_name)}\s+#\((.*?)\)\s+[A-Za-z_]\w*\s*\("
    ) if template_name else None
    if not rtl_paths or not fu_instance or not any(
        re.search(rf"\.PIPE_PARAMETER\({re.escape(expected_pipe)}\)", match.group(1))
        for match in fu_instance.finditer(rtl_text)
    ):
        fail(f"generated RTL lacks an instance of {template_name} with PIPE_PARAMETER={expected_pipe}")
    print(f"PASS: {operation} assigned to {model}, Minimum II={expected_ii}, latency={expected_latency}")


if __name__ == "__main__":
    main()
