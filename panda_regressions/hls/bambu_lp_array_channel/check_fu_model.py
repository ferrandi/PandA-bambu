#!/usr/bin/env python3
# Copyright (C) 2026 Politecnico di Milano
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
"""Check the selected FU model, assignment, and FSM initiation interval."""

import os
from pathlib import Path
import re
import sys
import xml.etree.ElementTree as ET


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
        fail("Bambu did not emit technology_out.xml")

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
    if not scheduling_files:
        fail("Bambu did not emit a scheduling XML")
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


if __name__ == "__main__":
    main()
