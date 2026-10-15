#!/usr/bin/env python3
"""Unit tests for the isolated manual F1b artifact checker."""
import pathlib
import re
import tempfile
import unittest
import warnings
from contextlib import redirect_stderr
from io import StringIO

from check_write_compiler_artifacts import check_case, main

REPO = pathlib.Path(__file__).resolve().parents[3]
TMP_ROOT = REPO / "documentation" / "tmp" / "f4-guard-gaps" / "a-offline"


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
        self.write("stderr.log", "    Keeping legacy m_axi writes for bundle data: explicit write-burst request ignored because experimental-m-axi-burst=0\n")
        self.write("design.v", "MinimalAXI4AdapterSingleBeat adapter();\n")
        args = type("Args", (), {"output": self.root, "kind": "fallback", "bundle": "data",
                                  "direction": "writes", "reason": "feature-off", "feature_off": True})()
        self.assertEqual(check_case(args), 0)
        self.write("stderr.log", "    Keeping legacy m_axi writes for bundle data: request ignored because disabled\n")
        with self.assertRaisesRegex(ValueError, "reason mismatch"):
            check_case(args)

    def test_legacy_without_burst_request_rejects_feature_off_diagnostic(self):
        self.write("stdout.log", "    Keeping legacy m_axi writes for bundle data: explicit write-burst request ignored because experimental-m-axi-burst=0\n")
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

    def test_positive_copy_fixtures_use_distinct_unrestricted_buffers(self):
        source = (pathlib.Path(__file__).parent / "write_compiler_cases" / "write_cases.cpp").read_text()
        for name in ("copy_distinct_buffers", "copy_distinct_buffers_reversed"):
            block = source.split(f'extern "C" void {name}', 1)[1].split('extern "C"', 1)[0]
            self.assertNotIn("restrict", block)
            self.assertIn("int *unused", block)
            self.assertIn("bundle=output", block)
            self.assertIn("bundle=input", block)
            self.assertIn("dst[i] = src[i]", block)

    def test_same_bundle_read_write_source_is_not_another_bundle_alias_case(self):
        source = (pathlib.Path(__file__).parent / "write_compiler_cases" / "write_cases.cpp").read_text()
        block = source.split("extern \"C\" void read_write_same_bundle", 1)[1]
        self.assertIn("port=data offset=direct bundle=shared max_read_burst_length=16", block)
        self.assertIn("port=data offset=direct bundle=shared max_write_burst_length=16", block)
        self.assertIn("data[i] = data[i] + 1", block)

    def test_distinct_buffers_on_same_bundle_still_require_legacy_fallback(self):
        source = (pathlib.Path(__file__).parent / "write_compiler_cases" / "write_cases.cpp").read_text()
        block = source.split('extern "C" void read_write_distinct_buffers_same_bundle', 1)[1].split('extern "C"', 1)[0]
        self.assertIn("port=dst offset=direct bundle=shared max_write_burst_length=16", block)
        self.assertIn("port=src offset=direct bundle=shared max_read_burst_length=16", block)
        self.assertIn("dst[i] = src[i]", block)

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
        for invalid_value in ("max_write_burst_length=0", "write_burst_outstanding=2",
                              "write_burst_outstanding=17", "write_fifo_depth=48",
                              "write_fifo_depth=8"):
            self.assertNotIn(invalid_value, ordinary)
        invalid_dir = cases / "invalid_profiles"
        expected = {
            "invalid_write_profile.cpp": "max_write_burst_length=0",
            "write_burst_257.cpp": "max_write_burst_length=257",
            "write_outstanding_zero.cpp": "write_burst_outstanding=0",
            "write_fifo_out_of_range.cpp": "write_fifo_depth=4097",
            "write_outstanding_without_burst.cpp": "write_burst_outstanding=1",
            "write_attributes_non_m_axi.cpp": "mode=array",
            "outstanding_two_profile.cpp": "write_burst_outstanding=2",
            "invalid_write_outstanding.cpp": "write_burst_outstanding=17",
            "invalid_write_fifo_non_power_two.cpp": "write_fifo_depth=48",
            "invalid_write_fifo_too_shallow.cpp": "write_fifo_depth=8",
            "invalid_write_fifo_without_burst.cpp": "write_fifo_depth=64",
            "conflicting_write_burst_profile.cpp": "max_write_burst_length=32",
            "conflicting_write_fifo_profile.cpp": "write_fifo_depth=64",
        }
        for filename, pragma in expected.items():
            text = (invalid_dir / filename).read_text()
            self.assertEqual(text.count('extern "C" void '), 1, filename)
            self.assertIn(pragma, text, filename)

    def test_profile_matrix_covers_supported_boundaries_and_inactive_attributes(self):
        source = (pathlib.Path(__file__).parent / "write_compiler_cases" / "write_profile_matrix.cpp").read_text()
        for witness in ("max_write_burst_length=1", "max_write_burst_length=3",
                        "max_write_burst_length=256", "write_burst_outstanding=1",
                        "write_burst_outstanding=16", "write_duplicate_equal_profile"):
            self.assertIn(witness, source)
        for top in ("write_burst_one", "write_burst_three_default_fifo", "write_burst_256",
                    "write_outstanding_one", "write_outstanding_sixteen",
                    "write_duplicate_equal_profile"):
            self.assertEqual(source.count(f'extern "C" void {top}('), 1, top)

if __name__ == "__main__":
    unittest.main()
