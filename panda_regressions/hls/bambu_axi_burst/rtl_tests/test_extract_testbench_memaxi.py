import copy
import importlib.util
import os
import subprocess
import sys
import tempfile
import unittest
import xml.etree.ElementTree as ET
from pathlib import Path


TESTS = Path(__file__).resolve().parent
ROOT = TESTS.parents[3]
EXTRACTOR_PATH = TESTS / "extract_testbench_memaxi.py"
T4B_EXTRACTOR_PATH = ROOT / "panda_regressions/hls/bambu_axi_burst/t4b_tests/extract_t4b_cells.py"
XML_PATH = ROOT / "etc/libtech/NC_TESTBENCH_IPs.xml"
TMP_ROOT = Path(os.environ.get("BURST_TEST_TMPDIR", Path.cwd())) / f"out_{Path(__file__).stem}"
SPEC = importlib.util.spec_from_file_location("t4b_extractor", T4B_EXTRACTOR_PATH)
T4B_EXTRACTOR = importlib.util.module_from_spec(SPEC)
assert SPEC.loader is not None
SPEC.loader.exec_module(T4B_EXTRACTOR)


class TestbenchMemaxiExtractorTests(unittest.TestCase):
    def setUp(self):
        TMP_ROOT.mkdir(parents=True, exist_ok=True)
        self.temp = tempfile.TemporaryDirectory(prefix="f0-extractor-", dir=TMP_ROOT)
        self.addCleanup(self.temp.cleanup)
        self.scratch = Path(self.temp.name)

    def write_cell_xml(self, body=None):
        source_root = ET.parse(XML_PATH).getroot()
        cells = [cell for cell in source_root.findall(".//cell")
                 if cell.findtext("name") == "TestbenchMEMAXI"]
        self.assertEqual(len(cells), 1)
        cell = copy.deepcopy(cells[0])
        if body is not None:
            module = cell.find("circuit/module_o")
            functionality = module.find("NP_functionality")
            functionality.set("VERILOG_PROVIDED", body)
        synthetic_root = ET.Element("technology")
        synthetic_root.append(cell)
        path = self.scratch / "synthetic.xml"
        ET.ElementTree(synthetic_root).write(path, encoding="unicode")
        return path

    def run_extractor(self, xml_path, output_path):
        env = os.environ.copy()
        temp_root = str(TMP_ROOT)
        env.update({"TMPDIR": temp_root, "TMP": temp_root, "TEMP": temp_root,
                    "PYTHONDONTWRITEBYTECODE": "1"})
        return subprocess.run(
            [sys.executable, "-B", str(EXTRACTOR_PATH), "--xml", str(xml_path),
             "--out", str(output_path), "--t4b-extractor", str(T4B_EXTRACTOR_PATH)],
            cwd=self.scratch, env=env, text=True, stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT, check=False,
        )

    def test_extracts_current_body_without_legacy_pointer_rewrites(self):
        output = self.scratch / "current.sv"
        result = self.run_extractor(XML_PATH, output)
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn("0 Questa local-variable rewrites", result.stdout)

        module, body = T4B_EXTRACTOR.find_cell(XML_PATH, "TestbenchMEMAXI")
        expected_body = T4B_EXTRACTOR.normalize_questa_body("TestbenchMEMAXI", body)
        extracted = output.read_text(encoding="utf-8")
        self.assertTrue(extracted.endswith(expected_body + "\nendmodule\n"))
        self.assertIn("(reset == 1'b0)", extracted)
        self.assertIn("localparam integer BITSIZE_araddr = 32;", extracted)
        self.assertIn("localparam integer BITSIZE_rdata = 32;", extracted)
        self.assertEqual(extracted.count("automatic ptr_t currAddr;"), 0)
        self.assertEqual(extracted.count("automatic ptr_t endAddr;"), 0)
        self.assertEqual(module.get("id"), "TestbenchMEMAXI")

    def test_normalizes_four_legacy_pointer_locals_on_same_interface(self):
        legacy_body = "\n".join((
            "automatic ptr_t currAddr;",
            "automatic ptr_t endAddr;",
            "automatic ptr_t currAddr;",
            "automatic ptr_t endAddr;",
        ))
        output = self.scratch / "legacy.sv"
        result = self.run_extractor(self.write_cell_xml(legacy_body), output)
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn("4 Questa local-variable rewrites", result.stdout)
        extracted = output.read_text(encoding="utf-8")
        self.assertEqual(extracted.count("ptr_t currAddr;"), 2)
        self.assertEqual(extracted.count("ptr_t endAddr;"), 2)
        self.assertEqual(extracted.count("automatic ptr_t"), 0)
        self.assertIn("input wire [31:0] araddr", extracted)
        self.assertIn("output wire [31:0] rdata", extracted)

    def test_rejects_partial_legacy_pointer_local_sets(self):
        for count in (1, 3):
            with self.subTest(count=count):
                declarations = [
                    "automatic ptr_t currAddr;",
                    "automatic ptr_t endAddr;",
                    "automatic ptr_t currAddr;",
                ][:count]
                output = self.scratch / f"partial-{count}.sv"
                result = self.run_extractor(
                    self.write_cell_xml("\n".join(declarations)), output)
                self.assertNotEqual(result.returncode, 0, result.stdout)
                self.assertIn("expected four TestbenchMEMAXI local pointer declarations",
                              result.stdout)
                self.assertIn(f"found {count}", result.stdout)
                self.assertFalse(output.exists())

    def test_reports_missing_cell_and_missing_hdl_body(self):
        missing_cell_xml = self.scratch / "missing-cell.xml"
        ET.ElementTree(ET.Element("technology")).write(missing_cell_xml, encoding="unicode")
        missing_cell = self.run_extractor(missing_cell_xml, self.scratch / "missing.sv")
        self.assertNotEqual(missing_cell.returncode, 0)
        self.assertIn("expected exactly one XML cell named TestbenchMEMAXI, found 0",
                      missing_cell.stdout)

        empty_body_xml = self.write_cell_xml("")
        empty_body = self.run_extractor(empty_body_xml, self.scratch / "empty.sv")
        self.assertNotEqual(empty_body.returncode, 0)
        self.assertIn("TestbenchMEMAXI has an empty HDL implementation", empty_body.stdout)


if __name__ == "__main__":
    unittest.main()
