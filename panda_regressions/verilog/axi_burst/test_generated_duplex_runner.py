from __future__ import annotations

import os
import hashlib
import shutil
import subprocess
import unittest
import uuid
from pathlib import Path


ROOT = Path(__file__).resolve().parents[3]
RUNNER = ROOT / "panda_regressions/verilog/axi_burst/rtl_tests/run_generated_duplex_tests.sh"
SCRATCH_ROOT = Path(os.environ.get("BURST_TEST_TMPDIR", Path.cwd())) / f"out_{Path(__file__).stem}"
SCENARIOS = (
    "LOAD_WAIT_STORE", "STORE_WAIT_LOAD", "WRITE_STALL_READ_PROGRESS", "READ_STALL_WRITE_PROGRESS",
    "SIMULTANEOUS", "CONFIG_ISOLATION", "ZERO_READ", "ZERO_WRITE", "UNPAIRED_DESCRIPTORS",
    "INPLACE_POINTWISE", "BRESP_PEER_DRAIN", "RRESP_PEER_DRAIN", "FAULT_RESET_RESTART",
    "RESET_STALLED_CHANNELS",
)
PROFILES = ("same_base_pointwise", "same_base_asymmetric_64")
RESET_VARIANTS = ("sync-high", "sync-low", "async-high", "async-low")


