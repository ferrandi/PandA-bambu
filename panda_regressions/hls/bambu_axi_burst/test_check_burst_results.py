"""Offline fail-closed tests for the Mantis cycle/profile gate."""

from __future__ import annotations

import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest


HERE = Path(__file__).resolve().parent
REPO_ROOT = HERE.parents[2]
TMP_ROOT = REPO_ROOT / "documentation" / "tmp"
CHECKER = HERE / "check_burst_results.py"
HEADER = "case\tcycles\tsource\tburst_max\toutstanding\tfifo_depth\taddr_bits\n"
CASES = [(f"sum-o{o}-n{n}", 1092 if n == 1024 else 2116, o)
         for n in (1024, 2048) for o in (5, 8, 16)]


class BurstResultGateTests(unittest.TestCase):
    def setUp(self):
        TMP_ROOT.mkdir(parents=True, exist_ok=True)
        self.temp = tempfile.TemporaryDirectory(prefix="test-bambu-axi-burst-gate-", dir=TMP_ROOT)
        self.root = Path(self.temp.name)
        self.fixture = self.root / "fixture"
        self.fixture.mkdir()
        self.expectations = self.fixture / "cycle_expectations.tsv"
        rows = []
        self.results = self.root / "results"
        for case, cycles, outstanding in CASES:
            source = f"profile_o{outstanding}.cpp"
            (self.fixture / source).write_text(
                "#pragma HLS interface mode=m_axi max_read_burst_length=16 "
                f"num_read_outstanding={outstanding} read_fifo_depth=256\n", encoding="utf-8")
            rows.append(f"{case}\t{cycles}\t{source}\t16\t{outstanding}\t256\t32\n")
            case_dir = self.results / "AXI-BURST" / case
            case_dir.mkdir(parents=True)
            (case_dir / "bambu_results.xml").write_text(
                f'<results><evaluation CYCLES="{cycles}"/></results>\n', encoding="utf-8")
            (case_dir / "kernel.v").write_text(
                f"MinimalAXI4MasterPipelined #(.B_MAX(16), .MAX_OUTSTANDING({outstanding}), "
                ".FIFO_DEPTH(256), .BITSIZE_in1(2), .BITSIZE_in2(32), .BITSIZE_in3(32), "
                ".BITSIZE_in4(32), .BITSIZE_in5(32), .BITSIZE_out1(32), "
                ".BITSIZE_m_axi_araddr(32), .BITSIZE_m_axi_arlen(8), .BITSIZE_m_axi_arsize(3), "
                ".BITSIZE_m_axi_arburst(2), .BITSIZE_m_axi_arid(1), .BITSIZE_m_axi_rdata(32), "
                ".BITSIZE_m_axi_rresp(2), .BITSIZE_m_axi_rid(1)) engine ();\n", encoding="utf-8")
        self.expectations.write_text(HEADER + "".join(rows), encoding="utf-8")
        (self.fixture / "bambu_axi_burst_boundary.cpp").write_text(
            "#pragma HLS interface mode=m_axi max_read_burst_length=256 "
            "num_read_outstanding=16 read_fifo_depth=4096\n", encoding="utf-8")
        boundary = self.results / "AXI-BURST" / "valid-boundary-maxima"
        boundary.mkdir()
        (boundary / "bambu_results.xml").write_text(
            "<results><evaluation CYCLES=\"3\"/></results>\n", encoding="utf-8")
        (boundary / "kernel.v").write_text(
            "MinimalAXI4MasterPipelined #(.B_MAX(256), .MAX_OUTSTANDING(16), "
            ".FIFO_DEPTH(4096), .BITSIZE_in1(2), .BITSIZE_in2(32), .BITSIZE_in3(32), "
            ".BITSIZE_in4(32), .BITSIZE_in5(32), .BITSIZE_out1(32), "
            ".BITSIZE_m_axi_araddr(32), .BITSIZE_m_axi_arlen(8), .BITSIZE_m_axi_arsize(3), "
            ".BITSIZE_m_axi_arburst(2), .BITSIZE_m_axi_arid(1), .BITSIZE_m_axi_rdata(32), "
            ".BITSIZE_m_axi_rresp(2), .BITSIZE_m_axi_rid(1)) engine ();\n", encoding="utf-8")

    def tearDown(self):
        self.temp.cleanup()

    def run_gate(self):
        return subprocess.run(
            [sys.executable, "-B", str(CHECKER), "--results", str(self.results),
             "--expectations", str(self.expectations)], text=True, capture_output=True,
            env=os.environ.copy(), check=False)

    def test_valid_cycle_and_profile_pass(self):
        result = self.run_gate()
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("PASS: 6 cases", result.stdout)

    def test_wrong_cycle_count_fails(self):
        case = self.results / "AXI-BURST" / CASES[0][0]
        (case / "bambu_results.xml").write_text(
            '<results><evaluation CYCLES="1093"/></results>\n', encoding="utf-8")
        self.assertNotEqual(self.run_gate().returncode, 0)

    def test_missing_width_parameters_fail(self):
        # An instance that only carries the three profile parameters leaves the
        # port widths at their XML defaults, which is not what the generator
        # emits and is not a witness that the widths were wired.
        case = self.results / "AXI-BURST" / CASES[0][0]
        (case / "kernel.v").write_text(
            "MinimalAXI4MasterPipelined #(.B_MAX(16), .MAX_OUTSTANDING(5), "
            ".FIFO_DEPTH(256)) engine ();\n", encoding="utf-8")
        self.assertIn("does not contain the expected burst profile", self.run_gate().stderr)

    def test_truncated_count_width_fails(self):
        # Reading the count width from the wrong operand yields a one-bit
        # BITSIZE_in5; the engine then fetches a single element and the
        # co-simulation hangs instead of finishing.
        case = self.results / "AXI-BURST" / CASES[0][0]
        (case / "kernel.v").write_text(
            "MinimalAXI4MasterPipelined #(.B_MAX(16), .MAX_OUTSTANDING(5), "
            ".FIFO_DEPTH(256), .BITSIZE_in5(1), .BITSIZE_m_axi_araddr(32), "
            ".BITSIZE_m_axi_rdata(32)) engine ();\n", encoding="utf-8")
        self.assertIn("does not contain the expected burst profile", self.run_gate().stderr)

    def test_missing_xml_fails(self):
        case = self.results / "AXI-BURST" / CASES[0][0]
        (case / "bambu_results.xml").unlink()
        self.assertIn("missing Bambu results XML", self.run_gate().stderr)

    def test_malformed_xml_fails(self):
        case = self.results / "AXI-BURST" / CASES[0][0]
        (case / "bambu_results.xml").write_text("<results>", encoding="utf-8")
        self.assertIn("malformed/unreadable", self.run_gate().stderr)

    def test_missing_expected_case_fails(self):
        shutil.rmtree(self.results / "AXI-BURST" / CASES[0][0])
        self.assertIn("missing Bambu results XML", self.run_gate().stderr)

    def test_missing_cycles_attribute_fails(self):
        case = self.results / "AXI-BURST" / CASES[0][0]
        (case / "bambu_results.xml").write_text("<results><evaluation/></results>", encoding="utf-8")
        self.assertIn("expected exactly one evaluation CYCLES", self.run_gate().stderr)

    def test_malformed_cycles_value_fails(self):
        case = self.results / "AXI-BURST" / CASES[0][0]
        (case / "bambu_results.xml").write_text(
            '<results><evaluation CYCLES="1092x"/></results>', encoding="utf-8")
        self.assertIn("malformed evaluation CYCLES", self.run_gate().stderr)

    def test_missing_expected_profile_fails(self):
        (self.fixture / "profile_o5.cpp").unlink()
        self.assertIn("expected profile source is missing", self.run_gate().stderr)

    def test_wrong_generated_rtl_profile_fails(self):
        case = self.results / "AXI-BURST" / CASES[0][0]
        rtl = (case / "kernel.v").read_text(encoding="utf-8")
        (case / "kernel.v").write_text(rtl.replace("FIFO_DEPTH(256)", "FIFO_DEPTH(128)"),
                                        encoding="utf-8")
        self.assertIn("does not contain the expected burst profile", self.run_gate().stderr)

    def add_latency_case(self, case: str, latency: str, read_delay: str, write_delay: str) -> None:
        """Append a gate case whose bundle declares 'latency' and whose testbench memory
        model is driven with the delays named by the caller."""

        source = f"latency_o1_{latency}.cpp"
        (self.fixture / source).write_text(
            "#pragma HLS interface mode=m_axi max_read_burst_length=16 "
            f"num_read_outstanding=1 read_fifo_depth=256 latency={latency}\n", encoding="utf-8")
        self.expectations.write_text(
            self.expectations.read_text(encoding="utf-8") + f"{case}\t7\t{source}\t16\t1\t256\t32\n",
            encoding="utf-8")
        case_dir = self.results / "AXI-BURST" / case
        case_dir.mkdir(parents=True)
        (case_dir / "bambu_results.xml").write_text(
            '<results><evaluation CYCLES="7"/></results>\n', encoding="utf-8")
        (case_dir / "kernel.v").write_text(
            "MinimalAXI4MasterPipelined #(.B_MAX(16), .MAX_OUTSTANDING(1), .FIFO_DEPTH(256), "
            ".BITSIZE_in1(2), .BITSIZE_in2(32), .BITSIZE_in3(32), .BITSIZE_in4(32), "
            ".BITSIZE_in5(32), .BITSIZE_out1(32), .BITSIZE_m_axi_araddr(32), "
            ".BITSIZE_m_axi_arlen(8), .BITSIZE_m_axi_arsize(3), .BITSIZE_m_axi_arburst(2), "
            ".BITSIZE_m_axi_arid(1), .BITSIZE_m_axi_rdata(32), .BITSIZE_m_axi_rresp(2), "
            ".BITSIZE_m_axi_rid(1)) engine ();\n", encoding="utf-8")
        (case_dir / "bambu_testbench.v").write_text(
            f"TestbenchMEMAXI #(.index(3), .WRITE_DELAY({write_delay}), .READ_DELAY({read_delay}) "
            ") axi4_tb ();\n", encoding="utf-8")

    def test_bundle_latency_reaches_the_memory_model(self):
        self.add_latency_case("lat-o1-n1024", "128", "128", "128")
        result = self.run_gate()
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_bundle_latency_ignored_by_the_memory_model_fails(self):
        # The pragma says 128, but the generated model still answers at the global
        # 64/1: the attribute never reached the testbench, which must be a failure.
        self.add_latency_case("lat-o1-n1024", "128", "64", "1")
        self.assertIn("carries no .READ_DELAY(128)", self.run_gate().stderr)

    def test_bundle_latency_missing_from_write_side_fails(self):
        self.add_latency_case("lat-o1-n1024", "128", "128", "1")
        self.assertIn("carries no .WRITE_DELAY(128)", self.run_gate().stderr)


if __name__ == "__main__":
    unittest.main()
