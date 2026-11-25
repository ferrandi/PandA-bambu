#!/usr/bin/env python3
"""Offline tests for the always-on fast STORE-ACK artifact checker."""

import argparse
import os
import pathlib
import shutil
import sys
import tempfile
import unittest

HERE = pathlib.Path(__file__).resolve().parent
TMP_ROOT = pathlib.Path(os.environ.get("BURST_TEST_TMPDIR", pathlib.Path.cwd())) / f"out_{pathlib.Path(__file__).stem}"
sys.path.insert(0, str(HERE))
import check_duplex_fast_store_ack_artifacts as checker


def verilog(writer=True, reader=True, fast="1", write_fifo=32, read_fifo=64):
    lines = ["module kernel;"]
    if reader:
        lines.append(
            f"MinimalAXI4MasterPipelined #(.B_MAX(16), .MAX_OUTSTANDING(1), "
            f".FIFO_DEPTH({read_fifo})) reader_engine (.clock(clock));")
    if writer:
        lines.append(
            f"MinimalAXI4MasterPipelinedWrite #(.B_MAX(16), .MAX_OUTSTANDING(1), "
            f".FIFO_DEPTH({write_fifo}), .FAST_STORE_ACK({fast})) writer_engine (.clock(clock));")
    lines.append("endmodule")
    return "\n".join(lines) + "\n"


def library(default="1"):
    return ("module MinimalAXI4MasterPipelinedWrite(clock); "
            f"parameter FAST_STORE_ACK={default}; endmodule\n")