class GeneratedDuplexRunnerTests(unittest.TestCase):
    def setUp(self):
        SCRATCH_ROOT.mkdir(parents=True, exist_ok=True)
        self.scratch = SCRATCH_ROOT / uuid.uuid4().hex
        self.scratch.mkdir()
        self.addCleanup(shutil.rmtree, self.scratch, ignore_errors=True)
        self.repo = self.scratch / "repo"
        self.family = self.repo / "panda_regressions/hls/bambu_axi_burst"
        self.rtl_tests = self.repo / "panda_regressions/verilog/axi_burst/rtl_tests"
        self.rtl_tests.mkdir(parents=True)
        self.family.mkdir(parents=True, exist_ok=True)
        shutil.copy2(RUNNER, self.rtl_tests / RUNNER.name)
        shutil.copy2(ROOT / "panda_regressions/hls/bambu_axi_burst/parallel_helpers.sh",
                     self.family / "parallel_helpers.sh")
        shutil.copy2(ROOT / "panda_regressions/verilog/axi_burst/rtl_tests/modelsim_helpers.sh",
                     self.rtl_tests / "modelsim_helpers.sh")
        lookup = self.repo / "etc/libtech/backend/simulation/modelsim/tool_lookup.sh"
        lookup.parent.mkdir(parents=True)
        shutil.copy2(ROOT / "etc/libtech/backend/simulation/modelsim/tool_lookup.sh", lookup)
        for path in (
            self.rtl_tests / "tb_generated_duplex.sv",
            self.family / "compiler_cases/duplex_effect_family_cases.cpp",
        ):
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text("# stub\n")
        checker = self.family / "check_duplex_effect_analysis_artifacts.py"
        checker.write_text("""from __future__ import annotations
import argparse, hashlib
from pathlib import Path
p = argparse.ArgumentParser()
p.add_argument('top'); p.add_argument('--output', required=True)
p.add_argument('--source', required=True); p.add_argument('--bambu', required=True)
p.add_argument('--plugin', required=True); p.add_argument('--checker', required=True)
p.add_argument('--runner', required=True)
p.add_argument('--functional-only', action='store_true')
a = p.parse_args(); out = Path(a.output); fields = {}
for line in (out / 'provenance.txt').read_text().splitlines():
    key, value = line.split('=', 1); fields[key] = value
required = {'top','source','source_sha256','bambu','bambu_sha256','plugin','plugin_sha256',
            'checker','checker_sha256','runner','runner_sha256','rtl','rtl_sha256',
            'architecture','architecture_sha256','stdout','stdout_sha256','stderr','stderr_sha256'}
assert fields.keys() == required, fields.keys()
assert a.functional_only
assert fields['top'] == a.top
for key, path in [('source', a.source), ('bambu', a.bambu), ('plugin', a.plugin),
                  ('checker', a.checker), ('runner', a.runner)]:
    assert Path(fields[key]).resolve() == Path(path).resolve()
    assert fields[key + '_sha256'] == hashlib.sha256(Path(path).read_bytes()).hexdigest()
for stem in ('rtl', 'architecture', 'stdout', 'stderr'):
    artifact = Path(fields[stem])
    assert artifact.is_file() and artifact.resolve().is_relative_to(out.resolve())
    assert fields[f'{stem}_sha256'] == hashlib.sha256(artifact.read_bytes()).hexdigest()
""")
        self.bin = self.scratch / "bin"
        self.bin.mkdir()
        self.prefix = self.scratch / "install"
        bambu = self.prefix / "bin/bambu"
        bambu.parent.mkdir(parents=True)
        bambu.write_text(self._bambu_stub())
        bambu.chmod(0o755)
        (self.prefix / "settings.sh").write_text(
            'export BAMBU_HLS="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"\n'
        )
        plugin = self.prefix / "lib/panda/clang-16/ASTAnalyzer.so"
        plugin.parent.mkdir(parents=True)
        plugin.write_text("plugin stub\n")
        for tool in ("vsim", "vlib", "vmap", "vlog"):
            self._stub(tool, self._modelsim_stub(tool))
        self.env = os.environ.copy()
        self.env.update({
            "PATH": f"{self.bin}:{self.env['PATH']}",
            "BURST_DUPLEX_SIM_OUTSIDE_SANDBOX": "1",
            "TMPDIR": str(self.scratch), "TMP": str(self.scratch), "TEMP": str(self.scratch),
            "SIM_JOBS": "2",
            "SIM_MODE": "pass", "SIM_ACTIVE_FILE": str(self.scratch / "active"),
            "SIM_MAX_FILE": str(self.scratch / "max_active"),
            "SIM_VLOG_CALLS": str(self.scratch / "vlog_calls"),
        })

    @staticmethod
    def _bambu_stub():
        return """#!/bin/bash
set -euo pipefail
if [[ ${1:-} == --version ]]; then echo 'Bambu stub 1.0'; exit 0; fi
profile=
for arg in "$@"; do
  if [[ $arg == --debug-classes* || $arg == --bambu-parameter=print-ir-manager=* || $arg == --print-dot ]]; then
    echo "Release stub rejects debug option: $arg" >&2
    exit 64
  fi
  [[ $arg == --top-fname=* ]] && profile=${arg#*=}
done
[[ -n $profile ]]
printf 'module %s(); endmodule\\n' "$profile" >"$profile.v"
printf 'module generated_library(); endmodule\\n' >panda_libtech.v
if [[ $profile == same_base_pointwise ]]; then
  read_burst=16; read_outstanding=1
else
  read_burst=8; read_outstanding=2
fi
mkdir -p panda-temp
printf '<root><function name="%s"><bundles><bundle name="shared" mode="m_axi" max_read_burst_length="%s" num_read_outstanding="%s" max_write_burst_length="16" write_burst_outstanding="1" read_fifo_depth="32" write_fifo_depth="16"/></bundles></function></root>\\n' \\
  "$profile" "$read_burst" "$read_outstanding" >panda-temp/architecture.xml
"""

    @staticmethod
    def _modelsim_stub(tool):
        if tool != "vsim":
            if tool == "vlog":
                return "#!/bin/bash\nprintf '%s\\n' \"$PWD\" >>\"$SIM_VLOG_CALLS\"\nexit 0\n"
            return "#!/bin/bash\nexit 0\n"
        return """#!/bin/bash
set -euo pipefail
if [[ ${1:-} == -version ]]; then echo 'ModelSim stub 1.0'; exit 0; fi
scenario= profile=
reset_active=0 reset_async=0
for arg in "$@"; do
  [[ $arg == +CASE=* ]] && scenario=${arg#*=}
  [[ $arg == +PROFILE=* ]] && profile=${arg#*=}
  [[ $arg == -gRESET_ACTIVE_LEVEL=* ]] && reset_active=${arg#*=}
  [[ $arg == -gRESET_ASYNC=* ]] && reset_async=${arg#*=}
done
mode=${SIM_MODE:-pass}
fault_expected=0
case $scenario in BRESP_PEER_DRAIN|RRESP_PEER_DRAIN|FAULT_RESET_RESTART) fault_expected=1 ;; esac
fault_arg=0
for arg in "$@"; do [[ $arg == +AXI_BURST_EXPECT_FAULT ]] && fault_arg=1; done
[[ $fault_expected == $fault_arg ]] || { echo "unexpected expected-fault plusarg selection" >&2; exit 41; }
if [[ $mode == nonzero && $scenario == ZERO_READ ]]; then exit 19; fi
if [[ $mode == wrong && $scenario == LOAD_WAIT_STORE ]]; then
  echo "GENERATED_DUPLEX PASS CASE=WRONG PROFILE=$profile"; exit 0
fi
if [[ $mode == missing && $scenario == STORE_WAIT_LOAD ]]; then exit 0; fi
if [[ $mode == omit && $profile == same_base_asymmetric_64 && $scenario == FAULT_RESET_RESTART ]]; then exit 0; fi
active_file=$SIM_ACTIVE_FILE
max_file=$SIM_MAX_FILE
exec 9>"${active_file}.lock"
flock 9
active=$(cat "$active_file" 2>/dev/null || echo 0)
active=$((active + 1)); echo "$active" >"$active_file"
max=$(cat "$max_file" 2>/dev/null || echo 0)
(( active <= max )) || echo "$active" >"$max_file"
flock -u 9
sleep 0.02
flock 9
active=$(cat "$active_file"); echo "$((active - 1))" >"$active_file"
flock -u 9
echo "# GENERATED_DUPLEX PASS CASE=$scenario PROFILE=$profile"
if [[ $scenario == RESET_STALLED_CHANNELS ]]; then
  for channel in AR R_RESPONSE_WAIT AW W B_RESPONSE_WAIT; do
    echo "# GENERATED_RESET_CHANNEL_PASS channel=$channel async=$reset_async active_level=$reset_active"
  done
  echo "# GENERATED_RESET_R_RESPONSE_WAIT descriptor_count=1 credits=31 RVALID=0 RREADY=1"
  echo "# GENERATED_RESET_B_RESPONSE_WAIT BVALID=0 BREADY=1"
  echo "# GENERATED_CACHE_RESET_PASS async=$reset_async active_level=$reset_active"
fi
case $scenario in
  BRESP_PEER_DRAIN)
    echo '# AXI_BURST_FAULT engine=write reason=BRESP instance=tb_generated_duplex.aggregator.write_engine.report_axi_write_diagnostic opcode=1 size=32 address=00000400 count=1 payload=00000000 resp=10 id=0 descriptors=1 remaining=1'
    [[ ${SIM_MODE:-pass} == missing-fault ]] || echo '# AXI_BURST_PEER_FAULT engine=read reason=peer instance=tb_generated_duplex.aggregator.read_engine.report_axi_read_diagnostic opcode=-1 size=0 address=00000000 count=0 resp=00 id=0 last=0 descriptors=0 remaining=0'
    [[ ${SIM_MODE:-pass} == missing-return-fault ]] || echo '# AXI_BURST_PEER_FAULT engine=write reason=peer instance=tb_generated_duplex.aggregator.write_engine.report_axi_write_diagnostic opcode=-1 size=0 address=00000000 count=0 payload=00000000 resp=00 id=0 descriptors=0 remaining=0' ;;
  RRESP_PEER_DRAIN)
    echo '# AXI_BURST_FAULT engine=read reason=RRESP instance=tb_generated_duplex.aggregator.read_engine.report_axi_read_diagnostic opcode=0 size=32 address=00000100 count=8 resp=10 id=0 last=0 descriptors=1 remaining=8'
    echo '# AXI_BURST_PEER_FAULT engine=write reason=peer instance=tb_generated_duplex.aggregator.write_engine.report_axi_write_diagnostic opcode=-1 size=0 address=00000000 count=0 payload=00000000 resp=00 id=0 descriptors=0 remaining=0'
    [[ ${SIM_MODE:-pass} == missing-return-fault ]] || echo '# AXI_BURST_PEER_FAULT engine=read reason=peer instance=tb_generated_duplex.aggregator.read_engine.report_axi_read_diagnostic opcode=-1 size=0 address=00000000 count=0 resp=00 id=0 last=0 descriptors=0 remaining=0' ;;
  FAULT_RESET_RESTART)
    echo '# AXI_BURST_FAULT engine=read reason=RRESP instance=tb_generated_duplex.aggregator.read_engine.report_axi_read_diagnostic opcode=0 size=32 address=00000100 count=8 resp=10 id=0 last=0 descriptors=1 remaining=8'
    if [[ ${SIM_MODE:-pass} == reset-peer ]]; then
      echo '# AXI_BURST_PEER_FAULT engine=write reason=peer instance=tb_generated_duplex.aggregator.write_engine.report_axi_write_diagnostic opcode=-1 size=0 address=00000000 count=0 payload=00000000 resp=00 id=0 descriptors=0 remaining=0'
    fi ;;
esac
if [[ ${SIM_MODE:-pass} == unexpected && $scenario == LOAD_WAIT_STORE ]]; then
  echo '# AXI_BURST_FAULT engine=read reason=RRESP instance=unexpected opcode=0 size=32 address=00000100 count=1 resp=10 id=0 last=0 descriptors=1 remaining=1'
fi
"""

    def _stub(self, name, content):
        path = self.bin / name
        path.write_text(content)
        path.chmod(0o755)

    def _run(self, mode="pass", output_name="fresh", outside=True, job_overrides=None):
        env = self.env.copy()
        env["SIM_MODE"] = mode
        env.update(job_overrides or {})
        if not outside:
            env.pop("BURST_DUPLEX_SIM_OUTSIDE_SANDBOX", None)
        output = self.scratch / output_name
        result = subprocess.run(
            ["/bin/bash", str(self.rtl_tests / RUNNER.name), str(self.prefix / "bin/bambu"), str(output)],
            cwd=self.repo, env=env, text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=False,
        )
        return result, output

    def test_requires_outside_sandbox_before_creating_output(self):
        result, output = self._run(outside=False)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("BURST_DUPLEX_SIM_OUTSIDE_SANDBOX=1", result.stderr)
        self.assertFalse(output.exists())

    def test_invalid_parallelism_fails_before_output_creation(self):
        result, output = self._run(output_name="invalid-jobs", job_overrides={"SIM_JOBS": "0"})
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("SIM_JOBS must be a positive integer", result.stderr)
        self.assertFalse(output.exists())

    def test_wrong_or_missing_case_marker_fails_even_on_zero_exit(self):
        for mode in ("wrong", "missing"):
            with self.subTest(mode=mode):
                result, _ = self._run(mode, f"{mode}-output")
                self.assertNotEqual(result.returncode, 0)
                self.assertIn("missing exact success marker", result.stderr)

    def test_nonzero_simulation_status_is_propagated(self):
        result, _ = self._run("nonzero", "nonzero-output")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("failed rc=19", result.stderr)

    def test_unexpected_normal_diagnostic_is_rejected(self):
        result, _ = self._run("unexpected", "unexpected-diagnostic")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("wrong AXI diagnostic count", result.stderr)

    def test_missing_expected_peer_record_is_rejected(self):
        for mode in ("missing-fault", "missing-return-fault"):
            with self.subTest(mode=mode):
                result, _ = self._run(mode, f"missing-peer-diagnostic-{mode}")
                self.assertNotEqual(result.returncode, 0)
                self.assertIn("wrong AXI diagnostic count", result.stderr)

    def test_immediate_reset_rejects_a_peer_record(self):
        result, _ = self._run("reset-peer", "reset-peer-diagnostic")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("wrong AXI diagnostic count", result.stderr)

    def test_scenario_omission_prevents_aggregate_success(self):
        result, output = self._run("omit", "omit-output")
        self.assertNotEqual(result.returncode, 0)
        self.assertNotIn("GENERATED_DUPLEX PASS: 14 scenarios x 2 profiles plus 4 reset variants", result.stdout)
        self.assertFalse((output / "aggregate.log").exists())

    def test_all_scenario_profile_markers_and_bounded_pool_are_required(self):
        result, output = self._run()
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("GENERATED_DUPLEX PASS: 14 scenarios x 2 profiles plus 4 reset variants", result.stdout)
        for profile in PROFILES:
            manifest = output / profile / "generated" / "provenance.txt"
            self.assertTrue(manifest.is_file())
            fields = dict(line.split("=", 1) for line in manifest.read_text().splitlines())
            for stem in ("rtl", "architecture", "stdout", "stderr"):
                artifact = Path(fields[stem])
                self.assertTrue(artifact.is_file())
                self.assertTrue(artifact.resolve().is_relative_to(manifest.parent.resolve()))
                self.assertEqual(fields[f"{stem}_sha256"],
                                 hashlib.sha256(artifact.read_bytes()).hexdigest())
            argv = (output / profile / "compiler_argv.txt").read_text()
            self.assertNotIn("--debug-classes", argv)
            self.assertNotIn("print-ir-manager", argv)
            self.assertNotIn("--print-dot", argv)
            if profile == "same_base_pointwise":
                self.assertIn("--extra-cc-options=-DAXI_DUPLEX_EFFECT_CASE_16", argv)
            else:
                self.assertIn("--extra-cc-options=-DAXI_DUPLEX_EFFECT_CASE_15", argv)
            for scenario in SCENARIOS:
                log = output / profile / "scenarios" / f"{scenario}.log"
                self.assertIn(f"GENERATED_DUPLEX PASS CASE={scenario} PROFILE={profile}", log.read_text())
        for variant in RESET_VARIANTS:
            reset_mode, reset_level = variant.split("-")
            profile_out = output / f"same_base_pointwise_reset_{variant}"
            argv = (profile_out / "compiler_argv.txt").read_text()
            self.assertIn(f"--reset-type={reset_mode}", argv)
            self.assertIn(f"--reset-level={reset_level}", argv)
            reset_log = (profile_out / "scenarios/RESET_STALLED_CHANNELS.log").read_text()
            active = int(reset_level == "high")
            asynchronous = int(reset_mode == "async")
            for channel in ("AR", "R_RESPONSE_WAIT", "AW", "W", "B_RESPONSE_WAIT"):
                self.assertIn(
                    f"GENERATED_RESET_CHANNEL_PASS channel={channel} async={asynchronous} active_level={active}",
                    reset_log,
                )
            self.assertIn("GENERATED_RESET_B_RESPONSE_WAIT BVALID=0 BREADY=1", reset_log)
            self.assertIn(
                "GENERATED_RESET_R_RESPONSE_WAIT descriptor_count=1 credits=31 RVALID=0 RREADY=1",
                reset_log,
            )
            self.assertIn(
                f"GENERATED_CACHE_RESET_PASS async={asynchronous} active_level={active}", reset_log
            )
        manifest = output / "scenario-sha256.txt"
        entries = manifest.read_text().splitlines()
        self.assertEqual(len(entries), len(PROFILES) * len(SCENARIOS) + len(RESET_VARIANTS))
        seen = set()
        for entry in entries:
            expected_digest, relative_path = entry.split("  ", 1)
            self.assertNotIn(relative_path, seen)
            seen.add(relative_path)
            artifact = output / relative_path
            self.assertTrue(artifact.is_file(), relative_path)
            self.assertEqual(hashlib.sha256(artifact.read_bytes()).hexdigest(), expected_digest,
                             relative_path)
        expected_paths = {
            f"{profile}/scenarios/{scenario}.log"
            for profile in PROFILES for scenario in SCENARIOS
        }
        expected_paths.update(
            f"same_base_pointwise_reset_{variant}/scenarios/RESET_STALLED_CHANNELS.log"
            for variant in RESET_VARIANTS
        )
        self.assertEqual(seen, expected_paths)
        maximum = int((self.scratch / "max_active").read_text())
        self.assertGreater(maximum, 1)
        self.assertLessEqual(maximum, 2)
        provenance = (output / "provenance.txt").read_text()
        self.assertIn("scenario_jobs=1", provenance)
        compile_dirs = Path(self.env["SIM_VLOG_CALLS"]).read_text().splitlines()
        self.assertEqual(len(compile_dirs), len(PROFILES) + len(RESET_VARIANTS))
        for profile in PROFILES:
            sim_provenance = (output / profile / "provenance.txt").read_text()
            self.assertIn("modelsim_elaboration_args=-gADDRESS_BITS", sim_provenance)
            self.assertNotIn("verilator", sim_provenance.lower())
            sim_dir = output / profile / "sim"
            self.assertEqual(sum(Path(path).resolve() == sim_dir.resolve() for path in compile_dirs), 1,
                             compile_dirs)
            command = (output / profile / "scenarios" / f"{SCENARIOS[0]}.raw.log.command").read_text()
            address_bits = 32 if profile == "same_base_pointwise" else 64
            read_max = 16 if address_bits == 32 else 8
            read_outstanding = 1 if address_bits == 32 else 2
            for parameter, value in (
                ("ADDRESS_BITS", address_bits), ("READ_B_MAX", read_max),
                ("READ_OUTSTANDING", read_outstanding), ("READ_FIFO_DEPTH", 32),
                ("WRITE_B_MAX", 16), ("WRITE_OUTSTANDING", 1), ("WRITE_FIFO_DEPTH", 16),
            ):
                self.assertIn(f"-g{parameter}={value}", command)
        for variant in RESET_VARIANTS:
            reset_mode, reset_level = variant.split("-")
            active = int(reset_level == "high")
            asynchronous = int(reset_mode == "async")
            reset_path = f"same_base_pointwise_reset_{variant}"
            command = (output / reset_path / "scenarios/RESET_STALLED_CHANNELS.raw.log.command").read_text()
            self.assertIn(f"-gRESET_ACTIVE_LEVEL={active}", command)
            self.assertIn(f"-gRESET_ASYNC={asynchronous}", command)
            sim_dir = output / reset_path / "sim"
            self.assertEqual(sum(Path(path).resolve() == sim_dir.resolve() for path in compile_dirs), 1,
                             compile_dirs)
        self.assertTrue(manifest.is_file())


if __name__ == "__main__":
    unittest.main()
