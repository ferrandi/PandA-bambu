from __future__ import annotations

import copy
import importlib.util
import os
import re
import shutil
import subprocess
import sys
import tempfile
import unittest
import xml.etree.ElementTree as ET
from pathlib import Path


TESTS = Path(__file__).resolve().parent
ROOT = TESTS.parents[3]
XML = ROOT / "etc/libtech/NC_AXI_IPs.xml"
EXTRACTOR_PATH = TESTS / "extract_write_engine.py"
SPEC = importlib.util.spec_from_file_location("extract_write_engine", EXTRACTOR_PATH)
EXTRACTOR = importlib.util.module_from_spec(SPEC)
assert SPEC.loader is not None
SPEC.loader.exec_module(EXTRACTOR)


class WriteEngineExtractorTests(unittest.TestCase):
    def setUp(self):
        temp_root = Path(os.environ.get("BURST_TEST_TMPDIR", Path.cwd())) / f"out_{Path(__file__).stem}"
        temp_root.mkdir(parents=True, exist_ok=True)
        temp = tempfile.TemporaryDirectory(prefix="write-engine-extract-", dir=temp_root)
        self.addCleanup(temp.cleanup)
        self.scratch = Path(temp.name)

    def synthetic_xml(self, mutation):
        root = ET.parse(XML).getroot()
        found = [cell for cell in root.findall(".//cell")
                 if cell.findtext("name") == EXTRACTOR.CELL_NAME]
        self.assertEqual(len(found), 1)
        cell = copy.deepcopy(found[0])
        mutation(cell.find("circuit/module_o"))
        output = self.scratch / "synthetic.xml"
        document = ET.Element("technology")
        document.append(cell)
        ET.ElementTree(document).write(output, encoding="unicode")
        return output

    def test_extracts_body_and_library_derived_widths(self):
        generated = EXTRACTOR.extract(XML)
        self.assertIn("module MinimalAXI4MasterPipelinedWrite", generated)
        self.assertIn("parameter integer BITSIZE_in4 = 32", generated)
        self.assertIn("parameter integer BITSIZE_m_axi_awaddr = 32", generated)
        self.assertIn("parameter integer BITSIZE_m_axi_awid = 1", generated)
        self.assertIn("input wire [BITSIZE_in4-1:0] in4", generated)
        self.assertIn("output wire [BITSIZE_m_axi_awaddr-1:0] m_axi_awaddr", generated)
        self.assertIn("(reset == 1'b0)", generated)
        self.assertIn("assign m_axi_wstrb", generated)
        self.assertTrue(generated.endswith("endmodule\n"))

    def test_cli_extracts_shared_memory_with_string_default(self):
        output = self.scratch / "memory.sv"
        result = subprocess.run(
            [sys.executable, str(EXTRACTOR_PATH), "--xml", str(XML.with_name("NC_MEM_IPs.xml")),
             "--out", str(output), "--target", "STD_SDP_BRAMFW"],
            text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=False)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn('parameter MEMORY_INIT_file = "array_a.mem"', output.read_text())

    def test_fifo_includes_shared_memory_dependency(self):
        generated = EXTRACTOR.extract(XML.with_name("NC_MEM_IPs.xml"), "STD_FIFO")
        self.assertEqual(generated.count("module STD_SDP_BRAMFW #("), 1)
        self.assertIn('parameter MEMORY_INIT_file = "array_a.mem"', generated)
        self.assertIn('.MEMORY_INIT_file("")', generated)
        self.assertIn('.HIGH_LATENCY(0)', generated)

    def test_fifo_without_dependencies_only_emits_controller(self):
        generated = EXTRACTOR.extract(XML.with_name("NC_MEM_IPs.xml"), "STD_FIFO", include_dependencies=False)
        self.assertIn("STD_SDP_BRAMFW #(", generated)
        self.assertNotIn("module STD_SDP_BRAMFW", generated)

    def test_dependency_uses_colocated_memory_snapshot(self):
        axi_snapshot = self.scratch / "NC_AXI_IPs.xml"
        axi_snapshot.write_bytes(XML.read_bytes())
        memory = ET.parse(XML.with_name("NC_MEM_IPs.xml"))
        cell = next(cell for cell in memory.getroot().findall(".//cell")
                    if cell.findtext("name") == "STD_SDP_BRAMFW")
        implementation = cell.find("circuit/module_o/NP_functionality")
        implementation.set("VERILOG_PROVIDED",
                           implementation.get("VERILOG_PROVIDED") + "\n// colocated-memory-snapshot\n")
        memory.write(self.scratch / "NC_MEM_IPs.xml", encoding="unicode")
        generated = EXTRACTOR.extract(axi_snapshot, "MinimalAXI4MasterPipelined")
        self.assertIn("// colocated-memory-snapshot", generated)

    def test_serialize_until_b_mutation_targets_active_outstanding_guard(self):
        generated = EXTRACTOR.extract(XML)
        target = "(descriptor_count_reg >= MAX_OUTSTANDING)"
        replacement = "((descriptor_count_reg >= MAX_OUTSTANDING) || (descriptor_count_reg != 0))"
        self.assertEqual(generated.count(target), 1)

        runner = (TESTS / "run_write_burst_engine.sh").read_text()
        testbench = (TESTS / "tb_write_burst_engine.sv").read_text()
        self.assertEqual(runner.count(f'"{target}"'), 1)
        self.assertEqual(runner.count(f'"{replacement}"'), 1)
        self.assertIn("build_run_o O2-D16 16 2 16 32 MULTI_OUTSTANDING TWO_AW_BEFORE_B", runner)
        self.assertIn("TWO_AW_BEFORE_B \"F2_TWO_AW_BEFORE_FIRST_B_REQUIRED\"", runner)
        self.assertIn("F2_TWO_AW_BEFORE_FIRST_B_REQUIRED", testbench)

    def test_cli_extracts_reader_when_target_is_selected(self):
        output = self.scratch / "reader.sv"
        result = subprocess.run(
            [sys.executable, str(EXTRACTOR_PATH), "--xml", str(XML), "--out", str(output),
             "--target", "MinimalAXI4MasterPipelined"],
            text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=False)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("Extracted MinimalAXI4MasterPipelined from", result.stdout)
        self.assertIn("module MinimalAXI4MasterPipelined #(", output.read_text())

    def test_cli_keeps_writer_as_default_target(self):
        output = self.scratch / "writer-default.sv"
        result = subprocess.run(
            [sys.executable, str(EXTRACTOR_PATH), "--xml", str(XML), "--out", str(output)],
            text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=False)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("Extracted MinimalAXI4MasterPipelinedWrite from", result.stdout)
        self.assertIn("module MinimalAXI4MasterPipelinedWrite #(", output.read_text())

    def test_writer_fast_store_ack_defaults_to_fast_mode(self):
        generated = EXTRACTOR.extract(XML)
        self.assertRegex(generated, r"parameter integer FAST_STORE_ACK = 1")

    def test_cli_rejects_unknown_target(self):
        output = self.scratch / "invalid.sv"
        result = subprocess.run(
            [sys.executable, str(EXTRACTOR_PATH), "--xml", str(XML), "--out", str(output),
             "--target", "SomeOtherCell"],
            text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=False)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("invalid choice", result.stderr)
        self.assertFalse(output.exists())

    def test_real_xml_reset_placeholders_and_all_mode_polarity_variants(self):
        source_body = EXTRACTOR._raw_body(XML, EXTRACTOR.CELL_NAME)
        self.assertIn("1RESET_EDGE", source_body)
        self.assertIn("1RESET_VALUE", source_body)
        variants = (
            (False, True, "reset == 1'b0", None),
            (False, False, "reset == 1'b1", None),
            (True, True, "reset == 1'b0", "or negedge reset"),
            (True, False, "reset == 1'b1", "or posedge reset"),
        )
        for async_reset, active_low, reset_value, sensitivity in variants:
            with self.subTest(async_reset=async_reset, active_low=active_low):
                generated = EXTRACTOR.extract(XML, async_reset=async_reset,
                                              active_low=active_low)
                self.assertRegex(generated, r"always @\(posedge clock\s+" +
                                 (re.escape(sensitivity) if sensitivity else r"\)"))
                self.assertIn(reset_value, generated)
                self.assertNotIn("1RESET_EDGE", generated)
                self.assertNotIn("1RESET_VALUE", generated)

    def test_reader_async_reset_conditions_use_reset_placeholder_directly(self):
        for active_low, value in ((True, "0"), (False, "1")):
            with self.subTest(active_low=active_low):
                generated = EXTRACTOR.extract(XML, target="MinimalAXI4MasterPipelined",
                                              async_reset=True, active_low=active_low)
                self.assertRegex(generated, rf"if \(reset == 1'b{value}\)\s+begin")
                self.assertIn(f"if (reset == 1'b{value} || activate_configure ||", generated)
                self.assertEqual(generated.count("wire reset_n ="), 1)
                self.assertNotIn("if (!reset_n", generated)
                self.assertIn(f"wire reset_n = !(reset == 1'b{value});", generated)

    @unittest.skipUnless(os.environ.get("BURST_TEST_VIVADO_OOC") == "1",
                         "set BURST_TEST_VIVADO_OOC=1 to run optional Vivado OOC synthesis")
    def test_async_reader_vivado_ooc_both_reset_polarities(self):
        vivado = os.environ.get("VIVADO_BIN") or shutil.which("vivado")
        self.assertTrue(vivado, "VIVADO_BIN or vivado on PATH is required for OOC synthesis")
        part = os.environ.get("VIVADO_PART", "xcu250-figd2104-2L-e")

        for active_low, polarity in ((True, "active-low"), (False, "active-high")):
            with self.subTest(polarity=polarity):
                source = self.scratch / f"reader-{polarity}.sv"
                source.write_text(EXTRACTOR.extract(XML, target="MinimalAXI4MasterPipelined",
                                                    async_reset=True, active_low=active_low))
                marker = f"ASYNC_READ_OOC_PASS polarity={polarity}"
                tcl = self.scratch / f"reader-{polarity}.tcl"
                tcl.write_text(
                    f'if {{[catch {{read_verilog -sv {{{source}}}}} read_error]}} {{\n'
                    '    puts stderr "ASYNC_READ_OOC_READ_FAILED: $read_error"\n'
                    "    exit 1\n"
                    "}\n"
                    "if {[catch {synth_design -top MinimalAXI4MasterPipelined "
                    f"-part {part} -mode out_of_context}} synth_error]}} {{\n"
                    '    puts stderr "ASYNC_READ_OOC_SYNTH_FAILED: $synth_error"\n'
                    "    exit 1\n"
                    "}\n"
                    f'puts "{marker}"\n'
                    "exit 0\n")
                try:
                    result = subprocess.run(
                        [vivado, "-mode", "batch", "-nojournal", "-nolog", "-source", str(tcl)],
                        cwd=self.scratch, text=True, stdout=subprocess.PIPE,
                        stderr=subprocess.PIPE, check=False, timeout=300)
                except subprocess.TimeoutExpired as error:
                    stdout = error.stdout or ""
                    stderr = error.stderr or ""
                    if isinstance(stdout, bytes):
                        stdout = stdout.decode(errors="replace")
                    if isinstance(stderr, bytes):
                        stderr = stderr.decode(errors="replace")
                    self.fail(f"Vivado OOC synthesis timed out for {polarity}; "
                              f"stdout:\n{stdout}\nstderr:\n{stderr}")
                output = result.stdout + result.stderr
                self.assertEqual(result.returncode, 0,
                                 f"Vivado OOC synthesis failed for {polarity}:\n{output}")
                self.assertRegex(output, rf"(?m)^{re.escape(marker)}$")

    def test_missing_library_port_is_rejected_not_inferred(self):
        def remove_port(module):
            functionality = module.find("NP_functionality")
            functionality.set("LIBRARY", functionality.get("LIBRARY").replace("m_axi_awaddr ", ""))

        path = self.synthetic_xml(remove_port)
        with self.assertRaisesRegex(ValueError, "LIBRARY is missing port m_axi_awaddr"):
            EXTRACTOR.extract(path)

    def test_missing_library_parameter_is_rejected_not_inferred(self):
        def remove_parameter(module):
            functionality = module.find("NP_functionality")
            functionality.set("LIBRARY", functionality.get("LIBRARY").replace("B_MAX ", ""))

        path = self.synthetic_xml(remove_parameter)
        with self.assertRaisesRegex(ValueError, "LIBRARY is missing parameter B_MAX"):
            EXTRACTOR.extract(path)

    def test_missing_xml_port_descriptor_is_rejected(self):
        def remove_descriptor(module):
            port = next(item for item in module.findall("port_o") if item.get("id") == "m_axi_awaddr")
            port.remove(port.find("structural_type_descriptor"))

        with self.assertRaisesRegex(ValueError, "unsupported structural type for port m_axi_awaddr"):
            EXTRACTOR.extract(self.synthetic_xml(remove_descriptor))

    def test_duplicate_library_token_is_rejected(self):
        def duplicate_token(module):
            functionality = module.find("NP_functionality")
            functionality.set("LIBRARY", functionality.get("LIBRARY") + " m_axi_awaddr")

        with self.assertRaisesRegex(ValueError, "duplicate token"):
            EXTRACTOR.extract(self.synthetic_xml(duplicate_token))


if __name__ == "__main__":
    unittest.main()
