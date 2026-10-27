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
)
PROFILES = ("same_base_pointwise", "same_base_asymmetric_64")


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
        for path in (
            self.rtl_tests / "tb_generated_duplex.sv",
            self.family / "compiler_cases/duplex_effects/same_base_pointwise.cpp",
            self.family / "compiler_cases/duplex_effects/same_base_asymmetric_64.cpp",
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
a = p.parse_args(); out = Path(a.output); fields = {}
for line in (out / 'provenance.txt').read_text().splitlines():
    key, value = line.split('=', 1); fields[key] = value
required = {'top','source','source_sha256','bambu','bambu_sha256','plugin','plugin_sha256',
            'checker','checker_sha256','runner','runner_sha256','raw_ir','raw_ir_sha256',
            'post_interface_raw','post_interface_raw_sha256'}
assert fields.keys() == required, fields.keys()
assert fields['top'] == a.top
for key, path in [('source', a.source), ('bambu', a.bambu), ('plugin', a.plugin),
                  ('checker', a.checker), ('runner', a.runner)]:
    assert Path(fields[key]).resolve() == Path(path).resolve()
    assert fields[key + '_sha256'] == hashlib.sha256(Path(path).read_bytes()).hexdigest()
raw = Path(fields['raw_ir'])
assert raw.is_file() and raw.resolve().is_relative_to(out.resolve())
assert fields['raw_ir_sha256'] == hashlib.sha256(raw.read_bytes()).hexdigest()
post_raw = Path(fields['post_interface_raw'])
assert post_raw.is_file() and post_raw.resolve().is_relative_to(out.resolve())
assert fields['post_interface_raw_sha256'] == hashlib.sha256(post_raw.read_bytes()).hexdigest()
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
        self._stub("verilator", self._verilator_stub())
        self.env = os.environ.copy()
        self.env.update({
            "PATH": f"{self.bin}:{self.env['PATH']}",
            "BURST_DUPLEX_SIM_OUTSIDE_SANDBOX": "1",
            "TMPDIR": str(self.scratch), "TMP": str(self.scratch), "TEMP": str(self.scratch),
            "SIM_JOBS": "2", "DUPLEX_G1_JOBS": "3",
            "SIM_MODE": "pass", "SIM_ACTIVE_FILE": str(self.scratch / "active"),
            "SIM_MAX_FILE": str(self.scratch / "max_active"),
        })

    @staticmethod
    def _bambu_stub():
        return """#!/bin/bash
set -euo pipefail
if [[ ${1:-} == --version ]]; then echo 'Bambu stub 1.0'; exit 0; fi
profile=
for arg in "$@"; do [[ $arg == --top-fname=* ]] && profile=${arg#*=}; done
[[ -n $profile ]]
printf 'module %s(); endmodule\\n' "$profile" >"$profile.v"
printf 'module generated_library(); endmodule\\n' >panda_libtech.v
mkdir -p panda-temp
printf 'raw-ir %s\\n' "$profile" >panda-temp/before_Frontend::InterfaceInfer.raw
printf 'post-raw-ir %s\\n' "$profile" >panda-temp/after_Frontend::InterfaceInfer.raw
"""

    @staticmethod
    def _verilator_stub():
        return """#!/bin/bash
set -euo pipefail
if [[ ${1:-} == --version ]]; then echo 'Verilator 5.stub'; exit 0; fi
mdir=
while (($#)); do if [[ $1 == --Mdir ]]; then mdir=$2; shift 2; else shift; fi; done
mkdir -p "$mdir"
cat >"$mdir/Vtb_generated_duplex" <<'PROGRAM'
#!/bin/bash
set -euo pipefail
scenario=${1#*=}
profile=${0%/obj/Vtb_generated_duplex}; profile=${profile##*/}
mode=${SIM_MODE:-pass}
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
echo "GENERATED_DUPLEX PASS CASE=$scenario PROFILE=$profile"
PROGRAM
chmod +x "$mdir/Vtb_generated_duplex"
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

    def test_scenario_omission_prevents_aggregate_success(self):
        result, output = self._run("omit", "omit-output")
        self.assertNotEqual(result.returncode, 0)
        self.assertNotIn("GENERATED_DUPLEX PASS: 13 scenarios x 2 profiles", result.stdout)
        self.assertFalse((output / "aggregate.log").exists())

    def test_all_scenario_profile_markers_and_bounded_pool_are_required(self):
        result, output = self._run()
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("GENERATED_DUPLEX PASS: 13 scenarios x 2 profiles", result.stdout)
        for profile in PROFILES:
            manifest = output / profile / "generated" / "provenance.txt"
            self.assertTrue(manifest.is_file())
            fields = dict(line.split("=", 1) for line in manifest.read_text().splitlines())
            for stem in ("raw_ir", "post_interface_raw"):
                artifact = Path(fields[stem])
                self.assertTrue(artifact.is_file())
                self.assertTrue(artifact.resolve().is_relative_to(manifest.parent.resolve()))
                self.assertEqual(fields[f"{stem}_sha256"],
                                 hashlib.sha256(artifact.read_bytes()).hexdigest())
            for scenario in SCENARIOS:
                log = output / profile / "scenarios" / f"{scenario}.log"
                self.assertIn(f"GENERATED_DUPLEX PASS CASE={scenario} PROFILE={profile}", log.read_text())
        manifest = output / "scenario-sha256.txt"
        entries = manifest.read_text().splitlines()
        self.assertEqual(len(entries), len(PROFILES) * len(SCENARIOS))
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
        self.assertEqual(seen, expected_paths)
        maximum = int((self.scratch / "max_active").read_text())
        self.assertGreater(maximum, 1)
        self.assertLessEqual(maximum, 3)
        provenance = (output / "provenance.txt").read_text()
        self.assertIn("scenario_jobs=3", provenance)
        self.assertTrue(manifest.is_file())


if __name__ == "__main__":
    unittest.main()
