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
TMP_ROOT = Path(os.environ.get("BURST_TEST_TMPDIR", Path.cwd())) / f"out_{Path(__file__).stem}"
CHECKER = HERE / "check_burst_results.py"
HEADER = ("case\tmode\tcycles\tsource\tburst_max\toutstanding\tfifo_depth\t"
          "write_burst_max\twrite_outstanding\twrite_fifo_depth\taddr_bits\n")
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
            rows.append(f"{case}\tread\t{cycles}\t{source}\t16\t{outstanding}\t256\t-\t-\t-\t32\n")
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
                stream.write(
                    f"{case}\tread\t{cycles}\t{source}\t16\t1\t256\t-\t-\t-\t{addr_bits}\n")
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
        self.add_duplex_case("duplex-symmetric-n1024", "7", read_profile=(16, 1, 64),
                             write_profile=(16, 1, 32))
        self.add_duplex_case("duplex-asymmetric-n1024", "8", read_profile=(16, 2, 64),
                             write_profile=(8, 1, 16))
        self.add_duplex_case("duplex-copy-symmetric-n1024", "5158", read_profile=(16, 1, 64),
                             write_profile=(16, 1, 32), copy_ports=True)
        self.add_duplex_case("duplex-copy-asymmetric-n1024", "2622", read_profile=(16, 2, 64),
                             write_profile=(8, 1, 16), copy_ports=True)
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
        self.assertIn("PASS: 10 read cases, 3 write cases, 4 duplex cases, and 1 legacy cases",
                      result.stdout)

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
            self.expectations.read_text(encoding="utf-8") +
            f"{case}\tread\t7\t{source}\t16\t1\t256\t-\t-\t-\t32\n",
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
            f"{case}\twrite\t{cycles}\t{source}\t-\t-\t-\t16\t1\t32\t"
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

    def add_duplex_case(self, case, cycles, *, read_profile=(16, 1, 32),
                        write_profile=(16, 1, 16), addr_bits=32, rtl=None,
                        copy_ports=False):
        bundle = "shared"
        source_name = "bambu_axi_burst_duplex_copy.cpp" if copy_ports else "bambu_axi_burst_duplex.cpp"
        source = self.fixture / source_name
        if copy_ports:
            source.write_text(
                "#if DUPLEX_PROFILE == 1\n"
                "#pragma HLS interface mode=m_axi port=src offset=direct bundle=shared "
                "max_read_burst_length=16 num_read_outstanding=1 read_fifo_depth=64\n"
                "#pragma HLS interface mode=m_axi port=dst offset=direct bundle=shared "
                "max_write_burst_length=16 write_burst_outstanding=1 write_fifo_depth=32\n"
                "#elif DUPLEX_PROFILE == 2\n"
                "#pragma HLS interface mode=m_axi port=src offset=direct bundle=shared "
                "max_read_burst_length=16 num_read_outstanding=2 read_fifo_depth=64\n"
                "#pragma HLS interface mode=m_axi port=dst offset=direct bundle=shared "
                "max_write_burst_length=8 write_burst_outstanding=1 write_fifo_depth=16\n"
                "#endif\n", encoding="utf-8")
        else:
            source.write_text(
                "#if DUPLEX_PROFILE == 1\n"
                "#pragma HLS interface mode=m_axi port=data offset=direct bundle=shared "
                "max_read_burst_length=16 num_read_outstanding=1 read_fifo_depth=64\n"
                "#elif DUPLEX_PROFILE == 2\n"
                "#pragma HLS interface mode=m_axi port=data offset=direct bundle=shared "
                "max_read_burst_length=16 num_read_outstanding=2 read_fifo_depth=64\n"
                "#endif\n"
                "#pragma HLS interface mode=m_axi port=data offset=direct bundle=shared "
                "max_write_burst_length=16 write_burst_outstanding=1 write_fifo_depth=32\n"
                "#if DUPLEX_PROFILE == 2\n"
                "#pragma HLS interface mode=m_axi port=data offset=direct bundle=shared "
                "max_write_burst_length=8 write_burst_outstanding=1 write_fifo_depth=16\n"
                "#endif\n", encoding="utf-8")
        self.expectations.write_text(
            self.expectations.read_text(encoding="utf-8") +
            f"{case}\tduplex\t{cycles}\t{source_name}\t"
            f"{read_profile[0]}\t{read_profile[1]}\t{read_profile[2]}\t"
            f"{write_profile[0]}\t{write_profile[1]}\t{write_profile[2]}\t{addr_bits}\n",
            encoding="utf-8")
        case_dir = self.results / "AXI-BURST" / case
        case_dir.mkdir(parents=True, exist_ok=True)
        (case_dir / "bambu_results.xml").write_text(
            f'<results><evaluation CYCLES="{cycles}"/></results>\n', encoding="utf-8")
        architecture = case_dir / "panda-temp" / "architecture.xml"
        architecture.parent.mkdir()
        parameter_xml = ('<parameters><parameter port="src" bundle="shared"/>'
                         '<parameter port="dst" bundle="shared"/></parameters>' if copy_ports else '')
        architecture.write_text(
            '<module><function name="kernel"><bundles><bundle name="shared" mode="m_axi" '
            f'max_read_burst_length="{read_profile[0]}" num_read_outstanding="{read_profile[1]}" '
            f'read_fifo_depth="{read_profile[2]}" max_write_burst_length="{write_profile[0]}" '
            f'write_burst_outstanding="{write_profile[1]}" write_fifo_depth="{write_profile[2]}"/>'
            f'</bundles>{parameter_xml}</function></module>\n', encoding="utf-8")
        read_b, read_o, read_fifo = read_profile
        write_b, write_o, write_fifo = write_profile
        read_signals = ("araddr", "arlen", "arsize", "arburst", "arid", "arvalid", "rready")
        write_signals = ("awaddr", "awlen", "awsize", "awburst", "awid", "awvalid",
                         "wdata", "wstrb", "wlast", "wvalid", "bready")
        assignments = "".join(
            f"assign p_m_axi_{bundle}_{signal} = read_core_{signal};\n"
            for signal in read_signals) + "".join(
            f"assign p_m_axi_{bundle}_{signal} = write_core_{signal};\n"
            for signal in write_signals)
        read_inputs = ("arready", "rdata", "rresp", "rlast", "rvalid")
        write_inputs = ("awready", "wready", "bresp", "bvalid")
        read_input_ports = ", ".join(
            f".m_axi_{signal}(p_m_axi_{bundle}_{signal})" for signal in read_inputs)
        read_input_ports += f", .m_axi_rid((|p_m_axi_{bundle}_rid))"
        write_input_ports = ", ".join(
            f".m_axi_{signal}(p_m_axi_{bundle}_{signal})" for signal in write_inputs)
        write_input_ports += f", .m_axi_bid((|p_m_axi_{bundle}_bid))"
        read_output_ports = ", ".join(
            f".m_axi_{signal}(read_core_{signal})" for signal in read_signals)
        write_output_ports = ", ".join(
            f".m_axi_{signal}(write_core_{signal})" for signal in write_signals)
        read_instance = (
            f"MinimalAXI4MasterPipelined #(.B_MAX({read_b}), .MAX_OUTSTANDING({read_o}), "
            f".FIFO_DEPTH({read_fifo}), .BITSIZE_m_axi_araddr({addr_bits}), "
            f".BITSIZE_in4({addr_bits}), .BITSIZE_m_axi_rdata(32), .BITSIZE_in5(32)) read_engine ("
            f"{read_output_ports}, {read_input_ports});\n")
        write_instance = (
            f"MinimalAXI4MasterPipelinedWrite #(.B_MAX({write_b}), .MAX_OUTSTANDING({write_o}), "
            f".FIFO_DEPTH({write_fifo}), .BITSIZE_m_axi_awaddr({addr_bits}), "
            f".BITSIZE_in4({addr_bits}), .BITSIZE_in5(32), .BITSIZE_in3(32), "
            ".BITSIZE_m_axi_wdata(32)) write_engine ("
            f"{write_output_ports}, {write_input_ports});\n")
        physical_bundle = ("module shared_bambu_artificial_ParmMgr_duplex_aggregator_modgen;\n"
                           f"wire p_m_axi_{bundle}_arvalid;\nwire p_m_axi_{bundle}_awvalid;\n")
        (case_dir / "kernel.v").write_text(
            rtl if rtl is not None else physical_bundle + assignments + read_instance +
            write_instance + "endmodule\n",
            encoding="utf-8")

    def test_duplex_symmetric_and_asymmetric_profiles_pass(self):
        result = self.run_gate()
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("4 duplex cases", result.stdout)

    def test_duplex_inventory_requires_both_positive_rows_and_modes(self):
        original = self.expectations.read_text(encoding="utf-8").splitlines()
        for case in ("duplex-symmetric-n1024", "duplex-asymmetric-n1024"):
            with self.subTest(case=case):
                self.expectations.write_text("\n".join(
                    line for line in original if not line.startswith(case + "\t")) + "\n",
                    encoding="utf-8")
                result = self.run_gate()
                self.assertNotEqual(result.returncode, 0)
                self.assertIn("does not cover required cycle cases", result.stderr)
        self.expectations.write_text("\n".join(original) + "\n", encoding="utf-8")
        self.expectations.write_text(self.expectations.read_text(encoding="utf-8").replace(
            "duplex-symmetric-n1024\tduplex\t", "duplex-symmetric-n1024\twrite\t", 1),
            encoding="utf-8")
        result = self.run_gate()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("required cycle case mode does not match inventory", result.stderr)

    def test_copy_duplex_inventory_and_bundle_mapping_are_fail_closed(self):
        original = self.expectations.read_text(encoding="utf-8").splitlines()
        for case in ("duplex-copy-symmetric-n1024", "duplex-copy-asymmetric-n1024"):
            with self.subTest(case=case):
                self.expectations.write_text("\n".join(
                    line for line in original if not line.startswith(case + "\t")) + "\n",
                    encoding="utf-8")
                result = self.run_gate()
                self.assertNotEqual(result.returncode, 0)
                self.assertIn("does not cover required cycle cases", result.stderr)
        self.expectations.write_text("\n".join(original) + "\n", encoding="utf-8")

        case = self.results / "AXI-BURST" / "duplex-copy-symmetric-n1024"
        architecture = case / "panda-temp" / "architecture.xml"
        original_architecture = architecture.read_text(encoding="utf-8")
        architecture.write_text(original_architecture.replace(
            'port="dst" bundle="shared"', 'port="dst" bundle="other"'), encoding="utf-8")
        result = self.run_gate()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("dst must map exactly once to duplex bundle shared", result.stderr)
        architecture.write_text(original_architecture, encoding="utf-8")

        source = self.fixture / "bambu_axi_burst_duplex_copy.cpp"
        original_source = source.read_text(encoding="utf-8")
        source.write_text(original_source.replace("port=src", "port=source", 2), encoding="utf-8")
        result = self.run_gate()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("source must declare src-read and dst-write", result.stderr)
        source.write_text(original_source, encoding="utf-8")

    def test_copy_duplex_rejects_duplicate_read_engine(self):
        case = self.results / "AXI-BURST" / "duplex-copy-symmetric-n1024"
        kernel = case / "kernel.v"
        original = kernel.read_text(encoding="utf-8")
        read_start = original.index("MinimalAXI4MasterPipelined #")
        write_start = original.index("MinimalAXI4MasterPipelinedWrite #", read_start)
        read_instance = original[read_start:write_start]
        kernel.write_text(original[:write_start] + read_instance + original[write_start:],
                          encoding="utf-8")
        result = self.run_gate()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("lacks the expected read/write duplex profiles", result.stderr)

    def test_copy_duplex_rejects_engines_in_separate_carriers(self):
        case = self.results / "AXI-BURST" / "duplex-copy-symmetric-n1024"
        kernel = case / "kernel.v"
        original = kernel.read_text(encoding="utf-8")
        read_start = original.index("MinimalAXI4MasterPipelined #")
        write_start = original.index("MinimalAXI4MasterPipelinedWrite #", read_start)
        endmodule = original.rindex("endmodule")
        read_instance = original[read_start:write_start]
        write_instance = original[write_start:endmodule]
        read_assignments = "\n".join(
            line for line in original.splitlines()
            if line.startswith("assign p_m_axi_shared_") and "= read_core_" in line)
        write_assignments = "\n".join(
            line for line in original.splitlines()
            if line.startswith("assign p_m_axi_shared_") and "= write_core_" in line)
        carrier_signals = "wire p_m_axi_shared_arvalid; wire p_m_axi_shared_awvalid;\n"
        kernel.write_text(
            "module read_carrier;\n" + carrier_signals + read_assignments + "\n" +
            read_instance + "endmodule\n" +
            "module write_carrier;\n" + carrier_signals + write_assignments + "\n" +
            write_instance + "endmodule\n",
            encoding="utf-8")
        result = self.run_gate()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("lacks the expected read/write duplex profiles", result.stderr)

    def test_copy_duplex_scans_all_rtl_files_before_passing(self):
        case = self.results / "AXI-BURST" / "duplex-copy-symmetric-n1024"
        extra_read = (
            "module extra_read;\n"
            "MinimalAXI4MasterPipelined #(.B_MAX(16), .MAX_OUTSTANDING(1), "
            ".FIFO_DEPTH(64)) extra_engine ();\n"
            "endmodule\n")
        legacy_adapter = "module extra_adapter; MinimalAXI4AdapterSingleBeat extra (); endmodule\n"
        for prefix, content, diagnostic in (
                ("aaa", extra_read, "lacks the expected read/write duplex profiles"),
                ("zzz", extra_read, "lacks the expected read/write duplex profiles"),
                ("aaa", legacy_adapter, "contains a legacy memory adapter"),
                ("zzz", legacy_adapter, "contains a legacy memory adapter")):
            with self.subTest(prefix=prefix, content=content.splitlines()[0]):
                extra = case / f"{prefix}_extra.v"
                extra.write_text(content, encoding="utf-8")
                result = self.run_gate()
                self.assertNotEqual(result.returncode, 0)
                self.assertIn(diagnostic, result.stderr)
                extra.unlink()

    def test_duplex_rejects_missing_directional_core(self):
        case = self.results / "AXI-BURST" / "duplex-symmetric-n1024"
        rtl = (case / "kernel.v").read_text(encoding="utf-8")
        (case / "kernel.v").write_text(rtl.split("MinimalAXI4MasterPipelinedWrite", 1)[0],
                                        encoding="utf-8")
        self.assertIn("lacks the expected read/write duplex profiles", self.run_gate().stderr)

    def test_duplex_rejects_incoherent_architecture_xml_profile(self):
        architecture = (self.results / "AXI-BURST" / "duplex-symmetric-n1024" /
                        "panda-temp" / "architecture.xml")
        architecture.write_text(architecture.read_text(encoding="utf-8").replace(
            'write_fifo_depth="32"', 'write_fifo_depth="64"'), encoding="utf-8")
        self.assertIn("architecture XML duplex bundle profile does not match expectations",
                      self.run_gate().stderr)

    def test_duplex_rejects_swapped_or_mismatched_direction_profiles(self):
        case = self.results / "AXI-BURST" / "duplex-symmetric-n1024"
        rtl = (case / "kernel.v").read_text(encoding="utf-8")
        (case / "kernel.v").write_text(rtl.replace(".FIFO_DEPTH(32)", ".FIFO_DEPTH(128)", 1),
                                        encoding="utf-8")
        self.assertIn("lacks the expected read/write duplex profiles", self.run_gate().stderr)

    def test_duplex_rejects_wrong_address_and_data_widths(self):
        case = self.results / "AXI-BURST" / "duplex-symmetric-n1024"
        original = (case / "kernel.v").read_text(encoding="utf-8")
        mutations = (
            (".BITSIZE_m_axi_araddr(32)", ".BITSIZE_m_axi_araddr(64)"),
            (".BITSIZE_m_axi_awaddr(32)", ".BITSIZE_m_axi_awaddr(64)"),
            (".BITSIZE_m_axi_rdata(32)", ".BITSIZE_m_axi_rdata(16)"),
            (".BITSIZE_m_axi_wdata(32)", ".BITSIZE_m_axi_wdata(16)"),
        )
        for expected, replacement in mutations:
            with self.subTest(parameter=expected):
                (case / "kernel.v").write_text(original.replace(expected, replacement),
                                                encoding="utf-8")
                self.assertIn("lacks the expected read/write duplex profiles",
                              self.run_gate().stderr)

    def test_duplex_requires_profiles_and_widths_on_same_instances(self):
        case = self.results / "AXI-BURST" / "duplex-symmetric-n1024"
        (case / "kernel.v").write_text(
            "wire p_m_axi_shared_arvalid; wire p_m_axi_shared_awvalid;\n"
            "MinimalAXI4MasterPipelined #(.B_MAX(16), .MAX_OUTSTANDING(1), .FIFO_DEPTH(32)) read ("
            ".m_axi_arready(p_m_axi_shared_arready), .m_axi_rdata(p_m_axi_shared_rdata), "
            ".m_axi_rvalid(p_m_axi_shared_rvalid));\n"
            "MinimalAXI4MasterPipelinedWrite #(.B_MAX(16), .MAX_OUTSTANDING(1), .FIFO_DEPTH(16), "
            ".BITSIZE_m_axi_awaddr(32), .BITSIZE_in4(32), .BITSIZE_in5(32), .BITSIZE_in3(32), "
            ".BITSIZE_m_axi_wdata(32)) write (.m_axi_awready(p_m_axi_shared_awready), "
            ".m_axi_wready(p_m_axi_shared_wready), .m_axi_bresp(p_m_axi_shared_bresp), "
            ".m_axi_bvalid(p_m_axi_shared_bvalid));\n", encoding="utf-8")
        self.assertIn("lacks the expected read/write duplex profiles", self.run_gate().stderr)

    def test_duplex_rejects_profile_and_width_parameters_split_across_instances(self):
        case = self.results / "AXI-BURST" / "duplex-symmetric-n1024"
        (case / "kernel.v").write_text(
            "wire p_m_axi_shared_arvalid; wire p_m_axi_shared_awvalid;\n"
            "MinimalAXI4MasterPipelined #(.B_MAX(16), .MAX_OUTSTANDING(1), .FIFO_DEPTH(32)) profile ( );\n"
            "MinimalAXI4MasterPipelined #(.BITSIZE_m_axi_araddr(32), .BITSIZE_in4(32), "
            ".BITSIZE_m_axi_rdata(32), .BITSIZE_in5(32)) widths ( );\n"
            "MinimalAXI4MasterPipelinedWrite #(.B_MAX(16), .MAX_OUTSTANDING(1), .FIFO_DEPTH(16)) "
            "write_profile ( );\n"
            "MinimalAXI4MasterPipelinedWrite #(.BITSIZE_m_axi_awaddr(32), .BITSIZE_in4(32), "
            ".BITSIZE_in5(32), .BITSIZE_in3(32), .BITSIZE_m_axi_wdata(32)) write_widths ( );\n",
            encoding="utf-8")
        self.assertIn("lacks the expected read/write duplex profiles", self.run_gate().stderr)

    def test_duplex_rejects_legacy_adapter_coexisting(self):
        case = self.results / "AXI-BURST" / "duplex-symmetric-n1024"
        with (case / "kernel.v").open("a", encoding="utf-8") as stream:
            stream.write("MinimalAXI4AdapterSingleBeat legacy ();\n")
        self.assertIn("contains a legacy memory adapter", self.run_gate().stderr)

    def test_duplex_rejects_missing_shared_bundle_physical_channels(self):
        case = self.results / "AXI-BURST" / "duplex-symmetric-n1024"
        rtl = (case / "kernel.v").read_text(encoding="utf-8").replace("shared", "other")
        (case / "kernel.v").write_text(rtl, encoding="utf-8")
        self.assertIn("lacks the expected read/write duplex profiles", self.run_gate().stderr)

    def test_duplex_rejects_disconnected_critical_engine_ports(self):
        case = self.results / "AXI-BURST" / "duplex-symmetric-n1024"
        kernel = case / "kernel.v"
        original = kernel.read_text(encoding="utf-8")
        for connected, disconnected in (
                (".m_axi_arvalid(read_core_arvalid)", ".m_axi_arvalid()"),
                (".m_axi_awvalid(write_core_awvalid)", ".m_axi_awvalid()"),
                (".m_axi_bready(write_core_bready)", ".m_axi_bready()")):
            with self.subTest(port=connected):
                kernel.write_text(original.replace(connected, disconnected, 1), encoding="utf-8")
                self.assertIn("lacks the expected read/write duplex profiles",
                              self.run_gate().stderr)

    def test_duplex_rejects_ports_and_instances_split_across_modules(self):
        case = self.results / "AXI-BURST" / "duplex-symmetric-n1024"
        kernel = case / "kernel.v"
        original = kernel.read_text(encoding="utf-8")
        marker = "MinimalAXI4MasterPipelinedWrite"
        split = original.index(marker)
        carrier = original[:split] + "endmodule\n"
        write_module = "module write_submodule;\n" + original[split:]
        write_module = write_module.rsplit("endmodule", 1)[0] + "endmodule\n"
        kernel.write_text(carrier + write_module, encoding="utf-8")
        self.assertIn("lacks the expected read/write duplex profiles", self.run_gate().stderr)

    def test_duplex_rejects_or_reduced_non_id_input(self):
        case = self.results / "AXI-BURST" / "duplex-symmetric-n1024"
        kernel = case / "kernel.v"
        original = kernel.read_text(encoding="utf-8")
        kernel.write_text(original.replace(
            ".m_axi_rdata(p_m_axi_shared_rdata)",
            ".m_axi_rdata((|p_m_axi_shared_rdata))", 1), encoding="utf-8")
        self.assertIn("lacks the expected read/write duplex profiles", self.run_gate().stderr)

    def test_duplex_rejects_direct_physical_id_connections(self):
        case = self.results / "AXI-BURST" / "duplex-symmetric-n1024"
        kernel = case / "kernel.v"
        original = kernel.read_text(encoding="utf-8")
        for reduced, direct in (
                (".m_axi_rid((|p_m_axi_shared_rid))", ".m_axi_rid(p_m_axi_shared_rid)"),
                (".m_axi_bid((|p_m_axi_shared_bid))", ".m_axi_bid(p_m_axi_shared_bid)")):
            with self.subTest(port=reduced):
                kernel.write_text(original.replace(reduced, direct, 1), encoding="utf-8")
                self.assertIn("lacks the expected read/write duplex profiles",
                              self.run_gate().stderr)

    def test_valid_write_cycle_and_profile_pass(self):
        self.add_write_case()
        result = self.run_gate()
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("10 read cases, 4 write cases, 4 duplex cases", result.stdout)

    def test_write_rejects_wrong_mode(self):
        self.add_write_case()
        self.expectations.write_text(self.expectations.read_text(encoding="utf-8").replace(
            "write-n1024\twrite\t", "write-n1024\tWRITE\t"), encoding="utf-8")
        self.assertIn("mode must be 'read', 'write', or 'duplex'", self.run_gate().stderr)

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
