#!/usr/bin/env python3
import tempfile
import json
import os
import subprocess
import unittest
import xml.etree.ElementTree as ET
from pathlib import Path

from check_bundle_preanalysis_artifacts import CheckError, verify

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[2]
TMP_ROOT = Path(os.environ.get("BURST_TEST_TMPDIR", Path.cwd())) / f"out_{Path(__file__).stem}"


class BundlePreanalysisCheckerTests(unittest.TestCase):
    def setUp(self):
        TMP_ROOT.mkdir(parents=True, exist_ok=True)
        self.temp = tempfile.TemporaryDirectory(prefix="bundle-preanalysis-", dir=TMP_ROOT)
        self.addCleanup(self.temp.cleanup)
        self.output = Path(self.temp.name)
        (self.output / "panda-temp").mkdir()
        (self.output / "stdout.log").write_text("compiler stdout marker\n")
        (self.output / "stderr.log").write_text("compiler stderr marker\n")

    def set_artifacts(self, rtl, bundles, log="", parameters=()):
        (self.output / "design.v").write_text(rtl)
        (self.output / "stdout.log").write_text(log or "compiler stdout marker\n")
        root = ET.Element("module")
        function = ET.SubElement(root, "function", {"name": "design"})
        collection = ET.SubElement(function, "bundles")
        for name, attrs in bundles.items():
            ET.SubElement(collection, "bundle", {"name": name, **attrs})
        parameter_list = ET.SubElement(function, "parameters")
        for attrs in parameters:
            ET.SubElement(parameter_list, "parameter", attrs)
        ET.ElementTree(root).write(self.output / "panda-temp/architecture.xml")

    def set_dataflow_artifacts(self, top, rtl, functions, log="", stderr="", raw=""):
        (self.output / f"{top}.v").write_text(rtl)
        (self.output / "stdout.log").write_text(log or "compiler stdout marker\n")
        (self.output / "stderr.log").write_text(stderr or "compiler stderr marker\n")
        root = ET.Element("module")
        for name, bundles, parameters in functions:
            function = ET.SubElement(root, "function", {"name": name})
            collection = ET.SubElement(function, "bundles")
            for bundle_name, attrs in bundles.items():
                ET.SubElement(collection, "bundle", {"name": bundle_name, **attrs})
            parameter_list = ET.SubElement(function, "parameters")
            for attrs in parameters:
                ET.SubElement(parameter_list, "parameter", attrs)
        ET.ElementTree(root).write(self.output / "panda-temp/architecture.xml")
        raw_path = self.output / "panda-temp/after_Frontend::InterfaceInfer.raw"
        if raw:
            raw_path.write_text(raw)
        else:
            raw_path.unlink(missing_ok=True)

    def test_runner_creates_missing_parent_for_nested_relative_output(self):
        with tempfile.TemporaryDirectory(prefix="bundle-preanalysis-runner-", dir=TMP_ROOT) as temp:
            launch = Path(temp) / "launch"
            launch.mkdir()
            prefix = Path(temp) / "install"
            bambu = prefix / "bin/bambu"
            bambu.parent.mkdir(parents=True)
            invocation = Path(temp) / "bambu-invocation.txt"
            bambu.write_text(
                "#!/usr/bin/env bash\n"
                "set -euo pipefail\n"
                "if [[ ${1:-} == --version ]]; then echo 'stub bambu'; exit 0; fi\n"
                "printf '%s\\n' \"$PWD\" >\"$BAMBU_INVOCATION\"\n"
                "exit 27\n"
            )
            bambu.chmod(0o755)
            (prefix / "settings.sh").write_text(
                f'export BAMBU_HLS="{prefix}"\n'
            )
            plugin = prefix / "lib/panda/clang-16/ASTAnalyzer.so"
            plugin.parent.mkdir(parents=True)
            plugin.write_text("stub plugin\n")
            relative_output = Path("created") / "by" / "runner" / "out_nested"
            env = os.environ.copy()
            env["BAMBU_INVOCATION"] = str(invocation)
            result = subprocess.run(
                [str(HERE / "run_bundle_preanalysis_tests.sh"), str(bambu), str(relative_output)],
                cwd=launch,
                env=env,
                text=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                check=False,
            )
            self.assertEqual(result.returncode, 27, result.stderr)
            output = launch / relative_output
            first_case = output / "distinct_normal"
            self.assertTrue(first_case.is_dir())
            self.assertTrue((first_case / "stdout.log").is_file())
            self.assertEqual(invocation.read_text().strip(), str(first_case.resolve()))
            self.assertNotIn("cannot create directory", result.stderr)

    @staticmethod
    def burst_rtl(read=True, write=True, legacy=False):
        parts = []
        if legacy:
            parts.append("MinimalAXI4AdapterSingleBeat adapter (.m_axi_araddr(p_m_axi_input_araddr));")
        if read:
            parts.extend(("MinimalAXI4MasterPipelined read_engine();",
                          "fuselector_input_configure_read;", "fuselector_input_burst_read;"))
        if write:
            parts.extend(("MinimalAXI4MasterPipelinedWrite write_engine (.m_axi_awready(p_m_axi_output_awready));",
                          "fuselector_output_configure_write;", "fuselector_output_store_write;",
                          "fuselector_output_drain_write;"))
        if not read and not write and not legacy:
            parts.append("MinimalAXI4AdapterSingleBeat legacy_engine();")
        return "\n".join(parts)

    def test_distinct_profiles_and_all_five_abi_operations_are_required(self):
        self.set_artifacts(
            self.burst_rtl(),
            {"input": {"mode": "m_axi", "max_read_burst_length": "16"},
             "output": {"mode": "m_axi", "max_write_burst_length": "16",
                        "write_burst_outstanding": "1"}},
            "Configured read burst regions for bundle input (B=16)\n"
            "Configured write burst region for bundle output (B=16)\n")
        verify(self.output, "design", "distinct")

        self.set_artifacts(self.burst_rtl().replace("fuselector_output_drain_write;", ""),
                           {"input": {"mode": "m_axi", "max_read_burst_length": "16"},
                            "output": {"mode": "m_axi", "max_write_burst_length": "16",
                                       "write_burst_outstanding": "1"}},
                           "Configured read burst regions for bundle input (B=16)\n"
                           "Configured write burst region for bundle output (B=16)\n")
        with self.assertRaisesRegex(CheckError, "CONFIGURE/STORE/DRAIN"):
            verify(self.output, "design", "distinct")

    def test_each_required_compiler_log_must_be_nonempty(self):
        self.set_artifacts(
            self.burst_rtl(),
            {"input": {"mode": "m_axi", "max_read_burst_length": "16"},
             "output": {"mode": "m_axi", "max_write_burst_length": "16",
                        "write_burst_outstanding": "1"}},
            "Configured read burst regions for bundle input (B=16)\n"
            "Configured write burst region for bundle output (B=16)\n")
        (self.output / "stdout.log").write_text("")
        with self.assertRaisesRegex(CheckError, "required non-empty compiler log missing: .*stdout.log"):
            verify(self.output, "design", "distinct")

        self.set_artifacts(
            self.burst_rtl(),
            {"input": {"mode": "m_axi", "max_read_burst_length": "16"},
             "output": {"mode": "m_axi", "max_write_burst_length": "16",
                        "write_burst_outstanding": "1"}},
            "Configured read burst regions for bundle input (B=16)\n"
            "Configured write burst region for bundle output (B=16)\n")
        (self.output / "stderr.log").write_text("")
        with self.assertRaisesRegex(CheckError, "required non-empty compiler log missing: .*stderr.log"):
            verify(self.output, "design", "distinct")

    def test_directional_case_rejects_spurious_read_burst(self):
        self.set_artifacts(
            self.burst_rtl(read=False, write=True, legacy=True),
            {"input": {"mode": "m_axi", "max_read_burst_length": "16"},
             "output": {"mode": "m_axi", "max_write_burst_length": "16",
                        "write_burst_outstanding": "1"}},
            "Keeping legacy m_axi accesses for bundle input: read-burst CFG/SSA recognition failed or bundle is not exclusive\n"
            "Configured write burst region for bundle output (B=16)\n")
        verify(self.output, "design", "write_direction_only")

        self.set_artifacts(self.burst_rtl(read=True, write=True, legacy=True),
                           {"input": {"mode": "m_axi", "max_read_burst_length": "16"},
                            "output": {"mode": "m_axi", "max_write_burst_length": "16",
                                       "write_burst_outstanding": "1"}},
                           "Keeping legacy m_axi accesses for bundle input: read-burst CFG/SSA recognition failed or bundle is not exclusive\n"
                           "Configured write burst region for bundle output (B=16)\n")
        with self.assertRaisesRegex(CheckError, "read-burst engine presence mismatch"):
            verify(self.output, "design", "write_direction_only")

        self.set_artifacts(self.burst_rtl(read=False, write=True, legacy=True),
                           {"input": {"mode": "m_axi", "max_read_burst_length": "16"},
                            "output": {"mode": "m_axi", "max_write_burst_length": "16",
                                       "write_burst_outstanding": "1"}},
                           "Keeping legacy m_axi accesses for bundle input: read-burst CFG/SSA recognition failed or bundle is not exclusive\n"
                           "Keeping legacy m_axi writes for bundle output: unexpected fallback\n"
                           "Configured write burst region for bundle output (B=16)\n")
        with self.assertRaisesRegex(CheckError, "spurious legacy fallback diagnostic"):
            verify(self.output, "design", "write_direction_only")

        self.set_artifacts(self.burst_rtl(read=False, write=True, legacy=True).replace(
                               ".m_axi_awready(p_m_axi_output_awready)",
                               ".m_axi_awready(p_m_axi_input_awready)"),
                           {"input": {"mode": "m_axi", "max_read_burst_length": "16"},
                            "output": {"mode": "m_axi", "max_write_burst_length": "16",
                                       "write_burst_outstanding": "1"}},
                           "Keeping legacy m_axi accesses for bundle input: read-burst CFG/SSA recognition failed or bundle is not exclusive\n"
                           "Configured write burst region for bundle output (B=16)\n")
        with self.assertRaisesRegex(CheckError, "write-burst engine is not wired"):
            verify(self.output, "design", "write_direction_only")

        self.set_artifacts(self.burst_rtl(read=False, write=True, legacy=True).replace(
                               ".m_axi_araddr(p_m_axi_input_araddr)",
                               ".m_axi_araddr(p_m_axi_output_araddr)"),
                           {"input": {"mode": "m_axi", "max_read_burst_length": "16"},
                            "output": {"mode": "m_axi", "max_write_burst_length": "16",
                                       "write_burst_outstanding": "1"}},
                           "Keeping legacy m_axi accesses for bundle input: read-burst CFG/SSA recognition failed or bundle is not exclusive\n"
                           "Configured write burst region for bundle output (B=16)\n")
        with self.assertRaisesRegex(CheckError, "legacy read engine is not wired"):
            verify(self.output, "design", "write_direction_only")

    def test_legacy_fallback_requires_diagnostics_and_absent_burst_structure(self):
        self.set_artifacts(self.burst_rtl(read=False, write=False),
                           {"shared": {"mode": "m_axi", "max_read_burst_length": "16",
                                       "max_write_burst_length": "16", "write_burst_outstanding": "1"}},
                           "Keeping legacy m_axi writes for bundle shared: bundle contains reads; "
                           "same-bundle read/write requires duplex support\n"
                           "Keeping legacy m_axi accesses for bundle shared: bundle contains stores\n",
                           parameters=({"port": "src", "bundle": "shared"},
                                       {"port": "dst", "bundle": "shared"}))
        verify(self.output, "design", "same_bundle_legacy")

        self.set_artifacts(self.burst_rtl(read=False, write=False),
                           {"shared": {"mode": "m_axi", "max_read_burst_length": "16",
                                       "max_write_burst_length": "16", "write_burst_outstanding": "1"}},
                           "Keeping legacy m_axi writes for bundle shared: bundle contains reads; "
                           "same-bundle read/write requires duplex support\n"
                           "Keeping legacy m_axi accesses for bundle shared: bundle contains stores\n",
                           parameters=({"port": "data", "bundle": "shared"},))
        with self.assertRaisesRegex(CheckError, "distinct src and dst ports"):
            verify(self.output, "design", "same_bundle_legacy")
        self.set_artifacts(self.burst_rtl(read=False, write=False),
                           {"shared": {"mode": "m_axi", "max_read_burst_length": "16",
                                       "max_write_burst_length": "16", "write_burst_outstanding": "1"}}, "",
                           parameters=({"port": "src", "bundle": "shared"},
                                       {"port": "dst", "bundle": "shared"}))
        with self.assertRaisesRegex(CheckError, "fallback diagnostic missing"):
            verify(self.output, "design", "same_bundle_legacy")

    @staticmethod
    def inplace_duplex_rtl():
        return "\n".join((
            "module shared_bambu_artificial_ParmMgr_duplex_aggregator_modgen(input p_m_axi_shared_araddr, input p_m_axi_shared_awaddr);",
            "endmodule",
            "MinimalAXI4MasterPipelined read_engine();",
            "MinimalAXI4MasterPipelinedWrite write_engine();",
            "fuselector_shared_duplex_read_endpoint_configure_read;",
            "fuselector_shared_duplex_read_endpoint_burst_read;",
            "fuselector_shared_duplex_write_endpoint_configure_write;",
            "fuselector_shared_duplex_write_endpoint_store_write;",
            "fuselector_shared_duplex_write_endpoint_drain_write;",
            "shared_bambu_artificial_ParmMgr_duplex_aggregator_modgen duplex_aggregator_0 (",
            "  .p_m_axi_shared_araddr(araddr), .p_m_axi_shared_awaddr(awaddr));",
        ))

    def set_inplace_duplex(self, rtl=None, log=None, bundles=None, parameters=None):
        self.set_artifacts(
            self.inplace_duplex_rtl() if rtl is None else rtl,
            bundles or {"shared": {"mode": "m_axi", "max_read_burst_length": "16",
                                    "max_write_burst_length": "16", "write_burst_outstanding": "1"}},
            log or "Activated same-base pointwise duplex m_axi bundle shared for root design\n",
            parameters=parameters or ({"port": "data", "bundle": "shared"},))

    @staticmethod
    def runtime_duplex_rtl():
        return "\n".join((
            "module shared_bambu_artificial_ParmMgr_duplex_aggregator_modgen(input p_m_axi_shared_araddr, input p_m_axi_shared_awaddr);",
            "endmodule",
            "MinimalAXI4MasterPipelined #(.B_MAX(16), .MAX_OUTSTANDING(1), .FIFO_DEPTH(32), .PEER_FAULT_ENABLE(1)) read_engine();",
            "MinimalAXI4MasterPipelinedWrite #(.B_MAX(16), .MAX_OUTSTANDING(1), .FIFO_DEPTH(32), .PEER_FAULT_ENABLE(1)) write_engine();",
            "fuselector_shared_duplex_read_endpoint_configure_read;",
            "fuselector_shared_duplex_read_endpoint_burst_read;",
            "fuselector_shared_duplex_write_endpoint_configure_write;",
            "fuselector_shared_duplex_write_endpoint_store_write;",
            "fuselector_shared_duplex_write_endpoint_drain_write;",
            "shared_bambu_artificial_ParmMgr_duplex_aggregator_modgen duplex_aggregator_0 (",
            "  .p_m_axi_shared_araddr(araddr), .p_m_axi_shared_awaddr(awaddr));",
        ))

    def set_runtime_duplex(self, rtl=None, log=None, proof=None,
                           bundles=None, parameters=None):
        self.set_artifacts(
            self.runtime_duplex_rtl() if rtl is None else rtl,
            bundles or {"shared": {"mode": "m_axi", "max_read_burst_length": "16",
                                    "num_read_outstanding": "1", "read_fifo_depth": "32",
                                    "max_write_burst_length": "16", "write_burst_outstanding": "1"}},
            log or "Activated guarded distinct-base duplex for bundle shared "
                   "(overlap uses serialized scalar transfers)\n",
            parameters=parameters or ({"port": "src", "bundle": "shared", "source_restrict": "0",
                                       "index": "0"},
                                      {"port": "dst", "bundle": "shared", "source_restrict": "0",
                                       "index": "1"}))
        if proof is None:
            proof = {"bundle": "shared", "semantic_proof": True,
                     "proof_kind": "distinct_base_runtime_guarded_pointwise",
                     "runtime_range_guard": True,
                     "read_owner_argument_index": 428892,
                     "write_owner_argument_index": 428893}
        (self.output / "stderr.log").write_text(
            "AXI_BURST_DUPLEX_PROOF " + json.dumps(proof) + "\n")

    def test_runtime_guarded_duplex_requires_complete_positive_witnesses(self):
        self.set_runtime_duplex()
        verify(self.output, "design", "runtime_duplex")

        self.set_runtime_duplex(proof=None)
        (self.output / "stderr.log").write_text("compiler stderr without proof\n")
        with self.assertRaisesRegex(CheckError, "exactly one structured duplex proof"):
            verify(self.output, "design", "runtime_duplex")

        wrong_proof = {"bundle": "shared", "semantic_proof": True,
                       "proof_kind": "distinct_base_restricted_pointwise",
                       "runtime_range_guard": False,
                       "read_owner_argument_index": 428892,
                       "write_owner_argument_index": 428893}
        self.set_runtime_duplex(proof=wrong_proof)
        with self.assertRaisesRegex(CheckError, "not a successful runtime-range-guarded"):
            verify(self.output, "design", "runtime_duplex")

        self.set_runtime_duplex(log="compiler succeeded without activation\n")
        with self.assertRaisesRegex(CheckError, "activation marker is missing"):
            verify(self.output, "design", "runtime_duplex")

        self.set_runtime_duplex(rtl=self.runtime_duplex_rtl().replace(
            "MinimalAXI4MasterPipelinedWrite #(.B_MAX(16), .MAX_OUTSTANDING(1), .FIFO_DEPTH(32), .PEER_FAULT_ENABLE(1)) write_engine();\n", ""))
        with self.assertRaisesRegex(CheckError, "exactly one write burst engine"):
            verify(self.output, "design", "runtime_duplex")

        self.set_runtime_duplex(rtl=self.runtime_duplex_rtl() +
                                "MinimalAXI4AdapterSingleBeat legacy_engine();\n")
        with self.assertRaisesRegex(CheckError, "legacy memory resource"):
            verify(self.output, "design", "runtime_duplex")

    def test_inplace_duplex_requires_complete_positive_witnesses(self):
        self.set_inplace_duplex()
        verify(self.output, "design", "inplace_duplex")

        self.set_inplace_duplex(rtl=self.inplace_duplex_rtl().replace(
            "MinimalAXI4MasterPipelinedWrite write_engine();\n", ""))
        with self.assertRaisesRegex(CheckError, "exactly one shared write-burst engine"):
            verify(self.output, "design", "inplace_duplex")

        self.set_inplace_duplex(rtl=self.inplace_duplex_rtl().replace(
            "MinimalAXI4MasterPipelined read_engine();\n", ""))
        with self.assertRaisesRegex(CheckError, "exactly one shared read-burst engine"):
            verify(self.output, "design", "inplace_duplex")

        missing_operation = self.inplace_duplex_rtl().replace(
            "fuselector_shared_duplex_write_endpoint_drain_write;\n", "")
        self.set_inplace_duplex(rtl=missing_operation)
        with self.assertRaisesRegex(CheckError, "ABI operations"):
            verify(self.output, "design", "inplace_duplex")

        self.set_inplace_duplex(rtl=self.inplace_duplex_rtl() +
                                "MinimalAXI4AdapterSingleBeat legacy_engine();\n")
        with self.assertRaisesRegex(CheckError, "legacy memory resource"):
            verify(self.output, "design", "inplace_duplex")

        disconnected = self.inplace_duplex_rtl().replace(
            ".p_m_axi_shared_awaddr(awaddr)", ".p_m_axi_shared_bvalid(bvalid)")
        self.set_inplace_duplex(rtl=disconnected)
        with self.assertRaisesRegex(CheckError, "disconnected from the shared read or write address"):
            verify(self.output, "design", "inplace_duplex")

        self.set_inplace_duplex(log="compiler succeeded without activation\n")
        with self.assertRaisesRegex(CheckError, "activation marker is missing"):
            verify(self.output, "design", "inplace_duplex")

    def test_write_only_requires_its_positive_configuration_witness(self):
        bundles = {"output": {"mode": "m_axi", "max_write_burst_length": "16",
                              "write_burst_outstanding": "1"}}
        positive = "Configured write burst region for bundle output (B=16)\n"
        self.set_artifacts(self.burst_rtl(read=False, write=True), bundles, positive)
        verify(self.output, "design", "write_only")

        self.set_artifacts(self.burst_rtl(read=False, write=True), bundles)
        with self.assertRaisesRegex(CheckError, "write-burst success diagnostic missing"):
            verify(self.output, "design", "write_only")

    def test_mixed_access_fallback_requires_exact_opaque_effect_reason(self):
        bundles = {"data": {"mode": "m_axi", "max_write_burst_length": "16",
                             "write_burst_outstanding": "1"}}
        reason = "opaque, volatile, atomic, or unclassified memory effects"
        self.set_artifacts(self.burst_rtl(read=False, write=False), bundles,
                           f"Keeping legacy m_axi writes for bundle data: {reason}\n")
        verify(self.output, "design", "mixed_legacy")

        self.set_artifacts(self.burst_rtl(read=False, write=False), bundles, "")
        with self.assertRaisesRegex(CheckError, "mixed-access fallback diagnostic missing"):
            verify(self.output, "design", "mixed_legacy")

        self.set_artifacts(self.burst_rtl(read=False, write=False), bundles,
                           "Keeping legacy m_axi writes for bundle data: write-burst CFG/SSA recognition failed\n")
        with self.assertRaisesRegex(CheckError, "mixed-access fallback diagnostic missing"):
            verify(self.output, "design", "mixed_legacy")

    def test_dataflow_forwarding_requires_default_task_port_without_burst_or_incr_error(self):
        functions = [
            ("dataflow_forwarded_input", {"input": {"mode": "m_axi"}},
             [{"port": "input", "bundle": "input"}]),
            ("forwarded_write", {"task_output": {"mode": "m_axi", "max_write_burst_length": "16",
                                                  "write_burst_outstanding": "1"}},
             [{"port": "dst", "bundle": "task_output"}]),
        ]
        debug = ("Analyzing function forwarded_write\n"
                 "Parameter dst forwarded from caller function\n"
                 "Protocol : default\nBundle : task_output\n"
                 "Analyzed function forwarded_write\n")
        raw = '@20 identifier_node strg: "task_output_bambu_artificial_ParmMgr_Write"\n'
        self.set_dataflow_artifacts("dataflow_forwarded_input", "wire idle;", functions,
                                    log=debug, raw=raw)
        verify(self.output, "dataflow_forwarded_input", "dataflow_forwarding")

        self.set_dataflow_artifacts("dataflow_forwarded_input", "wire idle;", functions,
                                    log=debug.replace("Parameter dst forwarded from caller function\n", ""),
                                    raw=raw)
        with self.assertRaisesRegex(CheckError, "parameter-forwarding witness"):
            verify(self.output, "dataflow_forwarded_input", "dataflow_forwarding")

        self.set_dataflow_artifacts("dataflow_forwarded_input", "wire idle;", functions,
                                    log=debug.replace("Bundle : task_output", "Bundle : input"), raw=raw)
        with self.assertRaisesRegex(CheckError, "adapted default protocol"):
            verify(self.output, "dataflow_forwarded_input", "dataflow_forwarding")

        self.set_dataflow_artifacts("dataflow_forwarded_input", "wire idle;", functions,
                                    log=debug, raw="@1 identifier_node strg: \"unrelated\"\n")
        with self.assertRaisesRegex(CheckError, "legacy write adapter"):
            verify(self.output, "dataflow_forwarded_input", "dataflow_forwarding")

        bad_functions = [functions[0], ("forwarded_write", {"task_output": {"mode": "default"}},
                                        [{"port": "dst", "bundle": "task_output"}])]
        self.set_dataflow_artifacts("dataflow_forwarded_input", "wire idle;", bad_functions,
                                    log=debug, raw=raw)
        with self.assertRaisesRegex(CheckError, "retain the original forwarded_write"):
            verify(self.output, "dataflow_forwarded_input", "dataflow_forwarding")

        self.set_dataflow_artifacts("dataflow_forwarded_input", "wire idle;", functions,
                                    log=debug + "Configured write bursts for bundle task_output require AXI INCREMENTAL burst type\n",
                                    raw=raw)
        with self.assertRaisesRegex(CheckError, "AXI INCREMENTAL requirement error"):
            verify(self.output, "dataflow_forwarded_input", "dataflow_forwarding")

        self.set_dataflow_artifacts("dataflow_forwarded_input",
                                    "MinimalAXI4MasterPipelinedWrite writer();\nfuselector_x_store_write;",
                                    functions, log=debug, raw=raw)
        with self.assertRaisesRegex(CheckError, "unexpectedly instantiated a write-burst engine"):
            verify(self.output, "dataflow_forwarded_input", "dataflow_forwarding")


if __name__ == "__main__":
    unittest.main()
