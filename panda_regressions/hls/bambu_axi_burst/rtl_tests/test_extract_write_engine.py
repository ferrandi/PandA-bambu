from __future__ import annotations

import copy
import importlib.util
import os
import re
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

    def test_serialize_until_b_mutation_targets_active_outstanding_guard(self):
        generated = EXTRACTOR.extract(XML)
        target = "(descriptor_count >= MAX_OUTSTANDING)"
        replacement = "((descriptor_count >= MAX_OUTSTANDING) || (descriptor_count != 0))"
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
