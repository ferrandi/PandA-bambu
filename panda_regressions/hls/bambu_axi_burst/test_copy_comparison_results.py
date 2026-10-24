"""Offline fail-closed tests for the controlled copy comparison checker."""
import hashlib
import os
from pathlib import Path
import shlex
import shutil
import sys
import tempfile
import unittest

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[2]
TMP_ROOT = Path(os.environ.get("BURST_TEST_TMPDIR", Path.cwd())) / f"out_{Path(__file__).stem}"
sys.path.insert(0, str(HERE))
import check_copy_comparison_results as checker


class CopyComparisonTests(unittest.TestCase):
    def setUp(self):
        TMP_ROOT.mkdir(parents=True, exist_ok=True)
        self.temp = tempfile.TemporaryDirectory(prefix="test-copy-comparison-", dir=TMP_ROOT)
        self.root = Path(self.temp.name)
        self.output = self.root / "out"
        self.output.mkdir()
        self.report = self.root / "report.json"
        self.inputs = {}
        names = {"source": "bambu_axi_burst_copy_comparison.cpp", "testbench": "copy_comparison_tb.cpp",
                 "bambu": "bambu", "settings": "settings.sh", "plugin": "ASTAnalyzer.so",
                 "runner": "run_copy_comparison_tests.sh"}
        for key, name in names.items():
            path = self.root / name
            if key == "source":
                content = "#define COPY_COMPARISON_MAXN 4096\n"
            elif key == "testbench":
                content = "#define COPY_COMPARISON_MAXN 4096\n#define COPY_COMPARISON_N 1024\n"
            else:
                content = f"fixture {key}\n"
            path.write_text(content, encoding="utf-8")
            self.inputs[key] = path
        self.rows = checker.read_inventory()
        for row in self.rows:
            for run in checker.RUNS:
                self.build_run(row["profile"], run, 100)

    def tearDown(self):
        self.temp.cleanup()

    def build_run(self, profile, run, cycles):
        config = checker.PROFILES[profile]
        macro, read_bundle, write_bundle = config["macro"], config["read_bundle"], config["write_bundle"]
        directory = self.output / profile / run
        directory.mkdir(parents=True, exist_ok=True)
        (directory / "bambu_results.xml").write_text(
            f'<results><evaluation CYCLES="{cycles}"/></results>\n', encoding="utf-8")
        (directory / "HLS_output" / "simulation").mkdir(parents=True, exist_ok=True)
        (directory / "HLS_output" / "simulation" / "testbench.log").write_text(
            f"COPY_COMPARISON_GOLDEN_PASS PROFILE={profile} N=1024\n", encoding="utf-8")
        bundles = {read_bundle: {"max_read_burst_length": 16, "num_read_outstanding": 1,
                                 "read_fifo_depth": 64}}
        bundles.setdefault(write_bundle, {}).update(
            {"max_write_burst_length": 16, "write_burst_outstanding": 1, "write_fifo_depth": 32})
        bundle_xml = "".join(
            f'<bundle mode="m_axi" name="{name}" latency="0" ' +
            " ".join(f'{key}="{value}"' for key, value in attrs.items()) + "></bundle>"
            for name, attrs in bundles.items())
        (directory / "panda-temp").mkdir(exist_ok=True)
        (directory / "panda-temp" / "architecture.xml").write_text(
            '<module><function name="kernel"><bundles>' + bundle_xml +
            '</bundles><parameters>'
            f'<parameter port="src" bundle="{read_bundle}"/><parameter port="dst" bundle="{write_bundle}"/>'
            '</parameters></function></module>\n', encoding="utf-8")
        if profile in {"burst-two-bundles", "duplex-shared"}:
            rtl = (
                f"module kernel(input [31:0] p_m_axi_{read_bundle}_araddr, "
                f"input [31:0] p_m_axi_{write_bundle}_awaddr, input p_m_axi_{read_bundle}_arready, "
                f"input [31:0] p_m_axi_{read_bundle}_rdata, input p_m_axi_{write_bundle}_awready, "
                f"input p_m_axi_{write_bundle}_wready, input [1:0] p_m_axi_{write_bundle}_bresp);\n"
                f"MinimalAXI4MasterPipelined #(.B_MAX({config['read_engine'][0]}), "
                f".MAX_OUTSTANDING({config['read_engine'][1]}), "
                f".FIFO_DEPTH({config['read_engine'][2]}), .BITSIZE_m_axi_araddr(32)) rd "
                f"(.m_axi_araddr(p_m_axi_{read_bundle}_araddr), "
                f".m_axi_arready(p_m_axi_{read_bundle}_arready), .m_axi_rdata(p_m_axi_{read_bundle}_rdata));\n"
                f"MinimalAXI4MasterPipelinedWrite #(.B_MAX({config['write_engine'][0]}), "
                f".MAX_OUTSTANDING({config['write_engine'][1]}), "
                f".FIFO_DEPTH({config['write_engine'][2]}), "
                f".BITSIZE_m_axi_awaddr(32)) wr (.m_axi_awaddr(p_m_axi_{write_bundle}_awaddr), "
                f".m_axi_awready(p_m_axi_{write_bundle}_awready), "
                f".m_axi_wready(p_m_axi_{write_bundle}_wready), "
                f".m_axi_bresp(p_m_axi_{write_bundle}_bresp));\nendmodule\n")
            if profile == "duplex-shared":
                stdout = "compiler stdout\n"
                stderr = ("    Activated distinct-base restricted pointwise duplex m_axi bundle shared "
                          "for root kernel\n")
            else:
                stdout = "burst engines generated\n"
                stderr = "compiler stderr\n"
        else:
            rtl = (f"module kernel(input [31:0] p_m_axi_{read_bundle}_araddr, "
                   f"input [31:0] p_m_axi_{write_bundle}_awaddr);\n")
            if profile == "legacy-two-bundles":
                rtl += (f"MinimalAXI4AdapterSingleBeat read_resource "
                        f"(.m_axi_araddr(p_m_axi_{read_bundle}_araddr), "
                        f".m_axi_awaddr(p_m_axi_{read_bundle}_awaddr));\n"
                        f"MinimalAXI4AdapterSingleBeat write_resource "
                        f"(.m_axi_araddr(p_m_axi_{write_bundle}_araddr), "
                        f".m_axi_awaddr(p_m_axi_{write_bundle}_awaddr));\n")
            else:
                rtl += (f"MinimalAXI4AdapterSingleBeat shared_resource "
                        f"(.m_axi_araddr(p_m_axi_{read_bundle}_araddr), "
                        f".m_axi_awaddr(p_m_axi_{write_bundle}_awaddr));\n")
            rtl += "endmodule\n"
            stdout = "legacy profile compiled without burst requests\n"
            stderr = (
                f"Keeping legacy m_axi reads for bundle {read_bundle}: explicit read-burst request ignored "
                "because experimental-m-axi-burst=0\n"
                f"Keeping legacy m_axi writes for bundle {write_bundle}: explicit write-burst request ignored "
                "because experimental-m-axi-burst=0\n")
        (directory / "kernel.v").write_text(rtl, encoding="utf-8")
        (directory / "stdout.log").write_text(stdout, encoding="utf-8")
        (directory / "stderr.log").write_text(stderr, encoding="utf-8")
        args = [str(self.inputs["bambu"]), str(self.inputs["source"]),
                *checker.COMMON_ARGS[:-2], f"--generate-tb={self.inputs['testbench']}",
                *checker.COMMON_ARGS[-2:],
                f"--bambu-parameter=experimental-m-axi-burst={config['feature']}",
                f"--extra-cc-options=-DCOPY_COMPARISON_PROFILE={macro}"]
        metadata = [f"profile={profile}", f"run={run}", f"macro={macro}",
                    f"feature={config['feature']}",
                    f"source={self.inputs['source']}", f"testbench={self.inputs['testbench']}",
                    "argv=" + shlex.join(args)]
        metadata.extend(f"{hashlib.sha256(path.read_bytes()).hexdigest()}  {path}"
                        for path in self.inputs.values())
        (directory / "provenance.txt").write_text("\n".join(metadata) + "\n", encoding="utf-8")
        self.write_manifest(directory)

    @staticmethod
    def write_manifest(directory):
        artifacts = sorted(checker.generated_files(directory))
        (directory / "artifact-sha256.txt").write_text(
            "".join(f"{hashlib.sha256(path.read_bytes()).hexdigest()}  {path}\n" for path in artifacts),
            encoding="utf-8")

    def verify_run(self, profile="burst-two-bundles", run="run1"):
        return checker.check_run(self.output / profile / run, profile, run, self.inputs)

    def verify(self):
        return checker.verify(self.output, self.inputs, self.report)

    def test_exact_inventory_and_equal_cycles_pass(self):
        report = self.verify()
        self.assertEqual(report["measurements"]["burst-two-bundles"]["run1"]["cycles"], 100)
        self.assertEqual(report["measurements"]["duplex-shared"]["run2"]["cycles"], 100)
        self.assertEqual(report["measurements"]["legacy-shared"]["run1"]["ratio_to_duplex_same_run"], 1)
        self.assertTrue(self.report.is_file())

    def test_burst_two_bundle_read_fifo_matches_duplex_profile(self):
        burst = checker.PROFILES["burst-two-bundles"]
        duplex = checker.PROFILES["duplex-shared"]
        self.assertEqual(burst["read_requested"], duplex["read_requested"])
        self.assertEqual(burst["write_requested"], duplex["write_requested"])
        self.assertEqual(burst["read_engine"], duplex["read_engine"])
        self.assertEqual(burst["write_engine"], duplex["write_engine"])

    def test_legacy_profiles_distinguish_requests_from_missing_engines(self):
        for profile in ("legacy-two-bundles", "legacy-shared"):
            config = checker.PROFILES[profile]
            self.assertEqual(config["read_requested"], (16, 1, 64))
            self.assertEqual(config["write_requested"], (16, 1, 32))
            self.assertIsNone(config["read_engine"])
            self.assertIsNone(config["write_engine"])
        report = self.verify()
        self.assertEqual(report["profiles"]["legacy-shared"]["requested_parameters"]["read"],
                         {"burst": 16, "outstanding": 1, "fifo": 64})
        self.assertIsNone(report["profiles"]["legacy-shared"]["actual_engine_parameters"]["read"])

    def test_missing_profile_run_is_rejected(self):
        shutil.rmtree(self.output / "legacy-shared" / "run2")
        with self.assertRaisesRegex(ValueError, "exactly four profiles x run1/run2"):
            self.verify()

    def test_extra_inventory_entry_is_rejected(self):
        (self.output / "other" / "run1").mkdir(parents=True)
        with self.assertRaisesRegex(ValueError, "exactly four profiles x run1/run2"):
            self.verify()

    def test_missing_cycles_is_rejected(self):
        path = self.output / "burst-two-bundles" / "run1" / "bambu_results.xml"
        path.write_text("<results><evaluation/></results>\n", encoding="utf-8")
        self.write_manifest(path.parent)
        with self.assertRaisesRegex(ValueError, "exactly one positive integer CYCLES"):
            self.verify_run()

    def test_duplicate_cycles_is_rejected(self):
        path = self.output / "burst-two-bundles" / "run1" / "bambu_results.xml"
        path.write_text('<results><evaluation CYCLES="10"/><evaluation CYCLES="10"/></results>\n',
                        encoding="utf-8")
        self.write_manifest(path.parent)
        with self.assertRaisesRegex(ValueError, "exactly one positive integer CYCLES"):
            self.verify_run()

    def test_zero_cycles_is_rejected(self):
        path = self.output / "burst-two-bundles" / "run1" / "bambu_results.xml"
        path.write_text('<results><evaluation CYCLES="0"/></results>\n', encoding="utf-8")
        self.write_manifest(path.parent)
        with self.assertRaisesRegex(ValueError, "exactly one positive integer CYCLES"):
            self.verify_run()

    def test_malformed_cycles_is_rejected(self):
        path = self.output / "burst-two-bundles" / "run1" / "bambu_results.xml"
        path.write_text('<results><evaluation CYCLES="1.5"/></results>\n', encoding="utf-8")
        self.write_manifest(path.parent)
        with self.assertRaisesRegex(ValueError, "exactly one positive integer CYCLES"):
            self.verify_run()

    def test_wrong_expectation_profile_or_bundle_is_rejected(self):
        wrong = self.root / "wrong.tsv"
        text = (HERE / "copy_comparison_expectations.tsv").read_text(encoding="utf-8")
        wrong.write_text(text.replace("burst-two-bundles\t1\tinput", "burst-two-bundles\t1\twrong"),
                         encoding="utf-8")
        with self.assertRaisesRegex(ValueError, "expectation profile parameters are invalid"):
            checker.read_inventory(wrong)

    def test_wrong_generated_profile_parameter_is_rejected(self):
        path = self.output / "burst-two-bundles" / "run1" / "kernel.v"
        path.write_text(path.read_text().replace(".B_MAX(16)", ".B_MAX(8)", 1), encoding="utf-8")
        self.write_manifest(path.parent)
        with self.assertRaisesRegex(ValueError, "read engine profile/width parameters"):
            self.verify_run()

    def test_wrong_generated_bundle_is_rejected(self):
        path = self.output / "burst-two-bundles" / "run1" / "kernel.v"
        path.write_text(path.read_text().replace("p_m_axi_input_araddr", "p_m_axi_wrong_araddr"),
                        encoding="utf-8")
        self.write_manifest(path.parent)
        with self.assertRaisesRegex(ValueError, "expected 32-bit AR address owner/bundle input missing"):
            self.verify_run()

    def test_profile_parameters_split_across_instances_are_rejected(self):
        path = self.output / "burst-two-bundles" / "run1" / "kernel.v"
        path.write_text(path.read_text().replace(
            ".MAX_OUTSTANDING(1), .FIFO_DEPTH(32)", ".MAX_OUTSTANDING(1)) rd (.m_axi_araddr());\n"
            "MinimalAXI4MasterPipelined #(.FIFO_DEPTH(32)"), encoding="utf-8")
        self.write_manifest(path.parent)
        with self.assertRaisesRegex(ValueError, "exactly one read and one write burst engine"):
            self.verify_run()

    def test_legacy_requires_positive_resource_witness(self):
        path = self.output / "legacy-shared" / "run1" / "kernel.v"
        path.write_text("module kernel(input [31:0] p_m_axi_shared_araddr, "
                        "input [31:0] p_m_axi_shared_awaddr); endmodule\n",
                        encoding="utf-8")
        self.write_manifest(path.parent)
        with self.assertRaisesRegex(ValueError, "legacy-shared must instantiate exactly 1 legacy resources"):
            self.verify_run("legacy-shared")

    def test_legacy_module_declaration_without_instance_is_rejected(self):
        path = self.output / "legacy-shared" / "run1" / "kernel.v"
        path.write_text(
            "module kernel(input [31:0] p_m_axi_shared_araddr, "
            "input [31:0] p_m_axi_shared_awaddr); endmodule\n"
            "module MinimalAXI4AdapterSingleBeat(input x); endmodule\n", encoding="utf-8")
        self.write_manifest(path.parent)
        with self.assertRaisesRegex(ValueError, "legacy-shared must instantiate exactly 1 legacy resources"):
            self.verify_run("legacy-shared")

    def test_legacy_two_bundle_requires_two_resources(self):
        path = self.output / "legacy-two-bundles" / "run1" / "kernel.v"
        path.write_text(
            "module kernel(input [31:0] p_m_axi_input_araddr, "
            "input [31:0] p_m_axi_output_awaddr);\n"
            "MinimalAXI4AdapterSingleBeat one_resource "
            "(.m_axi_araddr(p_m_axi_input_araddr), .m_axi_awaddr(p_m_axi_output_awaddr));\n"
            "endmodule\n", encoding="utf-8")
        self.write_manifest(path.parent)
        with self.assertRaisesRegex(ValueError, "legacy-two-bundles must instantiate exactly 2"):
            self.verify_run("legacy-two-bundles")

    def test_legacy_two_bundle_wrong_read_wiring_is_rejected(self):
        path = self.output / "legacy-two-bundles" / "run1" / "kernel.v"
        text = path.read_text(encoding="utf-8").replace(
            ".m_axi_araddr(p_m_axi_input_araddr)", ".m_axi_araddr(p_m_axi_output_araddr)")
        path.write_text(text, encoding="utf-8")
        self.write_manifest(path.parent)
        with self.assertRaisesRegex(ValueError, "legacy-two-bundles legacy resources have incorrect read/write bundle wiring"):
            self.verify_run("legacy-two-bundles")

    def test_legacy_shared_wrong_write_wiring_is_rejected(self):
        path = self.output / "legacy-shared" / "run1" / "kernel.v"
        text = path.read_text(encoding="utf-8").replace(
            ".m_axi_awaddr(p_m_axi_shared_awaddr)", ".m_axi_awaddr(p_m_axi_wrong_awaddr)")
        path.write_text(text, encoding="utf-8")
        self.write_manifest(path.parent)
        with self.assertRaisesRegex(ValueError, "legacy-shared legacy resources have incorrect read/write bundle wiring"):
            self.verify_run("legacy-shared")

    def test_source_maxn_mismatch_is_rejected(self):
        self.inputs["source"].write_text("#define COPY_COMPARISON_MAXN 2048\n", encoding="utf-8")
        with self.assertRaisesRegex(ValueError, "COPY_COMPARISON_MAXN must be the integer 4096"):
            self.verify()

    def test_testbench_maxn_mismatch_is_rejected(self):
        self.inputs["testbench"].write_text(
            "#define COPY_COMPARISON_MAXN 2048\n#define COPY_COMPARISON_N 1024\n", encoding="utf-8")
        with self.assertRaisesRegex(ValueError, "COPY_COMPARISON_MAXN must be the integer 4096"):
            self.verify()

    def test_testbench_n_mismatch_is_rejected(self):
        self.inputs["testbench"].write_text(
            "#define COPY_COMPARISON_MAXN 4096\n#define COPY_COMPARISON_N 512\n", encoding="utf-8")
        with self.assertRaisesRegex(ValueError, "COPY_COMPARISON_N must be the integer 1024"):
            self.verify()

    def test_missing_or_duplicate_copy_dimensions_are_rejected(self):
        self.inputs["testbench"].write_text("#define COPY_COMPARISON_MAXN 4096\n", encoding="utf-8")
        with self.assertRaisesRegex(ValueError, "expected exactly one COPY_COMPARISON_N definition"):
            self.verify()
        self.inputs["testbench"].write_text(
            "#define COPY_COMPARISON_MAXN 4096\n#define COPY_COMPARISON_MAXN 4096\n"
            "#define COPY_COMPARISON_N 1024\n", encoding="utf-8")
        with self.assertRaisesRegex(ValueError, "expected exactly one COPY_COMPARISON_MAXN definition"):
            self.verify()

    def test_malformed_copy_dimensions_are_rejected(self):
        self.inputs["testbench"].write_text(
            "#define COPY_COMPARISON_MAXN LARGE\n#define COPY_COMPARISON_N 1024\n", encoding="utf-8")
        with self.assertRaisesRegex(ValueError, "COPY_COMPARISON_MAXN must be the integer 4096"):
            self.verify()

    def test_read_engine_wired_to_wrong_bundle_is_rejected(self):
        path = self.output / "burst-two-bundles" / "run1" / "kernel.v"
        path.write_text(path.read_text().replace("p_m_axi_input_rdata", "p_m_axi_output_rdata"),
                        encoding="utf-8")
        self.write_manifest(path.parent)
        with self.assertRaisesRegex(ValueError, "read engine m_axi_rdata is not wired to expected bundle input"):
            self.verify_run()

    def test_write_engine_wired_to_wrong_bundle_is_rejected(self):
        path = self.output / "burst-two-bundles" / "run1" / "kernel.v"
        path.write_text(path.read_text().replace("p_m_axi_output_wready", "p_m_axi_input_wready"),
                        encoding="utf-8")
        self.write_manifest(path.parent)
        with self.assertRaisesRegex(ValueError, "write engine m_axi_wready is not wired to expected bundle output"):
            self.verify_run()

    def test_unannotated_legacy_profiles_need_no_fallback_diagnostic(self):
        for profile in ("legacy-shared", "legacy-two-bundles"):
            for run in checker.RUNS:
                path = self.output / profile / run / "stdout.log"
                path.write_text("legacy profile compiled without burst requests\n", encoding="utf-8")
                self.write_manifest(path.parent)
                self.assertEqual(self.verify_run(profile, run), 100)

    def test_duplex_activation_allows_per_direction_fallback_diagnostics(self):
        path = self.output / "duplex-shared" / "run1" / "stderr.log"
        path.write_text("Keeping legacy m_axi writes for bundle shared: candidate rejected\n" +
                        path.read_text(), encoding="utf-8")
        self.write_manifest(path.parent)
        self.assertEqual(self.verify_run("duplex-shared"), 100)

    def test_duplex_residual_legacy_resource_is_rejected(self):
        path = self.output / "duplex-shared" / "run1" / "kernel.v"
        path.write_text(path.read_text().replace("endmodule", "MinimalAXI4AdapterSingleBeat legacy (); endmodule"),
                        encoding="utf-8")
        self.write_manifest(path.parent)
        with self.assertRaisesRegex(ValueError, "legacy memory resources"):
            self.verify_run("duplex-shared")

    def test_stdout_only_marker_is_rejected(self):
        run = self.output / "burst-two-bundles" / "run1"
        (run / "stdout.log").write_text("COPY_COMPARISON_GOLDEN_PASS PROFILE=burst-two-bundles N=1024\n",
                                        encoding="utf-8")
        sim = run / "HLS_output" / "simulation" / "testbench.log"
        sim.write_text("simulation passed\n", encoding="utf-8")
        self.write_manifest(run)
        with self.assertRaisesRegex(ValueError, "exact golden marker absent/duplicated"):
            self.verify_run()

    def test_wrong_profile_marker_is_rejected(self):
        run = self.output / "burst-two-bundles" / "run1"
        sim = run / "HLS_output" / "simulation" / "testbench.log"
        sim.write_text("COPY_COMPARISON_GOLDEN_PASS PROFILE=duplex-shared N=1024\n", encoding="utf-8")
        self.write_manifest(run)
        with self.assertRaisesRegex(ValueError, "exact golden marker absent/duplicated"):
            self.verify_run()

    def test_artifact_manifest_tampering_is_rejected(self):
        run = self.output / "burst-two-bundles" / "run1"
        manifest = run / "artifact-sha256.txt"
        manifest.write_text(manifest.read_text().replace("kernel.v", "missing.v"), encoding="utf-8")
        with self.assertRaisesRegex(ValueError, "manifest artifact missing/outside run"):
            self.verify_run()

    def test_provenance_tampering_is_rejected(self):
        path = self.output / "burst-two-bundles" / "run1" / "provenance.txt"
        path.write_text(path.read_text().replace("macro=1", "macro=4"), encoding="utf-8")
        with self.assertRaisesRegex(ValueError, "provenance identity mismatch"):
            self.verify_run()

    def test_provenance_input_hash_tampering_is_rejected(self):
        path = self.output / "burst-two-bundles" / "run1" / "provenance.txt"
        text = path.read_text(encoding="utf-8")
        old_hash = hashlib.sha256(self.inputs["runner"].read_bytes()).hexdigest()
        path.write_text(text.replace(old_hash, "0" * 64), encoding="utf-8")
        with self.assertRaisesRegex(ValueError, "provenance hash missing/mismatched for runner"):
            self.verify_run()

    def test_wrong_feature_flag_in_provenance_is_rejected(self):
        path = self.output / "legacy-shared" / "run1" / "provenance.txt"
        text = path.read_text(encoding="utf-8").replace(
            "--bambu-parameter=experimental-m-axi-burst=0",
            "--bambu-parameter=experimental-m-axi-burst=1")
        path.write_text(text, encoding="utf-8")
        with self.assertRaisesRegex(ValueError, "expected exactly one experimental-m-axi-burst=0"):
            self.verify_run("legacy-shared")

    def test_missing_feature_flag_in_provenance_is_rejected(self):
        path = self.output / "legacy-shared" / "run1" / "provenance.txt"
        text = path.read_text(encoding="utf-8").replace(
            "--bambu-parameter=experimental-m-axi-burst=0 ", "")
        path.write_text(text, encoding="utf-8")
        with self.assertRaisesRegex(ValueError, "expected exactly one experimental-m-axi-burst=0"):
            self.verify_run("legacy-shared")

    def test_duplicate_feature_flag_in_provenance_is_rejected(self):
        path = self.output / "legacy-shared" / "run1" / "provenance.txt"
        text = path.read_text(encoding="utf-8")
        text = text.replace("--bambu-parameter=experimental-m-axi-burst=0",
                            "--bambu-parameter=experimental-m-axi-burst=0 "
                            "--bambu-parameter=experimental-m-axi-burst=0")
        path.write_text(text, encoding="utf-8")
        with self.assertRaisesRegex(ValueError, "expected exactly one experimental-m-axi-burst=0"):
            self.verify_run("legacy-shared")

    def test_generate_tb_order_in_provenance_is_enforced(self):
        path = self.output / "burst-two-bundles" / "run1" / "provenance.txt"
        text = path.read_text(encoding="utf-8")
        testbench_arg = f"--generate-tb={self.inputs['testbench']}"
        text = text.replace(testbench_arg + " ", "")
        text = text.replace("--extra-cc-options=-DBAMBU ",
                            "--extra-cc-options=-DBAMBU " + testbench_arg + " ")
        path.write_text(text, encoding="utf-8")
        with self.assertRaisesRegex(ValueError, "invocation common settings differ"):
            self.verify_run()

    def test_wrong_architecture_requested_parameter_is_rejected(self):
        path = self.output / "burst-two-bundles" / "run1" / "panda-temp" / "architecture.xml"
        path.write_text(path.read_text(encoding="utf-8").replace(
            'read_fifo_depth="64"', 'read_fifo_depth="32"'), encoding="utf-8")
        self.write_manifest(path.parents[1])
        with self.assertRaisesRegex(ValueError, "must explicitly request read_fifo_depth=64"):
            self.verify_run()

    def test_missing_architecture_requested_parameter_is_rejected(self):
        path = self.output / "legacy-shared" / "run1" / "panda-temp" / "architecture.xml"
        path.write_text(path.read_text(encoding="utf-8").replace(' read_fifo_depth="64"', ""),
                        encoding="utf-8")
        self.write_manifest(path.parents[1])
        with self.assertRaisesRegex(ValueError, "must explicitly request read_fifo_depth=64"):
            self.verify_run("legacy-shared")

    def test_wrong_architecture_src_mapping_is_rejected(self):
        path = self.output / "legacy-two-bundles" / "run1" / "panda-temp" / "architecture.xml"
        path.write_text(path.read_text(encoding="utf-8").replace(
            'port="src" bundle="input"', 'port="src" bundle="output"'), encoding="utf-8")
        self.write_manifest(path.parents[1])
        with self.assertRaisesRegex(ValueError, "architecture.xml src/dst bundle mapping is incorrect"):
            self.verify_run("legacy-two-bundles")

    def test_nonzero_architecture_latency_is_rejected(self):
        path = self.output / "duplex-shared" / "run1" / "panda-temp" / "architecture.xml"
        path.write_text(path.read_text(encoding="utf-8").replace('latency="0"', 'latency="1"'),
                        encoding="utf-8")
        self.write_manifest(path.parents[1])
        with self.assertRaisesRegex(ValueError, "must explicitly have latency=0"):
            self.verify_run("duplex-shared")

    def test_legacy_missing_each_feature_off_diagnostic_is_rejected(self):
        for direction in ("read", "write"):
            path = self.output / "legacy-shared" / "run1" / "stderr.log"
            text = path.read_text(encoding="utf-8")
            text = "\n".join(line for line in text.splitlines()
                               if f"explicit {direction}-burst request ignored" not in line) + "\n"
            path.write_text(text, encoding="utf-8")
            self.write_manifest(path.parent)
            with self.assertRaisesRegex(ValueError,
                                        f"expected exactly one feature-off {direction} diagnostic"):
                self.verify_run("legacy-shared")
            self.build_run("legacy-shared", "run1", 100)

    def test_legacy_wrong_feature_off_bundle_is_rejected(self):
        path = self.output / "legacy-two-bundles" / "run1" / "stderr.log"
        path.write_text(path.read_text(encoding="utf-8").replace(
            "bundle input", "bundle output", 1), encoding="utf-8")
        self.write_manifest(path.parent)
        with self.assertRaisesRegex(ValueError, "feature-off read diagnostic for bundle input"):
            self.verify_run("legacy-two-bundles")

    def test_legacy_wrong_feature_off_reason_is_rejected(self):
        path = self.output / "legacy-shared" / "run1" / "stderr.log"
        path.write_text(path.read_text(encoding="utf-8").replace(
            "explicit read-burst request ignored", "candidate rejected"), encoding="utf-8")
        self.write_manifest(path.parent)
        with self.assertRaisesRegex(ValueError, "feature-off read diagnostic for bundle shared"):
            self.verify_run("legacy-shared")

    def test_legacy_feature_off_diagnostic_on_stdout_is_rejected(self):
        run = self.output / "legacy-shared" / "run1"
        diagnostic = ("Keeping legacy m_axi reads for bundle shared: explicit read-burst request ignored "
                      "because experimental-m-axi-burst=0\n")
        stderr = run / "stderr.log"
        stderr.write_text(stderr.read_text(encoding="utf-8").replace(diagnostic, ""), encoding="utf-8")
        stdout = run / "stdout.log"
        stdout.write_text(stdout.read_text(encoding="utf-8") + diagnostic, encoding="utf-8")
        self.write_manifest(run)
        with self.assertRaisesRegex(ValueError, "feature-off read diagnostic for bundle shared"):
            self.verify_run("legacy-shared")

    def test_duplex_candidate_fallback_diagnostic_does_not_replace_activation(self):
        path = self.output / "duplex-shared" / "run1" / "stderr.log"
        path.write_text("Keeping legacy m_axi reads for bundle shared: candidate rejected\n" +
                        path.read_text(encoding="utf-8"), encoding="utf-8")
        self.write_manifest(path.parent)
        self.assertEqual(self.verify_run("duplex-shared"), 100)

    def test_run_cycles_must_match_within_each_profile(self):
        xml = self.output / "duplex-shared" / "run2" / "bambu_results.xml"
        xml.write_text('<results><evaluation CYCLES="101"/></results>\n', encoding="utf-8")
        self.write_manifest(xml.parent)
        with self.assertRaisesRegex(ValueError, "duplex-shared: run1/run2 CYCLES are not reproducible"):
            self.verify()


if __name__ == "__main__":
    unittest.main()
