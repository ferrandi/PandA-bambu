#!/usr/bin/env python3
"""Unit tests for the isolated manual F1b artifact checker."""
import pathlib
import os
import re
import tempfile
import unittest
import warnings
from contextlib import redirect_stderr
from io import StringIO

from check_write_compiler_artifacts import check_case, main
from compiler_case_manifest import load_cases, validate_fixture

REPO = pathlib.Path(__file__).resolve().parents[3]
TMP_ROOT = pathlib.Path(os.environ.get("BURST_TEST_TMPDIR", pathlib.Path.cwd())) / f"out_{pathlib.Path(__file__).stem}"


class CheckerTests(unittest.TestCase):
    def setUp(self):
        TMP_ROOT.mkdir(parents=True, exist_ok=True)
        self.temp = tempfile.TemporaryDirectory(prefix="write-compiler-checker-", dir=TMP_ROOT)
        self.root = pathlib.Path(self.temp.name)

    def tearDown(self):
        self.temp.cleanup()

    def write(self, name, content):
        path = self.root / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(content, encoding="utf-8")

    def write_write_success_artifacts(self, ir="pre-InterfaceInfer IR", log="    Configured write burst region for bundle output (B=16)\n"):
        self.write("design.bambuir", ir)
        self.write("stderr.log", log)
        self.write("design.v", "MinimalAXI4MasterPipelinedWrite writer();\n"
                    "fuselector_output_configure_write;\n"
                    "fuselector_output_store_write;\n"
                    "fuselector_output_drain_write;\n")
        self.write("panda_libtech.v", "module MinimalAXI4MasterPipelinedWrite; endmodule\n")

    def write_copy_success_artifacts(self):
        self.write("design.bambuir", "pre-InterfaceInfer source IR without transformed operations\n")
        self.write("stderr.log", "    Configured read burst regions for bundle input (B=16)\n"
                    "    Configured write burst region for bundle output (B=16)\n")
        self.write("design.v", "MinimalAXI4MasterPipelined reader();\n"
                    "MinimalAXI4MasterPipelinedWrite writer();\n"
                    "fuselector_input_configure_read;\nfuselector_input_burst_read;\n"
                    "fuselector_output_configure_write;\nfuselector_output_store_write;\n"
                    "fuselector_output_drain_write;\n"
                    "assign p_m_axi_input_araddr = burst_araddr;\n"
                    "assign p_m_axi_output_awaddr = burst_awaddr;\n")
        self.write("panda_libtech.v", "module MinimalAXI4MasterPipelined; endmodule\n"
                    "module MinimalAXI4MasterPipelinedWrite; endmodule\n")

    def test_accepted_uses_diagnostic_and_top_level_operation_signals(self):
        self.write_write_success_artifacts()
        args = type("Args", (), {"output": self.root, "kind": "accepted", "count_pattern": None})()
        self.assertEqual(check_case(args), 0)

    def test_accepted_rejects_pragma_echo_without_success_diagnostic(self):
        self.write_write_success_artifacts(log="#pragma HLS max_write_burst_length=16\n")
        args = type("Args", (), {"output": self.root, "kind": "accepted", "count_pattern": None})()
        with self.assertRaisesRegex(ValueError, "success diagnostic absent"):
            check_case(args)

    def test_count_pattern_matches_retained_ir_without_regex_warning(self):
        retained_ir = "function write_count region.bound\n"
        self.write_write_success_artifacts(retained_ir)
        args = type("Args", (), {
            "output": self.root,
            "kind": "accepted",
            "count_pattern": r"(%?[A-Za-z0-9_.]*n|region\.bound|write_count)",
        })()
        with warnings.catch_warnings():
            warnings.simplefilter("error", FutureWarning)
            self.assertEqual(check_case(args), 0)

    def test_copy_requires_both_directions_and_bundle_wiring(self):
        self.write_copy_success_artifacts()
        args = type("Args", (), {"output": self.root, "kind": "accepted", "count_pattern": None,
                                  "copy": True})()
        self.assertEqual(check_case(args), 0)

    def test_copy_rejects_writer_only_evidence(self):
        self.write_write_success_artifacts()
        args = type("Args", (), {"output": self.root, "kind": "accepted", "count_pattern": None,
                                  "copy": True})()
        with self.assertRaisesRegex(ValueError, "copy read-region success diagnostic"):
            check_case(args)

    def test_copy_rejects_bundle_wiring_mismatch(self):
        self.write_copy_success_artifacts()
        self.write("design.v", "MinimalAXI4MasterPipelined reader();\n"
                    "MinimalAXI4MasterPipelinedWrite writer();\n"
                    "fuselector_input_configure_read;\nfuselector_input_burst_read;\n"
                    "fuselector_output_configure_write;\nfuselector_output_store_write;\n"
                    "fuselector_output_drain_write;\n"
                    "assign p_m_axi_output_araddr = burst_araddr;\n"
                    "assign p_m_axi_input_awaddr = burst_awaddr;\n")
        args = type("Args", (), {"output": self.root, "kind": "accepted", "count_pattern": None,
                                  "copy": True})()
        with self.assertRaisesRegex(ValueError, "read component address is not wired"):
            check_case(args)

    def test_copy_requires_exact_distinct_read_and_write_profiles(self):
        self.write_copy_success_artifacts()
        args = type("Args", (), {
            "output": self.root,
            "kind": "accepted",
            "count_pattern": None,
            "copy": True,
            "expected_read_burst": 16,
            "expected_write_burst": 16,
            "expected_read_bundle": "input",
            "expected_write_bundle": "output",
        })()
        self.assertEqual(check_case(args), 0)
        self.write("stderr.log", "Configured read burst regions for bundle shared (B=16)\n"
                    "Configured write burst region for bundle shared (B=16)\n")
        with self.assertRaisesRegex(ValueError, "distinct bundles"):
            check_case(args)
        self.write("stderr.log", "Configured read burst regions for bundle input (B=8)\n"
                    "Configured write burst region for bundle output (B=16)\n")
        with self.assertRaisesRegex(ValueError, "read region uses B=8, expected B=16"):
            check_case(args)

    def test_inactive_shared_bundle_requires_one_writer_and_no_legacy_adapter(self):
        self.write_write_success_artifacts()
        self.write("design.v", "MinimalAXI4MasterPipelinedWrite burst_engine();\n"
                    "fuselector_output_configure_write;\n"
                    "fuselector_output_store_write;\n"
                    "fuselector_output_drain_write;\n"
                    "assign p_m_axi_output_awaddr = burst_awaddr;\n")
        args = type("Args", (), {"output": self.root, "kind": "accepted", "count_pattern": None,
                                  "single_write_bundle": "output"})()
        self.assertEqual(check_case(args), 0)
        self.write("design.v", "MinimalAXI4MasterPipelinedWrite burst_engine();\n"
                    "MinimalAXI4AdapterSingleBeat legacy();\n"
                    "fuselector_output_configure_write;\n"
                    "fuselector_output_store_write;\n"
                    "fuselector_output_drain_write;\n"
                    "assign p_m_axi_output_awaddr = burst_awaddr;\n")
        with self.assertRaisesRegex(ValueError, "legacy memory resource"):
            check_case(args)

    def test_fallback_requires_reason_legacy_and_no_residual_ops(self):
        self.write("stdout.log", "    Keeping legacy m_axi writes for bundle data: unsupported stride\n")
        self.write("design.v", "MinimalAXI4AdapterSingleBeat adapter();\n")
        self.write("panda_libtech.v", "module MinimalAXI4MasterPipelinedWrite; endmodule\n")
        args = type("Args", (), {"output": self.root, "kind": "fallback", "bundle": "data",
                                  "direction": "writes", "reason": "unsupported stride"})()
        self.assertEqual(check_case(args), 0)

    def test_fallback_rejects_pragma_path_or_banner_echo_with_legacy_rtl(self):
        self.write("stdout.log", "#pragma HLS interface max_write_burst_length=16 unsupported stride\n"
                    "/tmp/source.cpp: unsupported stride\n"
                    "bambu-feature/axi-loops: unsupported stride\n")
        self.write("design.v", "MinimalAXI4AdapterSingleBeat adapter();\n")
        args = type("Args", (), {"output": self.root, "kind": "fallback", "bundle": "data",
                                  "direction": "writes", "reason": "unsupported stride"})()
        with self.assertRaisesRegex(ValueError, "legacy fallback diagnostic absent"):
            check_case(args)

    def test_fallback_rejects_wrong_bundle_or_wrong_complete_reason(self):
        self.write("stdout.log", "    Keeping legacy m_axi writes for bundle other: unsupported stride\n")
        self.write("design.v", "MinimalAXI4AdapterSingleBeat adapter();\n")
        args = type("Args", (), {"output": self.root, "kind": "fallback", "bundle": "data",
                                  "direction": "writes", "reason": "unsupported stride"})()
        with self.assertRaisesRegex(ValueError, "legacy fallback diagnostic absent"):
            check_case(args)

    def test_fallback_requires_exact_reason_and_distinguishes_missing_diagnostic(self):
        self.write("design.v", "MinimalAXI4AdapterSingleBeat adapter();\n")
        args = type("Args", (), {"output": self.root, "kind": "fallback", "bundle": "data",
                                  "direction": "writes", "reason": "cache-enabled bundle"})()
        with self.assertRaisesRegex(ValueError, "legacy fallback diagnostic absent"):
            check_case(args)
        self.write("stdout.log", "Keeping legacy m_axi writes for bundle data: cache-enabled bundle extra\n")
        with self.assertRaisesRegex(ValueError, "legacy fallback reason mismatch"):
            check_case(args)
        self.write("stdout.log", "Keeping legacy m_axi writes for bundle data: cache-enabled bundle\n")
        self.assertEqual(check_case(args), 0)

    def test_feature_off_requires_exact_diagnostic_and_legacy_structure(self):
        self.write("stderr.log", "    Keeping legacy m_axi writes for bundle data: explicit write-burst request ignored because m-axi-burst=0\n")
        self.write("design.v", "MinimalAXI4AdapterSingleBeat adapter();\n")
        args = type("Args", (), {"output": self.root, "kind": "fallback", "bundle": "data",
                                  "direction": "writes", "reason": "feature-off", "feature_off": True})()
        self.assertEqual(check_case(args), 0)
        self.write("stderr.log", "    Keeping legacy m_axi writes for bundle data: request ignored because disabled\n")
        with self.assertRaisesRegex(ValueError, "reason mismatch"):
            check_case(args)

    def test_feature_off_read_fallback_accepts_exact_read_diagnostic(self):
        self.write("stderr.log", "    Keeping legacy m_axi reads for bundle input: "
                    "explicit read-burst request ignored because m-axi-burst=0\n")
        self.write("design.v", "MinimalAXI4AdapterSingleBeat adapter();\n")
        args = type("Args", (), {
            "output": self.root,
            "kind": "fallback",
            "bundle": "input",
            "direction": "reads",
            "reason": "explicit read-burst request ignored because m-axi-burst=0",
            "feature_off": False,
        })()
        self.assertEqual(check_case(args), 0)

    def test_legacy_without_burst_request_rejects_feature_off_diagnostic(self):
        self.write("stdout.log", "    Keeping legacy m_axi writes for bundle data: explicit write-burst request ignored because m-axi-burst=0\n")
        self.write("design.v", "MinimalAXI4AdapterSingleBeat adapter();\n")
        args = type("Args", (), {"output": self.root, "kind": "legacy", "bundle": "data",
                                  "require_legacy": True})()
        with self.assertRaisesRegex(ValueError, "emitted without an explicit burst request"):
            check_case(args)
        self.write("stdout.log", "bambu-feature/axi-loops\n")
        self.assertEqual(check_case(args), 0)

    def test_legacy_requires_bundle_and_positive_resource_even_with_empty_logs(self):
        self.write("stdout.log", "")
        self.write("design.v", "MinimalAXI4AdapterSingleBeat adapter();\n")
        missing_bundle = type("Args", (), {"output": self.root, "kind": "legacy",
                                            "bundle": "", "require_legacy": True})()
        with self.assertRaisesRegex(ValueError, "require --bundle"):
            check_case(missing_bundle)
        missing_flag = type("Args", (), {"output": self.root, "kind": "legacy",
                                          "bundle": "data", "require_legacy": False})()
        with self.assertRaisesRegex(ValueError, "require --require-legacy"):
            check_case(missing_flag)
        args = type("Args", (), {"output": self.root, "kind": "legacy",
                                  "bundle": "data", "require_legacy": True})()
        self.assertEqual(check_case(args), 0)
        self.write("design.v", "module unrelated; endmodule\n")
        with self.assertRaisesRegex(ValueError, "does not instantiate a legacy/cache"):
            check_case(args)

    def test_legacy_cli_requires_both_guard_flags(self):
        for argv, expected in ((["legacy", "--output", str(self.root)], "--bundle"),
                               (["legacy", "--output", str(self.root), "--bundle", "data"],
                                "--require-legacy")):
            stderr = StringIO()
            with redirect_stderr(stderr), self.assertRaises(SystemExit) as error:
                main(argv)
            self.assertEqual(error.exception.code, 2)
            self.assertIn(expected, stderr.getvalue())

    def test_fallback_rejects_residual_drain(self):
        self.write("stdout.log", "    Keeping legacy m_axi writes for bundle data: unsupported stride\n")
        self.write("design.v", "MinimalAXI4AdapterSingleBeat adapter();\nfuselector_output_drain_write;\n")
        args = type("Args", (), {"output": self.root, "kind": "fallback", "bundle": "data",
                                  "direction": "writes", "reason": "unsupported stride"})()
        with self.assertRaisesRegex(ValueError, "residual read/write burst operation"):
            check_case(args)

    def test_fallback_rejects_residual_read_operation_without_reader_component(self):
        self.write("stdout.log", "    Keeping legacy m_axi accesses for bundle shared: mixed-mode bundle\n")
        self.write("design.v", "MinimalAXI4AdapterSingleBeat adapter();\nfuselector_input_configure_read;\n")
        args = type("Args", (), {"output": self.root, "kind": "fallback", "bundle": "shared",
                                  "direction": "accesses", "reason": "mixed-mode bundle"})()
        with self.assertRaisesRegex(ValueError, "residual read/write burst operation"):
            check_case(args)

    def test_fallback_rejects_read_burst_component_too(self):
        self.write("stdout.log", "    Keeping legacy m_axi accesses for bundle shared: mixed-mode bundle\n")
        self.write("design.v", "MinimalAXI4AdapterSingleBeat adapter();\nMinimalAXI4MasterPipelined reader();\n")
        args = type("Args", (), {"output": self.root, "kind": "fallback", "bundle": "shared",
                                  "direction": "accesses", "reason": "mixed-mode bundle"})()
        with self.assertRaisesRegex(ValueError, "top-level RTL selects"):
            check_case(args)

    def test_fallback_rejects_unexpected_reason(self):
        self.write("stdout.log", "    Keeping legacy m_axi writes for bundle data: unsupported stride\n")
        self.write("design.v", "MinimalAXI4AdapterSingleBeat adapter();\n")
        args = type("Args", (), {"output": self.root, "kind": "fallback", "bundle": "data",
                                  "direction": "writes", "reason": "cache"})()
        with self.assertRaisesRegex(ValueError, "legacy fallback reason mismatch"):
            check_case(args)

    def test_invalid_rejects_needle_in_pragma_path_banner_or_dump_echo(self):
        needle = "Maximum AXI burst length must be an integer in [1,256]"
        self.write("stdout.log", f"#pragma HLS {needle}\n"
                    f"/build/{needle}.cpp: failed\n"
                    f"bambu-feature/{needle}: enabled\n"
                    f"Bitwidth dump: {needle}\n")
        args = type("Args", (), {"output": self.root, "kind": "invalid",
                                  "reason": re.escape(needle)})()
        with self.assertRaisesRegex(ValueError, "diagnostic did not match"):
            check_case(args)

    def test_invalid_accepts_error_arrow_diagnostic_but_rejects_emitted_rtl(self):
        self.write("stdout.log", "error -> Maximum AXI burst length must be an integer in [1,256]\n")
        args = type("Args", (), {"output": self.root, "kind": "invalid",
                                  "reason": r"Maximum AXI burst length must be an integer in \[1,256\]"})()
        self.assertEqual(check_case(args), 0)
        self.write("generated.sv", "module partial_output; endmodule\n")
        with self.assertRaisesRegex(ValueError, "emitted RTL"):
            check_case(args)

    def test_invalid_accepts_file_location_error_diagnostic(self):
        self.write("stdout.log", "/build/case.cpp:2:69: error: Maximum AXI burst length must be an integer in [1,256]\n")
        args = type("Args", (), {"output": self.root, "kind": "invalid",
                                  "reason": r"Maximum AXI burst length must be an integer in \[1,256\]"})()
        self.assertEqual(check_case(args), 0)

    def test_invalid_accepts_plugin_error_from_cc_output(self):
        self.write("panda-temp/__cc_output0",
                   "error -> Maximum AXI burst length must be an integer in [1,256]\n")
        args = type("Args", (), {"output": self.root, "kind": "invalid",
                                  "reason": r"Maximum AXI burst length must be an integer in \[1,256\]"})()
        self.assertEqual(check_case(args), 0)

    def test_profile_verifies_architecture_attributes(self):
        self.write("design.bambuir", "call void @configure_write(i32 %n)\ncall void @store_write()\ncall void @drain_write()\n")
        self.write("design.v", "MinimalAXI4MasterPipelinedWrite writer();\n")
        self.write("architecture.xml", '<architecture><function><bundles><bundle name="data" write_burst_outstanding="1" write_fifo_depth="64"/></bundles></function></architecture>')
        args = type("Args", (), {"output": self.root, "kind": "profile", "expect_profile_attribute": ["write_burst_outstanding=1", "write_fifo_depth=64"]})()
        self.assertEqual(check_case(args), 0)

    def test_profile_verifies_effective_defaults_in_write_engine_rtl(self):
        self.write("design.v", "MinimalAXI4MasterPipelinedWrite #(.B_MAX(3), .MAX_OUTSTANDING(1), .FIFO_DEPTH(8)) writer();\n")
        args = type("Args", (), {
            "output": self.root,
            "kind": "profile",
            "expect_profile_attribute": [],
            "expect_engine_parameter": ["B_MAX=3", "MAX_OUTSTANDING=1", "FIFO_DEPTH=8"],
        })()
        self.assertEqual(check_case(args), 0)
        args.expect_engine_parameter = ["B_MAX=3", "MAX_OUTSTANDING=1", "FIFO_DEPTH=16"]
        with self.assertRaisesRegex(ValueError, "one write engine.*FIFO_DEPTH=16"):
            check_case(args)

    def test_profile_rejects_engine_parameters_split_across_instances(self):
        self.write("design.v", "MinimalAXI4MasterPipelinedWrite #(.B_MAX(3), .MAX_OUTSTANDING(1)) first();\n"
                    "MinimalAXI4MasterPipelinedWrite #(.FIFO_DEPTH(8)) second();\n")
        args = type("Args", (), {
            "output": self.root,
            "kind": "profile",
            "expect_profile_attribute": [],
            "expect_engine_parameter": ["B_MAX=3", "MAX_OUTSTANDING=1", "FIFO_DEPTH=8"],
        })()
        with self.assertRaisesRegex(ValueError, "one write engine"):
            check_case(args)

    def test_profile_rejects_missing_serialized_value(self):
        self.write("design.bambuir", "call void @configure_write(i32 %n)\ncall void @store_write()\ncall void @drain_write()\n")
        self.write("design.v", "MinimalAXI4MasterPipelinedWrite writer();\n")
        self.write("architecture.xml", '<architecture><function><bundles><bundle name="data" write_burst_outstanding="1" write_fifo_depth="32"/></bundles></function></architecture>')
        args = type("Args", (), {"output": self.root, "kind": "profile", "expect_profile_attribute": ["write_fifo_depth=64"]})()
        with self.assertRaisesRegex(ValueError, "does not serialize write_fifo_depth=64"):
            check_case(args)

    def _fixture_block(self, source, name):
        return source.split(f'extern "C" void {name}', 1)[1].split('extern "C"', 1)[0]

    def test_positive_copy_fixtures_state_the_cross_bundle_restrict_contract(self):
        """A pair of distinct m_axi bundles is coalesced only when the caller states that the two
        ranges are disjoint: the accepted copy fixtures must carry __restrict on both pointers."""
        source = (pathlib.Path(__file__).parent / "write_compiler_cases" / "write_cases.cpp").read_text()
        for name in ("copy_distinct_buffers", "copy_distinct_buffers_reversed"):
            block = self._fixture_block(source, name)
            self.assertEqual(block.count("__restrict"), 2, name)
            self.assertIn("int *unused", block)
            self.assertIn("bundle=output", block)
            self.assertIn("bundle=input", block)
            self.assertIn("dst[i] = src[i]", block)

    def test_float_copy_has_bit_preserving_fixture_and_modelsims_on_off_gate(self):
        directory = pathlib.Path(__file__).parent
        source = (directory / "write_compiler_cases" / "write_cases.cpp").read_text()
        block = self._fixture_block(source, "copy_float_payload")
        self.assertEqual(block.count("__restrict"), 2)
        self.assertIn("float *__restrict dst", block)
        self.assertIn("const float *__restrict src", block)
        self.assertIn("bundle=output max_write_burst_length=16", block)
        self.assertIn("bundle=input max_read_burst_length=16", block)
        self.assertIn("dst[i] = src[i]", block)

        testbench = (directory / "bambu_axi_burst_write_tb.cpp").read_text()
        float_testbench = testbench.split("#ifdef BURST_TEST_FLOAT_COPY", 1)[1].split("#else", 1)[0]
        self.assertIn("memcpy", float_testbench)
        self.assertIn("__m_param_size(0)", float_testbench)
        self.assertIn("__m_param_size(1)", float_testbench)
        self.assertIn("0x80000000u", float_testbench)
        self.assertIn("AXI_BURST_FLOAT_COPY_GOLDEN_PASS N=%d", float_testbench)
        self.assertIn("const int n = 257", float_testbench)
        self.assertGreater(float_testbench.index("copy_float_payload(dst, src, n)"),
                           float_testbench.index("memcpy(&dst[i], &canary"))
        self.assertGreater(float_testbench.index("if (__m_param_size(0)"),
                           float_testbench.index("copy_float_payload(dst, src, n)"))
        self.assertIn("BURST_TEST_MAXN * sizeof(float)", float_testbench)

        gate = (directory / "run_write_compiler_tests.sh").read_text()
        self.assertIn("run_accepted copy_float_payload", gate)
        self.assertIn("--expected-read-bundle input --expected-write-bundle output", gate)
        self.assertIn("--simulate --simulator=MODELSIM", gate)
        self.assertIn("for feature in 1 0", gate)
        self.assertIn("--tb-param-size=src:16384 --tb-param-size=dst:16384", gate)
        self.assertIn("--bundle input --direction reads", gate)
        self.assertIn("1 input accesses", gate)
        self.assertNotIn("--simulator=VERILATOR", gate)

    def test_unused_nonrestrict_write_owner_does_not_block_cross_bundle_copy(self):
        source = (pathlib.Path(__file__).parent / "write_compiler_cases" / "write_cases.cpp").read_text()
        block = self._fixture_block(source, "copy_distinct_with_unused_write_owner")
        self.assertEqual(block.count("__restrict"), 2)
        self.assertIn("int *unused", block)
        self.assertIn("port=unused offset=direct bundle=output", block)
        self.assertIn("port=dst offset=direct bundle=output max_write_burst_length=16", block)
        self.assertIn("port=src offset=direct bundle=input max_read_burst_length=16", block)
        gate = (pathlib.Path(__file__).parent / "run_write_compiler_tests.sh").read_text()
        self.assertIn("run_case copy_distinct_with_unused_write_owner", gate)
        self.assertIn("--copy --single-write-bundle output", gate)

    def test_unrestricted_copy_fixture_keeps_the_scalar_fallback(self):
        """The corpus must keep the unrestricted twin, whose cross-bundle bursts are refused."""
        source = (pathlib.Path(__file__).parent / "write_compiler_cases" / "write_cases.cpp").read_text()
        block = self._fixture_block(source, "copy_unrestricted_buffers")
        self.assertNotIn("__restrict", block)
        self.assertIn("bundle=output", block)
        self.assertIn("bundle=input", block)
        self.assertIn("dst[i] = src[i]", block)
        gate = (pathlib.Path(__file__).parent / "run_write_compiler_tests.sh").read_text()
        self.assertIn("run_fallback copy_unrestricted_buffers", gate)

    def test_same_bundle_read_write_source_is_not_another_bundle_alias_case(self):
        source = (pathlib.Path(__file__).parent / "write_compiler_cases" / "write_cases.cpp").read_text()
        block = source.split("extern \"C\" void read_write_same_bundle", 1)[1]
        self.assertIn("port=data offset=direct bundle=shared max_read_burst_length=16", block)
        self.assertIn("port=data offset=direct bundle=shared max_write_burst_length=16", block)
        self.assertIn("data[i] = data[i] + 1", block)

    def test_distinct_buffers_on_same_bundle_are_a_guarded_duplex_source_shape(self):
        source = (pathlib.Path(__file__).parent / "write_compiler_cases" / "write_cases.cpp").read_text()
        block = source.split('extern "C" void read_write_distinct_buffers_same_bundle', 1)[1].split('extern "C"', 1)[0]
        self.assertIn("port=dst offset=direct bundle=shared max_write_burst_length=16", block)
        self.assertIn("port=src offset=direct bundle=shared max_read_burst_length=16", block)
        self.assertIn("dst[i] = src[i]", block)

    def write_duplex_artifacts(self, activation_bundle="shared", fallback_line="",
                               instantiate_aggregator=True):
        self.write("design.bambuir", "pre-InterfaceInfer IR with two distinct pointers on one bundle\n")
        self.write("stderr.log",
                   f"    Activated guarded distinct-base duplex for bundle {activation_bundle} "
                   "(overlap uses serialized scalar transfers)\n" + fallback_line)
        rtl = ("module shared_bambu_artificial_ParmMgr_duplex_read_endpoint; endmodule\n"
               "module shared_bambu_artificial_ParmMgr_duplex_write_endpoint; endmodule\n"
               "module shared_bambu_artificial_ParmMgr_duplex_aggregator_modgen; endmodule\n"
               "  MinimalAXI4MasterPipelined duplex_read_engine();\n"
               "  MinimalAXI4MasterPipelinedWrite duplex_write_engine();\n"
               "  output [31:0] p_m_axi_shared_awaddr;\n"
               "  input [31:0] p_m_axi_shared_araddr;\n")
        if instantiate_aggregator:
            rtl += ("  shared_bambu_artificial_ParmMgr_duplex_aggregator_modgen duplex_aggregator_0 "
                    "(.burst_awaddr(burst_awaddr));\n")
        self.write("design.v", rtl)

    def duplex_args(self, bundle="shared"):
        return type("Args", (), {"output": self.root, "kind": "duplex", "bundle": bundle})()

    def test_duplex_kind_accepts_guarded_duplex_structure(self):
        self.write_duplex_artifacts()
        self.assertEqual(check_case(self.duplex_args()), 0)

    def test_duplex_kind_rejects_legacy_fallback_on_the_bundle(self):
        self.write_duplex_artifacts(
            fallback_line="    Keeping legacy m_axi writes for bundle shared: bundle contains reads; "
                          "same-bundle read/write requires duplex support\n")
        with self.assertRaisesRegex(ValueError, "fell back to legacy for bundle shared"):
            check_case(self.duplex_args())

    def test_duplex_kind_requires_activation_for_the_checked_bundle(self):
        self.write_duplex_artifacts(activation_bundle="other")
        with self.assertRaisesRegex(ValueError,
                                    "exactly one guarded distinct-base duplex activation for bundle shared"):
            check_case(self.duplex_args())

    def test_duplex_kind_requires_instantiated_aggregator(self):
        self.write_duplex_artifacts(instantiate_aggregator=False)
        with self.assertRaisesRegex(ValueError, "duplex aggregator is not instantiated"):
            check_case(self.duplex_args())

    def test_duplex_cli_requires_bundle(self):
        stderr = StringIO()
        with redirect_stderr(stderr), self.assertRaises(SystemExit) as error:
            main(["duplex", "--output", str(self.root)])
        self.assertEqual(error.exception.code, 2)
        self.assertIn("--bundle", stderr.getvalue())

    def test_seeded_reinvocation_witness_uses_one_bundle_and_runtime_seed(self):
        source = (pathlib.Path(__file__).parent / "write_compiler_cases" / "write_cases.cpp").read_text()
        block = source.split('extern "C" void write_count_seed', 1)[1].split('extern "C"', 1)[0]
        self.assertIn("bundle=output max_write_burst_length=16", block)
        self.assertIn("dst[i] = seed + i", block)

    def test_inactive_shared_bundle_witnesses_cover_both_argument_orders(self):
        source = (pathlib.Path(__file__).parent / "write_compiler_cases" / "write_cases.cpp").read_text()
        for name in ("write_with_unused_same_bundle", "write_with_unused_same_bundle_reversed"):
            block = source.split(f'extern "C" void {name}', 1)[1].split('extern "C"', 1)[0]
            self.assertEqual(block.count("bundle=output"), 2)
            self.assertIn("max_write_burst_length=16", block)
            self.assertIn("dst[i] = i + 7", block)
            self.assertIn("unused", block)

    def test_invalid_profiles_are_isolated_from_shared_fixture(self):
        cases = pathlib.Path(__file__).parent / "write_compiler_cases"
        ordinary = (cases / "write_cases.cpp").read_text()
        b1_signature = 'extern "C" void write_count_seed_single_beat'
        self.assertEqual(ordinary.count(b1_signature), 1)
        b1_block = b1_signature + ordinary.split(b1_signature, 1)[1].split('extern "C"', 1)[0]
        self.assertIn("write_fifo_depth=8", b1_block)
        self.assertNotIn("max_write_burst_length", b1_block)
        ordinary = ordinary.replace(b1_block, "", 1)
        for invalid_value in ("max_write_burst_length=0", "write_burst_outstanding=17",
                              "write_fifo_depth=48", "write_fifo_depth=8"):
            self.assertNotIn(invalid_value, ordinary)
        manifest = pathlib.Path(__file__).with_name("compiler_profile_cases.tsv")
        fixture = cases / "write_profile_matrix.cpp"
        profile_cases = load_cases(manifest)
        validate_fixture(profile_cases, fixture, "write")
        invalid = {case.legacy_id: case for case in profile_cases
                   if case.direction == "write" and case.outcome == "invalid"}
        self.assertEqual(set(invalid), {
            "invalid-zero", "invalid-burst-257", "invalid-outstanding-zero", "invalid-outstanding",
            "invalid-fifo-4097", "invalid-fifo-not-power-two", "invalid-fifo-below-bmax",
            "non-m-axi-profile", "conflicting-write-profile", "conflicting-write-fifo-profile",
        })
        source = fixture.read_text()
        for case in invalid.values():
            self.assertIn(f"defined({case.macro})", source)
            self.assertEqual(source.count(f'void {case.top}('), 1, case.legacy_id)
        for top, pragma in {
            "invalid_write_profile": "max_write_burst_length=0",
            "write_burst_257": "max_write_burst_length=257",
            "write_outstanding_zero": "write_burst_outstanding=0",
            "write_fifo_out_of_range": "write_fifo_depth=4097",
            "write_attributes_non_m_axi": "mode=array",
            "invalid_write_outstanding": "write_burst_outstanding=17",
            "invalid_write_fifo_non_power_two": "write_fifo_depth=48",
            "invalid_write_fifo_too_shallow": "write_fifo_depth=8",
            "conflicting_write_burst_profile": "max_write_burst_length=32",
            "conflicting_write_fifo_profile": "write_fifo_depth=64",
        }.items():
            self.assertIn(pragma, source[source.index(f"void {top}("):], top)

    def test_profiles_with_default_burst_are_not_invalid_fixtures(self):
        """An O/FIFO hint without max burst defaults to one beat in each direction."""
        cases = pathlib.Path(__file__).parent / "write_compiler_cases"
        source = (cases / "write_profile_matrix.cpp").read_text()
        self.assertIn("WRITE_PROFILE_OUTSTANDING_2", source)
        self.assertIn("write_burst_outstanding=2", source)
        def case_branch(macro):
            match = re.search(rf"#(?:if|elif) defined\({macro}\)(.*?)(?=\n#elif defined|\Z)", source, re.S)
            self.assertIsNotNone(match, macro)
            return match.group(1)

        default_burst = case_branch("WRITE_PROFILE_OUTSTANDING_DEFAULT_BURST")
        self.assertIn("write_burst_outstanding=1", default_burst)
        self.assertNotIn("max_write_burst_length", default_burst)
        default_fifo = case_branch("WRITE_PROFILE_FIFO_DEFAULT_BURST")
        self.assertIn("write_fifo_depth=64", default_fifo)
        self.assertNotIn("max_write_burst_length", default_fifo)
        profiles = {case.legacy_id: case for case in load_cases(pathlib.Path(__file__).with_name("compiler_profile_cases.tsv"))}
        self.assertEqual((profiles["outstanding-default-burst"].burst,
                          profiles["outstanding-default-burst"].outstanding,
                          profiles["outstanding-default-burst"].fifo), ("1", "1", "2"))
        self.assertEqual((profiles["fifo-default-burst"].burst,
                          profiles["fifo-default-burst"].outstanding,
                          profiles["fifo-default-burst"].fifo), ("1", "16", "64"))
        runner = (pathlib.Path(__file__).parent / "run_write_compiler_tests.sh").read_text()
        self.assertIn("write_max=1(single-beat-default:fifo-hint) write_outstanding=16(default) "
                      "write_fifo=64(explicit)", runner)

    def test_profile_matrix_covers_supported_boundaries_and_inactive_attributes(self):
        source = (pathlib.Path(__file__).parent / "write_compiler_cases" / "write_profile_matrix.cpp").read_text()
        for witness in ("max_write_burst_length=1", "max_write_burst_length=3",
                        "max_write_burst_length=256", "write_burst_outstanding=1",
                        "write_burst_outstanding=3", "write_burst_outstanding=16",
                        "write_duplicate_equal_profile"):
            self.assertIn(witness, source)
        for top in ("write_burst_one", "write_burst_three_default_fifo", "write_burst_256",
                    "write_outstanding_one", "write_outstanding_three", "write_outstanding_sixteen",
                    "write_duplicate_equal_profile", "write_outstanding_with_default_burst",
                    "write_fifo_with_default_burst", "outstanding_two_profile", "valid_write_profile"):
            self.assertEqual(source.count(f'extern "C" void {top}('), 1, top)

    def test_profile_manifest_preserves_legacy_case_ids_and_fixture_coverage(self):
        directory = pathlib.Path(__file__).parent
        cases = load_cases(directory / "compiler_profile_cases.tsv")
        fixture = directory / "write_compiler_cases" / "write_profile_matrix.cpp"
        validate_fixture(cases, fixture, "write")
        read_fixture = directory / "compiler_cases" / "axi_read_profile_cases.cpp"
        validate_fixture(cases, read_fixture, "read")
        self.assertEqual(sum(case.direction == "write" for case in cases), 21)
        self.assertEqual(sum(case.direction == "read" for case in cases), 20)
        self.assertEqual(len(cases), 41)
        self.assertEqual(len({case.legacy_id for case in cases}), len(cases))

if __name__ == "__main__":
    unittest.main()
