import hashlib
import importlib.util
import json
import os
import re
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


TESTS = Path(__file__).resolve().parent
EXTRACTOR_PATH = TESTS / "extract_t4b_cells.py"
SPEC = importlib.util.spec_from_file_location("extract_t4b_cells", EXTRACTOR_PATH)
EXTRACTOR = importlib.util.module_from_spec(SPEC)
assert SPEC.loader is not None
SPEC.loader.exec_module(EXTRACTOR)
ROOT = EXTRACTOR.ROOT
TMP_ROOT = Path(os.environ.get("BURST_TEST_TMPDIR", Path.cwd())) / f"out_{Path(__file__).stem}"


class T4BModelHarnessTests(unittest.TestCase):
    def setUp(self):
        TMP_ROOT.mkdir(parents=True, exist_ok=True)
        self.temp = tempfile.TemporaryDirectory(prefix="t4b-offline-", dir=TMP_ROOT)
        self.addCleanup(self.temp.cleanup)
        self.scratch = Path(self.temp.name)

    def test_extractor_preserves_original_cell_bodies_and_hashes(self):
        out = self.scratch / "hdl"
        manifest = EXTRACTOR.extract(
            ROOT / "etc/libtech/NC_AXI_IPs.xml",
            ROOT / "etc/libtech/NC_TESTBENCH_IPs.xml",
            out,
        )
        for cell_name, xml, output in (
            ("MinimalAXI4MasterPipelined", ROOT / "etc/libtech/NC_AXI_IPs.xml", "MinimalAXI4MasterPipelined.sv"),
            ("TestbenchMEMAXI", ROOT / "etc/libtech/NC_TESTBENCH_IPs.xml", "TestbenchMEMAXI.sv"),
        ):
            module, expected_body = EXTRACTOR.find_cell(xml, cell_name)
            extracted = (out / output).read_text()
            expected_body = EXTRACTOR.normalize_questa_body(cell_name, expected_body)
            self.assertTrue(extracted.endswith(expected_body + "\nendmodule\n"))
            self.assertIn(f"module {cell_name} #(", extracted)
            self.assertEqual(
                manifest["extracted_hdl"][output],
                hashlib.sha256((out / output).read_bytes()).hexdigest(),
            )
            self.assertEqual(module.get("id"), cell_name)
        self.assertEqual(len(manifest["sources"]), 2)
        memory_hdl = (out / "TestbenchMEMAXI.sv").read_text()
        self.assertIn("(reset == 1'b0)", memory_hdl)
        self.assertIn("localparam integer BITSIZE_araddr = 32;", memory_hdl)
        self.assertIn("localparam integer BITSIZE_rdata = 32;", memory_hdl)
        engine_hdl = (out / "MinimalAXI4MasterPipelined.sv").read_text()
        self.assertIn("localparam integer BITSIZE_m_axi_araddr = 32;", engine_hdl)
        self.assertIn("localparam integer BITSIZE_m_axi_rdata = 32;", engine_hdl)
        self.assertIn("localparam integer BITSIZE_in5 = 32;", engine_hdl)
        self.assertIn("localparam integer BITSIZE_out1 = 32;", engine_hdl)
        self.assertIn("MAX_OUTSTANDING", engine_hdl)
        declared_widths = set(re.findall(r"localparam integer (BITSIZE_\w+) =", engine_hdl))
        referenced_widths = set(re.findall(r"\bBITSIZE_\w+\b", engine_hdl))
        self.assertEqual(referenced_widths - declared_widths, set())

    def test_questa_rewrites_fix_forward_net_and_block_local_declarations(self):
        out = self.scratch / "questa-hdl"
        EXTRACTOR.extract(
            ROOT / "etc/libtech/NC_AXI_IPs.xml",
            ROOT / "etc/libtech/NC_TESTBENCH_IPs.xml",
            out,
        )
        engine = (out / "MinimalAXI4MasterPipelined.sv").read_text()
        self.assertEqual(engine.count("wire burst_outstanding;"), 1)
        self.assertEqual(engine.count("assign burst_outstanding = (descriptor_count != 0);"), 1)
        self.assertLess(
            engine.index("reg [DESC_COUNT_W-1:0] descriptor_count;"),
            engine.index("assign burst_outstanding = (descriptor_count != 0);"),
        )

        memory = (out / "TestbenchMEMAXI.sv").read_text()
        self.assertEqual(memory.count("typedef int unsigned ptr_t;"), 1)
        self.assertLess(memory.index("module TestbenchMEMAXI"), memory.index("typedef int unsigned ptr_t;"))
        self.assertEqual(memory.count("automatic ptr_t currAddr;"), 0)
        self.assertEqual(memory.count("automatic ptr_t endAddr;"), 0)
        self.assertEqual(memory.count("ptr_t currAddr;"), 0)
        self.assertEqual(memory.count("ptr_t endAddr;"), 0)

    def test_questa_rewrites_legacy_pointer_locals_and_accepts_current_body(self):
        legacy_body = "\n".join((
            "automatic ptr_t currAddr;",
            "automatic ptr_t endAddr;",
            "automatic ptr_t currAddr;",
            "automatic ptr_t endAddr;",
        ))
        normalized_legacy = EXTRACTOR.normalize_questa_body("TestbenchMEMAXI", legacy_body)
        self.assertEqual(normalized_legacy.count("automatic ptr_t"), 0)
        self.assertEqual(normalized_legacy.count("ptr_t currAddr;"), 2)
        self.assertEqual(normalized_legacy.count("ptr_t endAddr;"), 2)

        current_body = "always @(posedge clock) begin\n    read_count <= read_count + 1;\nend"
        self.assertEqual(
            EXTRACTOR.normalize_questa_body("TestbenchMEMAXI", current_body),
            current_body,
        )

        with self.assertRaisesRegex(ValueError, "found 1"):
            EXTRACTOR.normalize_questa_body(
                "TestbenchMEMAXI", "automatic ptr_t currAddr;"
            )

    def test_extractor_defaults_resolve_from_its_actual_repository_location(self):
        out = self.scratch / "default-path-hdl"
        result = subprocess.run(
            [sys.executable, "-B", str(EXTRACTOR_PATH), "--outdir", str(out)],
            cwd=self.scratch, text=True, stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT, check=False,
        )
        self.assertEqual(result.returncode, 0, result.stdout)
        manifest = json.loads((out / "extract_manifest.json").read_text())
        self.assertIn(str(ROOT / "etc/libtech/NC_AXI_IPs.xml"), manifest["sources"])
        self.assertIn(str(ROOT / "etc/libtech/NC_TESTBENCH_IPs.xml"), manifest["sources"])
        self.assertTrue((out / "MinimalAXI4MasterPipelined.sv").is_file())
        self.assertTrue((out / "TestbenchMEMAXI.sv").is_file())

    def test_n1_rready_driver_changes_only_after_negedge(self):
        testbench = (TESTS / "tb_t4b_slave.sv").read_text()
        scoreboard = testbench.split("always @(posedge clock) begin : scoreboard", 1)[1]
        scoreboard = scoreboard.split("    initial begin : run_test", 1)[0]
        self.assertNotRegex(scoreboard, r"\brready\s*=")

        # The first RVALID is recorded by the posedge scoreboard.  The
        # coroutine awakened by that event must wait for a negedge before it
        # drives RREADY, avoiding simulator active-region ordering races.
        self.assertRegex(
            testbench,
            r"wait\s*\(first_rvalid_cycle\s*>=\s*0\);\s*"
            r"latency\s*=\s*first_rvalid_cycle\s*-\s*first_ar_cycle;"
            r"[\s\S]*?@\(negedge\s+clock\);\s*rready\s*=\s*1'b1;",
        )

        # The drain-side deassertion is also edge-separated from the
        # posedge scoreboard update of accepted_r.
        self.assertRegex(
            testbench,
            r"wait\s*\(accepted_r\s*==\s*total_beats\);\s*"
            r"@\(negedge\s+clock\);\s*rready\s*=\s*1'b0;",
        )

    def test_prepare_only_generates_auditable_n1_and_n1b_commands(self):
        out = self.scratch / "t4b-prepared"
        env = os.environ.copy()
        env.update({"TMPDIR": str(TMP_ROOT), "TMP": str(TMP_ROOT), "TEMP": str(TMP_ROOT),
                    "MODELSIM_ROOT": str(self.scratch / "no-modelsim")})
        result = subprocess.run(
            ["bash", str(TESTS / "run_t4b_models.sh"), "--prepare-only", str(out)],
            cwd=ROOT, env=env, text=True, stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT, check=False,
        )
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn("ModelSim was not invoked", result.stdout)
        plan = (out / "planned_runs.tsv").read_text().splitlines()
        self.assertEqual(len(plan), 1 + 1 + 56)
        self.assertIn("N1\t-\t-\t-\t16\t64\t63\t33\t1", plan)
        self.assertTrue(any(row.startswith("N1b\t16\t16\t256\t2048\t64\t63\t33\t2") for row in plan))
        manifest = json.loads((out / "run_manifest.json").read_text())
        self.assertEqual(manifest["n1"]["required_ar_before_r_handshake"], 16)
        self.assertEqual(manifest["n1b"]["repeats"], 2)
        self.assertIn("etc/libtech/NC_AXI_IPs.xml", manifest["input_sha256"])
        self.assertTrue(any(key.endswith("TestbenchMEMAXI.sv") for key in manifest["input_sha256"]))
        self.assertFalse((out / "modelsim_provenance.txt").exists())


if __name__ == "__main__":
    unittest.main()
