#!/usr/bin/env python3
"""Negative offline tests for the copy-baseline artifact gate."""
import csv
import hashlib
from pathlib import Path
import shutil
import sys
import tempfile
import unittest

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[2]
TMP_ROOT = REPO / "documentation" / "tmp"
sys.path.insert(0, str(HERE))
import check_copy_baseline_results as checker


class CopyBaselineGateTests(unittest.TestCase):
    def setUp(self):
        TMP_ROOT.mkdir(parents=True, exist_ok=True)
        self.temp = tempfile.TemporaryDirectory(prefix="test-copy-baseline-", dir=TMP_ROOT)
        self.root = Path(self.temp.name)
        self.source = self.root / "copy_baseline.cpp"
        shutil.copy2(HERE / "copy_baseline.cpp", self.source)
        self.testbench = self.root / "copy_baseline_tb.cpp"
        shutil.copy2(HERE / "copy_baseline_tb.cpp", self.testbench)
        self.bambu = self.root / "bambu"
        self.settings = self.root / "settings.sh"
        self.plugin = self.root / "ASTAnalyzer.so"
        for path in (self.bambu, self.settings, self.plugin):
            path.write_text(path.name + " fixture\n", encoding="utf-8")
        self.expectations = self.root / "copy_cycle_expectations.tsv"
        shutil.copy2(HERE / "copy_cycle_expectations.tsv", self.expectations)
        expectations_text = self.expectations.read_text(encoding="utf-8")
        self.expectations.write_text(expectations_text, encoding="utf-8")
        self.results = self.root / "results"
        self.rows = checker.read_rows(self.expectations, self.source, self.testbench)
        self._build_case(self.rows[0], 100)
        self._build_case(self.rows[1], 200)
        self._build_case(self.rows[2], 300)
        (self.results / "provenance.txt").write_text(
            f"output={self.results.resolve()}\n", encoding="utf-8")

    def tearDown(self):
        self.temp.cleanup()

    def _build_case(self, row, cycles):
        case = row["case"]
        profile, macro, read_bundle, write_bundle = checker.CASES[case]
        case_dir = self.results / case
        case_dir.mkdir(parents=True)
        expected_cycles = row["cycles"]
        (case_dir / "bambu_results.xml").write_text(
            f'<results><evaluation CYCLES="{expected_cycles}"/></results>\n', encoding="utf-8")
        (case_dir / "stdout.log").write_text("COPY_BASELINE_GOLDEN_PASS N=1024\n", encoding="utf-8")
        (case_dir / "stderr.log").write_text("", encoding="utf-8")
        simulation_log = case_dir / "HLS_output" / "simulation" / "testbench.log"
        simulation_log.parent.mkdir(parents=True)
        simulation_log.write_text("COPY_BASELINE_GOLDEN_PASS N=1024\n", encoding="utf-8")
        if profile == "burst":
            rtl = (
                "module kernel(input p_m_axi_input_araddr, input p_m_axi_output_awaddr);\n"
                "wire burst_araddr; wire burst_awaddr;\n"
                "assign p_m_axi_input_araddr = burst_araddr; assign p_m_axi_output_awaddr = burst_awaddr;\n"
                "MinimalAXI4MasterPipelined #(.B_MAX(16), .MAX_OUTSTANDING(1), .FIFO_DEPTH(32), .BITSIZE_m_axi_araddr(32), .BITSIZE_in4(32), .BITSIZE_m_axi_rdata(32), .BITSIZE_in5(32)) read_engine (.m_axi_araddr(burst_araddr));\n"
                "MinimalAXI4MasterPipelinedWrite #(.B_MAX(16), .MAX_OUTSTANDING(1), .FIFO_DEPTH(32), .BITSIZE_in4(32), .BITSIZE_in5(32), .BITSIZE_m_axi_awaddr(32), .BITSIZE_in3(32), .BITSIZE_m_axi_wdata(32)) write_engine (.m_axi_awaddr(burst_awaddr));\n"
                "wire configure_read; wire burst_read; wire configure_write; wire store_write;\n"
                "endmodule\n")
        else:
            rtl = (
                f"module kernel(input p_m_axi_{read_bundle}_araddr, input p_m_axi_{write_bundle}_awaddr);\n"
                f"MinimalAXI4AdapterSingleBeat legacy_adapter (.m_axi_araddr(p_m_axi_{read_bundle}_araddr), "
                f".m_axi_awaddr(p_m_axi_{write_bundle}_awaddr));\nendmodule\n")
        (case_dir / "kernel.v").write_text(rtl, encoding="utf-8")
        provenance = [f"case={case}", f"profile={profile}", f"macro={macro}",
                      f"source={self.source}", f"testbench={self.testbench}",
                      "argv=bambu copy_baseline.cpp --top-fname=kernel --compiler=I386_CLANG16 "
                      "--generate-interface=INFER --device-name=xcu250\\,-2L\\,figd2104 "
                      "--AXI-burst-type=INCREMENTAL --mem-delay-read=64 --mem-delay-write=2 "
                      "--tb-queue-size=32 --memory-allocation-policy=NO_BRAM --distram-threshold=0 "
                      "--clock-period=3.33 --simulate --simulator=MODELSIM "
                      "--generate-tb=copy_baseline_tb.cpp --no-clean "
                      "--bambu-parameter=experimental-m-axi-burst=1 "
                      f"--extra-cc-options=-DCOPY_BASELINE_PROFILE={macro}"]
        for path in (self.source, self.testbench, self.bambu, self.settings, self.plugin):
            provenance.append(f"{hashlib.sha256(path.read_bytes()).hexdigest()}  {path}")
        (case_dir / "provenance.txt").write_text("\n".join(provenance) + "\n", encoding="utf-8")
        self._write_manifest(case_dir)

    @staticmethod
    def _write_manifest(case_dir):
        artifacts = sorted(path for path in case_dir.rglob("*")
                           if path.is_file() and (path.suffix.lower() in {".v", ".sv", ".xml"}
                                                  or path.name == "testbench.log"))
        (case_dir / "artifact-sha256.txt").write_text(
            "".join(f"{hashlib.sha256(path.read_bytes()).hexdigest()}  {path.absolute()}\n"
                    for path in artifacts), encoding="utf-8")

    def _check_row(self, index=0):
        return checker.check_case(self.results, self.rows[index], self.source, self.testbench,
                                  self.bambu, self.settings, self.plugin)

    def test_positive_profiles_have_complete_artifacts(self):
        self.assertEqual([self._check_row(index) for index in range(3)], [5158, 66565, 69634])

    def test_missing_xml_is_rejected(self):
        (self.results / self.rows[0]["case"] / "bambu_results.xml").unlink()
        self._write_manifest(self.results / self.rows[0]["case"])
        with self.assertRaisesRegex(ValueError, "bambu_results.xml missing"):
            self._check_row()

    def test_missing_manifest_is_rejected(self):
        (self.results / self.rows[0]["case"] / "artifact-sha256.txt").unlink()
        with self.assertRaisesRegex(ValueError, "artifact manifest missing"):
            self._check_row()

    def test_mutated_xml_is_rejected(self):
        xml = self.results / self.rows[0]["case"] / "bambu_results.xml"
        xml.write_text(xml.read_text().replace("5158", "5159"), encoding="utf-8")
        with self.assertRaisesRegex(ValueError, "artifact digest mismatch"):
            self._check_row()

    def test_mutated_rtl_is_rejected(self):
        rtl = self.results / self.rows[0]["case"] / "kernel.v"
        rtl.write_text(rtl.read_text().replace("endmodule", "wire changed; endmodule"), encoding="utf-8")
        with self.assertRaisesRegex(ValueError, "artifact digest mismatch"):
            self._check_row()

    def test_mutated_simulation_log_is_rejected_by_manifest(self):
        log = self.results / self.rows[0]["case"] / "HLS_output" / "simulation" / "testbench.log"
        log.write_text("COPY_BASELINE_GOLDEN_PASS N=1023\n", encoding="utf-8")
        with self.assertRaisesRegex(ValueError, "artifact digest mismatch"):
            self._check_row()

    def test_marker_in_stdout_does_not_replace_exact_simulation_log(self):
        log = self.results / self.rows[0]["case"] / "HLS_output" / "simulation" / "testbench.log"
        log.write_text("simulation failed\n", encoding="utf-8")
        self._write_manifest(self.results / self.rows[0]["case"])
        with self.assertRaisesRegex(ValueError, "no copy golden/canary/source-integrity pass marker"):
            self._check_row()

    def test_missing_exact_simulation_log_is_rejected(self):
        log = self.results / self.rows[0]["case"] / "HLS_output" / "simulation" / "testbench.log"
        log.unlink()
        self._write_manifest(self.results / self.rows[0]["case"])
        with self.assertRaisesRegex(ValueError, "expected HLS_output/simulation/testbench.log missing"):
            self._check_row()

    def test_write_engine_wrong_widths_are_rejected(self):
        rtl = self.results / self.rows[0]["case"] / "kernel.v"
        rtl.write_text(rtl.read_text().replace(".BITSIZE_m_axi_wdata(32)", ".BITSIZE_m_axi_wdata(16)"),
                       encoding="utf-8")
        self._write_manifest(self.results / self.rows[0]["case"])
        with self.assertRaisesRegex(ValueError, "required B/O/F/width profile"):
            self._check_row()

    def test_copy_maxn_must_match_source_and_testbench(self):
        self.testbench.write_text(self.testbench.read_text().replace(
            "#define COPY_BASELINE_MAXN 4096", "#define COPY_BASELINE_MAXN 2048"), encoding="utf-8")
        with self.assertRaisesRegex(ValueError, "must agree on COPY_BASELINE_MAXN=4096"):
            checker.read_rows(self.expectations, self.source, self.testbench)

    def test_invalid_manifest_digest_is_rejected(self):
        case_dir = self.results / self.rows[0]["case"]
        manifest = case_dir / "artifact-sha256.txt"
        lines = manifest.read_text(encoding="utf-8").splitlines()
        lines[0] = "0" * 64 + lines[0][64:]
        manifest.write_text("\n".join(lines) + "\n", encoding="utf-8")
        with self.assertRaisesRegex(ValueError, "artifact digest mismatch"):
            self._check_row()

    def test_external_manifest_path_is_rejected(self):
        case_dir = self.results / self.rows[0]["case"]
        manifest = case_dir / "artifact-sha256.txt"
        external = self.source
        manifest.write_text(f"{hashlib.sha256(external.read_bytes()).hexdigest()}  {external}\n",
                            encoding="utf-8")
        with self.assertRaisesRegex(ValueError, "outside case directory"):
            self._check_row()

    def test_symlink_manifest_artifact_is_rejected(self):
        case_dir = self.results / self.rows[0]["case"]
        rtl = case_dir / "kernel.v"
        content = rtl.read_text()
        rtl.unlink()
        outside = self.root / "outside.v"
        outside.write_text(content, encoding="utf-8")
        rtl.symlink_to(outside)
        self._write_manifest(case_dir)
        with self.assertRaisesRegex(ValueError, "symlink"):
            self._check_row()

    def test_invalid_or_missing_cycle_signature_is_rejected(self):
        original = self.expectations.read_text(encoding="utf-8")
        for invalid in ("PENDING", "0", "-1", ""):
            with self.subTest(cycles=invalid):
                text = original.replace("\t5158\t", f"\t{invalid}\t")
                self.expectations.write_text(text, encoding="utf-8")
                expected_error = "incomplete expectation" if not invalid else "cycle signature must be a positive integer"
                with self.assertRaisesRegex(ValueError, expected_error):
                    checker.read_rows(self.expectations, self.source, self.testbench)
        self.expectations.write_text(original, encoding="utf-8")

    def test_run_provenance_must_match_results_directory(self):
        path = self.results / "provenance.txt"
        path.write_text("output=/elsewhere\n", encoding="utf-8")
        with self.assertRaisesRegex(ValueError, "output does not match"):
            checker.check_run_provenance(self.results)

    def test_case_provenance_must_match_case_directory(self):
        path = self.results / self.rows[0]["case"] / "provenance.txt"
        path.write_text(path.read_text().replace("case=copy-burst-two-bundles",
                                                "case=copy-legacy-two-bundles"), encoding="utf-8")
        with self.assertRaisesRegex(ValueError, "case provenance identity"):
            self._check_row()

    def test_wrong_signature_is_rejected(self):
        row = dict(self.rows[0], cycles="101")
        with self.assertRaisesRegex(ValueError, "expected 101 cycles, observed 5158"):
            checker.check_case(self.results, row, self.source, self.testbench,
                               self.bambu, self.settings, self.plugin)

    def test_wrong_configuration_is_rejected(self):
        path = self.results / self.rows[0]["case"] / "provenance.txt"
        path.write_text(path.read_text().replace("COPY_BASELINE_PROFILE=1", "COPY_BASELINE_PROFILE=3"),
                        encoding="utf-8")
        with self.assertRaisesRegex(ValueError, "argv has an incorrect/missing compile or simulation setting"):
            self._check_row()

    def test_missing_required_case_is_rejected(self):
        lines = self.expectations.read_text(encoding="utf-8").splitlines()
        self.expectations.write_text("\n".join(lines[:-1]) + "\n", encoding="utf-8")
        with self.assertRaisesRegex(ValueError, "exactly the three required cases"):
            checker.read_rows(self.expectations, self.source, self.testbench)

    def test_profile_parameters_split_across_instances_are_rejected(self):
        rtl = self.results / self.rows[0]["case"] / "kernel.v"
        rtl.write_text(
            "module kernel(input p_m_axi_input_araddr, input p_m_axi_output_awaddr);\n"
            "wire burst_araddr; wire burst_awaddr;\n"
            "assign p_m_axi_input_araddr = burst_araddr; assign p_m_axi_output_awaddr = burst_awaddr;\n"
            "MinimalAXI4MasterPipelined #(.B_MAX(16), .MAX_OUTSTANDING(1)) read_engine (.m_axi_araddr(burst_araddr));\n"
            "MinimalAXI4MasterPipelined #(.FIFO_DEPTH(32)) second_read_engine ();\n"
            "MinimalAXI4MasterPipelinedWrite #(.B_MAX(16), .MAX_OUTSTANDING(1), .FIFO_DEPTH(32)) write_engine (.m_axi_awaddr(burst_awaddr));\n"
            "wire configure_read; wire burst_read; wire configure_write; wire store_write;\nendmodule\n",
            encoding="utf-8")
        self._write_manifest(self.results / self.rows[0]["case"])
        with self.assertRaisesRegex(ValueError, "expected exactly one MinimalAXI4MasterPipelined instance"):
            self._check_row()

    def test_extra_malformed_write_engine_instance_is_rejected(self):
        rtl = self.results / self.rows[0]["case"] / "kernel.v"
        rtl.write_text(rtl.read_text().replace(
            "wire configure_read;",
            "MinimalAXI4MasterPipelinedWrite #(.B_MAX(16)) broken_writer ();\nwire configure_read;"),
            encoding="utf-8")
        self._write_manifest(self.results / self.rows[0]["case"])
        with self.assertRaisesRegex(ValueError,
                                    "expected exactly one MinimalAXI4MasterPipelinedWrite instance"):
            self._check_row()


if __name__ == "__main__":
    unittest.main()
