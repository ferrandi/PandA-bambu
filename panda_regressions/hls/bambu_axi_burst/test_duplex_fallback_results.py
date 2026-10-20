#!/usr/bin/env python3
"""Offline fail-closed tests for check_duplex_fallback_results.py."""
from __future__ import annotations

import hashlib
import os
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
TMP_ROOT = Path(os.environ.get("BURST_TEST_TMPDIR", Path.cwd())) / f"out_{Path(__file__).stem}"
TMP_ROOT.mkdir(parents=True, exist_ok=True)
os.environ.setdefault("TMPDIR", str(TMP_ROOT))
os.environ.setdefault("TMP", str(TMP_ROOT))
os.environ.setdefault("TEMP", str(TMP_ROOT))
sys.path.insert(0, str(Path(__file__).resolve().parent))
import check_duplex_fallback_results as checker


PROFILE_REASONS = {
    "phased_erew_reverse_same_bundle": (
        "1", "memory writes or opaque effects; prefetch ordering is not proven",
        "opaque, volatile, atomic, or unclassified memory effects"),
    "overlap_rw_same_bundle": (
        "2", "bundle contains stores", "bundle contains reads; same-bundle read/write requires duplex support"),
    "conditional_read_or_write_same_bundle": (
        "3", "memory writes or opaque effects; prefetch ordering is not proven",
        "opaque, volatile, atomic, or unclassified memory effects"),
    "residual_store_same_bundle": (
        "4", "memory writes or opaque effects; prefetch ordering is not proven",
        "opaque, volatile, atomic, or unclassified memory effects"),
}


def compiler_evidence(read_reason, write_reason, read_accesses=1, write_accesses=1,
                      read_requested=1, write_requested=1, candidate=0, activated=0,
                      duplex_reason="not eligible"):
    return (
        f"---Keeping legacy m_axi accesses for bundle shared: {read_reason}\n"
        f"---Keeping legacy m_axi writes for bundle shared: {write_reason}\n"
        f'AXI_BURST_PREANALYSIS bundle=shared owner_argument_index=1 read_accesses={read_accesses} '
        f'write_accesses={write_accesses} read_requested={read_requested} read_recognized=0 '
        f'read_fallback="{read_reason}" write_requested={write_requested} '
        f'write_recognized=0 write_fallback="{write_reason}" same_base_duplex_candidate={candidate} '
        f'same_base_duplex_reason="{duplex_reason}" production_activated={activated} '
        'activation_reason="not activated"\n'
    )


def valid_profile_evidence(profile, **overrides):
    _, read_reason, write_reason = PROFILE_REASONS[profile]
    access_counts = {
        "conditional_read_or_write_same_bundle": (2, 3),
        "residual_store_same_bundle": (1, 2),
    }.get(profile, (1, 1))
    values = {
        "read_reason": read_reason,
        "write_reason": write_reason,
        "read_accesses": access_counts[0],
        "write_accesses": access_counts[1],
        "duplex_reason": ("original effects are not fully classified ordinary accesses"
                          if profile in {"conditional_read_or_write_same_bundle",
                                         "residual_store_same_bundle"} else "not eligible"),
    }
    values.update(overrides)
    return compiler_evidence(**values)


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


class DuplexFallbackCheckerTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(dir=TMP_ROOT)
        self.base = Path(self.temporary.name)
        self.run_dir = self.base / "run"
        self.run_dir.mkdir()
        self.inputs = {}
        for key, filename in (("source", checker.SOURCE_NAME), ("testbench", checker.TESTBENCH_NAME),
                              ("runner", "run_duplex_fallback_tests.sh"), ("bambu", "bambu"),
                              ("settings", "settings.sh"), ("plugin", "ASTAnalyzer.so"),
                              ("simulator", "verilator")):
            path = self.base / filename
            path.write_text(f"fixture {key}\n", encoding="utf-8")
            self.inputs[key] = path
        self.inputs["checker"] = Path(checker.__file__).resolve()
        self.profiles = {}
        for profile, (macro, read_reason, write_reason) in PROFILE_REASONS.items():
            directory = self.run_dir / profile
            (directory / "HLS_output/simulation").mkdir(parents=True)
            (directory / "panda-temp").mkdir()
            (directory / "bambu_results.xml").write_text("<results/>\n", encoding="utf-8")
            read_accesses, write_accesses = {
                "conditional_read_or_write_same_bundle": (2, 3),
                "residual_store_same_bundle": (1, 2),
            }.get(profile, (1, 1))
            duplex_reason = ("original effects are not fully classified ordinary accesses"
                             if profile in {"conditional_read_or_write_same_bundle",
                                            "residual_store_same_bundle"} else "not eligible")
            (directory / "stdout.log").write_text(
                compiler_evidence(read_reason, write_reason, read_accesses, write_accesses,
                                  duplex_reason=duplex_reason), encoding="utf-8")
            (directory / "stderr.log").write_text("compiler stderr evidence\n", encoding="utf-8")
            (directory / "HLS_output/simulation/testbench.log").write_text(
                f"DUPLEX_FALLBACK_GOLDEN_PASS PROFILE={profile} N=38\n", encoding="utf-8")
            (directory / "panda-temp/architecture.xml").write_text(
                '<architecture><function name="bambu_axi_burst_duplex_fallback">'
                '<parameters><parameter port="data" bundle="shared"/></parameters>'
                '<bundles><bundle name="shared" mode="m_axi" max_read_burst_length="16" '
                'num_read_outstanding="1" read_fifo_depth="64" max_write_burst_length="16" '
                'write_burst_outstanding="1" write_fifo_depth="32"/></bundles></function></architecture>\n',
                encoding="utf-8")
            (directory / "bambu_axi_burst_duplex_fallback.v").write_text(
                "module top; MinimalAXI4AdapterSingleBeat legacy(); endmodule\n", encoding="utf-8")
            self.profiles[profile] = directory
            self.write_provenance(profile)
            self.write_artifact_manifest(profile)

    def tearDown(self):
        self.temporary.cleanup()

    def write_provenance(self, profile):
        directory = self.profiles[profile]
        macro = PROFILE_REASONS[profile][0]
        fields = {
            "git_head": "a" * 40,
            "profile": profile,
            "macro": macro,
            "feature": "1",
            "output": str(directory.resolve()),
            "argv": f"bambu --bambu-parameter=experimental-m-axi-burst=1 -DDUPLEX_FALLBACK_PROFILE={macro}",
        }
        for key, path in self.inputs.items():
            fields[key] = str(path.resolve())
            fields[f"{key}_sha256"] = digest(path)
        (directory / "provenance.txt").write_text(
            "".join(f"{key}={value}\n" for key, value in fields.items()), encoding="utf-8")

    def write_artifact_manifest(self, profile):
        directory = self.profiles[profile]
        artifacts = sorted(path for path in directory.rglob("*")
                           if path.is_file() and path.suffix.lower() in checker.ARTIFACT_SUFFIXES)
        (directory / "artifact-sha256.txt").write_text(
            "".join(f"{digest(path)}  {path.relative_to(directory)}\n" for path in artifacts),
            encoding="utf-8")

    def run_checker(self):
        report = getattr(self, "report_override", "report.json")
        return checker.main([
            str(self.run_dir), "--source", str(self.inputs["source"]),
            "--testbench", str(self.inputs["testbench"]), "--bambu", str(self.inputs["bambu"]),
            "--settings", str(self.inputs["settings"]), "--plugin", str(self.inputs["plugin"]),
            "--simulator", str(self.inputs["simulator"]), "--runner", str(self.inputs["runner"]),
            "--report", str(report),
        ])

    def rewrite_and_run(self, profile, relative_path, text, update_manifest=True):
        (self.profiles[profile] / relative_path).write_text(text, encoding="utf-8")
        if update_manifest:
            self.write_artifact_manifest(profile)
        return self.run_checker()

    def test_valid_four_profile_inventory_passes(self):
        self.assertEqual(self.run_checker(), 0)
        self.assertTrue((self.run_dir / "report.json").is_file())

    def test_runner_job_logs_directory_is_allowed(self):
        (self.run_dir / "job_logs").mkdir()
        self.assertEqual(self.run_checker(), 0)

    def test_missing_profile_fails(self):
        import shutil
        shutil.rmtree(self.profiles["overlap_rw_same_bundle"])
        self.assertEqual(self.run_checker(), 1)

    def test_missing_new_profile_fails_inventory(self):
        import shutil
        shutil.rmtree(self.profiles["conditional_read_or_write_same_bundle"])
        self.assertEqual(self.run_checker(), 1)

    def test_new_profile_macro_mismatch_fails(self):
        profile = "conditional_read_or_write_same_bundle"
        path = self.profiles[profile] / "provenance.txt"
        text = path.read_text(encoding="utf-8").replace("macro=3\n", "macro=4\n")
        text = text.replace("DUPLEX_FALLBACK_PROFILE=3", "DUPLEX_FALLBACK_PROFILE=4")
        path.write_text(text, encoding="utf-8")
        self.assertEqual(self.run_checker(), 1)

    def test_new_profile_wrong_fallback_reason_fails(self):
        profile = "conditional_read_or_write_same_bundle"
        self.assertEqual(self.run_checker(), 0)
        self.assertEqual(self.rewrite_and_run(profile, "stdout.log",
                                              valid_profile_evidence(profile,
                                                                     read_reason="wrong read reason")), 1)

    def test_new_profile_missing_directional_request_fails(self):
        profile = "residual_store_same_bundle"
        self.assertEqual(self.run_checker(), 0)
        self.assertEqual(self.rewrite_and_run(
            profile, "stdout.log", valid_profile_evidence(profile, write_requested=0)), 1)

    def test_new_profile_access_inventory_must_match_trace(self):
        profile = "conditional_read_or_write_same_bundle"
        self.assertEqual(self.rewrite_and_run(
            profile, "stdout.log", valid_profile_evidence(profile, read_accesses=1)), 1)

    def test_new_profile_candidate_or_activation_fails_closed(self):
        profile = "conditional_read_or_write_same_bundle"
        self.assertEqual(self.run_checker(), 0)
        self.assertEqual(self.rewrite_and_run(
            profile, "stdout.log", valid_profile_evidence(profile, candidate=1)), 1)
        self.assertEqual(self.rewrite_and_run(
            profile, "stdout.log", valid_profile_evidence(profile, activated=1)), 1)

    def test_new_profile_golden_marker_must_be_exact(self):
        profile = "residual_store_same_bundle"
        self.assertEqual(self.rewrite_and_run(profile, "HLS_output/simulation/testbench.log", ""), 1)
        self.assertEqual(self.rewrite_and_run(
            profile, "HLS_output/simulation/testbench.log",
            f"DUPLEX_FALLBACK_GOLDEN_PASS PROFILE={profile} N=37\n"), 1)

    def test_new_profile_residual_burst_resource_fails(self):
        profile = "residual_store_same_bundle"
        self.assertEqual(self.rewrite_and_run(
            profile, "bambu_axi_burst_duplex_fallback.v",
            "module top; MinimalAXI4AdapterSingleBeat legacy(); "
            "MinimalAXI4MasterPipelined engine(); endmodule\n"), 1)

    def test_new_profile_final_activation_marker_fails(self):
        profile = "residual_store_same_bundle"
        directory = self.profiles[profile]
        path = directory / "stderr.log"
        path.write_text(path.read_text(encoding="utf-8") +
                        "---Activated same-base pointwise duplex m_axi bundle shared for root kernel\n",
                        encoding="utf-8")
        self.write_artifact_manifest(profile)
        self.assertEqual(self.run_checker(), 1)

    def test_realistic_final_activation_markers_are_detected(self):
        activations = (
            "    Activated same-base pointwise duplex m_axi bundle shared for root kernel\n",
            "    Activated distinct-base restricted pointwise duplex m_axi bundle shared for root kernel\n",
            "---Activated same-base pointwise duplex m_axi bundle shared for root kernel\n",
            "---Activated distinct-base restricted pointwise duplex m_axi bundle shared for root kernel\n",
        )
        for activation in activations:
            with self.subTest(activation=activation.strip()):
                self.assertIsNotNone(checker.DUPLEX_ACTIVATION.search(activation))

    def test_unexpected_run_directory_fails(self):
        (self.run_dir / "unexpected").mkdir()
        self.assertEqual(self.run_checker(), 1)

    def test_empty_required_evidence_fails(self):
        self.assertEqual(self.rewrite_and_run("phased_erew_reverse_same_bundle", "stdout.log", ""), 1)

    def test_wrong_or_missing_marker_fails(self):
        profile = "phased_erew_reverse_same_bundle"
        self.assertEqual(self.rewrite_and_run(profile, "HLS_output/simulation/testbench.log",
                                              "DUPLEX_FALLBACK_GOLDEN_PASS PROFILE=wrong N=38\n"), 1)

    def test_duplicate_marker_fails(self):
        profile = "phased_erew_reverse_same_bundle"
        marker = f"DUPLEX_FALLBACK_GOLDEN_PASS PROFILE={profile} N=38\n"
        self.assertEqual(self.rewrite_and_run(profile, "HLS_output/simulation/testbench.log", marker * 2), 1)

    def test_missing_legacy_resource_fails(self):
        profile = "phased_erew_reverse_same_bundle"
        self.assertEqual(self.rewrite_and_run(profile, "bambu_axi_burst_duplex_fallback.v",
                                              "module top; endmodule\n"), 1)

    def test_unrelated_rtl_cannot_replace_exact_generated_top(self):
        profile = "phased_erew_reverse_same_bundle"
        directory = self.profiles[profile]
        (directory / "bambu_axi_burst_duplex_fallback.v").unlink()
        (directory / "unrelated.v").write_text(
            "module other; MinimalAXI4AdapterSingleBeat legacy(); endmodule\n", encoding="utf-8")
        self.write_artifact_manifest(profile)
        self.assertEqual(self.run_checker(), 1)

    def test_legacy_comment_or_module_definition_is_not_an_instance(self):
        profile = "phased_erew_reverse_same_bundle"
        rtl = ("/* MinimalAXI4AdapterSingleBeat commented(); */\n"
               "module MinimalAXI4AdapterSingleBeat(input wire a); endmodule\n"
               "module bambu_axi_burst_duplex_fallback; endmodule\n")
        self.assertEqual(self.rewrite_and_run(profile, "bambu_axi_burst_duplex_fallback.v", rtl), 1)

    def test_residual_burst_resource_and_abi_fail(self):
        profile = "phased_erew_reverse_same_bundle"
        self.assertEqual(self.rewrite_and_run(profile, "bambu_axi_burst_duplex_fallback.v",
                                              "module top; MinimalAXI4AdapterSingleBeat legacy(); "
                                              "MinimalAXI4MasterPipelined engine(); endmodule\n"), 1)
        self.assertEqual(self.rewrite_and_run(profile, "bambu_axi_burst_duplex_fallback.v",
                                              "module top; MinimalAXI4AdapterSingleBeat legacy(); "
                                              "wire x = fuselector_configure_read; endmodule\n"), 1)

    def test_wrong_or_missing_fallback_reason_fails(self):
        profile = "phased_erew_reverse_same_bundle"
        self.assertEqual(self.run_checker(), 0)
        self.assertEqual(self.rewrite_and_run(profile, "stdout.log",
                                              valid_profile_evidence(profile,
                                                                     read_reason="wrong reason")), 1)
        self.assertEqual(self.rewrite_and_run(profile, "stdout.log",
                                              valid_profile_evidence(profile)), 0)
        self.assertEqual(self.rewrite_and_run(profile, "stdout.log",
                                              valid_profile_evidence(profile,
                                                                     write_reason="wrong reason")), 1)
        self.assertEqual(self.rewrite_and_run(profile, "stdout.log", "compiler completed\n"), 1)

    def test_unanchored_echo_does_not_satisfy_fallback(self):
        profile = "phased_erew_reverse_same_bundle"
        echo = f"pragma fallback={PROFILE_REASONS[profile][1]}\n"
        self.assertEqual(self.rewrite_and_run(profile, "stdout.log", echo), 1)

    def test_artifact_digest_mismatch_fails(self):
        profile = "phased_erew_reverse_same_bundle"
        self.assertEqual(self.rewrite_and_run(profile, "stdout.log", "tampered\n", update_manifest=False), 1)

    def test_provenance_digest_mismatch_fails(self):
        path = self.inputs["source"]
        path.write_text("changed source\n", encoding="utf-8")
        self.assertEqual(self.run_checker(), 1)

    def test_provenance_identity_mismatch_fails(self):
        profile = "phased_erew_reverse_same_bundle"
        path = self.profiles[profile] / "provenance.txt"
        path.write_text(path.read_text(encoding="utf-8").replace("feature=1", "feature=0"), encoding="utf-8")
        self.assertEqual(self.run_checker(), 1)

    def test_manifest_rejects_out_of_profile_path(self):
        profile = "phased_erew_reverse_same_bundle"
        outside = self.base / "outside.log"
        outside.write_text("outside\n", encoding="utf-8")
        path = self.profiles[profile] / "artifact-sha256.txt"
        lines = path.read_text(encoding="utf-8").splitlines()
        lines.append(f"{digest(outside)}  {outside}")
        path.write_text("\n".join(lines) + "\n", encoding="utf-8")
        self.assertEqual(self.run_checker(), 1)

    def test_report_path_must_stay_inside_run_directory(self):
        self.report_override = self.base / "outside-report.json"
        self.assertEqual(self.run_checker(), 1)

    def test_symlinked_testbench_log_ancestor_fails(self):
        import shutil
        profile = "phased_erew_reverse_same_bundle"
        directory = self.profiles[profile]
        real_hls_output = self.base / "outside-HLS_output"
        shutil.move(directory / "HLS_output", real_hls_output)
        (directory / "HLS_output").symlink_to(real_hls_output, target_is_directory=True)
        self.write_artifact_manifest(profile)
        self.assertEqual(self.run_checker(), 1)

    def test_symlinked_architecture_file_fails(self):
        import shutil
        profile = "phased_erew_reverse_same_bundle"
        directory = self.profiles[profile]
        outside = self.base / "outside-panda-temp"
        shutil.move(directory / "panda-temp", outside)
        (directory / "panda-temp").symlink_to(outside, target_is_directory=True)
        self.write_artifact_manifest(profile)
        self.assertEqual(self.run_checker(), 1)


if __name__ == "__main__":
    unittest.main()
