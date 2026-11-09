#!/usr/bin/env python3
"""Behavioral smoke tests for the read-core ModelSim runner using fake PATH tools."""

from __future__ import annotations

import csv
import os
from pathlib import Path
import subprocess
import tempfile
import unittest


REPO = Path(__file__).resolve().parents[4]
RTL_TESTS = Path(__file__).resolve().parent
RUNNER = RTL_TESTS / "run_read_burst_modelsim.sh"
TMP_ROOT = Path(os.environ.get(
    "BURST_TEST_TMPDIR", Path.cwd()
)) / f"out_{Path(__file__).stem}"


VSIM = r'''#!/usr/bin/env python3
import os, pathlib, sys
args = sys.argv[1:]
if "-version" in args:
    print("QuestaSim fake 2026")
    raise SystemExit(0)
with open(os.environ["FAKE_MODELSIM_CALLS"], "a", encoding="utf-8") as stream:
    stream.write("vsim " + " ".join(args) + "\n")
case = next((arg.split("=", 1)[1] for arg in args if arg.startswith("+CASE=")), "")
top = next((arg for arg in args if arg.startswith("work.")), "")
expected_read_faults = {
    "ERROR": ["RRESP"],
    "BADALIGN": ["CONFIGURE_INVALID"], "OVERFLOW": ["CONFIGURE_INVALID"],
    "BADSIZE_CONFIG": ["CONFIGURE_INVALID"], "STORE": ["UNSUPPORTED_STORE"],
    "PENDING_SECOND": ["CONFIGURE_INVALID"],
    "COUNT_TRUNC_64": ["CONFIGURE_INVALID"], "RANGE_TOP_COUNT": ["CONFIGURE_INVALID"],
    "RANGE_LAST_BEAT_2": ["CONFIGURE_INVALID"],
    "RANGE_HIGH_MAX_CARRY_NONZERO": ["CONFIGURE_INVALID"],
    "RANGE_ZERO_MISALIGNED": ["CONFIGURE_INVALID"],
    "FALLBACK_WIDE_IN4": ["CONFIGURE_INVALID"],
    "RANGE_OVER_BASE4": ["CONFIGURE_INVALID"], "RANGE_OVER_PLUS1": ["CONFIGURE_INVALID"],
    "RANGE_OVER_HALF": ["CONFIGURE_INVALID"], "RANGE_OVER_MAX": ["CONFIGURE_INVALID"],
    "RANGE_TOP_OVER": ["CONFIGURE_INVALID"],
    "BADADDR_EMPTY": ["LOAD_INVALID"], "BADSIZE_EMPTY": ["LOAD_INVALID"],
    "BADADDR_FULL": ["LOAD_INVALID"], "BADSIZE_FULL": ["LOAD_INVALID"],
    "ADJACENT_CFG_TRUNC": ["CONFIGURE_INVALID"], "ADJACENT_CFG_SIZE": ["CONFIGURE_INVALID"],
    "BADRID": ["RID"], "FAULT_STALLED_AR": ["RID"],
    "EARLY_RLAST": ["RLAST_MISMATCH"], "MISSING_RLAST": ["RLAST_MISMATCH"],
    "CONFIG_PIPELINE_INVALID": ["CONFIGURE_INVALID"],
    "RRESP_FIRST": ["RRESP"], "RRESP_INTERMEDIATE": ["RRESP"], "RRESP_FINAL": ["RRESP"],
    "RRESP_STALLED_AR": ["RRESP"], "RRESP_RESERVATION_RACE": ["RRESP"],
    "RREADY_SIDEBAND": ["RRESP"],
}
fault_reasons = ["CONFIGURE_INVALID"] * 15 if case == "RANGE_ORACLE" else expected_read_faults.get(case, [])
if fault_reasons and "+AXI_BURST_EXPECT_FAULT" not in args:
    print("** Fatal: AXI_BURST_FAULT engine=read reason=" + fault_reasons[0])
    raise SystemExit(1)
for reason in fault_reasons:
    diagnostic = ("AXI_BURST_FAULT engine=read reason=" + reason +
                  " instance=" + (top.partition("work.")[2] or "tb_burst_engine") +
                  ".dut.report_axi_read_diagnostic opcode=-1 size=0 "
                  "address=00001000 count=1 resp=00 id=0 last=0 descriptors=1 remaining=1")
    mode = os.environ.get("FAKE_MODELSIM_MODE")
    if mode == "wrong-instance":
        top_name = top.partition("work.")[2] or "tb_burst_engine"
        diagnostic = diagnostic.replace(top_name + ".dut", top_name + ".other")
    elif mode == "missing-fault-field":
        diagnostic = diagnostic.replace(" id=0", "")
    print(diagnostic)
if case == "SMOKE":
    max_out = next((arg.split("=", 1)[1] for arg in args if arg.startswith("-gMAX_OUTSTANDING=")), "1")
    depth = next((arg.split("=", 1)[1] for arg in args if arg.startswith("-gFIFO_DEPTH=")), "256")
    if max_out not in [str(n) for n in range(1, 17)]:
        print("** Fatal: MAX_OUTSTANDING must be in [1,16]")
        if os.environ.get("FAKE_MODELSIM_MODE") == "extra-fatal": print("** Fatal: second fatal")
        marker_case = "SMOKE" if os.environ.get("FAKE_MODELSIM_MODE") == "invalid-final-smoke" else ""
        print("DUPLEX_READ PASS CASE=" + marker_case)
        raise SystemExit({"invalid-timeout": 124, "invalid-killed": 137, "invalid-crash": 139}.get(
            os.environ.get("FAKE_MODELSIM_MODE"), 1))
    try: d = int(depth)
    except ValueError: d = 0
    if d < 1 or d > 4096 or d & (d - 1):
        print("** Fatal: FIFO_DEPTH must be a power of two in [1,4096]")
        if os.environ.get("FAKE_MODELSIM_MODE") == "extra-fatal": print("** Fatal: second fatal")
        marker_case = "SMOKE" if os.environ.get("FAKE_MODELSIM_MODE") == "invalid-final-smoke" else ""
        print("DUPLEX_READ PASS CASE=" + marker_case)
        raise SystemExit({"invalid-timeout": 124, "invalid-killed": 137, "invalid-crash": 139}.get(
            os.environ.get("FAKE_MODELSIM_MODE"), 1))
if os.environ.get("FAKE_MODELSIM_MODE") == "unrelated-error":
    print("** Error: unrelated simulator error")
    raise SystemExit(1)
if top == "work.tb_configure_read_pipeline" and case == "READ_PEER_FAULT":
    diagnostic = ("AXI_BURST_PEER_FAULT engine=read reason=peer "
                  "instance=tb_configure_read_pipeline.dut.report_axi_read_diagnostic "
                  "opcode=-1 size=0 address=00000000 count=0 resp=00 id=0 last=0 "
                  "descriptors=0 remaining=0")
    if "+AXI_BURST_EXPECT_FAULT" not in args:
        print("** Fatal: " + diagnostic)
        raise SystemExit(1)
    mode = os.environ.get("FAKE_MODELSIM_MODE")
    if mode == "wrong-instance":
        diagnostic = diagnostic.replace(".dut.", ".other.")
    elif mode == "missing-fault-field":
        diagnostic = diagnostic.replace(" id=0", "")
    print(diagnostic)
if top == "work.tb_configure_read_pipeline":
    print("READ_CONFIG_PIPELINE PASS CASE=" + case)
else:
    marker_case = "WRONG" if os.environ.get("FAKE_MODELSIM_MODE") == "wrong-marker" else case
    print("DUPLEX_READ PASS CASE=" + marker_case)
    if case == "READY_CREDIT_PROOF":
        print("PASS READY_CREDIT_PROOF FULL O=16 D=32 AR=16")
        print("PASS READY_CREDIT_PROOF O=16 D=32 full-LOAD-credit-AR-response-drain N=1")
''' 


