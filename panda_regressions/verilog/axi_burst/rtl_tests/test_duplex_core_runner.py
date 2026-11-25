from pathlib import Path
import unittest


RUNNER = Path(__file__).with_name("run_duplex_core_tests.sh")


class DuplexCoreRunnerContractTests(unittest.TestCase):
    def test_only_intentional_fault_cases_enable_expected_fault_mode(self):
        source = RUNNER.read_text()
        self.assertIn("fault_arg=(+AXI_BURST_EXPECT_FAULT)", source)
        self.assertIn("RRESP_FAULT_LOCAL|BRESP_FAULT_LOCAL|PEER_BRESP_DRAIN", source)
        self.assertIn("PEER_RRESP_RESERVED_WRITE_DRAIN|PEER_RRESP_FINAL_BEAT|PEER_FAULT_BLOCKS_RESERVATION|PEER_FAULT_RESET_RESTART", source)
        self.assertIn('"${fault_arg[@]}"', source)

    def test_local_peer_diagnostics_have_fixed_order_and_complete_payload_checks(self):
        source = RUNNER.read_text()
        for record in (
            "reason=RRESP instance=${base_instance}.reader.report_axi_read_diagnostic",
            "reason=BRESP instance=${base_instance}.writer.report_axi_write_diagnostic",
            "reason=peer instance=${base_instance}.reader.report_axi_read_diagnostic",
            "reason=peer instance=${base_instance}.writer.report_axi_write_diagnostic",
        ):
            self.assertIn(record, source)
        self.assertIn("grep -E '^AXI_BURST_'", source)
        self.assertIn("wrong AXI diagnostic count", source)
        self.assertIn("malformed read fault payload", source)
        self.assertIn("malformed write fault payload", source)

    def test_peer_fault_cases_expect_reciprocal_diagnostics_in_order(self):
        source = RUNNER.read_text().split("local -a expected_faults=()", 1)[1]
        cases = (
            ("PEER_BRESP_DRAIN)", (
                "AXI_BURST_FAULT engine=write reason=BRESP",
                "AXI_BURST_PEER_FAULT engine=read reason=peer",
                "AXI_BURST_PEER_FAULT engine=write reason=peer",
            )),
            ("PEER_RRESP_RESERVED_WRITE_DRAIN|PEER_RRESP_FINAL_BEAT)", (
                "AXI_BURST_FAULT engine=read reason=RRESP",
                "AXI_BURST_PEER_FAULT engine=write reason=peer",
                "AXI_BURST_PEER_FAULT engine=read reason=peer",
            )),
            ("PEER_FAULT_BLOCKS_RESERVATION)", (
                "AXI_BURST_PEER_FAULT engine=read reason=peer",
                "AXI_BURST_PEER_FAULT engine=write reason=peer",
            )),
            ("PEER_FAULT_RESET_RESTART)", (
                "AXI_BURST_PEER_FAULT engine=write reason=peer",
                "AXI_BURST_PEER_FAULT engine=read reason=peer",
            )),
        )
        for branch, records in cases:
            with self.subTest(branch=branch):
                start = source.index(branch)
                body = source[start:source.index(";;", start)]
                positions = [body.index(record) for record in records]
                self.assertEqual(positions, sorted(positions))

    def test_completion_marker_and_existing_profile_loops_remain(self):
        source = RUNNER.read_text()
        self.assertIn('grep -Eq "^DUPLEX_CORES PASS CASE=$case_name "', source)
        self.assertIn('run_profile_cases 0 1 "${BASE_CASES[@]}"', source)
        self.assertIn('run_profile_cases 1 1 "${PEER_FAULT_CASES[@]}"', source)
        self.assertIn('run_profile_cases 0 0 "${BASE_CASES[@]}"', source)
        self.assertIn('run_profile_cases 1 0 "${PEER_FAULT_CASES[@]}"', source)


if __name__ == "__main__":
    unittest.main()
