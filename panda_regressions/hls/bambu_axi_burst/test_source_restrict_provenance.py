#!/usr/bin/env python3
from __future__ import annotations

import sys
import tempfile
import unittest
import xml.etree.ElementTree as ET
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
TMP_ROOT = ROOT / "documentation" / "tmp"
TMP_ROOT.mkdir(parents=True, exist_ok=True)
sys.path.insert(0, str(Path(__file__).resolve().parent))

from check_source_restrict_provenance import check_architecture


class SourceRestrictProvenanceTests(unittest.TestCase):
    def setUp(self) -> None:
        self.temp = tempfile.TemporaryDirectory(dir=TMP_ROOT)
        self.path = Path(self.temp.name) / "architecture.xml"

    def tearDown(self) -> None:
        self.temp.cleanup()

    def write_xml(self, params: dict[str, str]) -> None:
        root = ET.Element("module")
        function = ET.SubElement(root, "function", symbol="restrict_one")
        parameters = ET.SubElement(function, "parameters")
        for port, flag in params.items():
            ET.SubElement(parameters, "parameter", port=port, source_restrict=flag)
        ET.ElementTree(root).write(self.path, encoding="unicode")

    def test_both_restricted_parameters_are_exported(self) -> None:
        root = ET.Element("module")
        function = ET.SubElement(root, "function", symbol="restrict_both")
        parameters = ET.SubElement(function, "parameters")
        ET.SubElement(parameters, "parameter", port="input", source_restrict="1")
        ET.SubElement(parameters, "parameter", port="output", source_restrict="1")
        ET.ElementTree(root).write(self.path, encoding="unicode")
        check_architecture(self.path, "restrict_both")

    def test_one_restrict_parameter_does_not_mark_its_peer(self) -> None:
        self.write_xml({"input": "1", "output": "0"})
        check_architecture(self.path, "restrict_one")

    def test_missing_source_provenance_fails_closed(self) -> None:
        root = ET.Element("module")
        function = ET.SubElement(root, "function", symbol="restrict_one")
        parameters = ET.SubElement(function, "parameters")
        ET.SubElement(parameters, "parameter", port="input", source_restrict="1")
        ET.SubElement(parameters, "parameter", port="output")
        ET.ElementTree(root).write(self.path, encoding="unicode")
        with self.assertRaisesRegex(ValueError, "lacks canonical source_restrict"):
            check_architecture(self.path, "restrict_one")

    def test_zero_does_not_authorize_restrict(self) -> None:
        root = ET.Element("module")
        function = ET.SubElement(root, "function", symbol="restrict_typedef")
        parameters = ET.SubElement(function, "parameters")
        ET.SubElement(parameters, "parameter", port="input", source_restrict="0")
        ET.SubElement(parameters, "parameter", port="output", source_restrict="0")
        ET.ElementTree(root).write(self.path, encoding="unicode")
        with self.assertRaisesRegex(ValueError, "restrict_typedef.input has source_restrict=0, expected 1"):
            check_architecture(self.path, "restrict_typedef")


if __name__ == "__main__":
    unittest.main()