class FastStoreAckArtifactTests(unittest.TestCase):
    def setUp(self):
        TMP_ROOT.mkdir(parents=True, exist_ok=True)
        self.scratch = pathlib.Path(tempfile.mkdtemp(prefix="fast_store_ack_checker_", dir=TMP_ROOT))

    def tearDown(self):
        shutil.rmtree(self.scratch)

    def output(self, name, rtl=None, lib=None):
        directory = self.scratch / name
        directory.mkdir()
        if rtl is not None:
            (directory / "kernel.v").write_text(rtl, encoding="utf-8")
        if lib is not None:
            (directory / "panda_libtech.v").write_text(lib, encoding="utf-8")
        return directory

    def success_args(self, output, kind="duplex"):
        return argparse.Namespace(output=output, kind=kind,
                                  write_burst=16, write_outstanding=1, write_fifo=32,
                                  read_burst=16, read_outstanding=1, read_fifo=64)

    def test_duplex_checks_always_on_fast_ack_and_both_profiles(self):
        output = self.output("duplex", verilog(fast="1"))
        writers, readers = checker.check_success(self.success_args(output))
        self.assertEqual(writers[0]["FAST_STORE_ACK"], "1")
        self.assertEqual(readers[0], {"B_MAX": "16", "MAX_OUTSTANDING": "1", "FIFO_DEPTH": "64"})

    def test_writer_without_fast_ack_is_rejected(self):
        output = self.output("duplex-slow", verilog(fast="0"))
        with self.assertRaisesRegex(ValueError, "profile mismatch"):
            checker.check_success(self.success_args(output))

    def test_expected_parameters_must_be_on_one_writer_instance(self):
        rtl = """module kernel;
MinimalAXI4MasterPipelined #(.B_MAX(16), .MAX_OUTSTANDING(1), .FIFO_DEPTH(64)) r (.x(x));
MinimalAXI4MasterPipelinedWrite #(.B_MAX(16), .MAX_OUTSTANDING(1), .FIFO_DEPTH(32), .FAST_STORE_ACK(1)) w0 (.x(x));
MinimalAXI4MasterPipelinedWrite #(.B_MAX(8), .MAX_OUTSTANDING(1), .FIFO_DEPTH(16), .FAST_STORE_ACK(1)) w1 (.x(x));
endmodule
"""
        output = self.output("split", rtl)
        with self.assertRaisesRegex(ValueError, "expected 1 MinimalAXI4MasterPipelinedWrite"):
            checker.check_success(self.success_args(output))

    def test_reader_profile_comparison_is_structural(self):
        left = self.output("reader-left", verilog())
        right = self.output("reader-right", verilog(read_fifo=64))
        left_instances = checker.parameterized_instances(checker.read_top(left), "MinimalAXI4MasterPipelined")
        right_instances = checker.parameterized_instances(checker.read_top(right), "MinimalAXI4MasterPipelined")
        self.assertEqual(left_instances, right_instances)
        changed = self.output("reader-changed", verilog(read_fifo=128))
        self.assertNotEqual(left_instances, checker.parameterized_instances(
            checker.read_top(changed), "MinimalAXI4MasterPipelined"))

    def test_write_only_requires_fast_ack_and_module_default_one(self):
        output = self.output("write-only", verilog(reader=False, fast="1"), library(default="1"))
        writers, _ = checker.check_success(self.success_args(output, kind="write-only"))
        self.assertEqual(writers[0]["FAST_STORE_ACK"], "1")

    def test_write_only_rejects_module_default_zero(self):
        output = self.output("write-only-default-zero", verilog(reader=False, fast="1"),
                             library(default="0"))
        with self.assertRaisesRegex(ValueError, "default FAST_STORE_ACK is not proven to be 1"):
            checker.check_success(self.success_args(output, kind="write-only"))

    def test_write_only_rejects_missing_libtech_library(self):
        output = self.output("write-only-no-lib", verilog(reader=False, fast="1"))
        with self.assertRaisesRegex(ValueError, "expected one generated panda_libtech.v"):
            checker.check_success(self.success_args(output, kind="write-only"))

    def test_compare_writers_requires_identical_fast_ack_profiles(self):
        left = self.output("writer-left", verilog(reader=False, fast="1"))
        right = self.output("writer-right", verilog(reader=False, fast="1"))
        checker.compare_writers(left, right)

    def test_compare_writers_rejects_profile_drift(self):
        left = self.output("writer-base", verilog(reader=False, fast="1"))
        drift = self.output("writer-drift", verilog(reader=False, fast="1", write_fifo=64))
        with self.assertRaisesRegex(ValueError, "profile differs between the compared kernels"):
            checker.compare_writers(left, drift)

    def test_compare_writers_allows_fault_enable_and_width_differences(self):
        def writer(fault):
            return ("module kernel;\nMinimalAXI4MasterPipelinedWrite #(.B_MAX(16), .MAX_OUTSTANDING(1), "
                    f".FIFO_DEPTH(32), .PEER_FAULT_ENABLE({fault}), .FAST_STORE_ACK(1), "
                    ".BITSIZE_in3(32)) w (.clock(clock));\nendmodule\n")
        left = self.output("writer-duplex", writer(1))
        right = self.output("writer-standalone", writer(0))
        checker.compare_writers(left, right)

    def test_compare_writers_rejects_slow_writer(self):
        left = self.output("writer-slow-left", verilog(reader=False, fast="0"))
        right = self.output("writer-slow-right", verilog(reader=False, fast="0"))
        with self.assertRaisesRegex(ValueError, "FAST_STORE_ACK is not 1"):
            checker.compare_writers(left, right)

    def test_read_only_has_no_write_engine(self):
        output = self.output("read-only", verilog(writer=False))
        checker.check_success(self.success_args(output, kind="read-only"))

    def inert_args(self, output):
        return argparse.Namespace(output=output)

    def test_removed_option_still_builds_fast_ack_kernel(self):
        output = self.output("legacy-option", verilog(reader=False, fast="1"), library(default="1"))
        (output / "stdout.log").write_text("", encoding="utf-8")
        (output / "stderr.log").write_text("", encoding="utf-8")
        writers = checker.check_inert_option(self.inert_args(output))
        self.assertEqual(writers[0]["FAST_STORE_ACK"], "1")

    def test_removed_option_rejects_error_log_or_slow_kernel(self):
        output = self.output("legacy-error")
        (output / "stdout.log").write_text("", encoding="utf-8")
        (output / "stderr.log").write_text("error -> unknown parameter\n", encoding="utf-8")
        with self.assertRaisesRegex(ValueError, "removed option produced a compiler error"):
            checker.check_inert_option(self.inert_args(output))
        (output / "stderr.log").write_text("", encoding="utf-8")
        (output / "kernel.v").write_text(verilog(reader=False, fast="0"), encoding="utf-8")
        with self.assertRaisesRegex(ValueError, "not fast-ack"):
            checker.check_inert_option(self.inert_args(output))
        (output / "kernel.v").write_text(verilog(reader=False, fast="1"), encoding="utf-8")
        (output / "panda_libtech.v").write_text(library(default="0"), encoding="utf-8")
        with self.assertRaisesRegex(ValueError, "default FAST_STORE_ACK is not proven to be 1"):
            checker.check_inert_option(self.inert_args(output))


if __name__ == "__main__":
    unittest.main()
