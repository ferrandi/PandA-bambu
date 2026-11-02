#!/usr/bin/env python3
import argparse
import math
import re
import shlex
import sys
import xml.etree.ElementTree as ET
from pathlib import Path


NUMBER = r"[+-]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][+-]?\d+)?"
SLACK_TOLERANCE_NS = 0.0011


def fail(message):
   print(f"FAIL: {message}", file=sys.stderr)
   return 1


def positive_finite(value, label):
   try:
      number = float(value)
   except (TypeError, ValueError):
      raise ValueError(f"{label} is missing or not numeric: {value!r}")
   if not math.isfinite(number) or number <= 0:
      raise ValueError(f"{label} must be finite and positive, got {value!r}")
   return number


def main():
   parser = argparse.ArgumentParser(description="Check a Mantis Vivado timing evaluation")
   parser.add_argument("--writer", choices=("V", "H"), required=True)
   args = parser.parse_args()

   case_dir = Path.cwd()
   results_path = case_dir / "bambu_results.xml"
   try:
      root = ET.parse(results_path).getroot()
   except (OSError, ET.ParseError) as error:
      return fail(f"cannot parse {results_path}: {error}")

   evaluations = root.findall(".//evaluation")
   if len(evaluations) != 1:
      return fail(f"expected one evaluation record, found {len(evaluations)}")
   evaluation = evaluations[0]
   try:
      xml_slack = positive_finite(evaluation.get("CLOCK_SLACK"), "CLOCK_SLACK")
   except ValueError as error:
      return fail(str(error))

   try:
      command = shlex.split(root.get("args", ""))
   except ValueError as error:
      return fail(f"cannot parse Bambu command in {results_path}: {error}")
   writers = [token.split("=", 1)[1] for token in command if token.startswith("--writer=")]
   if writers != [args.writer]:
      return fail(f"expected command writer {args.writer}, found {writers}")

   simulation = root.find("./timing/simulation")
   if simulation is None or simulation.get("return_value") != "0":
      return fail("simulation is missing or did not return 0")
   runs = simulation.findall("./run")
   if not runs:
      return fail("simulation has no recorded runs")
   for index, run in enumerate(runs):
      try:
         positive_finite(run.text, f"simulation run {index + 1}")
      except ValueError as error:
         return fail(str(error))
   for metric in ("CYCLES", "TOTAL_CYCLES"):
      try:
         positive_finite(evaluation.get(metric), metric)
      except ValueError as error:
         return fail(str(error))

   outputs = root.find("./outputs")
   if outputs is None:
      return fail("results XML has no outputs record")
   output_files = {Path(node.text.strip()).name for node in outputs.findall("./file") if node.text}
   library = "panda_libtech.v" if args.writer == "V" else "panda_libtech.vhd"
   if library not in output_files or not (case_dir / library).is_file():
      return fail(f"expected generated {library} for writer {args.writer}")

   flow_dir = case_dir / "HLS_output" / "xilinx" / "flow_backend"
   timing_path = flow_dir / "post_route_timing_summary.rpt"
   status_path = flow_dir / "post_route_status.rpt"
   try:
      timing_report = timing_path.read_text(encoding="utf-8", errors="replace")
      route_status = status_path.read_text(encoding="utf-8", errors="replace")
   except OSError as error:
      return fail(f"missing post-route evidence: {error}")

   if not re.search(r"(?m)^\s*\|\s*Design State\s*:\s*Routed\s*$", timing_report):
      return fail("timing summary does not identify a routed design")
   setup_slacks = re.findall(
      rf"(?m)^\s*Setup\s*:\s*\d+\s+Failing Endpoints,\s*Worst Slack\s+({NUMBER})ns\b",
      timing_report,
   )
   if len(setup_slacks) != 1:
      return fail(f"expected one routed setup Worst Slack value, found {len(setup_slacks)}")
   try:
      route_slack = positive_finite(setup_slacks[0], "routed setup Worst Slack")
   except ValueError as error:
      return fail(str(error))
   if abs(route_slack - xml_slack) > SLACK_TOLERANCE_NS:
      return fail(
         f"CLOCK_SLACK {xml_slack:.12g} ns differs from routed report {route_slack:.12g} ns"
      )

   routing_errors = re.findall(r"(?m)^\s*# of nets with routing errors\.*\s*:\s*(\d+)\s*:", route_status)
   if len(routing_errors) != 1 or int(routing_errors[0]) != 0:
      return fail(f"expected zero routing errors, found {routing_errors}")

   print(
      f"PASS: writer {args.writer}, routed setup slack {route_slack:.3f} ns, "
      f"simulation cycles {evaluation.get('CYCLES')} (total {evaluation.get('TOTAL_CYCLES')})"
   )
   return 0


if __name__ == "__main__":
   sys.exit(main())
