# Author: Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>

import csv
import os
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

from compiler_case_manifest import load_cases, main, validate_fixture
from check_default_burst_artifacts import check_profile


HERE = Path(__file__).resolve().parent
MANIFEST = HERE / "compiler_profile_cases.tsv"


class CompilerCaseManifestTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        tmp_root = Path(os.environ.get("BURST_TEST_TMPDIR", Path.cwd() / "out_compiler_case_manifest"))
        tmp_root.mkdir(parents=True, exist_ok=True)
        cls.tmp_owner = tempfile.TemporaryDirectory(prefix="compiler-case-manifest-", dir=tmp_root)
        cls.old_tmp = {key: os.environ.get(key) for key in ("TMPDIR", "TMP", "TEMP")}
        for key in cls.old_tmp:
            os.environ[key] = cls.tmp_owner.name

    @classmethod
    def tearDownClass(cls):
        for key, value in cls.old_tmp.items():
            if value is None:
                os.environ.pop(key, None)
            else:
                os.environ[key] = value
        cls.tmp_owner.cleanup()

    def test_legacy_ids_and_read_write_fixtures_are_complete(self):
        cases = load_cases(MANIFEST)
        read = [case for case in cases if case.direction == "read"]
        write = [case for case in cases if case.direction == "write"]
        self.assertEqual(len(cases), 41)
        self.assertEqual(len(read), 20)
        self.assertEqual(len(write), 21)
        self.assertEqual(len({case.legacy_id for case in cases}), len(cases))
        self.assertEqual(len({case.tag for case in cases}), len(cases))
        validate_fixture(cases, HERE / "compiler_cases/axi_read_profile_cases.cpp", "read")
        validate_fixture(cases, HERE / "write_compiler_cases/write_profile_matrix.cpp", "write")
        self.assertIn("read_profile_invalid_duplicate_depth_comma", {case.legacy_id for case in read})
        explicit_single_beat = next(case for case in read if case.legacy_id == "explicit-burst-one")
        self.assertEqual((explicit_single_beat.burst, explicit_single_beat.outstanding,
                          explicit_single_beat.fifo), ("1", "3", "8"))
        self.assertIn("conflicting-write-fifo-profile", {case.legacy_id for case in write})

    def test_manifest_rejects_duplicate_ids(self):
        row = next(line for line in MANIFEST.read_text().splitlines() if line and not line.startswith("#"))
        with tempfile.TemporaryDirectory(dir=self.tmp_owner.name) as directory:
            duplicate = Path(directory) / "duplicate.tsv"
            duplicate.write_text("\n".join((row, row)) + "\n")
            with self.assertRaisesRegex(ValueError, "duplicate legacy id"):
                load_cases(duplicate)

    def test_default_profile_table_keeps_its_parameter_only_cases(self):
        manifest = HERE / "default_profile_cases.tsv"
        fixture = (HERE / "compiler_cases/axi_default_profile_cases.cpp").read_text()
        with manifest.open(encoding="utf-8", newline="") as stream:
            rows = list(csv.reader((line for line in stream if not line.startswith("#")), delimiter="|"))
        self.assertEqual({row[0] for row in rows}, {
            "default-read", "default-write", "default-duplex", "explicit-optout",
            "read-outstanding-only", "read-fifo-only", "read-outstanding-and-fifo",
            "write-outstanding-only", "write-fifo-only", "write-outstanding-and-fifo",
            "duplex-read-outstanding-only", "duplex-write-fifo-only",
            "read-override", "write-override", "invalid-burst-value",
        })
        self.assertEqual(len({row[0] for row in rows}), len(rows))
        self.assertTrue(all(len(row) == 16 for row in rows))
        by_tag = {row[0]: row for row in rows}
        for tag, origin in (("read-outstanding-only", "single-beat-default:outstanding-hint"),
                            ("read-fifo-only", "single-beat-default:fifo-hint"),
                            ("read-outstanding-and-fifo", "single-beat-default:outstanding+fifo-hints"),
                            ("write-outstanding-only", "single-beat-default:outstanding-hint"),
                            ("write-fifo-only", "single-beat-default:fifo-hint"),
                            ("write-outstanding-and-fifo", "single-beat-default:outstanding+fifo-hints")):
            self.assertEqual(by_tag[tag][14 if tag.startswith("read") else 15], origin)
            self.assertEqual(by_tag[tag][8 if tag.startswith("read") else 11], "1")
        self.assertEqual(by_tag["read-outstanding-only"][10], "4")
        self.assertEqual(by_tag["duplex-read-outstanding-only"][10], "4")
        self.assertEqual(by_tag["read-fifo-only"][10], "64")
        self.assertEqual(by_tag["read-outstanding-and-fifo"][10], "64")
        self.assertEqual((by_tag["duplex-read-outstanding-only"][8],
                          by_tag["duplex-read-outstanding-only"][11]), ("1", "16"))
        self.assertEqual((by_tag["duplex-write-fifo-only"][8],
                          by_tag["duplex-write-fifo-only"][11]), ("16", "1"))
        for row in rows:
            self.assertIn(f"defined({row[2]})", fixture)
            self.assertRegex(fixture, rf"\b{row[1]}\s*\(")

    def test_single_beat_checker_requires_the_hint_reason_and_resolved_engine(self):
        with tempfile.TemporaryDirectory(dir=self.tmp_owner.name) as directory:
            root = Path(directory)
            (root / "compiler.log").write_text(
                "AXI_BURST_PROFILE root=kernel bundle=data enabled=1 burst_type=INCREMENTAL(device) "
                "read_max=1(single-beat-default:outstanding-hint) read_outstanding=3(explicit) "
                "read_fifo=4(derived) write_max=16(default) write_outstanding=16(default) write_fifo=32(derived)\n"
                "AXI_BURST_DECISION root=kernel bundle=data read=recognized(B=1/single-beat-default:outstanding-hint,"
                "O=3/explicit,D=4; "
                "stage=preanalysis; reason=eligible) write=not-applicable-no-write-access duplex=not-candidate\n"
                "Configured read burst regions for bundle data (B=1)\n",
                encoding="utf-8",
            )
            (root / "kernel.v").write_text(
                "MinimalAXI4MasterPipelined #(.B_MAX(1), .MAX_OUTSTANDING(3), .FIFO_DEPTH(4)) reader;\n",
                encoding="utf-8",
            )
            (root / "architecture.xml").write_text(
                '<architecture><bundle name="data" mode="m_axi" num_read_outstanding="3"/></architecture>\n',
                encoding="utf-8",
            )
            check_profile(root, "read", "data", read_fifo=4, read_burst=1, read_outstanding=3,
                          read_burst_origin="single-beat-default:outstanding-hint")

    def test_executor_accepts_plugin_diagnostic_from_cc_output(self):
        with tempfile.TemporaryDirectory(dir=self.tmp_owner.name) as directory:
            root = Path(directory)
            install = root / "install"
            bambu = install / "bin/bambu"
            plugin = install / "lib/panda/clang-16/ASTAnalyzer.so"
            bambu.parent.mkdir(parents=True)
            plugin.parent.mkdir(parents=True)
            bambu.write_text(
                "#!/usr/bin/env python3\n"
                "from pathlib import Path\n"
                "import sys\n"
                "p = Path('panda-temp'); p.mkdir()\n"
                "(p / '__cc_output0').write_text('invalid profile from plugin')\n"
                "print('generic compiler failure')\n"
                "sys.exit(2)\n",
                encoding="utf-8",
            )
            bambu.chmod(0o755)
            plugin.write_text("test plugin", encoding="utf-8")
            output = root / "case"
            with patch.dict(os.environ, {"BAMBU_HLS": str(install)}):
                self.assertEqual(main(["execute", "--output", str(output), "--bambu", str(bambu),
                                       "--expected-failure", "invalid profile from plugin", "--",
                                       "fixture.cpp"]), 0)
            self.assertIn("fixture.cpp", (output / "argv.txt").read_text())
            self.assertIn("generic compiler failure", (output / "stdout.log").read_text())
            self.assertEqual((output / "exit_code.txt").read_text(), "2\n")


if __name__ == "__main__":
    unittest.main()
