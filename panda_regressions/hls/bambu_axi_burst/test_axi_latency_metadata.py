#!/usr/bin/env python3
"""Offline contract tests for AXI latency pragma/XML metadata handling."""

import unittest
import importlib.util
from pathlib import Path
import subprocess
from tempfile import TemporaryDirectory


ROOT = Path(__file__).resolve().parents[3]
RUNNER = Path(__file__).with_name("run_latency_metadata_tests.py")


def load_runner():
    spec = importlib.util.spec_from_file_location("latency_metadata_runner", RUNNER)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"cannot load {RUNNER}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def parse_uint32_decimal(text):
    """Mirror the production digit/overflow contract without host-sized casts."""
    if not text:
        raise ValueError("empty")
    value = 0
    maximum = (1 << 32) - 1
    for char in text:
        if char < "0" or char > "9":
            raise ValueError("non-decimal ASCII")
        digit = ord(char) - ord("0")
        if value > (maximum - digit) // 10:
            raise ValueError("uint32 overflow")
        value = value * 10 + digit
    return value


def agree_for_shared_bundle(root_values):
    """Absent metadata has the specified zero default; reject any mismatch."""
    values = [0 if value is None else parse_uint32_decimal(value) for value in root_values]
    return len(set(values)) <= 1


class AxiLatencyMetadataTest(unittest.TestCase):
    def test_compiled_suite_creates_its_output_directory(self):
        scratch = ROOT / "documentation" / "tmp"
        scratch.mkdir(parents=True, exist_ok=True)
        with TemporaryDirectory(dir=scratch) as temporary:
            output = Path(temporary) / "compiler" / "latency-metadata"
            prepared = load_runner().prepare_output(output)
            self.assertEqual(prepared, output.resolve(strict=True))
            self.assertTrue(prepared.is_dir())
            with self.assertRaises(FileExistsError):
                load_runner().prepare_output(output)

    def test_frontend_fixture_selection_isolated_per_root(self):
        self.assertEqual(
            load_runner().select_case_defines("latency_omitted,latency_zero"),
            ["--extra-cc-options=-DLATENCY_CASE_LATENCY_OMITTED",
             "--extra-cc-options=-DLATENCY_CASE_LATENCY_ZERO"],
        )
        self.assertEqual(
            load_runner().select_case_defines("latency_omitted,latency_zero", reverse_root_order=True)[-1],
            "--extra-cc-options=-DLATENCY_REVERSE_ROOT_ORDER",
        )

    def test_multi_root_acceptance_only_allows_backend_single_top_guard(self):
        recognizes = load_runner().is_expected_multitop_backend_guard
        self.assertTrue(recognizes(subprocess.CompletedProcess(
            args=[], returncode=11, stdout="Expected single top function name", stderr="")))
        self.assertFalse(recognizes(subprocess.CompletedProcess(
            args=[], returncode=11, stdout="unrelated frontend error", stderr="")))
        self.assertFalse(recognizes(subprocess.CompletedProcess(
            args=[], returncode=0, stdout="Expected single top function name", stderr="")))
        self.assertFalse(recognizes(subprocess.CompletedProcess(
            args=[], returncode=139, stdout="Expected single top function name", stderr="")))

    def test_uint32_boundaries_and_leading_zeroes(self):
        self.assertEqual(parse_uint32_decimal("0"), 0)
        self.assertEqual(parse_uint32_decimal("00042"), 42)
        self.assertEqual(parse_uint32_decimal("4294967295"), (1 << 32) - 1)

    def test_invalid_forms_and_overflow(self):
        for value in ("", "+1", "-1", " 1", "1 ", "1x", "1.0", "4294967296", "9" * 10000):
            with self.subTest(value=value[:24]):
                with self.assertRaises(ValueError):
                    parse_uint32_decimal(value)

    def test_absent_and_zero_agree_in_either_order(self):
        self.assertTrue(agree_for_shared_bundle([None, "0"]))
        self.assertTrue(agree_for_shared_bundle(["0", None]))
        self.assertTrue(agree_for_shared_bundle([None, None]))

    def test_nonzero_mismatch_rejected_independent_of_root_order(self):
        self.assertFalse(agree_for_shared_bundle(["64", "128"]))
        self.assertFalse(agree_for_shared_bundle(["128", "64"]))
        self.assertFalse(agree_for_shared_bundle([None, "64"]))
        self.assertFalse(agree_for_shared_bundle(["64", None]))

    def test_production_paths_keep_latency_as_metadata_only(self):
        plugin = (ROOT / "etc/clang_plugin/plugin_ASTAnalyzer.cpp").read_text(encoding="utf-8")
        manager_h = (ROOT / "src/HLS/hls_manager.hpp").read_text(encoding="utf-8")
        manager_cpp = (ROOT / "src/HLS/hls_manager.cpp").read_text(encoding="utf-8")
        infer = (ROOT / "src/frontend_analysis/IR_analysis/InterfaceInfer.cpp").read_text(encoding="utf-8")

        self.assertIn('attr_id == "latency"', plugin)
        self.assertIn('std::numeric_limits<std::uint32_t>::max()', plugin)
        self.assertIn("Invalid AXI interface attribute 'latency' for bundle '", manager_cpp)
        self.assertIn('mode->second != "m_axi"', manager_cpp)
        self.assertIn("ParseAXIInterfaceLatency(iface_attr, bundle_name)", manager_cpp)
        self.assertIn('iface_latency', manager_h)
        self.assertIn('FunctionArchitecture::iface_latency', manager_cpp)
        self.assertIn('to_iface_attr("iface_" + std::string(a.name()))', manager_cpp)
        self.assertIn('ParseAXIInterfaceLatency(iface_attrs, bundle_name)', infer)
        self.assertIn('std::string("bambu_axi_latency")', infer)
        self.assertIn('attribute::STRING, std::to_string(requested_axi_latency)', infer)
        self.assertIn('existing_latency_value != requested_latency_value', infer)
        self.assertNotIn('set_execution_time(requested_axi_latency', infer)
        self.assertNotIn('mem_delay_read', infer[infer.find('ParseAXIInterfaceLatency(iface_attrs, bundle_name)'):])


if __name__ == "__main__":
    unittest.main()