class ReadBurstModelSimRunnerTests(unittest.TestCase):
    def setUp(self) -> None:
        TMP_ROOT.mkdir(parents=True, exist_ok=True)
        self.temp = tempfile.TemporaryDirectory(prefix="read-runner-", dir=TMP_ROOT)
        self.root = Path(self.temp.name)
        self.bin = self.root / "bin"
        self.bin.mkdir()
        self.calls = self.root / "calls.log"
        for name in ("vsim", "vlib", "vmap", "vlog", "verilator"):
            path = self.bin / name
            if name == "vsim":
                path.write_text(VSIM, encoding="utf-8")
            else:
                path.write_text(
                    "#!/usr/bin/env bash\n"
                    "printf '%s %s\\n' \"$(basename \"$0\")\" \"$*\" >> \"$FAKE_MODELSIM_CALLS\"\n"
                    "exit 0\n",
                    encoding="utf-8",
                )
            path.chmod(0o755)
        self.env = os.environ.copy()
        self.env.update(
            PATH=f"{self.bin}:{self.env['PATH']}",
            TMPDIR=str(self.root), TMP=str(self.root), TEMP=str(self.root),
            FAKE_MODELSIM_CALLS=str(self.calls),
        )

    def tearDown(self) -> None:
        self.temp.cleanup()

    def test_profile_manifest_preserves_the_parameter_matrix(self) -> None:
        manifest = RTL_TESTS / "read_burst_profiles.tsv"
        with manifest.open(encoding="utf-8", newline="") as stream:
            rows = list(csv.reader((line for line in stream if not line.startswith("#")), delimiter="\t"))
        self.assertEqual(len(rows), 23)
        self.assertEqual(len({row[1] for row in rows}), len(rows))
        self.assertEqual(
            {row[1] for row in rows},
            {
                "B1-O1-D256", "B2-O1-D256", "B16-O1-D256", "B256-O1-D256",
                "B16-O3-D64", "B16-O15-D256", "B16-O16-D256", "B16-O2-D64",
                "B16-O3-D64-PENDING", "B16-O3-D1", "B16-O3-D2", "B16-O3-D16",
                "B16-O3-D64-SKID", "B16-O3-D512", "B16-O3-D4096",
                "B16-O16-D32", "B256-O2-D8", "B16-O1-D1", "B1-O1-D1", "B1-O3-D8",
                "B16-O1-D4096", "B1-O16-D16", "B1-O3-D8-FAULTS",
            },
        )
        self.assertTrue(all(len(row) == 7 for row in rows))
        self.assertTrue(all(row[5] in {"yes", "no"} for row in rows))
        self.assertTrue(all(int(row[2]) <= 256 and int(row[3]) <= 16 and int(row[4]) <= 4096 for row in rows))
        self.assertEqual(rows[2][6].split()[0], "ERROR")

    def test_duplicate_manifest_id_is_rejected_before_modelsim(self) -> None:
        manifest = RTL_TESTS / "read_burst_profiles.tsv"
        lines = manifest.read_text(encoding="utf-8").splitlines()
        duplicate = self.root / "duplicate-profiles.tsv"
        first_row = next(line for line in lines if line and not line.startswith("#"))
        duplicate.write_text("\n".join(lines + [first_row, ""]), encoding="utf-8")
        output = self.root / "duplicate-manifest"
        env = self.env | {"BURST_READ_PROFILE_MANIFEST": str(duplicate)}
        result = subprocess.run(
            ["bash", str(RUNNER), str(output), "credits"], cwd=REPO, env=env,
            text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=False,
        )
        self.assertEqual(result.returncode, 2, result.stderr)
        self.assertIn("invalid or duplicate reader profile manifest row", result.stderr)
        self.assertFalse(self.calls.exists())

    def run_runner(self, name: str, suite: str, mode: str = "") -> subprocess.CompletedProcess[str]:
        out = self.root / name
        env = self.env.copy()
        if mode:
            env["FAKE_MODELSIM_MODE"] = mode
        return subprocess.run(
            ["bash", str(RUNNER), str(out), suite],
            cwd=REPO,
            env=env,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            timeout=60,
            check=False,
        )

    def test_credit_witness_markers_and_configure_child_pass(self) -> None:
        result = self.run_runner("credits", "credits")
        self.assertEqual(result.returncode, 0, result.stderr)
        aggregate = (self.root / "credits" / "aggregate.log").read_text(encoding="utf-8")
        self.assertIn("PASS READY_CREDIT_PROOF FULL", aggregate)
        self.assertIn("full-LOAD-credit-AR-response-drain", aggregate)
        self.assertIn("DUPLEX_READ PASS CASE=CREDIT_LIMIT", aggregate)
        self.assertNotIn("verilator ", self.calls.read_text(encoding="utf-8"))

        configure_fault = (
            "AXI_BURST_PEER_FAULT engine=read reason=peer "
            "instance=tb_configure_read_pipeline.dut.report_axi_read_diagnostic "
            "opcode=-1 size=0 address=00000000 count=0 resp=00 id=0 last=0 "
            "descriptors=0 remaining=0"
        )
        configure = subprocess.run(
            ["bash", str(RTL_TESTS / "run_configure_read_pipeline.sh"), str(self.root / "configure")],
            cwd=REPO, env=self.env, text=True, stdout=subprocess.PIPE,
            stderr=subprocess.PIPE, timeout=30, check=False,
        )
        self.assertEqual(configure.returncode, 0, configure.stderr)
        self.assertIn("READ_CONFIG_PIPELINE PASS CASE=READ_RESET", configure.stdout)
        self.assertIn("READ_CONFIG_PIPELINE PASS CASE=READ_PEER_FAULT", configure.stdout)
        self.assertIn(configure_fault, configure.stdout)
        self.assertIn("+CASE=READ_PEER_FAULT +AXI_BURST_EXPECT_FAULT", self.calls.read_text(encoding="utf-8"))

    def test_configure_peer_fault_rejects_wrong_diagnostic(self) -> None:
        env = self.env.copy()
        env["FAKE_MODELSIM_MODE"] = "wrong-instance"
        result = subprocess.run(
            ["bash", str(RTL_TESTS / "run_configure_read_pipeline.sh"), str(self.root / "bad-configure")],
            cwd=REPO, env=env, text=True, stdout=subprocess.PIPE,
            stderr=subprocess.PIPE, timeout=30, check=False,
        )
        self.assertNotEqual(result.returncode, 0)

    def test_relative_path_tools_survive_runner_build_directory_changes(self) -> None:
        env = self.env.copy()
        env["PATH"] = f"bin:{env['PATH']}"
        output = self.root / "relative-path-run"
        result = subprocess.run(
            ["/bin/bash", str(RUNNER), str(output), "credits"],
            cwd=self.root, env=env, text=True, stdout=subprocess.PIPE,
            stderr=subprocess.PIPE, timeout=60, check=False,
        )
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        aggregate = (output / "aggregate.log").read_text(encoding="utf-8")
        self.assertIn("DUPLEX_READ PASS CASE=CREDIT_LIMIT", aggregate)

    def test_invalid_parameter_cases_require_named_single_fatal_and_smoke_selector(self) -> None:
        for suffix, mode in (("", ""), ("-final-smoke", "invalid-final-smoke")):
            result = self.run_runner("bounds" + suffix, "bounds", mode)
            self.assertEqual(result.returncode, 0, result.stderr)
            for name in ("O0", "O17", "D0", "D3", "D8192"):
                self.assertIn(f"PASS invalid parameters {name} rejected", result.stdout)
        calls = self.calls.read_text(encoding="utf-8")
        for parameter_case in ("-gMAX_OUTSTANDING=0", "-gMAX_OUTSTANDING=17", "-gFIFO_DEPTH=0",
                               "-gFIFO_DEPTH=3", "-gFIFO_DEPTH=8192"):
            self.assertIn(parameter_case, calls)
        for line in calls.splitlines():
            if "-gMAX_OUTSTANDING=0" in line or "-gMAX_OUTSTANDING=17" in line or \
                    any(value in line for value in ("-gFIFO_DEPTH=0", "-gFIFO_DEPTH=3", "-gFIFO_DEPTH=8192")):
                self.assertIn("+CASE=SMOKE", line)
        self.assertIn("+CASE=COUNT_TRUNC_64", calls)
        self.assertIn("-gADDR_BITS=64", calls)
        self.assertIn("-gIN_BITS=32", calls)
        self.assertIn("-gIN_BITS=65", calls)
        self.assertNotIn("verilator ", calls)

    def test_missing_marker_unrelated_error_and_multiple_fatals_fail_closed(self) -> None:
        marker = self.run_runner("bad-marker", "credits", "wrong-marker")
        self.assertNotEqual(marker.returncode, 0)
        self.assertIn("missing exact PASS marker", marker.stderr)

        error = self.run_runner("bad-error", "bounds", "unrelated-error")
        self.assertNotEqual(error.returncode, 0)
        self.assertIn("unrelated simulator error", error.stderr)

        extra = self.run_runner("bad-fatal", "bounds", "extra-fatal")
        self.assertNotEqual(extra.returncode, 0)
        self.assertIn("fatals=2", extra.stderr)

    def test_invalid_profile_fatal_with_timeout_or_crash_status_is_rejected(self) -> None:
        for mode, status in (("invalid-timeout", 124), ("invalid-killed", 124), ("invalid-crash", 139)):
            with self.subTest(status=status):
                result = self.run_runner(f"invalid-status-{mode}", "bounds", mode)
                self.assertNotEqual(result.returncode, 0)
                self.assertIn(f"status={status}", result.stderr)

    def test_fault_containment_scenarios_use_plusarg_and_exact_reason_sets(self) -> None:
        result = self.run_runner("transactions", "transactions")
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        calls = self.calls.read_text(encoding="utf-8").splitlines()
        fault_cases = (
            "ERROR", "BADALIGN", "OVERFLOW", "BADSIZE_CONFIG", "STORE", "RANGE_OVER_BASE4",
            "RANGE_OVER_PLUS1", "RANGE_OVER_HALF", "RANGE_OVER_MAX", "RANGE_TOP_OVER",
            "RANGE_ORACLE", "BADADDR_EMPTY", "BADSIZE_EMPTY", "BADADDR_FULL", "BADSIZE_FULL",
            "ADJACENT_CFG_TRUNC", "ADJACENT_CFG_SIZE", "PENDING_SECOND", "BADRID", "FAULT_STALLED_AR",
            "EARLY_RLAST", "MISSING_RLAST", "CONFIG_PIPELINE_INVALID", "RRESP_FIRST",
            "RRESP_INTERMEDIATE", "RRESP_FINAL", "RRESP_STALLED_AR", "RRESP_RESERVATION_RACE",
            "RREADY_SIDEBAND",
        )
        for case in fault_cases:
            matching = [line for line in calls if f"+CASE={case}" in line]
            self.assertTrue(matching, f"scenario did not run: {case}")
            self.assertTrue(all("+AXI_BURST_EXPECT_FAULT" in line for line in matching), case)
        normal = [line for line in calls if "+CASE=CONTINUOUS_CFG" in line]
        self.assertTrue(normal)
        self.assertTrue(all("+AXI_BURST_EXPECT_FAULT" not in line for line in normal))

    def test_count_trunc_faults_use_plusarg_and_exact_top_instance(self) -> None:
        result = self.run_runner("count-trunc", "bounds")
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        calls = self.calls.read_text(encoding="utf-8").splitlines()
        aggregate = (self.root / "count-trunc" / "aggregate.log").read_text(encoding="utf-8")
        fault_cases = (
            "COUNT_TRUNC_64", "RANGE_TOP_COUNT", "RANGE_LAST_BEAT_2",
            "RANGE_HIGH_MAX_CARRY_NONZERO", "RANGE_ZERO_MISALIGNED", "FALLBACK_WIDE_IN4",
        )
        self.assertEqual(aggregate.count(
            "AXI_BURST_FAULT engine=read reason=CONFIGURE_INVALID "
            "instance=tb_count_trunc.dut.report_axi_read_diagnostic"), len(fault_cases))
        for case in fault_cases:
            with self.subTest(case=case):
                matching = [line for line in calls if f"+CASE={case}" in line]
                self.assertTrue(matching, f"scenario did not run: {case}")
                self.assertTrue(all("+AXI_BURST_EXPECT_FAULT" in line for line in matching), case)
        wrong_instance = self.run_runner("count-trunc-wrong-instance", "bounds", "wrong-instance")
        self.assertNotEqual(wrong_instance.returncode, 0)
        self.assertIn("wrong instance or incomplete payload schema", wrong_instance.stderr)

    def test_fault_containment_rejects_wrong_instance_and_missing_payload_field(self) -> None:
        for mode in ("wrong-instance", "missing-fault-field"):
            with self.subTest(mode=mode):
                result = self.run_runner(mode, "transactions", mode)
                self.assertNotEqual(result.returncode, 0)
                self.assertIn("wrong instance or incomplete payload schema", result.stderr)


if __name__ == "__main__":
    unittest.main(verbosity=2)
