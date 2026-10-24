#!/usr/bin/env python3
"""Unit tests for read burst RTL profile matching."""
import pathlib
import sys
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from check_compiler_artifacts import (
    check_shared_profile_rtl,
    check_shared_profile_status,
    has_configured_engine,
)


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

    def test_shared_profile_accepts_both_roots_reaching_one_wrapper_profile(self):
        ok, reason = check_shared_profile_rtl(
            shared_profile_rtl(), ["renamed_sum", "configured32"], SHARED_PROFILE)
        self.assertTrue(ok, reason)

    def test_shared_profile_rejects_missing_second_root(self):
        ok, reason = check_shared_profile_rtl(
            shared_profile_rtl(include_second_root=False),
            ["renamed_sum", "configured32"], SHARED_PROFILE)
        self.assertFalse(ok)
        self.assertIn("missing root/PBI/datapath", reason)

    def test_shared_profile_rejects_root_wired_to_another_datapath(self):
        ok, reason = check_shared_profile_rtl(
            shared_profile_rtl(first_datapath="datapath_configured32"),
            ["renamed_sum", "configured32"], SHARED_PROFILE)
        self.assertFalse(ok)
        self.assertIn("does not instantiate exactly one datapath_renamed_sum", reason)

    def test_shared_profile_rejects_root_reaching_another_wrapper_type(self):
        ok, reason = check_shared_profile_rtl(
            shared_profile_rtl(first_wrapper="other_resource_wrapper"),
            ["renamed_sum", "configured32"], SHARED_PROFILE)
        self.assertFalse(ok)
        self.assertIn("must reach exactly one", reason)

    def test_shared_profile_rejects_wrong_engine_profile(self):
        wrong = ".B_MAX(32), .MAX_OUTSTANDING(1), .FIFO_DEPTH(32)"
        ok, reason = check_shared_profile_rtl(
            shared_profile_rtl(profile=wrong),
            ["renamed_sum", "configured32"], SHARED_PROFILE)
        self.assertFalse(ok)
        self.assertIn("expected profile on one instance", reason)

    def test_shared_profile_rejects_wrapper_missing_axi_read_wiring(self):
        rtl = shared_profile_rtl().replace(
            ".p_m_axi_data_rready(p_m_axi_data_rready));", ");")
        ok, reason = check_shared_profile_rtl(
            rtl, ["renamed_sum", "configured32"], SHARED_PROFILE)
        self.assertFalse(ok)
        self.assertIn("missing AXI read wiring", reason)

    def test_shared_profile_rejects_wrapper_rewired_to_another_bundle(self):
        rtl = shared_profile_rtl().replace(
            ".p_m_axi_data_araddr(p_m_axi_data_araddr)",
            ".p_m_axi_data_araddr(other_araddr)")
        ok, reason = check_shared_profile_rtl(
            rtl, ["renamed_sum", "configured32"], SHARED_PROFILE)
        self.assertFalse(ok)
        self.assertIn("does not preserve the shared AXI bundle", reason)

    def test_shared_profile_accepts_release_success_after_structural_proof(self):
        self.assertTrue(check_shared_profile_status(0, "", "")[0])

    def test_shared_profile_rejects_crash_markers_even_with_zero_status(self):
        self.assertFalse(check_shared_profile_status(
            0, "Segmentation fault (core dumped)\n", "")[0])
        self.assertFalse(check_shared_profile_status(
            0, "", "stack smashing detected\n")[0])

    def test_shared_profile_accepts_only_exact_debug_multitop_failure(self):
        self.assertTrue(check_shared_profile_status(
            1, "", KNOWN_MULTI_TOP_DIAGNOSTIC)[0])
        self.assertFalse(check_shared_profile_status(1, "", "error -> unrelated error\n")[0])
        self.assertFalse(check_shared_profile_status(
            0, KNOWN_MULTI_TOP_DIAGNOSTIC, "")[0])
        self.assertFalse(check_shared_profile_status(
            1, "", KNOWN_MULTI_TOP_DIAGNOSTIC + "error -> another failure\n")[0])
        self.assertFalse(check_shared_profile_status(
            139, "Segmentation fault", KNOWN_MULTI_TOP_DIAGNOSTIC)[0])
        self.assertFalse(check_shared_profile_status(
            0, "", KNOWN_MULTI_TOP_DIAGNOSTIC)[0])


def name_ports():
    return ", ".join(
        f".in{index}({'in5' if index == 5 else f'signal{index}'})"
        for index in range(1, 6))


SHARED_PROFILE = {"B_MAX": 16, "MAX_OUTSTANDING": 1, "FIFO_DEPTH": 32}


def shared_profile_rtl(profile=".B_MAX(16), .MAX_OUTSTANDING(1), .FIFO_DEPTH(32)",
                       first_datapath="datapath_renamed_sum",
                       first_wrapper="data_bambu_artificial_ParmMgr_modgen",
                       include_second_root=True):
    rtl = f"""
module renamed_sum;
  PBI_renamed_sum PBI_renamed_sum_i0 ();
endmodule
module PBI_renamed_sum;
  {first_datapath} Datapath_i ();
endmodule
module datapath_renamed_sum;
  {first_wrapper} #(.BITSIZE_in1(2)) wrapper0 (
    .p_m_axi_data_araddr(p_m_axi_data_araddr), .p_m_axi_data_arvalid(p_m_axi_data_arvalid),
    .p_m_axi_data_rdata(p_m_axi_data_rdata), .p_m_axi_data_rvalid(p_m_axi_data_rvalid),
    .p_m_axi_data_rready(p_m_axi_data_rready));
endmodule
"""
    if include_second_root:
        rtl += """
module configured32;
  PBI_configured32 PBI_configured32_i0 ();
endmodule
module PBI_configured32;
  datapath_configured32 Datapath_i ();
endmodule
module datapath_configured32;
  data_bambu_artificial_ParmMgr_modgen #(.BITSIZE_in1(2)) wrapper0 (
    .p_m_axi_data_araddr(p_m_axi_data_araddr), .p_m_axi_data_arvalid(p_m_axi_data_arvalid),
    .p_m_axi_data_rdata(p_m_axi_data_rdata), .p_m_axi_data_rvalid(p_m_axi_data_rvalid),
    .p_m_axi_data_rready(p_m_axi_data_rready));
endmodule
"""
    rtl += f"""
module data_bambu_artificial_ParmMgr_modgen;
  MinimalAXI4MasterPipelined #({profile}) burst_engine (
    .in1(a), .in2(b), .in3(c), .in4(d), .in5(e));
endmodule
"""
    return rtl


KNOWN_MULTI_TOP_DIAGNOSTIC = (
    "error -> Expected single top function name (top_symbols.size() == 1)\n"
    "\tvoid BackendWrapper::init(const HLS_managerRef&)\n"
    "\t/home/ci/src/wrapper/BackendWrapper.cpp:501\n"
)


if __name__ == "__main__":
    unittest.main()
