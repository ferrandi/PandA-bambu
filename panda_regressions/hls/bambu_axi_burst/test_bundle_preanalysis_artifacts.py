#!/usr/bin/env python3
import tempfile
import unittest
import xml.etree.ElementTree as ET
from pathlib import Path

from check_bundle_preanalysis_artifacts import CheckError, verify

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[2]
TMP_ROOT = REPO / "documentation" / "tmp"


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
        verify(self.output, "design", "inplace_legacy")

        with self.assertRaisesRegex(CheckError, "distinct src and dst ports"):
            verify(self.output, "design", "same_bundle_legacy")
        self.set_artifacts(self.burst_rtl(read=False, write=False),
                           {"shared": {"mode": "m_axi", "max_read_burst_length": "16",
                                       "max_write_burst_length": "16", "write_burst_outstanding": "1"}},
                           "Keeping legacy m_axi writes for bundle shared: bundle contains reads; "
                           "same-bundle read/write requires duplex support\n"
                           "Keeping legacy m_axi accesses for bundle shared: bundle contains stores\n",
                           parameters=({"port": "src", "bundle": "shared"},
                                       {"port": "dst", "bundle": "shared"}))
        with self.assertRaisesRegex(CheckError, "only the data port"):
            verify(self.output, "design", "inplace_legacy")

        self.set_artifacts(self.burst_rtl(read=False, write=False),
                           {"shared": {"mode": "m_axi", "max_read_burst_length": "16",
                                       "max_write_burst_length": "16", "write_burst_outstanding": "1"}}, "",
                           parameters=({"port": "src", "bundle": "shared"},
                                       {"port": "dst", "bundle": "shared"}))
        with self.assertRaisesRegex(CheckError, "fallback diagnostic missing"):
            verify(self.output, "design", "same_bundle_legacy")

        self.set_artifacts(self.burst_rtl(read=False, write=False),
                           {"shared": {"mode": "m_axi", "max_read_burst_length": "16",
                                       "max_write_burst_length": "16", "write_burst_outstanding": "1"}},
                           "Keeping legacy m_axi writes for bundle shared: bundle contains reads; "
                           "same-bundle read/write requires duplex support\n"
                           "Keeping legacy m_axi accesses for bundle shared: another reason\n",
                           parameters=({"port": "data", "bundle": "shared"},))
        with self.assertRaisesRegex(CheckError, "read direction"):
            verify(self.output, "design", "inplace_legacy")

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
