#!/usr/bin/env python3
"""Offline mutation tests for the generated independent-RW duplex checker."""
from __future__ import annotations

import hashlib
import json
import os
import sys
import tempfile
import unittest
from pathlib import Path

REPO = Path(__file__).resolve().parents[3]
TMP_ROOT = Path(os.environ.get("BURST_TEST_TMPDIR", Path.cwd())) / f"out_{Path(__file__).stem}"
TMP_ROOT.mkdir(parents=True, exist_ok=True)
sys.path.insert(0, str(Path(__file__).resolve().parent))
import check_duplex_independent_results as checker  # noqa: E402
import test_duplex_effect_analysis_artifacts as shared_fixtures  # noqa: E402


class DuplexIndependentArtifactTests(unittest.TestCase):
    def setUp(self) -> None:
        self.temp = tempfile.TemporaryDirectory(dir=TMP_ROOT)
        self.root = Path(self.temp.name)
        self.run = self.root / "run"
        self.run.mkdir()
        self.inputs = self.root / "inputs"
        self.inputs.mkdir()
        self.paths = {}
        for name in ("bambu", "settings", "plugin", "runner", "parallel_helper", "shared_runner"):
            filename = {"parallel_helper": "parallel_helpers.sh",
                        "shared_runner": "run_duplex_generated_family.sh"}.get(name, name)
            path = self.inputs / filename
            path.write_text(f"{name} fixture\n", encoding="utf-8")
            if name == "bambu":
                path.chmod(0o755)
            self.paths[name] = path
        simulator = self.inputs / "vco"
        simulator.write_text("#!/bin/sh\nprintf '%s\\n' 'Questa Sim fixture version'\n", encoding="utf-8")
        simulator.chmod(0o755)
        self.paths["simulator"] = simulator
        self.paths["simulator_invocation"] = self.inputs / "vsim"
        self.paths["simulator_invocation"].symlink_to(simulator.name)
        self.paths["runner"] = self.inputs / "runner"
        (self.inputs / "parallel_helpers.sh").write_text("parallel helper fixture\n", encoding="utf-8")
        source = self.inputs / checker.SOURCE_NAME
        testbench = self.inputs / checker.TESTBENCH_NAME
        source.write_bytes((Path(__file__).resolve().parent / checker.SOURCE_NAME).read_bytes())
        testbench.write_bytes((Path(__file__).resolve().parent / checker.TESTBENCH_NAME).read_bytes())
        self.paths["source"] = source
        self.paths["testbench"] = testbench
        self.args = type("Args", (), {
            "source": source, "testbench": testbench,
            "bambu": self.paths["bambu"], "settings": self.paths["settings"],
            "plugin": self.paths["plugin"], "simulator": self.paths["simulator"],
            "simulator_invocation": self.paths["simulator_invocation"],
            "runner": self.paths["runner"], "developer_proof": False,
        })()
        self.profile("symmetric")
        self.profile("asymmetric")

    def tearDown(self) -> None:
        self.temp.cleanup()

    def profile(self, name: str) -> Path:
        profile_dir = self.run / name
        profile_dir.mkdir(exist_ok=True)
        (profile_dir / "tmp").mkdir(exist_ok=True)
        data = checker.PROFILE_DATA[name]
        read_burst, read_outstanding, read_fifo = data["read"]
        write_burst, write_outstanding, write_fifo = data["write"]
        rtl = shared_fixtures.DuplexEffectArtifactTests.activated_rtl(
            checker.TOP, address_bits=64, read_burst=int(read_burst), read_outstanding=int(read_outstanding),
            write_burst=int(write_burst), write_outstanding=int(write_outstanding))
        read_prefix = "MinimalAXI4MasterPipelined #(.B_MAX"
        write_prefix = "MinimalAXI4MasterPipelinedWrite #(.B_MAX"
        read_start = rtl.index(read_prefix)
        read_end = rtl.index(") read_core", read_start)
        read_instance = rtl[read_start:read_end]
        rtl = rtl[:read_start] + read_instance.replace(".FIFO_DEPTH(32)", f".FIFO_DEPTH({read_fifo})") + rtl[read_end:]
        write_start = rtl.index(write_prefix)
        write_end = rtl.index(") write_core", write_start)
        write_instance = rtl[write_start:write_end]
        rtl = rtl[:write_start] + write_instance.replace(".FIFO_DEPTH(16)", f".FIFO_DEPTH({write_fifo})") + rtl[write_end:]
        self.write(profile_dir, f"{checker.TOP}.v", rtl)
        self.write(profile_dir, "bambu_results.xml", "<results/>\n")
        self.write(profile_dir, "stdout.log", "compiler stdout\n")
        traces = []
        for owner, reads, writes, read_recognized, write_recognized in (
                (429241, 1, 0, 1, 0), (429242, 0, 1, 0, 1)):
            traces.append(
                f'AXI_BURST_PREANALYSIS bundle=shared owner_argument_index={owner} '
                f'read_accesses={reads} write_accesses={writes} read_requested=1 '
                f'read_recognized={read_recognized} read_fallback="-" write_requested=1 '
                f'write_recognized={write_recognized} write_fallback="-" '
                f'same_base_duplex_candidate=1 same_base_duplex_reason="{checker.REASON}" '
                'production_activated=1 activation_reason="frozen application-wide proof and profile accepted"')
        stderr = (f"    {checker.ACTIVATION}\n{checker.WARNING}\n" +
                  "\n".join(traces) +
                  "\nAXI_BURST_DUPLEX_PROOF " + json.dumps(shared_fixtures.RECORD_DISTINCT,
                                                         separators=(",", ":")) + "\n")
        self.write(profile_dir, "stderr.log", stderr)
        self.write(profile_dir, "HLS_output/simulation/testbench.log",
                   f"DUPLEX_INDEPENDENT_GOLDEN_PASS PROFILE={name} N=38\n")
        self.write(profile_dir, "panda-temp/before_Frontend::InterfaceInfer.raw",
                   shared_fixtures.RAW_DISTINCT.replace('"distinct_bases_source_first"', f'"{checker.TOP}"')
                   .replace("@46 assign_stmt parent: @1 bb_index: 1 op: @45 op: @44",
                            "@46 assign_stmt parent: @1 bb_index: 1 op: @45 op: @35"))
        architecture = (
            f'<root><functions><function name="{checker.TOP}"><parameters>'
            '<parameter port="src" index="0" bundle="shared" source_restrict="1" />'
            '<parameter port="dst" index="1" bundle="shared" source_restrict="1" />'
            '</parameters><bundles>'
            f'<bundle name="shared" mode="m_axi" max_read_burst_length="{read_burst}" '
            f'num_read_outstanding="{read_outstanding}" read_fifo_depth="{read_fifo}" '
            f'max_write_burst_length="{write_burst}" write_burst_outstanding="{write_outstanding}" '
            f'write_fifo_depth="{write_fifo}" />'
            '</bundles></function></functions></root>\n')
        self.write(profile_dir, "panda-temp/architecture.xml", architecture)
        self.provenance(profile_dir, name)
        self.manifest(profile_dir)
        return profile_dir

    @staticmethod
    def write(root: Path, relative: str, value: str) -> Path:
        path = root / relative
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(value, encoding="utf-8")
        return path

    @staticmethod
    def digest(path: Path) -> str:
        return hashlib.sha256(path.read_bytes()).hexdigest()

    def provenance(self, profile_dir: Path, profile: str) -> None:
        inputs = {**self.paths, "checker": Path(checker.__file__).resolve()}
        rows = [f"profile={profile}", f"macro={checker.PROFILE_DATA[profile]['macro']}",
                "feature=1", f"output={profile_dir.resolve()}",
                f"read_profile={'/'.join(checker.PROFILE_DATA[profile]['read'])}",
                f"write_profile={'/'.join(checker.PROFILE_DATA[profile]['write'])}",
                f"proof_mode={'developer' if self.args.developer_proof else 'functional'}",
                f"cwd={profile_dir.resolve()}", f"TMPDIR={(profile_dir / 'tmp').resolve()}",
                "git_head=0123456789abcdef",
                f"simulator_invocation={self.paths['simulator_invocation']}",
                "simulator_version=Questa Sim fixture version",
                f"argv=bambu --top-fname={checker.TOP} --simulator=MODELSIM "
                "--bambu-parameter=m-axi-burst=1 "
                f"-DDUPLEX_INDEPENDENT_PROFILE={checker.PROFILE_DATA[profile]['macro']}" +
                (" --debug-classes=InterfaceInfer,BuildVirtualPhi,parametric_list_based"
                 if self.args.developer_proof else "")]
        for name in checker.INPUTS:
            path = inputs[name].resolve()
            rows.extend((f"{name}={path}", f"{name}_sha256={self.digest(path)}"))
        self.write(profile_dir, "provenance.txt", "\n".join(rows) + "\n")

    def manifest(self, profile_dir: Path) -> None:
        files = sorted(path for path in profile_dir.rglob("*") if path.is_file() and
                       path.name not in {"artifact-sha256.txt", "provenance.txt"} and
                       (path.suffix.lower() in checker.ARTIFACT_SUFFIXES or path.suffix == ".raw"))
        lines = [f"{self.digest(path)}  {path.relative_to(profile_dir)}" for path in files]
        self.write(profile_dir, "artifact-sha256.txt", "\n".join(lines) + "\n")

    def check(self) -> None:
        checker.check_profile(self.run / "symmetric", "symmetric", self.args)
        checker.check_profile(self.run / "asymmetric", "asymmetric", self.args)

    def refresh(self, profile: str) -> None:
        profile_dir = self.run / profile
        self.provenance(profile_dir, profile)
        self.manifest(profile_dir)

    def test_valid_symmetric_and_asymmetric_profiles_pass(self) -> None:
        self.check()

    def test_functional_mode_does_not_require_debug_proof_or_raw_ir(self) -> None:
        for profile in ("symmetric", "asymmetric"):
            profile_dir = self.run / profile
            raw = profile_dir / "panda-temp/before_Frontend::InterfaceInfer.raw"
            raw.unlink()
            stderr = profile_dir / "stderr.log"
            stderr.write_text("\n".join(
                line for line in stderr.read_text(encoding="utf-8").splitlines()
                if "AXI_BURST_PREANALYSIS" not in line and "AXI_BURST_DUPLEX_PROOF" not in line
            ) + "\n", encoding="utf-8")
            self.manifest(profile_dir)
        self.check()

    def test_developer_proof_mode_preserves_raw_and_preanalysis_requirements(self) -> None:
        self.args.developer_proof = True
        for profile in ("symmetric", "asymmetric"):
            self.refresh(profile)
        self.check()
        raw = self.run / "symmetric/panda-temp/before_Frontend::InterfaceInfer.raw"
        raw.unlink()
        self.manifest(self.run / "symmetric")
        with self.assertRaisesRegex(ValueError, "required non-empty panda-temp/before_Frontend::InterfaceInfer.raw"):
            checker.check_profile(self.run / "symmetric", "symmetric", self.args)

    def test_simulator_invocation_must_resolve_to_selected_simulator(self) -> None:
        self.args.simulator_invocation = self.paths["bambu"]
        with self.assertRaisesRegex(ValueError, "does not resolve to the selected simulator"):
            checker.check_profile(self.run / "symmetric", "symmetric", self.args)

    def test_simulator_version_must_match_selected_invocation(self) -> None:
        profile = self.run / "symmetric"
        provenance = profile / "provenance.txt"
        contents = provenance.read_text(encoding="utf-8")
        provenance.write_text(contents.replace(
            "simulator_version=Questa Sim fixture version",
            "simulator_version=untrusted simulator version"), encoding="utf-8")
        with self.assertRaisesRegex(ValueError, "provenance simulator_version does not match"):
            checker.check_profile(profile, "symmetric", self.args)

    def test_run_inventory_accepts_runner_logs_but_rejects_unexpected_directories(self) -> None:
        (self.run / "job_logs").mkdir()
        checker.check_run_inventory(self.run)
        (self.run / "unexpected").mkdir()
        with self.assertRaisesRegex(ValueError, "run directory inventory mismatch"):
            checker.check_run_inventory(self.run)

    def test_legacy_fallback_diagnostics_are_rejected(self) -> None:
        path = self.run / "symmetric/stderr.log"
        original = path.read_text()
        self.assertNotIn("Keeping legacy m_axi", original)
        checker.check_profile(self.run / "symmetric", "symmetric", self.args)
        path.write_text(original +
                        "Keeping legacy m_axi writes for bundle shared: write-burst CFG/SSA/effect recognition failed\n")
        self.refresh("symmetric")
        with self.assertRaisesRegex(ValueError, "unexpectedly fell back to legacy m_axi accesses"):
            checker.check_profile(self.run / "symmetric", "symmetric", self.args)
        path.write_text(original)
        self.refresh("symmetric")
        path.write_text(original.replace(f"    {checker.ACTIVATION}\n", f"---{checker.ACTIVATION}\n", 1))
        self.refresh("symmetric")
        checker.check_profile(self.run / "symmetric", "symmetric", self.args)
        path.write_text(original + f"{checker.ACTIVATION}\n")
        self.refresh("symmetric")
        with self.assertRaisesRegex(ValueError, "activation marker"):
            checker.check_profile(self.run / "symmetric", "symmetric", self.args)
        path.write_text(original.replace(f"    {checker.ACTIVATION}\n", "", 1))
        self.refresh("symmetric")
        with self.assertRaisesRegex(ValueError, "activation marker"):
            checker.check_profile(self.run / "symmetric", "symmetric", self.args)
        path.write_text(original)

    def test_legacy_or_missing_duplex_core_is_rejected(self) -> None:
        path = self.run / "symmetric/bambu_axi_burst_duplex_independent.v"
        original = path.read_text()
        for changed in (original + "MinimalAXI4AdapterSingleBeat legacy();\n",
                        original.replace("MinimalAXI4MasterPipelined #", "MissingReadCore #", 1)):
            path.write_text(changed)
            self.refresh("symmetric")
            with self.assertRaises(ValueError):
                checker.check_profile(self.run / "symmetric", "symmetric", self.args)
        path.write_text(original)

    def test_cross_instance_profile_mismatch_is_rejected(self) -> None:
        path = self.run / "asymmetric/bambu_axi_burst_duplex_independent.v"
        original = path.read_text()
        start = original.index("MinimalAXI4MasterPipelined #")
        end = original.index(") read_core", start)
        path.write_text(original[:end].replace(".FIFO_DEPTH(64)", ".FIFO_DEPTH(16)") + original[end:])
        self.refresh("asymmetric")
        with self.assertRaisesRegex(ValueError, "FIFO_DEPTH|fifo_depth"):
            checker.check_profile(self.run / "asymmetric", "asymmetric", self.args)

    def test_peer_fault_disconnection_is_rejected(self) -> None:
        path = self.run / "symmetric/bambu_axi_burst_duplex_independent.v"
        original = path.read_text()
        path.write_text(original.replace("wire read_peer_fault = write_core_fault;",
                                         "wire read_peer_fault = 1'b0;", 1))
        self.refresh("symmetric")
        with self.assertRaisesRegex(ValueError, "cross-connected"):
            checker.check_profile(self.run / "symmetric", "symmetric", self.args)

    def test_golden_marker_and_log_are_required(self) -> None:
        log = self.run / "asymmetric/HLS_output/simulation/testbench.log"
        original = log.read_text()
        log.write_text("simulation completed without marker\n")
        self.refresh("asymmetric")
        with self.assertRaisesRegex(ValueError, "golden marker"):
            checker.check_profile(self.run / "asymmetric", "asymmetric", self.args)
        log.unlink()
        self.manifest(self.run / "asymmetric")
        with self.assertRaisesRegex(ValueError, "testbench.log"):
            checker.check_profile(self.run / "asymmetric", "asymmetric", self.args)
        log.write_text(original)

    def test_lost_restrict_metadata_is_rejected(self) -> None:
        path = self.run / "symmetric/panda-temp/architecture.xml"
        original = path.read_text()
        path.write_text(original.replace('port="dst" index="1" bundle="shared" source_restrict="1"',
                                         'port="dst" index="1" bundle="shared" source_restrict="0"'))
        self.refresh("symmetric")
        with self.assertRaisesRegex(ValueError, "restricted shared-bundle metadata for dst"):
            checker.check_profile(self.run / "symmetric", "symmetric", self.args)

    def test_manifest_provenance_path_and_symlink_fail_closed(self) -> None:
        profile = self.run / "symmetric"
        manifest = profile / "artifact-sha256.txt"
        original_manifest = manifest.read_text()
        manifest.write_text(original_manifest.replace("bambu_axi_burst_duplex_independent.v", "../escape.v"))
        with self.assertRaises(ValueError):
            checker.check_profile(profile, "symmetric", self.args)
        first_line, *remaining = original_manifest.splitlines()
        manifest.write_text("0" * 64 + first_line[64:] + "\n" + "\n".join(remaining) + "\n")
        with self.assertRaisesRegex(ValueError, "digest mismatch"):
            checker.check_profile(profile, "symmetric", self.args)
        manifest.write_text(original_manifest)
        provenance = profile / "provenance.txt"
        original_provenance = provenance.read_text()
        provenance.write_text(original_provenance.replace("feature=1", "feature=0"))
        with self.assertRaisesRegex(ValueError, "provenance feature"):
            checker.check_profile(profile, "symmetric", self.args)
        provenance.write_text(original_provenance.replace(
            "source=" + str(self.paths["source"].resolve()), "source=/outside/fixture.cpp"))
        with self.assertRaisesRegex(ValueError, "provenance source"):
            checker.check_profile(profile, "symmetric", self.args)
        provenance.write_text(original_provenance)
        raw = profile / "panda-temp/before_Frontend::InterfaceInfer.raw"
        contents = raw.read_text()
        raw.unlink()
        raw.symlink_to(self.root / "outside.raw")
        (self.root / "outside.raw").write_text(contents)
        with self.assertRaisesRegex(ValueError, "symlink"):
            checker.check_profile(profile, "symmetric", self.args)

    def test_shared_runner_digest_and_symlink_fail_closed(self) -> None:
        profile = self.run / "symmetric"
        path = self.paths["shared_runner"]
        original = path.read_text(encoding="utf-8")
        path.write_text(original + "# changed\n", encoding="utf-8")
        with self.assertRaisesRegex(ValueError, "shared_runner_sha256"):
            checker.check_profile(profile, "symmetric", self.args)
        path.unlink()
        outside = self.inputs / "outside-runner.sh"
        outside.write_text(original, encoding="utf-8")
        path.symlink_to(outside)
        with self.assertRaisesRegex(ValueError, "shared_runner is a symlink"):
            checker.check_profile(profile, "symmetric", self.args)

    def test_manifest_allows_internal_directory_symlink_but_rejects_artifact_and_escape(self) -> None:
        profile = self.run / "symmetric"
        backend = profile / "HLS_output/simulation/modelsim_backend/work"
        backend.mkdir(parents=True)
        (backend / "HLS_output").symlink_to(profile / "HLS_output", target_is_directory=True)
        checker.check_profile(profile, "symmetric", self.args)

        artifact_link = profile / "HLS_output/simulation/linked.log"
        artifact_link.symlink_to(profile / "HLS_output/simulation/testbench.log")
        with self.assertRaisesRegex(ValueError, "artifact symlink"):
            checker.check_profile(profile, "symmetric", self.args)
        artifact_link.unlink()

        outside = self.root / "outside"
        outside.mkdir()
        escaping_link = profile / "HLS_output/simulation/outside"
        escaping_link.symlink_to(outside, target_is_directory=True)
        with self.assertRaisesRegex(ValueError, "escapes profile"):
            checker.check_profile(profile, "symmetric", self.args)


if __name__ == "__main__":
    unittest.main()
