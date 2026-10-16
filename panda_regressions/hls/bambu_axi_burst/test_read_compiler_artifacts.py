#!/usr/bin/env python3
"""Unit tests for read burst RTL profile matching."""
import pathlib
import sys
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from check_compiler_artifacts import has_configured_engine


EXPECTED = {"B_MAX": 16, "MAX_OUTSTANDING": 2, "FIFO_DEPTH": 64}


def engine(parameters, ports=None, name="engine"):
    if ports is None:
        ports = ", ".join(
            f".in{index}({'in5' if index == 5 else f'signal{index}'})"
            for index in range(1, 6))
    return ("MinimalAXI4MasterPipelined #( " + parameters +
            f" ) {name} ( {ports} );")


class ReadCompilerArtifactTests(unittest.TestCase):
    def test_accepts_exact_profile_and_runtime_count_input(self):
        rtl = engine(".B_MAX(16), .MAX_OUTSTANDING(2), .FIFO_DEPTH(64)")
        self.assertTrue(has_configured_engine(rtl, EXPECTED))

    def test_accepts_reordered_profile_with_interspersed_parameters(self):
        rtl = engine(".FIFO_DEPTH(64), .BITSIZE_in5(32), .B_MAX(16), "
                     ".EXTRA(1), .MAX_OUTSTANDING(2)",
                     ".in1(a), .in2(b), .in3(c), .in4(d), .in5(in5)")
        self.assertTrue(has_configured_engine(rtl, EXPECTED))

    def test_rejects_wrong_parameter_value(self):
        rtl = engine(".B_MAX(16), .MAX_OUTSTANDING(3), .FIFO_DEPTH(64)")
        self.assertFalse(has_configured_engine(rtl, EXPECTED))

    def test_rejects_profile_split_across_instances(self):
        rtl = engine(".B_MAX(16), .MAX_OUTSTANDING(2), .FIFO_DEPTH(32)", name="first")
        rtl += "\n" + engine(".B_MAX(16), .MAX_OUTSTANDING(1), .FIFO_DEPTH(64)", name="second")
        self.assertFalse(has_configured_engine(rtl, EXPECTED))

    def test_rejects_required_ports_attached_to_different_instance(self):
        expected_profile = engine(".B_MAX(16), .MAX_OUTSTANDING(2), .FIFO_DEPTH(64)",
                                  ports=".in1(a), .in2(b)", name="profile")
        ports = engine(".B_MAX(8), .MAX_OUTSTANDING(1), .FIFO_DEPTH(16)",
                       ports=name_ports(), name="ports")
        self.assertFalse(has_configured_engine(expected_profile + "\n" + ports, EXPECTED))

    def test_rejects_missing_runtime_count_input(self):
        ports = ", ".join(f".in{index}(signal{index})" for index in range(1, 5))
        rtl = engine(".B_MAX(16), .MAX_OUTSTANDING(2), .FIFO_DEPTH(64)", ports=ports)
        self.assertFalse(has_configured_engine(rtl, EXPECTED))

    def test_rejects_runtime_count_input_connected_to_other_signal(self):
        ports = name_ports().replace(".in5(in5)", ".in5(signal5)")
        rtl = engine(".B_MAX(16), .MAX_OUTSTANDING(2), .FIFO_DEPTH(64)", ports=ports)
        self.assertFalse(has_configured_engine(rtl, EXPECTED))


def name_ports():
    return ", ".join(
        f".in{index}({'in5' if index == 5 else f'signal{index}'})"
        for index in range(1, 6))


if __name__ == "__main__":
    unittest.main()
