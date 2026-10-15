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
HEADER = "case\tmode\tcycles\tsource\tburst_max\toutstanding\tfifo_depth\taddr_bits\n"
LEGACY_HEADER = "case\tcycles\tsource\n"
CASES = [(f"sum-o{o}-n{n}", 1092 if n == 1024 else 2116, o)
         for n in (1024, 2048) for o in (5, 8, 16)]
REQUIRED_READ_CASES = (
    ("sum-o1-n1024", 5124, 32, "sum_o1_n1024.cpp", "num_read_outstanding=1 read_fifo_depth=256"),
    ("sum-o1-n2048", 10244, 32, "sum_o1_n2048.cpp", "num_read_outstanding=1 read_fifo_depth=256"),
    ("sum-o1-n1024-addr64", 5124, 64, "sum_o1_addr64.cpp", "num_read_outstanding=1 read_fifo_depth=256"),
    ("sum-o1-n1024-lat128", 9220, 32, "sum_o1_lat128.cpp", "num_read_outstanding=1 read_fifo_depth=256 latency=128"),
)
REQUIRED_WRITE_CASES = (
    ("write-burst-n1024", 2068, 32),
    ("write-burst-n2048", 4116, 32),
    ("write-burst-n1024-addr64", 2068, 64),
)


class BurstResultGateTests(unittest.TestCase):
    def setUp(self):
        TMP_ROOT.mkdir(parents=True, exist_ok=True)
        self.temp = tempfile.TemporaryDirectory(prefix="test-bambu-axi-burst-gate-", dir=TMP_ROOT)
        self.root = Path(self.temp.name)
        self.fixture = self.root / "fixture"
        self.fixture.mkdir()
        self.expectations = self.fixture / "cycle_expectations.tsv"
        self.legacy_expectations = self.fixture / "legacy_write_expectations.tsv"
        rows = []
        self.results = self.root / "results"
        for case, cycles, outstanding in CASES:
            source = f"profile_o{outstanding}.cpp"
            (self.fixture / source).write_text(
                "#pragma HLS interface mode=m_axi max_read_burst_length=16 "
                f"num_read_outstanding={outstanding} read_fifo_depth=256\n", encoding="utf-8")
            rows.append(f"{case}\tread\t{cycles}\t{source}\t16\t{outstanding}\t256\t32\n")
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
        for case, cycles, addr_bits, source, attrs in REQUIRED_READ_CASES:
            (self.fixture / source).write_text(
                f"#pragma HLS interface mode=m_axi max_read_burst_length=16 {attrs}\n", encoding="utf-8")
            with self.expectations.open("a", encoding="utf-8") as stream:
                stream.write(f"{case}\tread\t{cycles}\t{source}\t16\t1\t256\t{addr_bits}\n")
            case_dir = self.results / "AXI-BURST" / case
            case_dir.mkdir(parents=True)
            (case_dir / "bambu_results.xml").write_text(
                f'<results><evaluation CYCLES="{cycles}"/></results>\n', encoding="utf-8")
            (case_dir / "kernel.v").write_text(
                f"MinimalAXI4MasterPipelined #(.B_MAX(16), .MAX_OUTSTANDING(1), "
                ".FIFO_DEPTH(256), .BITSIZE_in1(2), .BITSIZE_in2(32), .BITSIZE_in3(32), "
                f".BITSIZE_in4({addr_bits}), .BITSIZE_in5(32), .BITSIZE_out1(32), "
                f".BITSIZE_m_axi_araddr({addr_bits}), .BITSIZE_m_axi_arlen(8), "
                ".BITSIZE_m_axi_arsize(3), .BITSIZE_m_axi_arburst(2), .BITSIZE_m_axi_arid(1), "
                ".BITSIZE_m_axi_rdata(32), .BITSIZE_m_axi_rresp(2), .BITSIZE_m_axi_rid(1)) engine ();\n",
                encoding="utf-8")
            if case == "sum-o1-n1024-lat128":
                (case_dir / "bambu_testbench.v").write_text(
                    "TestbenchMEMAXI #(.READ_DELAY(128), .WRITE_DELAY(128)) axi4_tb ();\n",
                    encoding="utf-8")
        for case, cycles, addr_bits in REQUIRED_WRITE_CASES:
            self.add_write_case(case, str(cycles), awaddr_width=addr_bits)
        (self.fixture / "bambu_axi_burst_boundary.cpp").write_text(
            "#pragma HLS interface mode=m_axi max_read_burst_length=256 "
            "num_read_outstanding=16 read_fifo_depth=4096\n", encoding="utf-8")
        (self.fixture / "plain_m_axi_write.cpp").write_text(
            "#pragma HLS interface mode=m_axi port=dst\n", encoding="utf-8")
        self.legacy_expectations.write_text(
            LEGACY_HEADER + "plain-m-axi-write-n1024\t52\tplain_m_axi_write.cpp\n",
            encoding="utf-8")
        legacy = self.results / "AXI-BURST" / "plain-m-axi-write-n1024"
        legacy.mkdir(parents=True)
        (legacy / "bambu_results.xml").write_text(
            '<results><evaluation CYCLES="52"/></results>\n', encoding="utf-8")
        (legacy / "kernel.v").write_text("module kernel(input clk); endmodule\n", encoding="utf-8")
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
             "--expectations", str(self.expectations), "--legacy-expectations",
             str(self.legacy_expectations)], text=True, capture_output=True,
            env=os.environ.copy(), check=False)

    def test_valid_cycle_and_profile_pass(self):
        result = self.run_gate()
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("PASS: 10 read cases, 3 write cases, and 1 legacy cases", result.stdout)

    def test_each_required_write_case_cannot_be_removed_from_tsv(self):
        original = self.expectations.read_text(encoding="utf-8").splitlines()
        for case, _, _ in REQUIRED_WRITE_CASES:
            with self.subTest(case=case):
                self.expectations.write_text("\n".join(
                    line for line in original if not line.startswith(case + "\t")
                ) + "\n", encoding="utf-8")
                result = self.run_gate()
                self.assertNotEqual(result.returncode, 0)
                self.assertIn("expectation file does not cover required cycle cases", result.stderr)
        self.expectations.write_text("\n".join(original) + "\n", encoding="utf-8")

    def test_required_case_mode_must_match_inventory(self):
        self.expectations.write_text(self.expectations.read_text(encoding="utf-8").replace(
            "write-burst-n1024\twrite\t", "write-burst-n1024\tread\t", 1), encoding="utf-8")
        result = self.run_gate()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("required cycle case mode does not match inventory", result.stderr)

    def test_required_legacy_case_cannot_be_removed_from_tsv(self):
        self.legacy_expectations.write_text(LEGACY_HEADER, encoding="utf-8")
        result = self.run_gate()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("legacy expectation file contains no cases", result.stderr)

    def test_required_legacy_case_cannot_be_removed_with_other_rows_present(self):
        self.legacy_expectations.write_text(
            LEGACY_HEADER + "extra-legacy-case\t52\tplain_m_axi_write.cpp\n", encoding="utf-8")
        result = self.run_gate()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("legacy expectation file is missing required case: plain-m-axi-write-n1024",
                      result.stderr)

    def test_legacy_write_wrong_cycle_count_fails(self):
        case = self.results / "AXI-BURST" / "plain-m-axi-write-n1024"
        (case / "bambu_results.xml").write_text(
            '<results><evaluation CYCLES="53"/></results>\n', encoding="utf-8")
        self.assertIn("expected 52 cycles, got 53", self.run_gate().stderr)

    def test_legacy_write_rejects_configured_burst_core(self):
        case = self.results / "AXI-BURST" / "plain-m-axi-write-n1024"
        (case / "kernel.v").write_text(
            "MinimalAXI4MasterPipelined #(.B_MAX(16)) engine ();\n", encoding="utf-8")
        self.assertIn("unexpectedly contains configured burst core", self.run_gate().stderr)

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
        self.assertIn("does not contain the expected read burst profile", self.run_gate().stderr)

    def test_truncated_count_width_fails(self):
        # Reading the count width from the wrong operand yields a one-bit
        # BITSIZE_in5; the engine then fetches a single element and the
        # co-simulation hangs instead of finishing.
        case = self.results / "AXI-BURST" / CASES[0][0]
        (case / "kernel.v").write_text(
            "MinimalAXI4MasterPipelined #(.B_MAX(16), .MAX_OUTSTANDING(5), "
            ".FIFO_DEPTH(256), .BITSIZE_in5(1), .BITSIZE_m_axi_araddr(32), "
            ".BITSIZE_m_axi_rdata(32)) engine ();\n", encoding="utf-8")
        self.assertIn("does not contain the expected read burst profile", self.run_gate().stderr)

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
        self.assertIn("does not contain the expected read burst profile", self.run_gate().stderr)

    def add_latency_case(self, case: str, latency: str, read_delay: str, write_delay: str) -> None:
        """Append a gate case whose bundle declares 'latency' and whose testbench memory
        model is driven with the delays named by the caller."""

        source = f"latency_o1_{latency}.cpp"
        (self.fixture / source).write_text(
            "#pragma HLS interface mode=m_axi max_read_burst_length=16 "
            f"num_read_outstanding=1 read_fifo_depth=256 latency={latency}\n", encoding="utf-8")
        self.expectations.write_text(
            self.expectations.read_text(encoding="utf-8") + f"{case}\tread\t7\t{source}\t16\t1\t256\t32\n",
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

    def add_write_case(self, case="write-n1024", cycles="101", *, module="MinimalAXI4MasterPipelinedWrite",
                       profile=(16, 1, 32), count_width=32, awaddr_width=32,
                       expected_addr_width=None, include_rtl=True):
        source = f"writer_b16_o1_fifo32_{awaddr_width}.cpp"
        (self.fixture / source).write_text(
            "#pragma HLS interface mode=m_axi port=dst max_write_burst_length=16 "
            "write_burst_outstanding=1 write_fifo_depth=32\n", encoding="utf-8")
        self.expectations.write_text(
            self.expectations.read_text(encoding="utf-8") +
            f"{case}\twrite\t{cycles}\t{source}\t16\t1\t32\t"
            f"{expected_addr_width if expected_addr_width is not None else awaddr_width}\n",
            encoding="utf-8")
        case_dir = self.results / "AXI-BURST" / case
        case_dir.mkdir(parents=True)
        (case_dir / "bambu_results.xml").write_text(
            f'<results><evaluation CYCLES="{cycles}"/></results>\n', encoding="utf-8")
        if include_rtl:
            burst_max, outstanding, fifo = profile
            (case_dir / "kernel.v").write_text(
                f"{module} #(.B_MAX({burst_max}), .MAX_OUTSTANDING({outstanding}), "
                f".FIFO_DEPTH({fifo}), .BITSIZE_in4({awaddr_width}), .BITSIZE_in5({count_width}), "
                f".BITSIZE_m_axi_awaddr({awaddr_width}), .BITSIZE_in3(32), "
                ".BITSIZE_m_axi_wdata(32)) write_engine ();\n", encoding="utf-8")

    def test_valid_write_cycle_and_profile_pass(self):
        self.add_write_case()
        result = self.run_gate()
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("10 read cases, 4 write cases", result.stdout)

    def test_write_rejects_wrong_mode(self):
        self.add_write_case()
        self.expectations.write_text(self.expectations.read_text(encoding="utf-8").replace(
            "write-n1024\twrite\t", "write-n1024\tWRITE\t"), encoding="utf-8")
        self.assertIn("mode must be 'read' or 'write'", self.run_gate().stderr)

    def test_write_rejects_wrong_module(self):
        self.add_write_case(module="MinimalAXI4MasterPipelined")
        self.assertIn("expected write burst profile", self.run_gate().stderr)

    def test_write_rejects_wrong_profile(self):
        self.add_write_case(profile=(16, 2, 32))
        self.assertIn("expected write burst profile", self.run_gate().stderr)

    def test_write_rejects_truncated_count_width(self):
        self.add_write_case(count_width=1)
        self.assertIn("expected write burst profile", self.run_gate().stderr)

    def test_write_rejects_wrong_awaddr_width(self):
        self.add_write_case(awaddr_width=64, expected_addr_width=32)
        self.assertIn("expected write burst profile", self.run_gate().stderr)

    def test_write_rejects_missing_rtl(self):
        self.add_write_case(include_rtl=False)
        self.assertIn("generated RTL is missing", self.run_gate().stderr)

    def test_write_rejects_missing_cycles(self):
        self.add_write_case()
        case = self.results / "AXI-BURST" / "write-n1024"
        (case / "bambu_results.xml").write_text("<results><evaluation/></results>", encoding="utf-8")
        self.assertIn("expected exactly one evaluation CYCLES", self.run_gate().stderr)

    def test_split_profile_across_instances_is_rejected(self):
        self.add_write_case()
        case = self.results / "AXI-BURST" / "write-n1024"
        (case / "kernel.v").write_text(
            "MinimalAXI4MasterPipelinedWrite #(.B_MAX(16), .MAX_OUTSTANDING(1), "
            ".FIFO_DEPTH(32)) profile_engine ();\n"
            "MinimalAXI4MasterPipelinedWrite #(.BITSIZE_in4(32), .BITSIZE_in5(32), "
            ".BITSIZE_m_axi_awaddr(32), .BITSIZE_in3(32), .BITSIZE_m_axi_wdata(32)) width_engine ();\n",
            encoding="utf-8")
        self.assertIn("expected write burst profile", self.run_gate().stderr)

    def test_legacy_write_rejects_actual_write_burst_module_name(self):
        case = self.results / "AXI-BURST" / "plain-m-axi-write-n1024"
        (case / "kernel.v").write_text(
            "MinimalAXI4MasterPipelinedWrite #(.B_MAX(16)) engine ();\n", encoding="utf-8")
        self.assertIn("unexpectedly contains configured burst core", self.run_gate().stderr)

    def test_pending_write_signature_is_rejected(self):
        self.add_write_case(cycles="PENDING")
        self.assertIn("cycle signature is PENDING", self.run_gate().stderr)

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
